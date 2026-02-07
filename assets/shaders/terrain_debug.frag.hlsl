// Terrain Debug Visualization Fragment Shader (HLSL SM 6.0)
// Renders terrain debug overlays based on data texture and mode mask.
//
// Debug modes (bit flags in activeModeMask):
// Bit 0: Elevation Heatmap - Blue to red color ramp
// Bit 1: Terrain Type - Distinct color per type
// Bit 2: Chunk Boundary - White lines at chunk edges
// Bit 3: LOD Level - Color per chunk LOD
// Bit 4: Normals - RGB encoding of world-space normals
// Bit 5: Water Body ID - Unique color per water body
// Bit 6: Buildability - Green/red overlay

// Data texture layout:
// R = elevation (0-255, scaled from 0-31 * 8)
// G = terrain type (0-250, scaled from 0-9 * 25)
// B = flags (bit 0 = buildable, bit 1 = water body)
// A = LOD level (bits 0-1) | water body ID lower bits (bits 2-7)
//
// Normal texture layout (normalsTexture):
// R = normal.x * 0.5 + 0.5
// G = normal.y * 0.5 + 0.5
// B = normal.z * 0.5 + 0.5
// A = reserved

Texture2D<float4> dataTexture : register(t0);
Texture2D<float4> normalsTexture : register(t1);
SamplerState dataSampler : register(s0);

struct PSInput
{
    float4 position : SV_Position;
    float2 worldPos : TEXCOORD0;          // World XY position
    float2 uvCoords : TEXCOORD1;          // UV coords for texture
    float4 elevColorLow : COLOR0;         // Low elevation color
    float4 elevColorMid : COLOR1;         // Mid elevation color
    float4 elevColorHigh : COLOR2;        // High elevation color
    float4 buildColor : COLOR3;           // Buildable color
    float4 nobuildColor : COLOR4;         // Unbuildable color
    float4 chunkColor : COLOR5;           // Chunk boundary color
    float4 params : TEXCOORD2;            // chunkSize, lineThickness, opacity, activeModeMask
    float2 mapDims : TEXCOORD3;           // Map dimensions
};

struct PSOutput
{
    float4 color : SV_Target0;
};

// Debug mode bit flags
static const uint MODE_ELEVATION_HEATMAP = 0x01;
static const uint MODE_TERRAIN_TYPE = 0x02;
static const uint MODE_CHUNK_BOUNDARY = 0x04;
static const uint MODE_LOD_LEVEL = 0x08;
static const uint MODE_NORMALS = 0x10;
static const uint MODE_WATER_BODY_ID = 0x20;
static const uint MODE_BUILDABILITY = 0x40;

// Terrain type colors (matches TerrainType enum order)
static const float4 TERRAIN_COLORS[10] = {
    float4(0.6, 0.5, 0.4, 0.6),  // Substrate - Brown
    float4(0.5, 0.4, 0.3, 0.6),  // Ridge - Dark brown
    float4(0.0, 0.0, 0.3, 0.6),  // DeepVoid - Dark blue
    float4(0.0, 0.5, 0.8, 0.6),  // FlowChannel - Light blue
    float4(0.0, 0.3, 0.6, 0.6),  // StillBasin - Medium blue
    float4(0.0, 0.6, 0.0, 0.6),  // BiolumeGrove - Green
    float4(0.8, 0.2, 0.8, 0.6),  // PrismaFields - Magenta
    float4(0.2, 0.8, 0.5, 0.6),  // SporeFlats - Teal
    float4(0.4, 0.2, 0.0, 0.6),  // BlightMires - Dark orange
    float4(0.8, 0.4, 0.0, 0.6)   // EmberCrust - Orange
};

// LOD level colors
static const float4 LOD_COLORS[3] = {
    float4(0.0, 1.0, 0.0, 0.4),  // Green for LOD 0
    float4(1.0, 1.0, 0.0, 0.4),  // Yellow for LOD 1
    float4(1.0, 0.0, 0.0, 0.4)   // Red for LOD 2
};

// Compute anti-aliased grid line intensity
float gridLine(float coord, float spacing, float thickness)
{
    // Distance from nearest grid line
    float d = abs(frac(coord / spacing + 0.5) - 0.5) * spacing;

    // Use screen-space derivatives for consistent line width
    float lineWidth = thickness * 0.5;
    float derivative = fwidth(coord);

    // Anti-aliased step function
    float aa = derivative * 1.5;
    return 1.0 - smoothstep(lineWidth - aa, lineWidth + aa, d);
}

// Compute grid intensity for both X and Y directions
float grid(float2 worldPos, float spacing, float thickness)
{
    float lineX = gridLine(worldPos.x, spacing, thickness);
    float lineY = gridLine(worldPos.y, spacing, thickness);
    return max(lineX, lineY);
}

// Generate water body color from ID using hash
float4 getWaterBodyColor(uint bodyId)
{
    if (bodyId == 0) return float4(0, 0, 0, 0);

    // Knuth multiplicative hash
    uint hash = bodyId * 2654435761u;

    float r = float((hash >> 0) & 0xFF) / 255.0;
    float g = float((hash >> 8) & 0xFF) / 255.0;
    float b = float((hash >> 16) & 0xFF) / 255.0;

    // Ensure brightness
    float brightness = (r + g + b) / 3.0;
    if (brightness < 0.4)
    {
        float scale = 0.4 / (brightness + 0.001);
        r = min(1.0, r * scale);
        g = min(1.0, g * scale);
        b = min(1.0, b * scale);
    }

    return float4(r, g, b, 0.6);
}

// Alpha blend two colors
float4 alphaBlend(float4 src, float4 dst)
{
    float srcAlpha = src.a;
    float dstAlpha = dst.a * (1.0 - srcAlpha);
    float outAlpha = srcAlpha + dstAlpha;

    if (outAlpha < 0.001) return dst;

    return float4(
        (src.rgb * srcAlpha + dst.rgb * dstAlpha) / outAlpha,
        outAlpha
    );
}

PSOutput main(PSInput input)
{
    PSOutput output;

    // Extract parameters
    float chunkSize = input.params.x;
    float lineThickness = input.params.y;
    float opacity = input.params.z;
    uint activeModeMask = asuint(input.params.w);

    // Sample data texture
    float4 data = dataTexture.Sample(dataSampler, input.uvCoords);

    // Decode data
    float elevation = data.r;  // 0-1 range (0-31 scaled)
    uint terrainType = uint(data.g * 10.0 + 0.5);  // 0-9
    terrainType = min(terrainType, 9u);
    uint flags = uint(data.b * 255.0 + 0.5);
    bool isBuildable = (flags & 0x01) != 0;
    bool isWater = (flags & 0x02) != 0;
    uint lodAndWater = uint(data.a * 255.0 + 0.5);
    uint lodLevel = lodAndWater & 0x03;
    uint waterBodyId = (lodAndWater >> 2) & 0x3F;

    // Start with transparent
    float4 finalColor = float4(0, 0, 0, 0);

    // Layer 1: Elevation Heatmap
    if ((activeModeMask & MODE_ELEVATION_HEATMAP) != 0)
    {
        float4 elevColor;
        float normalizedElev = elevation;  // Already 0-1

        if (normalizedElev < 0.5)
        {
            // Low to mid
            float t = normalizedElev * 2.0;
            elevColor = lerp(input.elevColorLow, input.elevColorMid, t);
        }
        else
        {
            // Mid to high
            float t = (normalizedElev - 0.5) * 2.0;
            elevColor = lerp(input.elevColorMid, input.elevColorHigh, t);
        }

        finalColor = alphaBlend(elevColor, finalColor);
    }

    // Layer 2: Terrain Type
    if ((activeModeMask & MODE_TERRAIN_TYPE) != 0)
    {
        float4 typeColor = TERRAIN_COLORS[terrainType];
        typeColor.a *= opacity;
        finalColor = alphaBlend(typeColor, finalColor);
    }

    // Layer 3: Water Body ID
    if ((activeModeMask & MODE_WATER_BODY_ID) != 0 && isWater)
    {
        float4 waterColor = getWaterBodyColor(waterBodyId);
        waterColor.a *= opacity;
        finalColor = alphaBlend(waterColor, finalColor);
    }

    // Layer 4: Buildability
    if ((activeModeMask & MODE_BUILDABILITY) != 0)
    {
        float4 buildColor = isBuildable ? input.buildColor : input.nobuildColor;
        finalColor = alphaBlend(buildColor, finalColor);
    }

    // Layer 5: LOD Level
    if ((activeModeMask & MODE_LOD_LEVEL) != 0)
    {
        float4 lodColor = LOD_COLORS[min(lodLevel, 2u)];
        lodColor.a *= opacity;
        finalColor = alphaBlend(lodColor, finalColor);
    }

    // Layer 6: Normals visualization (RGB encoding)
    // World-space normals are encoded as: R=nx*0.5+0.5, G=ny*0.5+0.5, B=nz*0.5+0.5
    // This creates a color where:
    //   - Flat (up-facing) normals are green-ish (0.5, 1.0, 0.5) -> bright green
    //   - X-tilted slopes show red shift
    //   - Z-tilted slopes show blue shift
    if ((activeModeMask & MODE_NORMALS) != 0)
    {
        // Sample normals texture at same UV coordinates
        float4 normalData = normalsTexture.Sample(dataSampler, input.uvCoords);

        // Decode normal from 0-1 range back to -1 to +1
        // But for visualization, we display the encoded RGB directly as color
        // This gives intuitive color mapping:
        //   R channel = X component (red = pointing right)
        //   G channel = Y component (green = pointing up, bright green = flat terrain)
        //   B channel = Z component (blue = pointing forward)
        float4 normalColor = float4(normalData.rgb, opacity * 0.7);
        finalColor = alphaBlend(normalColor, finalColor);
    }

    // Layer 7: Chunk Boundaries (on top)
    if ((activeModeMask & MODE_CHUNK_BOUNDARY) != 0)
    {
        float chunkIntensity = grid(input.worldPos, chunkSize, lineThickness);

        if (chunkIntensity > 0.001)
        {
            float4 boundaryColor = float4(input.chunkColor.rgb, input.chunkColor.a * chunkIntensity);
            finalColor = alphaBlend(boundaryColor, finalColor);
        }
    }

    // Discard fully transparent pixels
    if (finalColor.a < 0.001)
    {
        discard;
    }

    output.color = finalColor;
    return output;
}

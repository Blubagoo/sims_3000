// Terrain Debug Visualization Vertex Shader (HLSL SM 6.0)
// Generates a quad covering the entire map on the terrain plane (Y=0).
// Projects quad vertices using the view-projection matrix.
//
// The debug overlay is rendered as a fullscreen quad on the terrain plane,
// with visualization computed procedurally in the fragment shader.
//
// Usage: Draw 6 vertices with no vertex buffer bound.
//   SDL_DrawGPUPrimitives(pass, 6, 1, 0, 0);

// Uniform buffer matching TerrainDebugUBO struct (192 bytes)
cbuffer DebugUniforms : register(b0)
{
    float4x4 viewProjection;      // 64 bytes: View-projection matrix
    float4 elevationColorLow;     // 16 bytes: Low elevation color (blue)
    float4 elevationColorMid;     // 16 bytes: Mid elevation color (yellow)
    float4 elevationColorHigh;    // 16 bytes: High elevation color (red)
    float4 buildableColor;        // 16 bytes: Buildable tile color (green)
    float4 unbuildableColor;      // 16 bytes: Unbuildable tile color (red)
    float4 chunkBoundaryColor;    // 16 bytes: Chunk boundary color (white)
    float2 mapSize;               //  8 bytes: Map dimensions (width, height)
    float chunkSize;              //  4 bytes: Chunk size in tiles (32)
    float lineThickness;          //  4 bytes: Line thickness
    float opacity;                //  4 bytes: Overall overlay opacity
    uint activeModeMask;          //  4 bytes: Bit mask of active modes
    float cameraDistance;         //  4 bytes: Camera distance for LOD fade
    float _padding;               //  4 bytes: Padding
};

struct VSOutput
{
    float4 position : SV_Position;
    float2 worldPos : TEXCOORD0;          // World XY position for data lookup
    float2 uvCoords : TEXCOORD1;          // UV coords for texture sampling
    float4 elevColorLow : COLOR0;         // Low elevation color
    float4 elevColorMid : COLOR1;         // Mid elevation color
    float4 elevColorHigh : COLOR2;        // High elevation color
    float4 buildColor : COLOR3;           // Buildable color
    float4 nobuildColor : COLOR4;         // Unbuildable color
    float4 chunkColor : COLOR5;           // Chunk boundary color
    float4 params : TEXCOORD2;            // chunkSize, lineThickness, opacity, activeModeMask
    float2 mapDims : TEXCOORD3;           // Map dimensions
};

VSOutput main(uint vertexID : SV_VertexID)
{
    VSOutput output;

    // Generate quad vertices covering the map
    // Quad corners: (0,0), (mapSize.x, 0), (mapSize.x, mapSize.y), (0, mapSize.y)
    // Two triangles: [0,1,2] and [0,2,3]
    //
    // Vertex layout for 6 vertices:
    // 0: (0, 0)           - bottom-left
    // 1: (mapSize.x, 0)   - bottom-right
    // 2: (mapSize.x, mapSize.y) - top-right
    // 3: (0, 0)           - bottom-left (repeated)
    // 4: (mapSize.x, mapSize.y) - top-right (repeated)
    // 5: (0, mapSize.y)   - top-left

    float2 corners[6];
    corners[0] = float2(0.0, 0.0);
    corners[1] = float2(mapSize.x, 0.0);
    corners[2] = float2(mapSize.x, mapSize.y);
    corners[3] = float2(0.0, 0.0);
    corners[4] = float2(mapSize.x, mapSize.y);
    corners[5] = float2(0.0, mapSize.y);

    // UV coordinates for texture sampling
    float2 uvs[6];
    uvs[0] = float2(0.0, 0.0);
    uvs[1] = float2(1.0, 0.0);
    uvs[2] = float2(1.0, 1.0);
    uvs[3] = float2(0.0, 0.0);
    uvs[4] = float2(1.0, 1.0);
    uvs[5] = float2(0.0, 1.0);

    float2 worldXY = corners[vertexID];
    float2 uv = uvs[vertexID];

    // Position on the terrain plane (Y = 0 in world space)
    // Map: X->X, Y->Z for terrain plane
    float3 worldPos = float3(worldXY.x, 0.0, worldXY.y);

    // Transform to clip space
    output.position = mul(viewProjection, float4(worldPos, 1.0));

    // Pass world XY and UV for fragment shader
    output.worldPos = worldXY;
    output.uvCoords = uv;

    // Pass colors with opacity
    output.elevColorLow = float4(elevationColorLow.rgb, elevationColorLow.a * opacity);
    output.elevColorMid = float4(elevationColorMid.rgb, elevationColorMid.a * opacity);
    output.elevColorHigh = float4(elevationColorHigh.rgb, elevationColorHigh.a * opacity);
    output.buildColor = float4(buildableColor.rgb, buildableColor.a * opacity);
    output.nobuildColor = float4(unbuildableColor.rgb, unbuildableColor.a * opacity);
    output.chunkColor = float4(chunkBoundaryColor.rgb, chunkBoundaryColor.a * opacity);

    // Pack parameters
    output.params = float4(chunkSize, lineThickness, opacity, asfloat(activeModeMask));
    output.mapDims = mapSize;

    return output;
}

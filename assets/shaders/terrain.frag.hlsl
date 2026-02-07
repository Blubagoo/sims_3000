// Terrain Fragment Shader (HLSL SM 6.0)
// Extends the base toon shader with terrain-specific visual treatment.
//
// Features:
// - 10-entry uniform buffer palette for base and emissive colors
// - Per-vertex terrain_type lookup into palette (no texture sampling)
// - 4 discrete lighting bands (lit, mid, shadow, deep shadow)
// - Animated glow behaviors per terrain type:
//   * Static: Substrate, Ridge
//   * Slow pulse (6-8s): DeepVoid, FlowChannel, StillBasin
//   * Subtle pulse (4s): BiolumeGrove
//   * Shimmer (random flicker): PrismaFields
//   * Rhythmic pulse (3s): SporeFlats
//   * Irregular bubble pulse: BlightMires
//   * Slow throb (5s): EmberCrust
// - Normal-based crevice glow for Ridge and EmberCrust
// - Shadow mapping support
// - Emissive output for bloom extraction (MRT)
//
// Resource bindings (per SDL_GPU conventions):
// - Uniform buffer 0 (space3): Lighting parameters
// - Uniform buffer 1 (space3): TerrainVisuals palette
// - Sampler 0 (space2): Shadow map sampler
// - Texture 0 (space2): Shadow map

// =============================================================================
// Terrain Type Constants (must match TerrainType enum)
// =============================================================================
static const uint TERRAIN_SUBSTRATE    = 0;
static const uint TERRAIN_RIDGE        = 1;
static const uint TERRAIN_DEEP_VOID    = 2;
static const uint TERRAIN_FLOW_CHANNEL = 3;
static const uint TERRAIN_STILL_BASIN  = 4;
static const uint TERRAIN_BIOLUME_GROVE = 5;
static const uint TERRAIN_PRISMA_FIELDS = 6;
static const uint TERRAIN_SPORE_FLATS  = 7;
static const uint TERRAIN_BLIGHT_MIRES = 8;
static const uint TERRAIN_EMBER_CRUST  = 9;

// =============================================================================
// Glow Animation Constants
// =============================================================================
static const float PI = 3.14159265359;
static const float TWO_PI = 6.28318530718;

// Pulse periods in seconds
static const float WATER_PULSE_PERIOD = 7.0;      // Slow pulse (6-8s average)
static const float BIOLUME_PULSE_PERIOD = 4.0;    // Subtle pulse
static const float PRISMA_SHIMMER_PERIOD = 0.2;   // Fast shimmer base
static const float SPORE_PULSE_PERIOD = 3.0;      // Rhythmic pulse
static const float BLIGHT_BUBBLE_PERIOD = 2.5;    // Irregular base
static const float EMBER_THROB_PERIOD = 5.0;      // Slow throb

// Amplitude constants
static const float PULSE_AMPLITUDE = 0.3;         // Standard pulse amplitude
static const float SHIMMER_AMPLITUDE = 0.4;       // Higher for shimmer effect
static const float SUBTLE_AMPLITUDE = 0.15;       // Subtle variation

// Crevice glow constants
static const float CREVICE_NORMAL_THRESHOLD = 0.85;
static const float CREVICE_MAX_BOOST = 2.0;

// =============================================================================
// Uniform Buffers
// =============================================================================

// Lighting parameters (fragment uniform slot 0)
cbuffer LightingUBO : register(b0, space3)
{
    float3 sunDirection;       // World-space light direction (normalized)
    float  globalAmbient;      // Global ambient strength
    float3 ambientColor;       // Ambient color
    float  shadowEnabled;      // 1.0 if shadows enabled
    float3 deepShadowColor;    // Deep shadow tint color
    float  shadowIntensity;    // Shadow darkness
    float3 shadowTintColor;    // Shadow tint toward teal
    float  shadowBias;         // Depth comparison bias
    float3 shadowColor;        // Color tint for shadowed areas
    float  shadowSoftness;     // Shadow edge softness
};

// Terrain visual palette (fragment uniform slot 1)
cbuffer TerrainVisuals : register(b1, space3)
{
    float4 base_colors[10];     // Base diffuse colors per terrain type
    float4 emissive_colors[10]; // Emissive RGB + intensity in alpha
    float  glow_time;           // Animation time in seconds
    float  sea_level;           // Sea level elevation
    float2 _padding;            // Alignment padding
};

// Shadow map sampling
SamplerComparisonState shadowSampler : register(s0, space2);
Texture2D<float> shadowMap : register(t0, space2);

// =============================================================================
// Fragment Input
// =============================================================================

struct PSInput
{
    float4 clipPosition   : SV_Position;
    float3 worldPosition  : TEXCOORD0;
    float3 worldNormal    : TEXCOORD1;
    float2 uv             : TEXCOORD2;
    nointerpolation uint terrainType : TEXCOORD3;
    float  elevation      : TEXCOORD4;
    float2 tileCoord      : TEXCOORD5;
    float4 lightSpacePos  : TEXCOORD6;
};

// =============================================================================
// Fragment Output (single target - MRT bloom disabled until bloom post-process is ready)
// =============================================================================

struct PSOutput
{
    float4 color    : SV_Target0;  // Main color output
    // NOTE: MRT bloom extraction (SV_Target1) disabled until pipeline supports 2 color targets
};

// =============================================================================
// Pseudo-Random Functions
// =============================================================================

// Hash function for pseudo-random noise
float hash(float2 p)
{
    float3 p3 = frac(float3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return frac((p3.x + p3.y) * p3.z);
}

// Value noise for shimmer effect
float noise(float2 p)
{
    float2 i = floor(p);
    float2 f = frac(p);

    // Cubic interpolation
    float2 u = f * f * (3.0 - 2.0 * f);

    return lerp(
        lerp(hash(i + float2(0.0, 0.0)), hash(i + float2(1.0, 0.0)), u.x),
        lerp(hash(i + float2(0.0, 1.0)), hash(i + float2(1.0, 1.0)), u.x),
        u.y
    );
}

// =============================================================================
// Toon Lighting Functions
// =============================================================================

// Quantize lighting into discrete bands
// Returns: band index (0 = deep shadow, 1 = shadow, 2 = mid, 3 = lit)
int quantizeLightingBand(float NdotL)
{
    // Remap from [-1, 1] to [0, 1]
    float intensity = NdotL * 0.5 + 0.5;

    // 4-band thresholds
    if (intensity < 0.2)
        return 0;  // Deep shadow
    else if (intensity < 0.4)
        return 1;  // Shadow
    else if (intensity < 0.7)
        return 2;  // Mid
    else
        return 3;  // Lit (full brightness)
}

// Get lighting multiplier for each band
float getBandMultiplier(int band)
{
    switch (band)
    {
        case 0: return 0.15;  // Deep shadow
        case 1: return 0.35;  // Shadow
        case 2: return 0.65;  // Mid
        case 3: return 1.0;   // Lit
        default: return 0.5;
    }
}

// Apply shadow color shift toward purple/teal for alien aesthetic
float3 applyShadowColorShift(float3 baseColor, int band, float3 deepShadow, float3 shadowTint)
{
    float3 result = baseColor;

    switch (band)
    {
        case 0:
            result = lerp(baseColor, deepShadow, 0.7);
            break;
        case 1:
            result = lerp(baseColor, shadowTint, 0.4);
            break;
        case 2:
            result = lerp(baseColor, shadowTint, 0.15);
            break;
        case 3:
            break;
    }

    return result;
}

// Calculate shadow factor using comparison sampler
float calculateShadow(float4 lightSpacePos, float NdotL)
{
    float3 projCoords = lightSpacePos.xyz / lightSpacePos.w;

    projCoords.x = projCoords.x * 0.5 + 0.5;
    projCoords.y = projCoords.y * -0.5 + 0.5;

    if (projCoords.x < 0.0 || projCoords.x > 1.0 ||
        projCoords.y < 0.0 || projCoords.y > 1.0 ||
        projCoords.z < 0.0 || projCoords.z > 1.0)
    {
        return 1.0;
    }

    float bias = shadowBias + shadowBias * (1.0 - abs(NdotL));
    float currentDepth = projCoords.z - bias;

    return shadowMap.SampleCmpLevelZero(shadowSampler, projCoords.xy, currentDepth);
}

// Apply shadow to color
float3 applyShadowToColor(float3 color, float shadowFactor, float intensity, float3 shadowTint)
{
    float3 shadowedColor = lerp(color, shadowTint, 0.5);
    shadowedColor *= (1.0 - intensity);
    return lerp(shadowedColor, color, shadowFactor);
}

// =============================================================================
// Glow Animation Functions
// =============================================================================

// Calculate glow multiplier based on terrain type and time
float calculateGlowMultiplier(uint terrainType, float time, float2 tileCoord)
{
    float glowMod = 1.0;

    switch (terrainType)
    {
        case TERRAIN_SUBSTRATE:
        case TERRAIN_RIDGE:
            // Static - no animation
            glowMod = 1.0;
            break;

        case TERRAIN_DEEP_VOID:
        case TERRAIN_FLOW_CHANNEL:
        case TERRAIN_STILL_BASIN:
            // Slow pulse (6-8s period)
            // Using slightly different periods for variety
            {
                float period = WATER_PULSE_PERIOD + (terrainType - TERRAIN_DEEP_VOID) * 0.5;
                float phase = sin(time * TWO_PI / period);
                glowMod = 1.0 + phase * PULSE_AMPLITUDE;
            }
            break;

        case TERRAIN_BIOLUME_GROVE:
            // Subtle pulse (4s period)
            {
                float phase = sin(time * TWO_PI / BIOLUME_PULSE_PERIOD);
                glowMod = 1.0 + phase * SUBTLE_AMPLITUDE;
            }
            break;

        case TERRAIN_PRISMA_FIELDS:
            // Shimmer effect (random flicker)
            {
                // Use tile coordinate and time for spatial and temporal variation
                float noiseVal = noise(tileCoord * 0.5 + float2(time * 5.0, time * 3.7));
                float fastPhase = sin(time * TWO_PI / PRISMA_SHIMMER_PERIOD);
                glowMod = 1.0 + (noiseVal * 2.0 - 1.0) * SHIMMER_AMPLITUDE * (0.5 + 0.5 * fastPhase);
            }
            break;

        case TERRAIN_SPORE_FLATS:
            // Rhythmic pulse (3s period)
            {
                float phase = sin(time * TWO_PI / SPORE_PULSE_PERIOD);
                // Sharper peaks for more rhythmic feel
                float sharpPhase = phase * phase * sign(phase);
                glowMod = 1.0 + sharpPhase * PULSE_AMPLITUDE;
            }
            break;

        case TERRAIN_BLIGHT_MIRES:
            // Irregular bubble pulse
            {
                // Multiple overlapping frequencies for irregular feel
                float phase1 = sin(time * TWO_PI / BLIGHT_BUBBLE_PERIOD);
                float phase2 = sin(time * TWO_PI / (BLIGHT_BUBBLE_PERIOD * 1.618)); // Golden ratio
                float phase3 = sin(time * TWO_PI / (BLIGHT_BUBBLE_PERIOD * 0.618));

                // Spatial variation based on tile
                float spatialVar = noise(tileCoord * 0.3);

                float combinedPhase = phase1 * 0.5 + phase2 * 0.3 + phase3 * 0.2;
                combinedPhase *= (0.7 + 0.3 * spatialVar);

                glowMod = 1.0 + combinedPhase * PULSE_AMPLITUDE;
            }
            break;

        case TERRAIN_EMBER_CRUST:
            // Slow throb (5s period)
            {
                float phase = sin(time * TWO_PI / EMBER_THROB_PERIOD);
                // Smooth sinusoidal throb
                glowMod = 1.0 + phase * PULSE_AMPLITUDE * 0.8;
            }
            break;

        default:
            glowMod = 1.0;
            break;
    }

    return saturate(glowMod);
}

// Calculate crevice glow boost for Ridge and EmberCrust
float calculateCreviceGlow(uint terrainType, float3 normal)
{
    // Only Ridge and EmberCrust have crevice glow
    if (terrainType != TERRAIN_RIDGE && terrainType != TERRAIN_EMBER_CRUST)
    {
        return 1.0;
    }

    // Calculate deviation from vertical
    // normal.y = 1.0 means perfectly flat (up-facing)
    // normal.y = 0.0 means perfectly vertical (cliff face)
    float verticalness = abs(normal.y);

    // If normal is mostly vertical, no boost
    if (verticalness > CREVICE_NORMAL_THRESHOLD)
    {
        return 1.0;
    }

    // Calculate boost based on deviation
    // More horizontal normal = more glow (crevices glow brighter)
    float deviation = 1.0 - verticalness;
    float boost = 1.0 + (CREVICE_MAX_BOOST - 1.0) * smoothstep(0.0, 1.0 - CREVICE_NORMAL_THRESHOLD, deviation);

    return boost;
}

// =============================================================================
// Main Fragment Shader
// =============================================================================

PSOutput main(PSInput input)
{
    PSOutput output;

    // DEBUG: Output bright magenta to test if fragment shader runs
    output.color = float4(1.0, 0.0, 1.0, 1.0);
    return output;

    // Get terrain type (clamped to valid range)
    uint terrainType = min(input.terrainType, 9u);

    // Look up colors from palette
    float4 baseColorAlpha = base_colors[terrainType];
    float4 emissiveColorIntensity = emissive_colors[terrainType];

    float3 baseColor = baseColorAlpha.rgb;
    float3 emissiveColor = emissiveColorIntensity.rgb;
    float baseIntensity = emissiveColorIntensity.a;

    // Normalize interpolated normal
    float3 normal = normalize(input.worldNormal);

    // Calculate N dot L
    float NdotL = dot(normal, sunDirection);

    // Quantize into discrete lighting bands
    int band = quantizeLightingBand(NdotL);

    // Get stepped lighting multiplier
    float lightMultiplier = getBandMultiplier(band);

    // Apply shadow color shifting for alien aesthetic
    float3 shiftedColor = applyShadowColorShift(
        baseColor,
        band,
        deepShadowColor,
        shadowTintColor
    );

    // Calculate shadow factor
    float shadowFactor = 1.0;
    if (shadowEnabled > 0.5)
    {
        shadowFactor = calculateShadow(input.lightSpacePos, NdotL);

        if (shadowSoftness < 0.5)
        {
            shadowFactor = shadowFactor > 0.5 ? 1.0 : 0.0;
        }
    }

    // Calculate diffuse contribution with stepped lighting
    float3 diffuse = shiftedColor * lightMultiplier;

    // Apply shadow
    if (shadowEnabled > 0.5 && shadowFactor < 1.0)
    {
        diffuse = applyShadowToColor(diffuse, shadowFactor, shadowIntensity, shadowColor);
    }

    // Calculate ambient contribution
    float3 ambientContribution = baseColor * ambientColor * globalAmbient;

    // Combine lighting (ambient + diffuse)
    float3 litColor = ambientContribution + diffuse;

    // ==========================================================================
    // Emissive Calculation
    // ==========================================================================

    // Calculate animated glow multiplier
    float glowMod = calculateGlowMultiplier(terrainType, glow_time, input.tileCoord);

    // Calculate crevice glow boost
    float creviceBoost = calculateCreviceGlow(terrainType, normal);

    // Calculate final emissive intensity
    float finalEmissiveIntensity = baseIntensity * glowMod * creviceBoost;

    // Emissive term (unaffected by lighting bands)
    float3 emissive = emissiveColor * finalEmissiveIntensity;

    // Final color = lit color + emissive
    float3 finalColor = litColor + emissive;
    finalColor = saturate(finalColor);

    // Output main color
    output.color = float4(finalColor, baseColorAlpha.a);

    // NOTE: Bloom extraction (MRT to SV_Target1) disabled until pipeline supports 2 color targets
    // Emissive is already added to finalColor above for the glow effect

    return output;
}

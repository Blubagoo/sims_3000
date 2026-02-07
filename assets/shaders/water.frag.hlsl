// Water Fragment Shader (HLSL SM 6.0)
// Semi-transparent water surface rendering with toon-stylized effects.
//
// Features:
// - Semi-transparent rendering: alpha 0.7-0.8
// - Depth test ON, depth write OFF (terrain beneath visible)
// - Very dark blue/teal base color (barely visible without glow)
// - Scrolling UV animation for rivers using flow_direction per-tile
// - Gentle UV distortion for ocean and lakes (sine-wave based)
// - Sine-wave normal perturbation for subtle toon-style surface highlights
// - Per-vertex shoreline emissive glow: shore_factor * emissive_color * boost
// - Animated emissive: ocean slow pulse (6s), lakes gentle pulse (8s), rivers static
// - No reflections, no refraction, no caustics (toon style)
//
// Resource bindings (per SDL_GPU conventions):
// - Uniform buffer 0 (space3): WaterVisualConfig

// =============================================================================
// Water Body Type Constants (must match WaterBodyType enum)
// =============================================================================
static const uint WATER_TYPE_OCEAN = 0;
static const uint WATER_TYPE_RIVER = 1;
static const uint WATER_TYPE_LAKE  = 2;

// =============================================================================
// Animation Constants
// =============================================================================
static const float PI = 3.14159265359;
static const float TWO_PI = 6.28318530718;

// Emissive pulse periods in seconds
static const float OCEAN_PULSE_PERIOD = 6.0;   // Slow pulse for ocean
static const float LAKE_PULSE_PERIOD = 8.0;    // Gentle pulse for lakes
// Rivers: no pulse (static) - flow animation handles visual interest

// UV scroll speed for rivers (tiles per second)
static const float RIVER_UV_SCROLL_SPEED = 0.3;

// Ocean/lake gentle UV distortion
static const float OCEAN_DISTORTION_AMPLITUDE = 0.02;
static const float OCEAN_DISTORTION_FREQUENCY = 0.5;
static const float LAKE_DISTORTION_AMPLITUDE = 0.015;
static const float LAKE_DISTORTION_FREQUENCY = 0.3;

// Normal perturbation for surface highlights
static const float NORMAL_PERTURBATION_AMPLITUDE = 0.15;
static const float NORMAL_PERTURBATION_FREQUENCY = 2.0;

// Shoreline glow boost
static const float SHORE_GLOW_BOOST = 2.5;

// Emissive pulse amplitude
static const float PULSE_AMPLITUDE = 0.25;

// =============================================================================
// Uniform Buffer
// =============================================================================

// Water visual configuration (fragment uniform slot 0)
cbuffer WaterVisualConfig : register(b0, space3)
{
    // Base color (very dark blue/teal)
    float4 base_color;           // RGB + alpha (0.7-0.8)

    // Ocean emissive color and intensity
    float4 ocean_emissive;       // RGB + intensity in alpha

    // River emissive color and intensity
    float4 river_emissive;       // RGB + intensity in alpha

    // Lake emissive color and intensity
    float4 lake_emissive;        // RGB + intensity in alpha

    // Animation time
    float glow_time;             // Time in seconds for animations

    // Flow direction components (for river UV scrolling)
    // Passed as dx, dy for the current tile's flow direction
    float flow_dx;               // X component of flow direction (-1, 0, 1)
    float flow_dy;               // Y component of flow direction (-1, 0, 1)

    // Water body type for current draw call
    uint water_body_type;        // 0=Ocean, 1=River, 2=Lake

    // Sun direction for surface highlights
    float3 sun_direction;        // Normalized world-space light direction
    float  _padding1;

    // Ambient lighting
    float  ambient_strength;     // Ambient contribution (0.0-1.0)
    float3 _padding2;
};

// =============================================================================
// Fragment Input
// =============================================================================

struct PSInput
{
    float4 clipPosition   : SV_Position;
    float3 worldPosition  : TEXCOORD0;
    float2 uv             : TEXCOORD1;
    float  shore_factor   : TEXCOORD2;
    nointerpolation uint water_body_id : TEXCOORD3;
    float4 lightSpacePos  : TEXCOORD4;
};

// =============================================================================
// Fragment Output (MRT for bloom extraction)
// =============================================================================

struct PSOutput
{
    float4 color    : SV_Target0;  // Main color output (RGBA with transparency)
    float4 emissive : SV_Target1;  // Emissive for bloom extraction
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

// =============================================================================
// UV Animation Functions
// =============================================================================

// Calculate scrolling UV offset for rivers
float2 calculateRiverUVScroll(float time, float dx, float dy)
{
    float2 flow_dir = float2(dx, dy);

    // Normalize flow direction (handle zero case)
    float flow_len = length(flow_dir);
    if (flow_len > 0.01)
    {
        flow_dir = flow_dir / flow_len;
    }
    else
    {
        flow_dir = float2(0.0, 1.0);  // Default to south if no flow
    }

    // Scroll UV in flow direction
    return flow_dir * RIVER_UV_SCROLL_SPEED * time;
}

// Calculate gentle UV distortion for ocean/lakes
float2 calculateWaterDistortion(float2 uv, float time, float amplitude, float frequency)
{
    float2 distortion;

    // Two overlapping sine waves for more organic feel
    distortion.x = amplitude * sin(uv.y * frequency * TWO_PI + time * 0.5);
    distortion.y = amplitude * sin(uv.x * frequency * TWO_PI * 1.3 + time * 0.4);

    // Add secondary wave for variation
    distortion.x += amplitude * 0.5 * sin(uv.y * frequency * TWO_PI * 2.0 + time * 0.7);
    distortion.y += amplitude * 0.5 * sin(uv.x * frequency * TWO_PI * 2.0 + time * 0.6);

    return distortion;
}

// =============================================================================
// Normal Perturbation for Surface Highlights
// =============================================================================

// Calculate perturbed normal for subtle toon-style surface highlights
float3 calculatePerturbedNormal(float2 uv, float time)
{
    // Start with up-facing normal (water is flat)
    float3 normal = float3(0.0, 1.0, 0.0);

    // Add sine-wave perturbation
    float perturbX = NORMAL_PERTURBATION_AMPLITUDE *
        sin(uv.x * NORMAL_PERTURBATION_FREQUENCY * TWO_PI + time * 0.8);
    float perturbZ = NORMAL_PERTURBATION_AMPLITUDE *
        sin(uv.y * NORMAL_PERTURBATION_FREQUENCY * TWO_PI + time * 0.6);

    // Perturb the normal
    normal.x += perturbX;
    normal.z += perturbZ;

    // Renormalize
    return normalize(normal);
}

// =============================================================================
// Toon Lighting (simplified for water)
// =============================================================================

// Calculate discrete lighting band for toon effect
float calculateToonLighting(float3 normal, float3 lightDir)
{
    float NdotL = dot(normal, lightDir);

    // Remap from [-1, 1] to [0, 1]
    float intensity = NdotL * 0.5 + 0.5;

    // 2-band quantization for subtle water highlights
    // Band 0: < 0.5 -> 0.7 (dark)
    // Band 1: >= 0.5 -> 1.0 (highlight)
    if (intensity < 0.5)
        return 0.7;
    else
        return 1.0;
}

// =============================================================================
// Emissive Pulse Animation
// =============================================================================

// Calculate emissive multiplier based on water type and time
float calculateEmissivePulse(uint water_type, float time)
{
    float pulse = 1.0;

    if (water_type == WATER_TYPE_OCEAN)
    {
        // Slow pulse (6s period)
        float phase = sin(time * TWO_PI / OCEAN_PULSE_PERIOD);
        pulse = 1.0 + phase * PULSE_AMPLITUDE;
    }
    else if (water_type == WATER_TYPE_LAKE)
    {
        // Gentle pulse (8s period)
        float phase = sin(time * TWO_PI / LAKE_PULSE_PERIOD);
        pulse = 1.0 + phase * PULSE_AMPLITUDE * 0.8;  // Slightly less than ocean
    }
    // Rivers: no pulse (flow handles visual interest)

    return saturate(pulse);
}

// =============================================================================
// Get Emissive Color for Water Type
// =============================================================================

float4 getEmissiveForWaterType(uint water_type)
{
    if (water_type == WATER_TYPE_OCEAN)
        return ocean_emissive;
    else if (water_type == WATER_TYPE_RIVER)
        return river_emissive;
    else
        return lake_emissive;
}

// =============================================================================
// Main Fragment Shader
// =============================================================================

PSOutput main(PSInput input)
{
    PSOutput output;

    // Get water type
    uint water_type = water_body_type;

    // ==========================================================================
    // UV Animation
    // ==========================================================================

    float2 animatedUV = input.uv;

    if (water_type == WATER_TYPE_RIVER)
    {
        // Rivers: scrolling UV based on flow direction
        float2 scroll = calculateRiverUVScroll(glow_time, flow_dx, flow_dy);
        animatedUV = input.uv + scroll;
    }
    else if (water_type == WATER_TYPE_OCEAN)
    {
        // Ocean: gentle distortion
        float2 distortion = calculateWaterDistortion(
            input.uv, glow_time,
            OCEAN_DISTORTION_AMPLITUDE, OCEAN_DISTORTION_FREQUENCY
        );
        animatedUV = input.uv + distortion;
    }
    else // Lake
    {
        // Lakes: subtle distortion (less than ocean)
        float2 distortion = calculateWaterDistortion(
            input.uv, glow_time,
            LAKE_DISTORTION_AMPLITUDE, LAKE_DISTORTION_FREQUENCY
        );
        animatedUV = input.uv + distortion;
    }

    // ==========================================================================
    // Surface Normal & Lighting
    // ==========================================================================

    // Calculate perturbed normal for surface highlights
    float3 normal = calculatePerturbedNormal(animatedUV, glow_time);

    // Calculate toon lighting
    float lightMult = calculateToonLighting(normal, sun_direction);

    // ==========================================================================
    // Base Color
    // ==========================================================================

    // Very dark blue/teal base (barely visible without glow)
    float3 litColor = base_color.rgb * lerp(ambient_strength, 1.0, lightMult);

    // ==========================================================================
    // Shoreline Emissive Glow
    // ==========================================================================

    // Get emissive color for this water type
    float4 emissiveData = getEmissiveForWaterType(water_type);
    float3 emissiveColor = emissiveData.rgb;
    float baseEmissiveIntensity = emissiveData.a;

    // Calculate emissive pulse
    float pulseMult = calculateEmissivePulse(water_type, glow_time);

    // Shoreline glow: boosted at shore_factor = 1.0
    // Interior water gets base emissive, shoreline gets boosted
    float shoreFactor = input.shore_factor;
    float shoreBoost = lerp(1.0, SHORE_GLOW_BOOST, shoreFactor);

    // Final emissive intensity
    float finalEmissiveIntensity = baseEmissiveIntensity * pulseMult * shoreBoost;

    // Emissive contribution
    float3 emissive = emissiveColor * finalEmissiveIntensity;

    // ==========================================================================
    // Final Color Composition
    // ==========================================================================

    // Final color = lit base + emissive
    float3 finalColor = litColor + emissive;
    finalColor = saturate(finalColor);

    // Semi-transparent alpha (0.7-0.8)
    float alpha = base_color.a;

    // Output main color with transparency
    output.color = float4(finalColor, alpha);

    // ==========================================================================
    // Bloom Extraction (MRT)
    // ==========================================================================

    // Output emissive for bloom extraction
    // Only output if shore factor is significant or intensity is high enough
    float bloomThreshold = 0.1;
    if (finalEmissiveIntensity > bloomThreshold || shoreFactor > 0.3)
    {
        // Boost shore emissive for bloom
        float bloomIntensity = finalEmissiveIntensity * (1.0 + shoreFactor);
        output.emissive = float4(emissive * (1.0 + shoreFactor * 0.5), bloomIntensity);
    }
    else
    {
        output.emissive = float4(0.0, 0.0, 0.0, 0.0);
    }

    return output;
}

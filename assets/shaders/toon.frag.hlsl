// Toon Fragment Shader (HLSL SM 6.0)
// Implements stepped/banded lighting for toon shading with alien bioluminescent aesthetic.
//
// Features:
// - 4 discrete lighting bands (lit, mid, shadow, deep shadow)
// - Shadow colors shift toward purple/teal per alien aesthetic
// - World-space directional light (fixed alien sun direction)
// - Configurable ambient term for minimum readability
// - Emissive term added unaffected by lighting bands
// - Shadow mapping from directional light
// - No specular highlights (intentional for toon style)
//
// Resource bindings (per SDL_GPU conventions):
// - Uniform buffer 0 (space3): Lighting parameters
// - Sampler 0 (space2): Shadow map sampler
// - Texture 0 (space2): Shadow map

// Lighting parameters uniform buffer (fragment uniform slot 0 -> space3 per SDL_GPU convention)
cbuffer LightingUBO : register(b0, space3)
{
    float3 sunDirection;       // World-space light direction (normalized, points toward light)
    float  globalAmbient;      // Global ambient strength (0.05-0.1 recommended)
    float3 ambientColor;       // Ambient color (shifted for bioluminescent environment)
    float  shadowEnabled;      // 1.0 if shadows enabled, 0.0 if disabled
    float3 deepShadowColor;    // Deep shadow tint color (#2A1B3D = 0.165, 0.106, 0.239)
    float  shadowIntensity;    // Shadow darkness (0.0-1.0, higher = darker shadows)
    float3 shadowTintColor;    // Shadow tint toward teal (0.1, 0.2, 0.25)
    float  shadowBias;         // Depth comparison bias
    float3 shadowColor;        // Color tint applied to shadowed areas (purple)
    float  shadowSoftness;     // Shadow edge softness (0.0 = hard, 1.0 = soft)
};

// Shadow map sampling
SamplerComparisonState shadowSampler : register(s0, space2);
Texture2D<float> shadowMap : register(t0, space2);

// Fragment input (from vertex shader)
struct PSInput
{
    float4 clipPosition   : SV_Position;
    float3 worldPosition  : TEXCOORD0;
    float3 worldNormal    : TEXCOORD1;
    float2 uv             : TEXCOORD2;
    float4 baseColor      : COLOR0;
    float4 emissiveColor  : COLOR1;
    float  ambientStrength: TEXCOORD3;
    float4 lightSpacePos  : TEXCOORD4;  // Position in light space for shadow mapping
};

// Fragment output
struct PSOutput
{
    float4 color : SV_Target0;
};

// Quantize lighting into discrete bands
// Returns: band index (0 = deep shadow, 1 = shadow, 2 = mid, 3 = lit)
int quantizeLightingBand(float NdotL)
{
    // Remap from [-1, 1] to [0, 1]
    float intensity = NdotL * 0.5 + 0.5;

    // 4-band thresholds:
    // Deep shadow: intensity < 0.2
    // Shadow:      0.2 <= intensity < 0.4
    // Mid:         0.4 <= intensity < 0.7
    // Lit:         intensity >= 0.7
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
    // Stepped intensity values for each band
    // These create the characteristic toon shading "steps"
    switch (band)
    {
        case 0: return 0.15;  // Deep shadow - very dark
        case 1: return 0.35;  // Shadow
        case 2: return 0.65;  // Mid
        case 3: return 1.0;   // Lit - full brightness
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
            // Deep shadow: strong shift toward deep purple (#2A1B3D)
            result = lerp(baseColor, deepShadow, 0.7);
            break;
        case 1:
            // Shadow: moderate shift toward teal
            result = lerp(baseColor, shadowTint, 0.4);
            break;
        case 2:
            // Mid: subtle color shift
            result = lerp(baseColor, shadowTint, 0.15);
            break;
        case 3:
            // Lit: no color shift
            break;
    }

    return result;
}

// Calculate shadow factor using PCF (Percentage Closer Filtering)
// Returns 1.0 for fully lit, 0.0 for fully shadowed
float calculateShadow(float4 lightSpacePos, float NdotL)
{
    // Perspective divide to get NDC coordinates
    float3 projCoords = lightSpacePos.xyz / lightSpacePos.w;

    // Transform from NDC [-1,1] to texture coordinates [0,1]
    // Note: Y is inverted for texture sampling
    projCoords.x = projCoords.x * 0.5 + 0.5;
    projCoords.y = projCoords.y * -0.5 + 0.5;

    // Check if position is outside shadow map bounds
    if (projCoords.x < 0.0 || projCoords.x > 1.0 ||
        projCoords.y < 0.0 || projCoords.y > 1.0 ||
        projCoords.z < 0.0 || projCoords.z > 1.0)
    {
        return 1.0;  // Outside shadow frustum = no shadow
    }

    // Apply slope-scaled bias based on surface angle to light
    float bias = shadowBias + shadowBias * (1.0 - abs(NdotL));

    // Current depth from light's perspective
    float currentDepth = projCoords.z - bias;

    // Simple shadow map comparison
    // Using comparison sampler for hardware PCF
    float shadow = shadowMap.SampleCmpLevelZero(shadowSampler, projCoords.xy, currentDepth);

    return shadow;
}

// Toon-appropriate shadow blending
// Applies shadow with clean edges appropriate for toon style
float3 applyShadowToColor(float3 color, float shadowFactor, float intensity, float3 shadowTint)
{
    // Lerp between shadowed and unshadowed based on shadow factor
    float3 shadowedColor = lerp(color, shadowTint, 0.5);  // Tint toward shadow color
    shadowedColor *= (1.0 - intensity);  // Darken

    return lerp(shadowedColor, color, shadowFactor);
}

PSOutput main(PSInput input)
{
    PSOutput output;

    // Normalize interpolated normal
    float3 normal = normalize(input.worldNormal);

    // Calculate N dot L (sunDirection points toward light, so no negation needed)
    float NdotL = dot(normal, sunDirection);

    // Quantize into discrete lighting bands
    int band = quantizeLightingBand(NdotL);

    // Get stepped lighting multiplier
    float lightMultiplier = getBandMultiplier(band);

    // Apply shadow color shifting for alien aesthetic
    float3 shiftedColor = applyShadowColorShift(
        input.baseColor.rgb,
        band,
        deepShadowColor,
        shadowTintColor
    );

    // Calculate shadow factor
    float shadowFactor = 1.0;
    if (shadowEnabled > 0.5)
    {
        shadowFactor = calculateShadow(input.lightSpacePos, NdotL);

        // For toon rendering, we want relatively clean shadow edges
        // Apply a slight step function to sharpen the shadow edge
        if (shadowSoftness < 0.5)
        {
            // Hard edges for toon style
            shadowFactor = shadowFactor > 0.5 ? 1.0 : 0.0;
        }
    }

    // Calculate diffuse contribution with stepped lighting
    float3 diffuse = shiftedColor * lightMultiplier;

    // Apply shadow to diffuse (shadows affect direct lighting, not ambient)
    if (shadowEnabled > 0.5 && shadowFactor < 1.0)
    {
        diffuse = applyShadowToColor(diffuse, shadowFactor, shadowIntensity, shadowColor);
    }

    // Determine effective ambient strength
    // Use per-instance override if provided (> 0), otherwise use global
    float ambient = (input.ambientStrength > 0.0) ? input.ambientStrength : globalAmbient;

    // Calculate ambient contribution
    // Ambient uses the original base color (no shadow shift) to maintain readability
    // Shadows don't affect ambient light (provides minimum visibility in dark environment)
    float3 ambientContribution = input.baseColor.rgb * ambientColor * ambient;

    // Combine lighting (ambient + diffuse)
    float3 litColor = ambientContribution + diffuse;

    // Add emissive term (unaffected by lighting bands)
    // emissiveColor.a contains intensity multiplier
    float3 emissive = input.emissiveColor.rgb * input.emissiveColor.a;

    // Final color = lit color + emissive (clamped to prevent HDR overflow)
    float3 finalColor = litColor + emissive;
    finalColor = saturate(finalColor);  // Clamp to [0, 1]

    // Output with base alpha
    output.color = float4(finalColor, input.baseColor.a);

    return output;
}

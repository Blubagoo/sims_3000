// Bloom Bright Pixel Extraction Fragment Shader (HLSL SM 6.0)
// Extracts bright pixels from scene color for bloom effect.
//
// Conservative threshold tuned for dark bioluminescent environment.
// Handles full emissive color range (cyan, green, amber, magenta).
//
// Resource bindings (per SDL_GPU conventions):
// - Texture slot 0 (space2): Scene color texture
// - Sampler slot 0 (space2): Linear sampler
// - Uniform buffer 0 (space3): Bloom extraction parameters

// Bloom extraction parameters
cbuffer BloomExtractUBO : register(b0, space3)
{
    float threshold;      // Brightness threshold (0.7 default, conservative for dark env)
    float softKnee;       // Soft threshold knee (0.5 for smoother falloff)
    float intensity;      // Pre-extraction intensity multiplier
    float _padding;       // Align to 16 bytes
};

// Texture samplers
Texture2D<float4> sceneTexture : register(t0, space2);
SamplerState linearSampler : register(s0, space2);

struct PSInput
{
    float4 position : SV_Position;
    float2 texCoord : TEXCOORD0;
};

struct PSOutput
{
    float4 color : SV_Target0;
};

// Calculate luminance using standard sRGB coefficients
// This properly weights the bioluminescent color range
float calculateLuminance(float3 color)
{
    // Rec. 709 coefficients for linear sRGB
    return dot(color, float3(0.2126, 0.7152, 0.0722));
}

// Soft threshold function for smoother bloom transition
// Avoids hard cutoff that creates banding artifacts
float softThreshold(float luminance, float thresh, float knee)
{
    // Soft knee curve: smoothstep from (threshold - knee) to (threshold + knee)
    float softLow = thresh - knee;
    float softHigh = thresh + knee;

    if (luminance <= softLow)
        return 0.0;
    if (luminance >= softHigh)
        return luminance - thresh;

    // Quadratic interpolation in the knee region
    float x = luminance - softLow;
    float width = softHigh - softLow;
    float normalized = x / width;
    return normalized * normalized * width * 0.5;
}

PSOutput main(PSInput input)
{
    PSOutput output;

    // Sample scene color
    float4 sceneColor = sceneTexture.Sample(linearSampler, input.texCoord);

    // Calculate luminance
    float luminance = calculateLuminance(sceneColor.rgb);

    // Apply soft threshold to extract bright pixels
    // The soft knee prevents harsh transitions
    float contribution = softThreshold(luminance, threshold, softKnee);

    // Calculate how much of the original color to keep
    // This preserves the color hue of emissive surfaces
    float brightness = max(luminance, 0.0001); // Prevent division by zero
    float multiplier = contribution / brightness;

    // Extract bright pixels with intensity scaling
    float3 bloomColor = sceneColor.rgb * multiplier * intensity;

    // Clamp to prevent fireflies (extremely bright pixels causing artifacts)
    bloomColor = min(bloomColor, 10.0);

    output.color = float4(bloomColor, 1.0);

    return output;
}

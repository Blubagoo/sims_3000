// Bloom Composite Fragment Shader (HLSL SM 6.0)
// Additively blends bloom with original scene color.
//
// Bloom is mandatory per canon (MIN_INTENSITY = 0.1f ensures bloom is never zero).
// This shader performs the final composite for the bioluminescent glow effect.
//
// Resource bindings (per SDL_GPU conventions):
// - Texture slot 0 (space2): Original scene color texture
// - Texture slot 1 (space2): Blurred bloom texture
// - Sampler slot 0 (space2): Linear sampler for bloom (soft upscale)
// - Sampler slot 1 (space2): Point sampler for scene (no filtering needed)
// - Uniform buffer 0 (space3): Composite parameters

// Composite parameters
cbuffer BloomCompositeUBO : register(b0, space3)
{
    float bloomIntensity; // Bloom strength multiplier (MIN_INTENSITY = 0.1f minimum)
    float exposure;       // Optional exposure adjustment (1.0 = no change)
    float _padding0;
    float _padding1;
};

// Texture samplers
Texture2D<float4> sceneTexture : register(t0, space2);
Texture2D<float4> bloomTexture : register(t1, space2);
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

PSOutput main(PSInput input)
{
    PSOutput output;

    // Sample original scene
    float4 sceneColor = sceneTexture.Sample(linearSampler, input.texCoord);

    // Sample bloom (upscaled from lower resolution via linear filtering)
    float4 bloomColor = bloomTexture.Sample(linearSampler, input.texCoord);

    // Additive blend: scene + bloom * intensity
    // The bloom adds a glow halo around bright (emissive) areas
    float3 finalColor = sceneColor.rgb + bloomColor.rgb * bloomIntensity;

    // Apply exposure adjustment (for HDR tone mapping compatibility)
    finalColor *= exposure;

    // No explicit tone mapping here - that's handled by the display pipeline
    // We preserve HDR values for potential future HDR display support

    // Clamp to prevent overflow in SDR displays
    // Keep some headroom for bloom to glow brightly
    finalColor = min(finalColor, 5.0);

    output.color = float4(finalColor, sceneColor.a);

    return output;
}

// Bloom Gaussian Blur Fragment Shader (HLSL SM 6.0)
// Separable Gaussian blur for bloom effect.
//
// Uses 9-tap Gaussian kernel for good quality/performance balance.
// Separable blur: run horizontal pass, then vertical pass.
//
// Resource bindings (per SDL_GPU conventions):
// - Texture slot 0 (space2): Source texture (extraction result or previous blur)
// - Sampler slot 0 (space2): Linear sampler (hardware bilinear for free samples)
// - Uniform buffer 0 (space3): Blur parameters

// Blur parameters
cbuffer BloomBlurUBO : register(b0, space3)
{
    float2 texelSize;     // 1.0 / textureSize
    float2 blurDirection; // (1,0) for horizontal, (0,1) for vertical
};

// Texture samplers
Texture2D<float4> sourceTexture : register(t0, space2);
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

// 9-tap Gaussian weights (sigma ~1.5, pre-normalized to sum to 1.0)
// These weights provide a good balance between blur quality and performance
// Kernel: [0.0162, 0.0540, 0.1216, 0.1903, 0.2358, 0.1903, 0.1216, 0.0540, 0.0162]
static const float WEIGHTS[5] = {
    0.2358,  // Center
    0.1903,  // +/- 1 texel
    0.1216,  // +/- 2 texels
    0.0540,  // +/- 3 texels
    0.0162   // +/- 4 texels
};

// Optimization: Use linear filtering to sample between texels
// This allows us to get 2 samples with 1 texture fetch
// Offset positions and combined weights for 5 texture fetches instead of 9
static const float OFFSETS[3] = {
    0.0,     // Center
    1.3846,  // Optimized offset for samples 1-2
    3.2308   // Optimized offset for samples 3-4
};

static const float OPT_WEIGHTS[3] = {
    0.2270,  // Center weight
    0.3162,  // Combined weight for samples 1-2
    0.0702   // Combined weight for samples 3-4
};

PSOutput main(PSInput input)
{
    PSOutput output;

    // Calculate blur step direction
    float2 step = texelSize * blurDirection;

    // Optimized 5-fetch Gaussian blur using hardware linear interpolation
    // This gives us the quality of a 9-tap filter with only 5 texture samples

    // Center sample
    float4 result = sourceTexture.Sample(linearSampler, input.texCoord) * OPT_WEIGHTS[0];

    // Positive and negative samples (symmetric kernel)
    [unroll]
    for (int i = 1; i < 3; ++i)
    {
        float2 offset = step * OFFSETS[i];
        result += sourceTexture.Sample(linearSampler, input.texCoord + offset) * OPT_WEIGHTS[i];
        result += sourceTexture.Sample(linearSampler, input.texCoord - offset) * OPT_WEIGHTS[i];
    }

    output.color = result;

    return output;
}

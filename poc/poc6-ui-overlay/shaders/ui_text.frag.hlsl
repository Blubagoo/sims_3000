// UI Text Fragment Shader (HLSL SM 6.0)
// Samples text atlas and applies color with alpha blending

Texture2D<float4> atlasTexture : register(t0, space2);
SamplerState atlasSampler : register(s0, space2);

struct PSInput
{
    float4 clipPosition : SV_Position;
    float2 texcoord     : TEXCOORD0;
    float4 color        : COLOR0;
};

struct PSOutput
{
    float4 color : SV_Target0;
};

PSOutput main(PSInput input)
{
    PSOutput output;

    // Sample the atlas - SDL_ttf uses alpha channel for glyph coverage
    float4 texColor = atlasTexture.Sample(atlasSampler, input.texcoord);

    // Apply text color with atlas alpha
    output.color = float4(input.color.rgb, input.color.a * texColor.a);

    return output;
}

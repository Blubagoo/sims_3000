// UI Text Vertex Shader (HLSL SM 6.0)
// Renders textured quads for text atlas glyphs

struct VSInput
{
    float2 position : TEXCOORD0;  // Screen-space position (0-1 normalized)
    float2 texcoord : TEXCOORD1;  // Texture coordinates
    float4 color    : TEXCOORD2;  // Text color
};

struct VSOutput
{
    float4 clipPosition : SV_Position;
    float2 texcoord     : TEXCOORD0;
    float4 color        : COLOR0;
};

VSOutput main(VSInput input)
{
    VSOutput output;

    // Convert from [0,1] screen space to [-1,1] clip space
    float2 clipPos = input.position * 2.0 - 1.0;
    clipPos.y = -clipPos.y;

    output.clipPosition = float4(clipPos, 0.0, 1.0);
    output.texcoord = input.texcoord;
    output.color = input.color;

    return output;
}

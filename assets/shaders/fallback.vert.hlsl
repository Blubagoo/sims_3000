// Fallback Vertex Shader (HLSL SM 6.0)
// Minimal solid-color shader used when regular shaders fail to load.
// Renders objects in bright magenta to make shader loading failures obvious.

// Vertex input
struct VSInput
{
    float3 position : TEXCOORD0;
};

// Vertex output
struct VSOutput
{
    float4 clipPosition : SV_Position;
};

// Simple identity transform - assumes clip-space input
VSOutput main(VSInput input)
{
    VSOutput output;
    output.clipPosition = float4(input.position, 1.0);
    return output;
}

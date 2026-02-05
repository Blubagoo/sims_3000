// UI Quad Vertex Shader (HLSL SM 6.0)
// Renders 2D colored quads in screen space for UI elements

// Vertex input - position and color per vertex
struct VSInput
{
    float2 position : TEXCOORD0;  // Screen-space position (0-1 normalized)
    float4 color    : TEXCOORD1;  // RGBA color
};

// Vertex output
struct VSOutput
{
    float4 clipPosition : SV_Position;
    float4 color        : COLOR0;
};

VSOutput main(VSInput input)
{
    VSOutput output;

    // Convert from [0,1] screen space to [-1,1] clip space
    // Y is flipped (0 at top, 1 at bottom in screen space)
    float2 clipPos = input.position * 2.0 - 1.0;
    clipPos.y = -clipPos.y;

    output.clipPosition = float4(clipPos, 0.0, 1.0);
    output.color = input.color;

    return output;
}

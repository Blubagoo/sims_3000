// Debug Bounding Box Fragment Shader (HLSL SM 6.0)
// Simple passthrough fragment shader for wireframe rendering.
//
// Input: Interpolated color from vertex shader
// Output: Final fragment color

struct PSInput
{
    float4 position : SV_Position;
    float4 color : COLOR0;
};

struct PSOutput
{
    float4 color : SV_Target0;
};

PSOutput main(PSInput input)
{
    PSOutput output;

    // Output the vertex color directly
    output.color = input.color;

    return output;
}

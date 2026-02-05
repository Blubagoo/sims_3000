// Outline Fragment Shader (HLSL SM 6.0)
// Outputs solid dark color for outline effect

// Fragment input
struct PSInput
{
    float4 clipPosition : SV_Position;
};

// Fragment output
struct PSOutput
{
    float4 color : SV_Target0;
};

PSOutput main(PSInput input)
{
    PSOutput output;

    // Output solid dark color for outline
    output.color = float4(0.1, 0.1, 0.1, 1.0);

    return output;
}

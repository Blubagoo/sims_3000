// Fallback Fragment Shader (HLSL SM 6.0)
// Outputs solid magenta (1.0, 0.0, 1.0) to indicate shader loading failure.
// This color is chosen because:
// 1. It's highly visible against most backgrounds
// 2. It's the traditional "missing texture" color in game development
// 3. It clearly signals something is wrong

// Fragment output
struct FSOutput
{
    float4 color : SV_Target0;
};

FSOutput main()
{
    FSOutput output;
    // Bright magenta - the universal "something went wrong" color
    output.color = float4(1.0, 0.0, 1.0, 1.0);
    return output;
}

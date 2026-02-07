// Debug Bounding Box Vertex Shader (HLSL SM 6.0)
// Transforms wireframe box vertices to clip space.
//
// Input: Position and color per vertex
// Output: Clip-space position and interpolated color
//
// Usage: Draw LINE_LIST primitives with vertex buffer bound.

// Uniform buffer matching DebugBBoxUBO struct
cbuffer BBoxUniforms : register(b0)
{
    float4x4 viewProjection;  // View-projection matrix
};

struct VSInput
{
    float3 position : TEXCOORD0;  // World-space position
    float4 color : TEXCOORD1;     // RGBA color
};

struct VSOutput
{
    float4 position : SV_Position;
    float4 color : COLOR0;
};

VSOutput main(VSInput input)
{
    VSOutput output;

    // Transform to clip space
    output.position = mul(viewProjection, float4(input.position, 1.0));

    // Pass through color
    output.color = input.color;

    return output;
}

// Fullscreen Quad Vertex Shader (HLSL SM 6.0)
// Generates fullscreen triangle from vertex ID (no vertex buffer needed).
//
// Technique: Uses a single oversized triangle covering the entire screen.
// This avoids vertex buffer setup and is more efficient than a quad.
//
// Usage: Draw 3 vertices with no vertex buffer bound.
//   SDL_DrawGPUPrimitives(pass, 3, 1, 0, 0);

struct VSOutput
{
    float4 position : SV_Position;
    float2 texCoord : TEXCOORD0;
};

VSOutput main(uint vertexID : SV_VertexID)
{
    VSOutput output;

    // Generate fullscreen triangle vertices from vertex ID
    // Vertex 0: (-1, -1) -> (0, 1) UV
    // Vertex 1: ( 3, -1) -> (2, 1) UV
    // Vertex 2: (-1,  3) -> (0, -1) UV
    float2 pos = float2((vertexID << 1) & 2, vertexID & 2);
    output.position = float4(pos * 2.0 - 1.0, 0.0, 1.0);

    // Flip Y for correct UV orientation (OpenGL/Vulkan convention)
    output.texCoord = float2(pos.x, 1.0 - pos.y);

    return output;
}

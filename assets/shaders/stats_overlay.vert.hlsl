/**
 * @file stats_overlay.vert.hlsl
 * @brief Vertex shader for stats overlay background quad.
 *
 * Transforms normalized screen coordinates [0,1] to clip space [-1,1].
 * Y is flipped for screen-space UI rendering.
 */

struct VSInput {
    float2 position : TEXCOORD0;  // Position in [0,1] normalized coords
    float4 color : TEXCOORD1;     // RGBA color
};

struct VSOutput {
    float4 position : SV_Position;
    float4 color : COLOR0;
};

VSOutput main(VSInput input) {
    VSOutput output;

    // Convert from [0,1] to [-1,1] clip space
    // Y is flipped: 0 at top, 1 at bottom -> -1 at top, 1 at bottom
    output.position.x = input.position.x * 2.0 - 1.0;
    output.position.y = input.position.y * 2.0 - 1.0;  // Flip Y
    output.position.z = 0.0;
    output.position.w = 1.0;

    output.color = input.color;

    return output;
}

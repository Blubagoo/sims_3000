// Toon Vertex Shader (HLSL SM 6.0)
// Transforms vertices with per-instance model matrices and passes data to fragment shader

// View-Projection uniform buffer (vertex uniform slot 0 -> space1 per SDL_GPU convention)
cbuffer ViewProjectionUBO : register(b0, space1)
{
    float4x4 viewProjection;
};

// Per-instance data stored in a structured buffer (vertex storage slot 0 -> t0, space0)
struct InstanceData
{
    float4x4 model;
    float4 color;
};
StructuredBuffer<InstanceData> instances : register(t0, space0);

// Vertex input
struct VSInput
{
    float3 position : TEXCOORD0;
    float3 normal   : TEXCOORD1;
    uint instanceID : SV_InstanceID;
};

// Vertex output / Fragment input
struct VSOutput
{
    float4 clipPosition : SV_Position;
    float3 worldPosition : TEXCOORD0;
    float3 worldNormal   : TEXCOORD1;
    float4 instanceColor : COLOR0;
};

VSOutput main(VSInput input)
{
    VSOutput output;

    // Get instance data
    InstanceData instance = instances[input.instanceID];

    // Transform position to world space
    float4 worldPos = mul(instance.model, float4(input.position, 1.0));
    output.worldPosition = worldPos.xyz;

    // Transform to clip space
    output.clipPosition = mul(viewProjection, worldPos);

    // Transform normal to world space (using inverse transpose for non-uniform scaling)
    // For now, assume uniform scaling so we can use the upper-left 3x3 of the model matrix
    float3x3 normalMatrix = (float3x3)instance.model;
    output.worldNormal = normalize(mul(normalMatrix, input.normal));

    // Pass through instance color
    output.instanceColor = instance.color;

    return output;
}

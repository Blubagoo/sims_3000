// Outline Vertex Shader (HLSL SM 6.0)
// Extrudes vertices along normals for inverted hull outline technique

// View-Projection uniform buffer (vertex uniform slot 0 -> space1 per SDL_GPU convention)
cbuffer ViewProjectionUBO : register(b0, space1)
{
    float4x4 viewProjection;
};

// Outline parameters uniform buffer (vertex uniform slot 1 -> space1 per SDL_GPU convention)
cbuffer OutlineParams : register(b1, space1)
{
    float outlineThickness; // Default: 0.02
    float3 _padding;        // Padding to align to 16 bytes
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

// Vertex output
struct VSOutput
{
    float4 clipPosition : SV_Position;
};

VSOutput main(VSInput input)
{
    VSOutput output;

    // Get instance data
    InstanceData instance = instances[input.instanceID];

    // Transform normal to world space
    float3x3 normalMatrix = (float3x3)instance.model;
    float3 worldNormal = normalize(mul(normalMatrix, input.normal));

    // Transform position to world space
    float4 worldPos = mul(instance.model, float4(input.position, 1.0));

    // Extrude vertex along normal (inverted hull technique)
    // The outline thickness controls how far we push vertices outward
    worldPos.xyz += worldNormal * outlineThickness;

    // Transform to clip space
    output.clipPosition = mul(viewProjection, worldPos);

    return output;
}

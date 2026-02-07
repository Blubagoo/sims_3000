// Toon Vertex Shader (HLSL SM 6.0)
// Transforms vertices with MVP matrices and passes data to fragment shader.
// Supports position, normal, and UV passthrough for toon rendering pipeline.
// Outputs light-space position for shadow mapping.
//
// Resource bindings (per SDL_GPU conventions):
// - Uniform buffer 0 (space1): ViewProjection matrix + LightViewProjection matrix
// - Storage buffer 0 (space0): Per-instance data (model matrix, colors, emissive)

// View-Projection uniform buffer (vertex uniform slot 0 -> space1 per SDL_GPU convention)
cbuffer ViewProjectionUBO : register(b0, space1)
{
    float4x4 viewProjection;
    float4x4 lightViewProjection;  // For shadow mapping
};

// Per-instance data stored in a structured buffer (vertex storage slot 0 -> t0, space0)
struct InstanceData
{
    float4x4 model;          // Model transformation matrix
    float4 baseColor;        // Base diffuse color (RGB) + alpha
    float4 emissiveColor;    // Emissive color (RGB) + intensity
    float ambientStrength;   // Per-instance ambient override (0 = use global)
    float3 _padding;         // Padding for 16-byte alignment
};
StructuredBuffer<InstanceData> instances : register(t0, space0);

// Vertex input
struct VSInput
{
    float3 position : TEXCOORD0;
    float3 normal   : TEXCOORD1;
    float2 uv       : TEXCOORD2;
    uint instanceID : SV_InstanceID;
};

// Vertex output / Fragment input
struct VSOutput
{
    float4 clipPosition   : SV_Position;
    float3 worldPosition  : TEXCOORD0;
    float3 worldNormal    : TEXCOORD1;
    float2 uv             : TEXCOORD2;
    float4 baseColor      : COLOR0;
    float4 emissiveColor  : COLOR1;
    float  ambientStrength: TEXCOORD3;
    float4 lightSpacePos  : TEXCOORD4;  // Position in light space for shadow mapping
};

VSOutput main(VSInput input)
{
    VSOutput output;

    // Get instance data
    InstanceData instance = instances[input.instanceID];

    // Transform position to world space
    float4 worldPos = mul(instance.model, float4(input.position, 1.0));
    output.worldPosition = worldPos.xyz;

    // Transform to clip space (MVP transform)
    output.clipPosition = mul(viewProjection, worldPos);

    // Transform normal to world space
    // For uniform scaling, we use the upper-left 3x3 of the model matrix.
    // For non-uniform scaling, proper inverse-transpose would be needed.
    float3x3 normalMatrix = (float3x3)instance.model;
    output.worldNormal = normalize(mul(normalMatrix, input.normal));

    // Pass through UV coordinates
    output.uv = input.uv;

    // Pass through instance colors
    output.baseColor = instance.baseColor;
    output.emissiveColor = instance.emissiveColor;
    output.ambientStrength = instance.ambientStrength;

    // Transform to light space for shadow mapping
    output.lightSpacePos = mul(lightViewProjection, worldPos);

    return output;
}

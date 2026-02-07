// Water Vertex Shader (HLSL SM 6.0)
// Specialized vertex shader for water surface mesh rendering.
//
// Transforms water vertices with MVP matrices and passes water-specific
// data to the fragment shader for animated effects and shoreline glow.
//
// Key features:
// - Reads shore_factor from vertex for shoreline glow effects
// - Passes water_body_id for per-body customization in fragment shader
// - UV coordinates for scrolling/distortion animations
// - Light space position for shadow mapping (optional)
//
// Resource bindings (per SDL_GPU conventions):
// - Uniform buffer 0 (space1): ViewProjection matrix + LightViewProjection matrix

// =============================================================================
// Uniform Buffers
// =============================================================================

// View-Projection uniform buffer (vertex uniform slot 0 -> space1 per SDL_GPU convention)
cbuffer ViewProjectionUBO : register(b0, space1)
{
    float4x4 viewProjection;
    float4x4 lightViewProjection;  // For shadow mapping
};

// =============================================================================
// Vertex Input
// =============================================================================

// Matches WaterVertex layout from WaterMesh.h (28 bytes)
// - position: vec3 (12 bytes, offset 0)
// - shore_factor: float (4 bytes, offset 12)
// - water_body_id: uint16 + padding (4 bytes, offset 16)
// - uv: vec2 (8 bytes, offset 20)
struct VSInput
{
    float3 position     : TEXCOORD0;  // World-space position
    float  shore_factor : TEXCOORD1;  // Shoreline proximity (0.0-1.0)
    uint2  body_data    : TEXCOORD2;  // [water_body_id, padding] (USHORT2)
    float2 uv           : TEXCOORD3;  // Texture coordinates
};

// =============================================================================
// Vertex Output / Fragment Input
// =============================================================================

struct VSOutput
{
    float4 clipPosition   : SV_Position;
    float3 worldPosition  : TEXCOORD0;
    float2 uv             : TEXCOORD1;
    float  shore_factor   : TEXCOORD2;
    nointerpolation uint water_body_id : TEXCOORD3;  // Flat interpolation - no blending
    float4 lightSpacePos  : TEXCOORD4;              // Position in light space for shadows
};

// =============================================================================
// Main Vertex Shader
// =============================================================================

VSOutput main(VSInput input)
{
    VSOutput output;

    // Position is already in world space (water mesh is not instanced)
    output.worldPosition = input.position;

    // Transform to clip space
    output.clipPosition = mul(viewProjection, float4(input.position, 1.0));

    // Pass through UV coordinates for wave animation
    output.uv = input.uv;

    // Pass through shore factor for shoreline glow
    output.shore_factor = input.shore_factor;

    // Extract water body ID (first component of body_data)
    output.water_body_id = input.body_data.x;

    // Transform to light space for shadow mapping
    output.lightSpacePos = mul(lightViewProjection, float4(input.position, 1.0));

    return output;
}

// Terrain Vertex Shader (HLSL SM 6.0)
// Specialized vertex shader for terrain mesh rendering.
//
// Transforms terrain vertices with MVP matrices and passes terrain-specific
// data to the fragment shader for palette-based coloring and glow effects.
//
// Key differences from generic toon.vert.hlsl:
// - Reads terrain_type and elevation from vertex attributes (not instance buffer)
// - No per-instance data - terrain mesh is not instanced
// - Outputs terrain_type for fragment shader palette lookup
//
// Resource bindings (per SDL_GPU conventions):
// - Uniform buffer 0 (space1): ViewProjection matrix + LightViewProjection matrix

// View-Projection uniform buffer (vertex uniform slot 0 -> space1 per SDL_GPU convention)
cbuffer ViewProjectionUBO : register(b0, space1)
{
    float4x4 viewProjection;
    float4x4 lightViewProjection;  // For shadow mapping
};

// Vertex input (matches TerrainVertex layout from TerrainVertex.h)
// Position, Normal, TerrainData (type + elevation), UV, TileCoord
struct VSInput
{
    float3 position     : TEXCOORD0;  // World-space position
    float3 normal       : TEXCOORD1;  // Surface normal
    uint2  terrainData  : TEXCOORD2;  // [terrain_type, elevation] (UBYTE2 unpacked to uint2)
    float2 uv           : TEXCOORD3;  // Texture coordinates
    float2 tileCoord    : TEXCOORD4;  // Tile position for effects
};

// Vertex output / Fragment input
struct VSOutput
{
    float4 clipPosition   : SV_Position;
    float3 worldPosition  : TEXCOORD0;
    float3 worldNormal    : TEXCOORD1;
    float2 uv             : TEXCOORD2;
    nointerpolation uint terrainType : TEXCOORD3;  // Flat interpolation - no blending
    float  elevation      : TEXCOORD4;             // Normalized elevation for effects
    float2 tileCoord      : TEXCOORD5;             // Tile position
    float4 lightSpacePos  : TEXCOORD6;             // Position in light space for shadows
};

VSOutput main(VSInput input)
{
    VSOutput output;

    // Position is already in world space (terrain mesh is not instanced)
    output.worldPosition = input.position;

    // Transform to clip space
    output.clipPosition = mul(viewProjection, float4(input.position, 1.0));

    // Normal is already in world space
    output.worldNormal = normalize(input.normal);

    // Pass through UV coordinates
    output.uv = input.uv;

    // Extract terrain type (first component of terrainData)
    // Using flat interpolation - no blending between terrain types
    output.terrainType = input.terrainData.x;

    // Normalize elevation to [0, 1] range (0-31 -> 0.0-1.0)
    output.elevation = float(input.terrainData.y) / 31.0;

    // Pass through tile coordinates for shader effects
    output.tileCoord = input.tileCoord;

    // Transform to light space for shadow mapping
    output.lightSpacePos = mul(lightViewProjection, float4(input.position, 1.0));

    return output;
}

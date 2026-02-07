// Debug Grid Vertex Shader (HLSL SM 6.0)
// Generates a grid quad covering the entire map on the XY plane (Z=0).
// Projects grid vertices using the view-projection matrix.
//
// The grid is rendered as a fullscreen quad on the terrain plane,
// with grid lines computed procedurally in the fragment shader.
//
// Usage: Draw 6 vertices with no vertex buffer bound.
//   SDL_DrawGPUPrimitives(pass, 6, 1, 0, 0);

// Uniform buffer matching DebugGridUBO struct
cbuffer GridUniforms : register(b0)
{
    float4x4 viewProjection;      // View-projection matrix
    float4 fineGridColor;         // Fine grid color (RGBA)
    float4 coarseGridColor;       // Coarse grid color (RGBA)
    float2 mapSize;               // Map dimensions (width, height) in tiles
    float fineGridSpacing;        // Fine grid tile spacing
    float coarseGridSpacing;      // Coarse grid tile spacing
    float lineThickness;          // Line thickness in world units
    float opacity;                // Overall opacity (for tilt fading)
    float cameraDistance;         // Camera distance for LOD
    float _padding;               // Align to 16 bytes
};

struct VSOutput
{
    float4 position : SV_Position;
    float2 worldPos : TEXCOORD0;      // World XY position for grid computation
    float4 fineColor : COLOR0;         // Fine grid color with opacity
    float4 coarseColor : COLOR1;       // Coarse grid color with opacity
    float fineSpacing : TEXCOORD1;     // Fine grid spacing
    float coarseSpacing : TEXCOORD2;   // Coarse grid spacing
    float thickness : TEXCOORD3;       // Line thickness
    float camDistance : TEXCOORD4;     // Camera distance
};

VSOutput main(uint vertexID : SV_VertexID)
{
    VSOutput output;

    // Generate quad vertices covering the map
    // Quad corners: (0,0), (mapSize.x, 0), (mapSize.x, mapSize.y), (0, mapSize.y)
    // Two triangles: [0,1,2] and [0,2,3]
    //
    // Vertex layout for 6 vertices:
    // 0: (0, 0)           - bottom-left
    // 1: (mapSize.x, 0)   - bottom-right
    // 2: (mapSize.x, mapSize.y) - top-right
    // 3: (0, 0)           - bottom-left (repeated)
    // 4: (mapSize.x, mapSize.y) - top-right (repeated)
    // 5: (0, mapSize.y)   - top-left

    float2 corners[6];
    corners[0] = float2(0.0, 0.0);
    corners[1] = float2(mapSize.x, 0.0);
    corners[2] = float2(mapSize.x, mapSize.y);
    corners[3] = float2(0.0, 0.0);
    corners[4] = float2(mapSize.x, mapSize.y);
    corners[5] = float2(0.0, mapSize.y);

    float2 worldXY = corners[vertexID];

    // Position on the terrain plane (Z = 0 in world space)
    // Note: Y-up coordinate system, grid is on XZ plane
    // We use X as east, Y as south (game convention), Z as elevation
    // For rendering, we map: X->X, Y->Z, elevation->Y
    float3 worldPos = float3(worldXY.x, 0.0, worldXY.y);

    // Transform to clip space
    output.position = mul(viewProjection, float4(worldPos, 1.0));

    // Pass world XY for fragment shader grid computation
    output.worldPos = worldXY;

    // Apply overall opacity to colors
    output.fineColor = float4(fineGridColor.rgb, fineGridColor.a * opacity);
    output.coarseColor = float4(coarseGridColor.rgb, coarseGridColor.a * opacity);

    // Pass grid parameters
    output.fineSpacing = fineGridSpacing;
    output.coarseSpacing = coarseGridSpacing;
    output.thickness = lineThickness;
    output.camDistance = cameraDistance;

    return output;
}

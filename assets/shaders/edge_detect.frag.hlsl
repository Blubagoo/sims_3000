// Edge Detection Fragment Shader (HLSL SM 6.0)
// Screen-space Sobel edge detection for cartoon outlines.
//
// Primary signal: Normal discontinuities
// Secondary signal: Linearized depth discontinuities
//
// Terrain-specific tuning (Ticket 3-035):
// - Cliff edges: Bold outlines from strong normal discontinuities (normal.y < 0.5)
// - Water shorelines: Visible outlines at water/land normal transitions
// - Gentle slopes: Depth threshold scales with distance to avoid noise
// - Terrain type boundaries: Color contrast provides separation; edge detection is bonus
//
// Handles perspective projection with non-linear depth buffer by
// linearizing depth values before Sobel filtering.
//
// Resource bindings (per SDL_GPU conventions):
// - Texture slot 0 (space2): Scene color texture
// - Texture slot 1 (space2): Normal buffer (view-space normals)
// - Texture slot 2 (space2): Depth buffer
// - Sampler slot 0 (space2): Point sampler for texel-accurate sampling
// - Uniform buffer 0 (space3): Edge detection parameters

// Edge detection parameters
cbuffer EdgeDetectionUBO : register(b0, space3)
{
    float4 outlineColor;      // Outline color (RGB) + alpha multiplier
    float2 texelSize;         // 1.0 / textureSize
    float normalThreshold;    // Threshold for normal discontinuities
    float depthThreshold;     // Threshold for depth discontinuities
    float nearPlane;          // Camera near plane (for depth linearization)
    float farPlane;           // Camera far plane (for depth linearization)
    float edgeThickness;      // Edge thickness in pixels (1.0 = single pixel)
    float _padding;           // Align to 16 bytes
};

// Texture samplers
Texture2D<float4> sceneColorTexture : register(t0, space2);
Texture2D<float4> normalTexture : register(t1, space2);
Texture2D<float> depthTexture : register(t2, space2);
SamplerState pointSampler : register(s0, space2);

struct PSInput
{
    float4 position : SV_Position;
    float2 texCoord : TEXCOORD0;
};

struct PSOutput
{
    float4 color : SV_Target0;
};

// Linearize depth from non-linear depth buffer (perspective projection)
// z_buffer ranges from 0 (near) to 1 (far) in reversed-z or standard depth
float linearizeDepth(float rawDepth)
{
    // Standard depth (0 = near, 1 = far):
    // Linear depth = near * far / (far - rawDepth * (far - near))
    // This gives linear depth in world units from near to far

    float z = rawDepth;
    return (nearPlane * farPlane) / (farPlane - z * (farPlane - nearPlane));
}

// Normalize linear depth to [0, 1] range for edge detection
float normalizedLinearDepth(float rawDepth)
{
    float linearDepthValue = linearizeDepth(rawDepth);
    // Normalize to [0, 1] where 0 = near plane, 1 = far plane
    return (linearDepthValue - nearPlane) / (farPlane - nearPlane);
}

// Sobel operator kernels
// Horizontal kernel (Gx):
//  -1  0  1
//  -2  0  2
//  -1  0  1
//
// Vertical kernel (Gy):
//  -1 -2 -1
//   0  0  0
//   1  2  1

// Calculate Sobel gradient magnitude for a single-channel value
float sobelSingle(float samples[9])
{
    // Gx = sum of (kernel * samples) for horizontal
    float gx = samples[0] * (-1.0) + samples[2] * (1.0) +
               samples[3] * (-2.0) + samples[5] * (2.0) +
               samples[6] * (-1.0) + samples[8] * (1.0);

    // Gy = sum of (kernel * samples) for vertical
    float gy = samples[0] * (-1.0) + samples[1] * (-2.0) + samples[2] * (-1.0) +
               samples[6] * (1.0)  + samples[7] * (2.0)  + samples[8] * (1.0);

    return sqrt(gx * gx + gy * gy);
}

// Calculate Sobel gradient magnitude for normals (vec3)
float sobelNormal(float3 samples[9])
{
    // Apply Sobel to each component and combine
    float gx_r = samples[0].x * (-1.0) + samples[2].x * (1.0) +
                 samples[3].x * (-2.0) + samples[5].x * (2.0) +
                 samples[6].x * (-1.0) + samples[8].x * (1.0);
    float gx_g = samples[0].y * (-1.0) + samples[2].y * (1.0) +
                 samples[3].y * (-2.0) + samples[5].y * (2.0) +
                 samples[6].y * (-1.0) + samples[8].y * (1.0);
    float gx_b = samples[0].z * (-1.0) + samples[2].z * (1.0) +
                 samples[3].z * (-2.0) + samples[5].z * (2.0) +
                 samples[6].z * (-1.0) + samples[8].z * (1.0);

    float gy_r = samples[0].x * (-1.0) + samples[1].x * (-2.0) + samples[2].x * (-1.0) +
                 samples[6].x * (1.0)  + samples[7].x * (2.0)  + samples[8].x * (1.0);
    float gy_g = samples[0].y * (-1.0) + samples[1].y * (-2.0) + samples[2].y * (-1.0) +
                 samples[6].y * (1.0)  + samples[7].y * (2.0)  + samples[8].y * (1.0);
    float gy_b = samples[0].z * (-1.0) + samples[1].z * (-2.0) + samples[2].z * (-1.0) +
                 samples[6].z * (1.0)  + samples[7].z * (2.0)  + samples[8].z * (1.0);

    float3 gx = float3(gx_r, gx_g, gx_b);
    float3 gy = float3(gy_r, gy_g, gy_b);

    // Combine gradient magnitudes
    return sqrt(dot(gx, gx) + dot(gy, gy));
}

// =============================================================================
// Terrain-Specific Edge Detection (Ticket 3-035)
// =============================================================================

// Terrain edge detection constants
static const float CLIFF_NORMAL_Y_THRESHOLD = 0.5;   // Cliff when normal.y < this
static const float CLIFF_EDGE_WEIGHT = 1.5;          // Boost factor for cliff edges
static const float SHORELINE_EDGE_WEIGHT = 1.25;     // Boost factor for shorelines
static const float GENTLE_SLOPE_ANGLE_COS = 0.94;    // cos(20 degrees) - gentle slope threshold
static const float DISTANCE_SCALE_MIN = 0.5;         // Min scaling at far distance
static const float DISTANCE_SCALE_MAX = 1.0;         // Max scaling at near distance

// Calculate edge weight multiplier based on terrain characteristics
// Uses center normal (samples[4]) to identify terrain features
float calculateTerrainEdgeWeight(float3 centerNormal, float normalEdge)
{
    float weight = 1.0;

    // Check if this is a cliff edge (normal pointing mostly horizontal)
    // Cliffs have normal.y close to 0 (vertical faces)
    if (abs(centerNormal.y) < CLIFF_NORMAL_Y_THRESHOLD)
    {
        // This is a cliff face - boost edge weight for bold outlines
        // The lower the normal.y, the steeper the cliff, the bolder the edge
        float cliffFactor = 1.0 - abs(centerNormal.y) / CLIFF_NORMAL_Y_THRESHOLD;
        weight *= lerp(1.0, CLIFF_EDGE_WEIGHT, cliffFactor);
    }

    // Detect shoreline by checking for strong normal discontinuity
    // combined with near-horizontal normals (water is always flat)
    // If normal edge is high and center normal is near-vertical, likely a shoreline
    if (normalEdge > 0.3 && abs(centerNormal.y) > 0.9)
    {
        // Potential shoreline - apply modest boost
        weight *= SHORELINE_EDGE_WEIGHT;
    }

    return weight;
}

// Scale depth threshold based on distance to reduce noise on gentle distant slopes
// At far distances, depth differences are smaller in screen space
float scaleDepthThresholdByDistance(float linearDepth)
{
    // Normalize depth to [0, 1] range based on near/far planes
    float normalizedDist = saturate((linearDepth - nearPlane) / (farPlane - nearPlane));

    // At near distances: use full threshold (1.0 scale)
    // At far distances: use reduced threshold (0.5 scale) to avoid noise
    float scale = lerp(DISTANCE_SCALE_MAX, DISTANCE_SCALE_MIN, normalizedDist);

    return scale;
}

// Check if this is a gentle slope that should suppress depth edges
// Gentle slopes (< ~20 degrees) should not produce depth artifacts
bool isGentleSlope(float3 centerNormal)
{
    // cos(angle) = normal.y for angle from vertical
    // Gentle slope: normal.y > cos(20 degrees) ~= 0.94
    return abs(centerNormal.y) > GENTLE_SLOPE_ANGLE_COS;
}

PSOutput main(PSInput input)
{
    PSOutput output;

    // Calculate offset for thickness (sample at edgeThickness pixels apart)
    float2 offset = texelSize * edgeThickness;

    // Sample 3x3 neighborhood for Sobel filter
    // Sample pattern (indices):
    // 0 1 2
    // 3 4 5
    // 6 7 8
    float2 offsets[9] = {
        float2(-offset.x, -offset.y), // 0: top-left
        float2(0.0,       -offset.y), // 1: top-center
        float2(offset.x,  -offset.y), // 2: top-right
        float2(-offset.x, 0.0),       // 3: middle-left
        float2(0.0,       0.0),       // 4: center
        float2(offset.x,  0.0),       // 5: middle-right
        float2(-offset.x, offset.y),  // 6: bottom-left
        float2(0.0,       offset.y),  // 7: bottom-center
        float2(offset.x,  offset.y)   // 8: bottom-right
    };

    // Sample normals (primary edge signal)
    float3 normalSamples[9];
    for (int i = 0; i < 9; ++i)
    {
        float2 sampleUV = input.texCoord + offsets[i];
        float4 normalSample = normalTexture.Sample(pointSampler, sampleUV);
        // Normals are stored as (N * 0.5 + 0.5), decode to [-1, 1]
        normalSamples[i] = normalSample.xyz * 2.0 - 1.0;
    }

    // Get center depth and normal for terrain-specific adjustments
    float rawCenterDepth = depthTexture.Sample(pointSampler, input.texCoord);
    float linearCenterDepth = linearizeDepth(rawCenterDepth);
    float3 centerNormal = normalSamples[4]; // Center sample

    // Sample depth (secondary edge signal)
    float depthSamples[9];
    for (int j = 0; j < 9; ++j)
    {
        float2 sampleUV = input.texCoord + offsets[j];
        float rawDepth = depthTexture.Sample(pointSampler, sampleUV);
        // Linearize for perspective-correct edge detection
        depthSamples[j] = normalizedLinearDepth(rawDepth);
    }

    // Calculate edge magnitudes
    float normalEdge = sobelNormal(normalSamples);
    float depthEdge = sobelSingle(depthSamples);

    // ==========================================================================
    // Terrain-Specific Edge Detection (Ticket 3-035)
    // ==========================================================================

    // Calculate terrain edge weight multiplier for cliffs and shorelines
    float terrainWeight = calculateTerrainEdgeWeight(centerNormal, normalEdge);

    // Scale depth threshold based on distance (reduces noise on gentle distant slopes)
    float distanceScale = scaleDepthThresholdByDistance(linearCenterDepth);
    float adjustedDepthThreshold = depthThreshold / distanceScale;

    // Check if this is a gentle slope - suppress depth edges to avoid noise
    bool gentleSlope = isGentleSlope(centerNormal);

    // Apply thresholds
    // Normal edges: apply terrain weight for bolder cliff/shoreline outlines
    float normalEdgeMask = step(normalThreshold, normalEdge * terrainWeight);

    // Depth edges: use distance-adjusted threshold and suppress on gentle slopes
    float depthEdgeMask = 0.0;
    if (!gentleSlope)
    {
        depthEdgeMask = step(adjustedDepthThreshold, depthEdge);
    }

    // Combine: normal edges are primary, depth edges fill gaps
    // Normal edges take precedence; depth edges are secondary
    // For terrain, normal edges carry most of the outline information
    float edgeMask = saturate(normalEdgeMask + depthEdgeMask * 0.3);

    // ==========================================================================
    // Final Color Composition
    // ==========================================================================

    // Sample original scene color
    float4 sceneColor = sceneColorTexture.Sample(pointSampler, input.texCoord);

    // Blend edge color with scene
    // outlineColor.a controls overall edge visibility
    float3 finalColor = lerp(sceneColor.rgb, outlineColor.rgb, edgeMask * outlineColor.a);

    output.color = float4(finalColor, sceneColor.a);

    return output;
}

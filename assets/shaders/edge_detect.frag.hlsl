// Edge Detection Fragment Shader (HLSL SM 6.0)
// Screen-space Sobel edge detection for cartoon outlines.
//
// Primary signal: Normal discontinuities
// Secondary signal: Linearized depth discontinuities
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

    // Apply thresholds
    float normalEdgeMask = step(normalThreshold, normalEdge);
    float depthEdgeMask = step(depthThreshold, depthEdge);

    // Combine: normal edges are primary, depth edges fill gaps
    // Normal edges take precedence; depth edges are secondary
    float edgeMask = saturate(normalEdgeMask + depthEdgeMask * 0.5);

    // Sample original scene color
    float4 sceneColor = sceneColorTexture.Sample(pointSampler, input.texCoord);

    // Blend edge color with scene
    // outlineColor.a controls overall edge visibility
    float3 finalColor = lerp(sceneColor.rgb, outlineColor.rgb, edgeMask * outlineColor.a);

    output.color = float4(finalColor, sceneColor.a);

    return output;
}

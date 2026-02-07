// Debug Grid Fragment Shader (HLSL SM 6.0)
// Renders procedural grid lines based on world position.
// Supports fine (16x16) and coarse (64x64) grid scales with different colors.
// Uses anti-aliased line rendering with smooth edges.
//
// Grid lines are computed using fwidth() for consistent screen-space thickness.

struct PSInput
{
    float4 position : SV_Position;
    float2 worldPos : TEXCOORD0;      // World XY position
    float4 fineColor : COLOR0;         // Fine grid color with opacity
    float4 coarseColor : COLOR1;       // Coarse grid color with opacity
    float fineSpacing : TEXCOORD1;     // Fine grid spacing
    float coarseSpacing : TEXCOORD2;   // Coarse grid spacing
    float thickness : TEXCOORD3;       // Line thickness
    float camDistance : TEXCOORD4;     // Camera distance for LOD
};

struct PSOutput
{
    float4 color : SV_Target0;
};

// Compute anti-aliased grid line intensity
// Returns 1.0 on the line, 0.0 off the line, with smooth transition
float gridLine(float coord, float spacing, float thickness)
{
    // Distance from nearest grid line
    float d = abs(frac(coord / spacing + 0.5) - 0.5) * spacing;

    // Use screen-space derivatives for consistent line width
    float lineWidth = thickness * 0.5;
    float derivative = fwidth(coord);

    // Anti-aliased step function
    float aa = derivative * 1.5;  // Smoothing width
    return 1.0 - smoothstep(lineWidth - aa, lineWidth + aa, d);
}

// Compute grid intensity for both X and Y directions
float grid(float2 worldPos, float spacing, float thickness)
{
    float lineX = gridLine(worldPos.x, spacing, thickness);
    float lineY = gridLine(worldPos.y, spacing, thickness);

    // Combine X and Y lines (max for OR-like behavior)
    return max(lineX, lineY);
}

PSOutput main(PSInput input)
{
    PSOutput output;

    // LOD: Fade fine grid at far distances
    // At close distances (< 100), show fine grid
    // At far distances (> 150), hide fine grid
    float fineGridFade = saturate((150.0 - input.camDistance) / 50.0);

    // Compute grid intensities
    float coarseIntensity = grid(input.worldPos, input.coarseSpacing, input.thickness * 2.0);
    float fineIntensity = grid(input.worldPos, input.fineSpacing, input.thickness) * fineGridFade;

    // Coarse grid takes priority (drawn on top)
    float4 finalColor = float4(0, 0, 0, 0);

    // Layer fine grid first
    if (fineIntensity > 0.001)
    {
        float4 fineWithIntensity = float4(input.fineColor.rgb, input.fineColor.a * fineIntensity);
        finalColor = fineWithIntensity;
    }

    // Layer coarse grid on top
    if (coarseIntensity > 0.001)
    {
        float4 coarseWithIntensity = float4(input.coarseColor.rgb, input.coarseColor.a * coarseIntensity);

        // Alpha blend coarse over fine
        float srcAlpha = coarseWithIntensity.a;
        float dstAlpha = finalColor.a * (1.0 - srcAlpha);
        float outAlpha = srcAlpha + dstAlpha;

        if (outAlpha > 0.001)
        {
            finalColor.rgb = (coarseWithIntensity.rgb * srcAlpha + finalColor.rgb * dstAlpha) / outAlpha;
            finalColor.a = outAlpha;
        }
        else
        {
            finalColor = coarseWithIntensity;
        }
    }

    // Discard fully transparent pixels for performance
    if (finalColor.a < 0.001)
    {
        discard;
    }

    output.color = finalColor;
    return output;
}

// Toon Fragment Shader (HLSL SM 6.0)
// Implements 3-band toon shading with fixed sun direction

// Fragment input (from vertex shader)
struct PSInput
{
    float4 clipPosition  : SV_Position;
    float3 worldPosition : TEXCOORD0;
    float3 worldNormal   : TEXCOORD1;
    float4 instanceColor : COLOR0;
};

// Fragment output
struct PSOutput
{
    float4 color : SV_Target0;
};

PSOutput main(PSInput input)
{
    PSOutput output;

    // Fixed sun direction (normalized)
    float3 sunDirection = normalize(float3(1.0, 2.0, 1.0));

    // Calculate diffuse lighting (N dot L)
    float3 normal = normalize(input.worldNormal);
    float NdotL = dot(normal, sunDirection);

    // Remap from [-1, 1] to [0, 1] for toon bands
    float lightIntensity = NdotL * 0.5 + 0.5;

    // 3-band toon shading
    // shadow < 0.3, mid < 0.7, highlight >= 0.7
    float toonIntensity;
    if (lightIntensity < 0.3)
    {
        // Shadow band
        toonIntensity = 0.3;
    }
    else if (lightIntensity < 0.7)
    {
        // Mid band
        toonIntensity = 0.6;
    }
    else
    {
        // Highlight band
        toonIntensity = 1.0;
    }

    // Apply toon intensity to instance color
    output.color = float4(input.instanceColor.rgb * toonIntensity, input.instanceColor.a);

    return output;
}

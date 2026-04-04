#version 460

layout (location = 0) in vec3 fragPos;
layout (location = 1) in vec3 fragNormal;
layout (location = 2) in vec3 viewPos;
layout (location = 3) in flat float fragExposure;
layout (location = 4) in flat vec3  fragAlbedo;
layout (location = 5) in flat float fragRoughness;
layout (location = 6) in flat float fragMetallic;
layout (location = 7) in flat float fragAO;

layout (location = 0) out vec4 outColor;

vec3 toneMapACES(vec3 color);

struct DirectionalLight
{
    vec4 directionAndIntensity;
    vec4 colorAndEnabled;
};

layout (std430, set = 0, binding = 2) readonly buffer SceneBuffer {
    DirectionalLight sun;
    vec4             ambient;
} sceneData;

void main()
{
    vec3 L = normalize(-sceneData.sun.directionAndIntensity.xyz);
    float intensity = sceneData.sun.directionAndIntensity.w;
    vec3 lightColor = sceneData.sun.colorAndEnabled.rgb;
    bool enabled = sceneData.sun.colorAndEnabled.w > 0.5;

    vec3 ambient = sceneData.ambient.rgb * sceneData.ambient.a * fragAlbedo * fragAO;

    if (!enabled)
    {
        vec3 color = ambient * fragExposure;
        color = toneMapACES(color);
        outColor = vec4(pow(color, vec3(1.0/2.2)), 1.0);
        return;
    }

    vec3 N = normalize(fragNormal);
    vec3 V = normalize(viewPos - fragPos);
    vec3 H = normalize(V + L);

    vec3 radiance = lightColor * intensity;

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, fragAlbedo, fragMetallic);

    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
    float D = DistributionGGX(N, H, fragRoughness);
    float G = GeometrySmith(N, V, L, fragRoughness);

    vec3 numerator = D * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - fragMetallic;

    float NdotL = max(dot(N, L), 0.0);
    vec3 Lo = (kD * fragAlbedo / PI + specular) * radiance * NdotL;

    vec3 color = Lo * fragExposure;
    color += ambient.rgb * sceneData.ambient.a;
    color = toneMapACES(color);
    color = pow(color, vec3(1.0/2.2));

    outColor = vec4(color, 1.0);
}

vec3 toneMapACES(vec3 color)
{
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((color * (a * color + b)) / (color * (c * color + d) + e), 0.0, 1.0);
}
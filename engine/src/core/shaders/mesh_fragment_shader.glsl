#version 460
#extension GL_EXT_nonuniform_qualifier : enable

const float PI = 3.14159265359;

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

layout (location = 0) in vec3 fragPos;
layout (location = 1) in vec3 fragNormal;
layout (location = 2) in vec3 viewPos;
layout (location = 3) in flat float fragExposure;
layout (location = 4) in flat vec3 fragAlbedo;
layout (location = 5) in flat float fragRoughness;
layout (location = 6) in flat float fragMetallic;
layout (location = 7) in flat float fragAO;
layout (location = 8) in flat uint fragMaterialIdx;
layout (location = 9) in flat uint fragIdx;
layout (location = 10) in vec2 fragTexCoord;
layout (location = 11) in vec4 fragTangent;

layout (location = 0) out vec4 outColor;
layout (location = 1) out uint outEntityId;

vec3 toneMapACES(vec3 color);

struct Material
{
    uint albedoIdx;
    uint normalIdx;
    uint roughnessIdx;
    uint metallicIdx;
    uint aoIdx;
    uint _padding[3];
};

layout (std430, set = 0, binding = 2) readonly buffer MaterialsBuffer {
    Material materials[];
} materialData;

struct DirectionalLight
{
    vec4 directionAndIntensity;
    vec4 colorAndEnabled;
};
struct PointLight
{
    vec4 positionAndIntensity;
    vec4 colorAndEnabled;
    vec4 range;
};
vec3 calcPointLight(PointLight light, vec3 N, vec3 V, vec3 fragPos, vec3 albedo, float roughness, float metallic, vec3 F0);

#define kMaxPointLights 16

layout (std430, set = 0, binding = 3) readonly buffer SceneBuffer {
    DirectionalLight sun;

    PointLight pointLights[kMaxPointLights];
    uint       pointLightCount;
    uint       _padding[3];
} sceneData;

layout (set = 0, binding = 4) uniform samplerCube skyboxSampler;
layout (set = 0, binding = 5) uniform samplerCube irradianceSampler;
layout (set = 0, binding = 6) uniform samplerCube prefilteredEnvSampler;
layout (set = 0, binding = 7) uniform sampler2D brdfLutSampler;
layout (set = 0, binding = 8) uniform sampler2D textures[];


void main()
{
    Material material = materialData.materials[fragMaterialIdx];
    vec3  texAlbedo    = texture(textures[nonuniformEXT(material.albedoIdx)],    fragTexCoord).rgb;
    vec3  texNormal    = texture(textures[nonuniformEXT(material.normalIdx)],    fragTexCoord).rgb;
    float texRoughness = texture(textures[nonuniformEXT(material.roughnessIdx)], fragTexCoord).r;
    float texMetallic  = texture(textures[nonuniformEXT(material.metallicIdx)],  fragTexCoord).r;
    float texAO        = texture(textures[nonuniformEXT(material.aoIdx)],        fragTexCoord).r;

    vec3  albedo    = texAlbedo * fragAlbedo;
    float roughness = max(texRoughness * fragRoughness, 0.05);
    float metallic  = texMetallic * fragMetallic;
    float ao        = texAO * fragAO;

    vec3 Ng = normalize(fragNormal);
    vec3 Tg = normalize(fragTangent.xyz - dot(fragTangent.xyz, Ng) * Ng);
    vec3 Bg = cross(Ng, Tg) * fragTangent.w;
    mat3 TBN = mat3(Tg, Bg, Ng);

    vec3 normalFromMap = texNormal * 2.0 - 1.0;
    vec3 N = Ng;
    vec3 V = normalize(viewPos - fragPos);
    vec3 R = reflect(-V, N);

    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    vec3 F_env = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
    vec3 kS_env = F_env;
    vec3 kD_env = (1.0 - kS_env) * (1.0 - metallic);

    const float ENV_SCALE_FACTOR = 10000.0;
    vec3 irradiance = texture(irradianceSampler, N).rgb * ENV_SCALE_FACTOR;
    vec3 diffuseIBL = irradiance * albedo;

    const float MAX_REFLECTION_LOD = 4.0;
    vec3 prefilteredColor = textureLod(prefilteredEnvSampler, R, roughness * MAX_REFLECTION_LOD).rgb * ENV_SCALE_FACTOR;
    vec2 brdf = texture(brdfLutSampler, vec2(max(dot(N, V), 0.0), roughness)).rg;
    vec3 specularIBL = prefilteredColor * (F_env * brdf.x + brdf.y);

    vec3 ambient = (kD_env * diffuseIBL + specularIBL) * ao;

    vec3 L = normalize(-sceneData.sun.directionAndIntensity.xyz);
    float intensity = sceneData.sun.directionAndIntensity.w;
    vec3 lightColor = sceneData.sun.colorAndEnabled.rgb;
    bool enabled = sceneData.sun.colorAndEnabled.w > 0.5;

    vec3 Lo = vec3(0.0);
    if (enabled)
    {
        vec3 H = normalize(V + L);
        vec3 radiance = lightColor * intensity;

        vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
        float D = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);

        vec3 numerator = D * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular = numerator / denominator;

        vec3 kS = F;
        vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);

        float NdotL = max(dot(N, L), 0.0);
        Lo = (kD * albedo / PI + specular) * radiance * NdotL;
    }

    for (uint i = 0u; i < sceneData.pointLightCount; ++i)
        Lo += calcPointLight(sceneData.pointLights[i], N, V, fragPos,
                             albedo, roughness, metallic, F0);

    vec3 color = (Lo + ambient) * fragExposure;

    color = toneMapACES(color);
    color = pow(color, vec3(1.0 / 2.2));

    outColor = vec4(color, 1.0);

    outEntityId = fragIdx;
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

vec3 calcPointLight(PointLight light, vec3 N, vec3 V, vec3 fragPos, vec3 albedo, float roughness, float metallic, vec3 F0)
{
    if (light.colorAndEnabled.w < 0.5)
    return vec3(0.0);

    vec3  lightPos  = light.positionAndIntensity.xyz;
    float intensity = light.positionAndIntensity.w;
    vec3  lightColor = light.colorAndEnabled.rgb;
    float range     = light.range.x;

    vec3  toLight  = lightPos - fragPos;
    float distance = length(toLight);

    if (distance > range)
        return vec3(0.0);

    vec3 L = toLight / max(distance, 0.0001);
    vec3 H = normalize(V + L);

    float attenuation = 1.0 / max(distance * distance, 0.0001);
    float rangeFade   = clamp(1.0 - pow(distance / range, 4.0), 0.0, 1.0);
    rangeFade *= rangeFade;
    vec3  radiance    = lightColor * intensity * attenuation * rangeFade;

    vec3  F = fresnelSchlick(max(dot(H, V), 0.0), F0);
    float D = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);

    vec3  numerator   = D * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3  specular    = numerator / denominator;

    vec3 kS = F;
    vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);

    float NdotL = max(dot(N, L), 0.0);
    return (kD * albedo / PI + specular) * radiance * NdotL;
}

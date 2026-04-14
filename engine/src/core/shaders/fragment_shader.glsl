#version 460
#extension GL_EXT_nonuniform_qualifier : enable

// --- BEGIN INCLUDE ---
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
// --- END INCLUDE ---

layout (location = 0) in vec3 fragPos;
layout (location = 1) in vec3 fragNormal;
layout (location = 2) in vec3 viewPos;
layout (location = 3) in flat float fragExposure;
layout (location = 4) in flat vec3 fragAlbedo;
layout (location = 5) in flat float fragRoughness;
layout (location = 6) in flat float fragMetallic;
layout (location = 7) in flat float fragAO;
layout (location = 8) in flat uint fragMaterialIdx;
layout (location = 9) in vec2 fragTexCoord;
layout (location = 10) in mat3 fragTBN;

layout (location = 0) out vec4 outColor;

vec3 toneMapACES(vec3 color);

struct DirectionalLight
{
    vec4 directionAndIntensity;
    vec4 colorAndEnabled;
};

layout (std430, set = 0, binding = 2) readonly buffer SceneBuffer {
    DirectionalLight sun;
    vec4 ambient;
} sceneData;

layout (set = 0, binding = 3) uniform samplerCube skyboxSampler;
layout (set = 0, binding = 4) uniform samplerCube irradianceSampler;
layout (set = 0, binding = 5) uniform sampler2D brdfLutSampler;
layout (set = 0, binding = 6) uniform sampler2D albedoSampler[];
layout (set = 0, binding = 7) uniform sampler2D normalSampler[];
layout (set = 0, binding = 8) uniform sampler2D roughnessSampler[];
layout (set = 0, binding = 9) uniform sampler2D metallicSampler[];
layout (set = 0, binding = 10) uniform sampler2D aoSampler[];


void main()
{
    vec3  texAlbedo    = texture(albedoSampler[fragMaterialIdx], fragTexCoord).rgb;
    vec3  texNormal    = texture(normalSampler[fragMaterialIdx], fragTexCoord).rgb;
    float texRoughness = texture(roughnessSampler[fragMaterialIdx], fragTexCoord).r;
    float texMetallic  = texture(metallicSampler[fragMaterialIdx], fragTexCoord).r;
    float texAO        = texture(aoSampler[fragMaterialIdx], fragTexCoord).r;

    vec3  albedo    = texAlbedo * fragAlbedo;
    float roughness = texRoughness * fragRoughness;
    float metallic  = texMetallic * fragMetallic;
    float ao        = texAO * fragAO;

    vec3 normalFromMap = texNormal * 2.0 - 1.0;
    vec3 N = normalize(fragTBN * normalFromMap);
    vec3 V = normalize(viewPos - fragPos);
    vec3 R = reflect(-V, N);

    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    vec3 F_env = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
    vec3 kS_env = F_env;
    vec3 kD_env = (1.0 - kS_env) * (1.0 - metallic);

    vec3 irradiance = texture(irradianceSampler, N).rgb * 1.5;
    vec3 diffuseIBL = irradiance * albedo;

    const float MAX_REFLECTION_LOD = 10.0;
    vec3 prefilteredColor = textureLod(skyboxSampler, R, roughness * MAX_REFLECTION_LOD).rgb;
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

    vec3 color = Lo * fragExposure + ambient;

    color = toneMapACES(color);
    color = pow(color, vec3(1.0 / 2.2));

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

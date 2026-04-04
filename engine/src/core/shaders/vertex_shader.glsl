#version 460
#extension GL_ARB_shader_draw_parameters: enable

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aUv;
layout (location = 3) in vec4 aTangent;

layout (location = 0) out vec3 fragPos;
layout (location = 1) out vec3 fragNormal;
layout (location = 2) out vec3 viewPos;
layout (location = 3) out flat float fragExposure;
layout (location = 4) out flat vec3  fragAlbedo;
layout (location = 5) out flat float fragRoughness;
layout (location = 6) out flat float fragMetallic;
layout (location = 7) out flat float fragAO;

layout (binding = 0) uniform Camera
{
    mat4 projection;
    mat4 view;
    vec4 positionAndExposure;
} camera;

struct MeshData
{
    mat4 model;
    vec4 albedoAndRoughness;
    vec4 pbrParams;
};
layout (std430, set = 0, binding = 1) readonly buffer TransformBuffer {
    MeshData data[];
} meshData;

void main()
{
    MeshData mData =  meshData.data[gl_DrawID];
    mat4 model = mData.model;
    fragAlbedo    = mData.albedoAndRoughness.xyz;
    fragRoughness = mData.albedoAndRoughness.w;
    fragMetallic  = mData.pbrParams.x;
    fragAO        = mData.pbrParams.y;
    vec4 worldPos = model * vec4(aPos, 1.0);

    fragPos = worldPos.xyz;
    fragNormal = normalize(mat3(model) * aNormal);
    viewPos = camera.positionAndExposure.xyz;
    fragExposure = camera.positionAndExposure.w;

    gl_Position = camera.projection * camera.view * worldPos;
}
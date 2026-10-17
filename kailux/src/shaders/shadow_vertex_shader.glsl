#version 460
#extension GL_ARB_shader_draw_parameters: enable

layout (location = 0) in vec3 aPos;

struct MeshMaterialData
{
    vec4 albedoAndRoughness;
    vec4 pbrParams;
    uint materialIdx;
    uint _padding[3];
};

struct MeshData
{
    mat4 model;
    vec4 boundingSphere;
    vec4 lodErrors;

    MeshMaterialData material;

    uint idx;
    uint lodCount;
    uint __padding[2];
};

layout (std430, set = 0, binding = 0) readonly buffer TransformBuffer {
    MeshData data[];
} meshData;

layout (push_constant) uniform Cascade
{
    mat4 lightViewProjection;
};

void main()
{
    mat4 model = meshData.data[gl_InstanceIndex].model;
    gl_Position = lightViewProjection * model * vec4(aPos, 1.0);
}
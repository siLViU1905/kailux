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
    uint batchIndex;
    uint __padding;
};

layout (std430, set = 0, binding = 0) readonly buffer TransformBuffer {
    MeshData data[];
} meshData;

layout (std430, set = 0, binding = 1) readonly buffer InstanceIdsBuffer {
    uint instanceIds[];
};

const uint kDirectInstances = 0xFFFFFFFFu;

layout (push_constant) uniform Cascade
{
    mat4 lightViewProjection;
    uint instanceBase;
};

void main()
{
    uint objectIndex = instanceBase == kDirectInstances
        ? uint(gl_InstanceIndex)
        : instanceIds[instanceBase + uint(gl_InstanceIndex)];
        
    mat4 model = meshData.data[objectIndex].model;
    gl_Position = lightViewProjection * model * vec4(aPos, 1.0);
}
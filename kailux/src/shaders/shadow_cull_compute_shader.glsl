#version 460

struct IndexedIndirectCommand
{
    uint indexCount;
    uint instanceCount;
    uint firstIndex;
    int vertexOffset;
    uint firstInstance;
};

struct MeshMaterialData
{
    vec4 albedoAndRoughness;
    vec4 pbrParams;
    uint materialIdx;
    uint _padding[3];
};

struct MeshData
{
    mat4 modelMatrix;
    vec4 boundingSphere;
    vec4 lodErrors;
    MeshMaterialData material;
    uint id;
    uint lodCount;
    uint batchIndex;
    uint __padding;
};

struct ShadowCullTarget
{
    vec4 planes[6];
    uvec4 info; // x - enabled
};

layout (std430, binding = 0) readonly buffer ModelBuffer
{
    MeshData objects[];
};

layout (std430, binding = 1) readonly buffer TargetsBuffer
{
    ShadowCullTarget targets[];
};

layout (std430, binding = 2) buffer ShadowCommandsBuffer
{
    IndexedIndirectCommand commands[];
};

layout (std430, binding = 3) writeonly buffer InstanceIdsBuffer
{
    uint instanceIds[];
};

layout (push_constant) uniform ShadowCullParams
{
    uint objectCount;
    uint listStride;
};

const uint kNearPlane = 4;

layout (local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

void main()
{
    uint gId = gl_GlobalInvocationID.x;
    uint list = gl_WorkGroupID.y;

    if (gId >= objectCount || targets[list].info.x == 0)
        return;

    mat4 modelMatrix = objects[gId].modelMatrix;
    vec4 localSphere = objects[gId].boundingSphere;

    vec3 worldCenter = (modelMatrix * vec4(localSphere.xyz, 1.0)).xyz;
    float maxScale = max(length(modelMatrix[0].xyz), max(length(modelMatrix[1].xyz), length(modelMatrix[2].xyz)));
    float worldRadius = localSphere.w * maxScale;

    for (uint i = 0; i < 6; ++i)
    {
        if (i == kNearPlane)
            continue;

        vec4 plane = targets[list].planes[i];
        if (dot(plane.xyz, worldCenter) + plane.w < -worldRadius)
            return;
    }
    
    uint base = list * listStride;
    uint command = base + objects[gId].batchIndex;
    uint slot = atomicAdd(commands[command].instanceCount, 1);
    instanceIds[base + commands[command].firstInstance + slot] = gId;
}
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
    MeshMaterialData material;
    uint id;
    uint _padding[3];
};


layout (std430, binding = 0) readonly buffer ModelBuffer
{
    MeshData objects[];
};

layout (std430, binding = 1) readonly buffer InputCommandsBuffer
{
    IndexedIndirectCommand inputCommands[];
};

layout (std430, binding = 2) writeonly buffer IndirectDrawBuffer
{
    IndexedIndirectCommand outputCommands[];
};

layout (std430, binding = 3) buffer CommandCountBuffer
{
    uint drawCount;
};

layout (push_constant) uniform CameraProperties
{
    vec4 frustumPlanes[6];
    uint totalObjects;
};

bool IsVisible(vec3 center, float radius)
{
    for (int i = 0; i < 6; i++)
    if (dot(frustumPlanes[i].xyz, center) + frustumPlanes[i].w < -radius)
    return false;

    return true;
}

layout (local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

void main() {
    uint gId = gl_GlobalInvocationID.x;

    if (gId >= totalObjects)
    return;

    mat4 modelMatrix = objects[gId].modelMatrix;

    vec3 worldCenter = (modelMatrix * vec4(0.0, 0.0, 0.0, 1.0)).xyz;


    float scaleX = length(modelMatrix[0].xyz);
    float scaleY = length(modelMatrix[1].xyz);
    float scaleZ = length(modelMatrix[2].xyz);
    float maxScale = max(scaleX, max(scaleY, max(scaleZ, 1.0)));

    float localRadius = 2.5;
    float worldRadius = localRadius * maxScale;

    if (IsVisible(worldCenter, worldRadius))
    {
        uint drawIndex = atomicAdd(drawCount, 1);
        outputCommands[drawIndex].indexCount = inputCommands[gId].indexCount;
        outputCommands[drawIndex].instanceCount = inputCommands[gId].instanceCount;
        outputCommands[drawIndex].firstIndex = inputCommands[gId].firstIndex;
        outputCommands[drawIndex].vertexOffset = inputCommands[gId].vertexOffset;
        outputCommands[drawIndex].firstInstance = gId;
    }
}
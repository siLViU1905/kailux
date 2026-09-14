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
    vec4 lodErrors; // x - LDO1...w - LOD4

    MeshMaterialData material;

    uint id;
    uint lodCount;
    uint __padding[2];
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
    vec4 cameraPosition;
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

#define kMaxGeometryLod 5

void main() {
    uint gId = gl_GlobalInvocationID.x;

    if (gId >= totalObjects)
        return;

    mat4 modelMatrix   = objects[gId].modelMatrix;
    vec4 localSphere   = objects[gId].boundingSphere;
    vec3 localCenter   = localSphere.xyz;
    float localRadius  = localSphere.w;

    vec3 worldCenter = (modelMatrix * vec4(localCenter, 1.0)).xyz;

    float scaleX = length(modelMatrix[0].xyz);
    float scaleY = length(modelMatrix[1].xyz);
    float scaleZ = length(modelMatrix[2].xyz);
    float maxScale = max(scaleX, max(scaleY, max(scaleZ, 1.0)));

    float worldRadius = localRadius * maxScale;

    if (IsVisible(worldCenter, worldRadius))
    {
        uint lodCount = objects[gId].lodCount;
        vec4 errors = objects[gId].lodErrors;

        float dist = max(distance(cameraPosition.xyz, worldCenter) - worldRadius, 0.001);

        uint lod = 0;
        for(uint i = 1; i < lodCount; ++i)
        {
            float pixelError = errors[i - 1] * maxScale * cameraPosition.w / dist;
            if (pixelError > 1.5)
                    break;
            lod = i;
        }

        uint drawIndex = atomicAdd(drawCount, 1);
        outputCommands[drawIndex] = inputCommands[gId * kMaxGeometryLod + lod];
        outputCommands[drawIndex].firstInstance = gId;
    }
}
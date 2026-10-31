#version 460
struct IndexedIndirectCommand
{
    uint indexCount;
    uint instanceCount;
    uint firstIndex;
    int vertexOffset;
    uint firstInstance;
};
layout (std430, binding = 0) readonly buffer BatchCommandsBuffer
{
    IndexedIndirectCommand batchCommands[];
};
layout (std430, binding = 1) writeonly buffer IndirectDrawBuffer
{
    IndexedIndirectCommand outputCommands[];
};
layout (std430, binding = 2) buffer CommandCountBuffer
{
    uint drawCount;
};
layout (push_constant) uniform CompactParams
{
    uint commandCount;
};
layout (local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

void main()
{
    uint gId = gl_GlobalInvocationID.x;
    if (gId >= commandCount)
        return;

    IndexedIndirectCommand command = batchCommands[gId];
    if (command.instanceCount == 0)
        return;

    uint drawIndex = atomicAdd(drawCount, 1);
    outputCommands[drawIndex] = command;
}
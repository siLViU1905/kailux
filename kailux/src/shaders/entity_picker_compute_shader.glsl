#version 460

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout (set = 0, binding = 0, r32ui) uniform readonly uimage2D idBuffer;

layout (set = 0, binding = 1) buffer PickingResult
{
    uint objectID;
} result;

layout (push_constant) uniform MouseCoords
{
    uint x;
    uint y;
} mouse;

void main()
{
    ivec2 size = imageSize(idBuffer);
    ivec2 texelCoords = ivec2(mouse.x, mouse.y);

    if (texelCoords.x >= 0 && texelCoords.y >= 0 &&
        texelCoords.x < size.x && texelCoords.y < size.y)
        result.objectID = imageLoad(idBuffer, texelCoords).r;
    else
        result.objectID = ~0u;
}
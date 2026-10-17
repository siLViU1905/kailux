#version 460

layout (location = 0) out vec4 outColor;
layout (location = 0) in vec2 inUV;

layout (set = 0, binding = 0) uniform usampler2D idBuffer;

layout (push_constant) uniform PushConstants
{
    vec4 outlineColor;
    uint selectedEntityId;
    uint _padding[3];
};

void main()
{
    const uint INVALID_ID = ~0u;
    if (selectedEntityId == INVALID_ID)
    discard;

    uint currentId = texture(idBuffer, inUV).r;
    vec2 texelSize = 1.0 / textureSize(idBuffer, 0);

    bool isEdge = false;

    if (currentId == selectedEntityId)
    {
        uint idLeft = texture(idBuffer, inUV + vec2(-texelSize.x, 0.0)).r;
        uint idRight = texture(idBuffer, inUV + vec2(texelSize.x, 0.0)).r;
        uint idUp = texture(idBuffer, inUV + vec2(0.0, texelSize.y)).r;
        uint idDown = texture(idBuffer, inUV + vec2(0.0, -texelSize.y)).r;

        if (idLeft != selectedEntityId || idRight != selectedEntityId ||
        idUp != selectedEntityId || idDown != selectedEntityId)
        isEdge = true;
    }
    else
    {
        uint idLeft = texture(idBuffer, inUV + vec2(-texelSize.x, 0.0)).r;
        uint idRight = texture(idBuffer, inUV + vec2(texelSize.x, 0.0)).r;
        uint idUp = texture(idBuffer, inUV + vec2(0.0, texelSize.y)).r;
        uint idDown = texture(idBuffer, inUV + vec2(0.0, -texelSize.y)).r;

        if (idLeft == selectedEntityId || idRight == selectedEntityId ||
        idUp == selectedEntityId || idDown == selectedEntityId)
        isEdge = true;
    }

    if (!isEdge)
    discard;

    outColor = vec4(outlineColor.xyz, 1.0);
}
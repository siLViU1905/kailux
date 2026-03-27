#version 460
#extension GL_ARB_shader_draw_parameters : enable

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUv;
layout(location = 3) in vec4 aTangent;

layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec3 fragNormal;

layout(binding = 0) uniform Camera
{
    mat4 projection;
    mat4 view;
    vec3 position;
}
camera;

void main()
{
    fragPos = aPos;
    fragNormal = aNormal;

    vec3 modifiedPos = aPos;
    modifiedPos.x += float(gl_DrawID) * 1.5;
    gl_Position = camera.projection * camera.view * vec4(modifiedPos, 1.0);
}
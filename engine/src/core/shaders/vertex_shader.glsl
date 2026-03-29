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
} camera;

layout(std430, set = 0, binding = 1) readonly buffer TransformBuffer {
    mat4 models[];
} transformData;

void main()
{
    mat4 model = transformData.models[gl_DrawID];
    vec4 worldPos = model * vec4(aPos, 1.0);

    fragPos = worldPos.xyz;
    fragNormal = normalize(mat3(model) * aNormal);

    gl_Position = camera.projection * camera.view * worldPos;
}
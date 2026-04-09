#version 460

layout(location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aUv;
layout (location = 3) in vec4 aTangent;

layout(location = 0) out vec3 fragTexCoord;

layout (set = 0, binding = 0) uniform Camera
{
    mat4 projection;
    mat4 view;
    vec4 positionAndExposure;
} camera;

void main() {
    fragTexCoord = aPos;

    mat4 viewNoTranslation = mat4(mat3(camera.view));
    vec4 pos = camera.projection * viewNoTranslation * vec4(aPos, 1.0);

    gl_Position = pos.xyww;
}
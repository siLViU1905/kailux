#version 460

layout (location = 0) in vec3 aPos;

layout (location = 0) out vec3 fragColor;

layout (set = 0, binding = 0) uniform Camera
{
    mat4 projection;
    mat4 view;
    vec4 positionAndExposure;
} camera;

layout (push_constant) uniform Push
{
    vec4 positionAndScale;
    vec4 color;
} push;

void main()
{
    vec3  position = push.positionAndScale.xyz;
    float scale    = push.positionAndScale.w;

    vec4 centerView = camera.view * vec4(position, 1.0);

    centerView.xy += aPos.xy * scale;

    gl_Position = camera.projection * centerView;

    fragColor = push.color.rgb;
}
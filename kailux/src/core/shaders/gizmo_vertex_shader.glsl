#version 460

layout (location = 0) in vec3 aPos;

layout (location = 0) out vec3 fragColor;

struct CameraData
{
    mat4 projection;
    mat4 view;
    vec4 positionAndExposure;
};

const uint kMaxCameraViews = 4;
layout(set=0, binding=0) uniform Camera
{
    CameraData cameras[kMaxCameraViews];
};

layout (push_constant) uniform Push
{
    vec4 positionAndScale;
    vec4 color;
    uint cameraIdx;
} push;

void main()
{
    vec3  position = push.positionAndScale.xyz;
    float scale    = push.positionAndScale.w;

    CameraData camera = cameras[push.cameraIdx];
    vec4 centerView = camera.view * vec4(position, 1.0);

    centerView.xy += aPos.xy * scale;

    gl_Position = camera.projection * centerView;

    fragColor = push.color.rgb;
}
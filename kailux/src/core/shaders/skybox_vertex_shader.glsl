#version 460

layout(location = 0) in vec3 aPos;

layout(location = 0) out vec3 fragTexCoord;

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

layout (push_constant) uniform CameraIndex
{
    uint cameraIdx;
};

void main() {
    fragTexCoord = aPos;

    CameraData camera = cameras[cameraIdx];
    mat4 viewNoTranslation = mat4(mat3(camera.view));
    vec4 pos = camera.projection * viewNoTranslation * vec4(aPos, 1.0);

    gl_Position = pos.xyww;
}
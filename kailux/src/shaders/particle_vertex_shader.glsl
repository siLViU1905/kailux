#version 460

struct Particle
{
    vec4  positionAndAge;
    vec4  velocityAndLife;
    uvec4 misc; // x tint RGBA8, y seed, z size scale (float bits)
};

struct Emitter
{
    vec4  positionAndShape;
    vec4  boxExtentsAndSpread;
    vec4  directionAndDrag;
    vec4  gravityAndTurbulence;
    vec4  speedAndLife;
    vec4  startColor;
    vec4  endColor;
    vec4  sizeAndStretch; // x start size, y end size, z stretch, w additive
    uvec4 range;
    uvec4 bursts;
};

struct CameraData
{
    mat4 projection;
    mat4 view;
    vec4 positionAndExposure;
};

const uint kMaxCameraViews = 4;
layout (set = 0, binding = 0) uniform Camera
{
    CameraData cameras[kMaxCameraViews];
};

layout (std430, set = 0, binding = 1) readonly buffer Particles
{
    Particle particles[];
};

layout (std430, set = 0, binding = 2) readonly buffer Emitters
{
    Emitter emitters[];
};

layout (push_constant) uniform Push
{
    uint emitterIndex;
    uint cameraIdx;
} push;

layout (location = 0) out vec2 fragUv;
layout (location = 1) out vec4 fragColor;
layout (location = 2) flat out float fragAdditive;

const vec2 kCorners[6] = vec2[](
    vec2(-1.0, -1.0), vec2( 1.0, -1.0), vec2( 1.0,  1.0),
    vec2(-1.0, -1.0), vec2( 1.0,  1.0), vec2(-1.0,  1.0)
);

void main()
{
    Particle p = particles[gl_InstanceIndex];
    Emitter  emitter = emitters[push.emitterIndex];

    float age  = p.positionAndAge.w;
    float life = p.velocityAndLife.w;

    fragUv       = vec2(0.0);
    fragColor    = vec4(0.0);
    fragAdditive = emitter.sizeAndStretch.w;

    if (age >= life)
    {
        gl_Position = vec4(2.0, 2.0, 2.0, 1.0);
        return;
    }

    float t    = clamp(age / life, 0.0, 1.0);
    float size = mix(emitter.sizeAndStretch.x, emitter.sizeAndStretch.y, t) * uintBitsToFloat(p.misc.z);
    vec4 color = mix(emitter.startColor, emitter.endColor, t) * unpackUnorm4x8(p.misc.x);

    CameraData camera = cameras[push.cameraIdx];
    vec2 corner = kCorners[gl_VertexIndex];
    vec4 center = camera.view * vec4(p.positionAndAge.xyz, 1.0);

    float stretch = emitter.sizeAndStretch.z;
    vec2  axis    = (mat3(camera.view) * p.velocityAndLife.xyz).xy;
    float speed   = length(axis);

    if (stretch > 0.0 && speed > 1e-4)
    {
        axis /= speed;
        vec2 side = vec2(-axis.y, axis.x);
        center.xy += axis * corner.y * (size + speed * stretch) + side * corner.x * size;
    }
    else
        center.xy += corner * size;

    gl_Position = camera.projection * center;
    fragUv      = corner;
    fragColor   = color;
}
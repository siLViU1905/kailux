#version 460

layout (local_size_x = 256) in;

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
    vec4  sizeAndStretch;
    uvec4 range;  // x first, y capacity, z spawn count, w flags
    uvec4 bursts; // x first, y count, z seed
};

struct Burst
{
    vec4  positionAndSpeed;
    vec4  color;
    uvec4 info; // x exclusive prefix end, y size (float bits)
};

layout (std430, binding = 0) buffer Particles
{
    Particle particles[];
};

layout (std430, binding = 1) readonly buffer Emitters
{
    Emitter emitters[];
};

layout (std430, binding = 2) readonly buffer Bursts
{
    Burst bursts[];
};

layout (std430, binding = 3) buffer SpawnCounters
{
    uint spawnCounters[];
};

layout (push_constant) uniform Push
{
    float deltaTime;
    float time;
    uint  seed;
    uint  emitterCount;
} push;

const uint  kShapeBox         = 1u;
const uint  kFlagConfineToBox = 1u;
const float kPi               = 3.14159265359;

uint hash(uint x);

float random01(inout uint state);
vec3 random_unit_vector(inout uint state);
vec3 random_cone(vec3 axis, float cosSpread, inout uint state);

void spawn(inout Particle p, Emitter emitter, uint spawnIndex, uint slot);

void main()
{
    uint emitterIndex = gl_WorkGroupID.y;
    if (emitterIndex >= push.emitterCount)
        return;

        Emitter emitter = emitters[emitterIndex];
        uint local = gl_GlobalInvocationID.x;
        if (local >= emitter.range.y)
            return;

        uint slot = emitter.range.x + local;
        Particle p = particles[slot];
        float dt = push.deltaTime;

            bool alive = p.positionAndAge.w < p.velocityAndLife.w;
    if (alive)
    {
        vec3 velocity = p.velocityAndLife.xyz;
        velocity += emitter.gravityAndTurbulence.xyz * dt;
        velocity *= max(0.0, 1.0 - emitter.directionAndDrag.w * dt);

        vec3 position = p.positionAndAge.xyz + velocity * dt;

        float turbulence = emitter.gravityAndTurbulence.w;
        if (turbulence > 0.0)
        {
            float phase = float(p.misc.y & 0xffffu) * (2.0 * kPi / 65535.0);
            position.x += sin(push.time * 1.7 + phase) * turbulence * dt;
            position.z += cos(push.time * 1.3 + phase * 1.31) * turbulence * dt;
        }

        float age = p.positionAndAge.w + dt;

        if (uint(emitter.positionAndShape.w) == kShapeBox && (emitter.range.w & kFlagConfineToBox) != 0u)
        {
            vec3 offset = abs(position - emitter.positionAndShape.xyz);
            if (any(greaterThan(offset, emitter.boxExtentsAndSpread.xyz)))
                age = p.velocityAndLife.w;
        }

        p.positionAndAge  = vec4(position, age);
        p.velocityAndLife = vec4(velocity, p.velocityAndLife.w);
        particles[slot] = p;
        return;
    }

    if (emitter.range.z == 0u)
        return;

    uint spawnIndex = atomicAdd(spawnCounters[emitterIndex], 1u);
    if (spawnIndex >= emitter.range.z)
        return;

    spawn(p, emitter, spawnIndex, slot);
    particles[slot] = p;
}

uint hash(uint x)
{
    x ^= x >> 16;
    x *= 0x7feb352du;
    x ^= x >> 15;
    x *= 0x846ca68bu;
    x ^= x >> 16;
    return x;
}

float random01(inout uint state)
{
    state = hash(state);
    return float(state >> 8) * (1.0 / 16777216.0);
}

vec3 random_unit_vector(inout uint state)
{
    float z   = random01(state) * 2.0 - 1.0;
    float phi = random01(state) * 2.0 * kPi;
    float r   = sqrt(max(0.0, 1.0 - z * z));
    return vec3(r * cos(phi), r * sin(phi), z);
}

vec3 random_cone(vec3 axis, float cosSpread, inout uint state)
{
    float z   = mix(cosSpread, 1.0, random01(state));
    float phi = random01(state) * 2.0 * kPi;
    float r   = sqrt(max(0.0, 1.0 - z * z));

    vec3 up      = abs(axis.y) < 0.999 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent = normalize(cross(up, axis));
    vec3 bitan   = cross(axis, tangent);

    return tangent * (r * cos(phi)) + bitan * (r * sin(phi)) + axis * z;
}

void spawn(inout Particle p, Emitter emitter, uint spawnIndex, uint slot)
{
    uint state = hash(slot ^ hash(push.seed + spawnIndex * 0x9e3779b9u));

    vec3  position;
    vec3  velocity;
    uint  tint      = 0xffffffffu;
    float sizeScale = 1.0;

    if (emitter.bursts.y > 0u)
    {
        uint b = emitter.bursts.x;
        uint last = emitter.bursts.x + emitter.bursts.y - 1u;
        while (b < last && spawnIndex >= bursts[b].info.x)
            ++b;

        Burst burst = bursts[b];
        position  = burst.positionAndSpeed.xyz;
        velocity  = random_unit_vector(state) * burst.positionAndSpeed.w * mix(0.35, 1.0, random01(state));
        tint      = packUnorm4x8(clamp(burst.color, 0.0, 1.0));
        sizeScale = uintBitsToFloat(burst.info.y);
    }
    else
    {
        position = emitter.positionAndShape.xyz;
        if (uint(emitter.positionAndShape.w) == kShapeBox)
        {
            vec3 r = vec3(random01(state), random01(state), random01(state)) * 2.0 - 1.0;
            position += r * emitter.boxExtentsAndSpread.xyz;
        }

        vec3  direction = random_cone(emitter.directionAndDrag.xyz, emitter.boxExtentsAndSpread.w, state);
        float speed     = mix(emitter.speedAndLife.x, emitter.speedAndLife.y, random01(state));
        velocity = direction * speed;
    }

    float life = mix(emitter.speedAndLife.z, emitter.speedAndLife.w, random01(state));

    p.positionAndAge  = vec4(position, 0.0);
    p.velocityAndLife = vec4(velocity, max(life, 1e-3));
    p.misc            = uvec4(tint, hash(state), floatBitsToUint(sizeScale), 0u);
}
#version 460

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;

void main()
{
    vec3 debugColor;

    if (gl_FrontFacing)
        debugColor = vec3(0.0, 1.0, 0.0);
    else
        debugColor = vec3(1.0, 0.0, 0.0);

    vec3 N = normalize(fragNormal);
    vec3 finalRGB = debugColor * (N.z * 0.5 + 0.5);
    outColor = vec4(finalRGB, 1.0);
}
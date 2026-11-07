#version 460

layout (location = 0) in vec2 fragUv;
layout (location = 1) in vec4 fragColor;
layout (location = 2) flat in float fragAdditive;

layout (location = 0) out vec4 outColor;

void main()
{
    float d = length(fragUv);
    if (d >= 1.0)
        discard;

    float coverage = 1.0 - smoothstep(0.35, 1.0, d);
    float alpha    = fragColor.a * coverage;
    
    outColor = vec4(fragColor.rgb * alpha, fragAdditive > 0.5 ? 0.0 : alpha);
}
#version 450

layout(binding = 0) uniform sampler2D uTex;

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outColor;

void main()
{
    vec4 c = texture(uTex, vUV);
    if (c.w < 0.00999999977648258209228515625)
    {
        discard;
    }
    outColor = c;
}


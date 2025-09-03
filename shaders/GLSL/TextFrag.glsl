#version 450

layout(binding = 0) uniform sampler2D uTex; // Texture del materiale

layout(location = 0) in vec2 vUV; // Elemento in ingresso da TextVert

layout(location = 0) out vec4 outColor; // Uscita RGBA del frammento

void main()
{
    vec4 c = texture(uTex, vUV); // Colore base

    outColor = c; // Colore in uscita del vertice
}


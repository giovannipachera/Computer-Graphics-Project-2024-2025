#version 450

// Elementi in ingresso da TextVertex/LogoVertex
layout(location = 1) in vec2 inUV; // Posizione
layout(location = 0) in vec2 inPos; // Coordinate

// Elementi in uscita verso TextFrag
layout(location = 0) out vec2 vUV; // UV del vertice

void main()
{
    vUV = inUV;

    gl_Position = vec4(inPos, 0.0, 1.0); // Posizione del vertice nello schermo
}


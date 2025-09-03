#version 450

// Elementi in ingresso da PhongVert
layout(location = 0) in vec2 vUV; // Coordinate per campionare la texture 2D
layout(location = 1) in vec3 vWorldPos; // Posizione del frammento nel mondo
layout(location = 2) in vec3 vWorldNorm; // Normale nel mondo

layout(location = 0) out vec4 outColor; // Uscita RGBA del frammento

layout(set = 0, binding = 1) uniform sampler2D uTex; // Texture del materiale

// GUBO: dati condivisi, specifici per tutti gli oggetti
layout(set = 0, binding = 2) uniform GlobalUBO {
    vec3 lightDir; // Da dove arriva la luce
    vec4 lightColor; // Che colore/intensità ha la luce
    vec3 eyePos; // Dove si trova la camera
    vec4 eyeDir; // In che direzione guarda la camera
} gubo;

void main() {
    vec3 N = normalize(vWorldNorm);  // Verso della superficie
    vec3 L = normalize(-gubo.lightDir);  // Da dove arriva la luce
    vec3 V = normalize(gubo.eyePos - vWorldPos); // Dove si trova la camera rispetto al punto
    vec3 H = normalize(L + V); // Vettore Half-Way

    float NdotL = max(dot(N, L), 0.0); // Coseno angolo Normale superficie - Vettore luce
    float NdotH = max(dot(N, H), 0.0); // Coseno angolo Normale superficie - Vettore Half-Way

    // Componenti Phong
    float ambient  = 0.18; // Luce ambiente sobria
    float diffuse  = NdotL; // Quanta superficie guarda la luce
    float specular = (NdotL > 0.0) ? pow(NdotH, 32.0) : 0.0; // Bagliore

    vec4 albedo = texture(uTex, vUV); // Colore base del materiale

    // Luce direzionale modulata dal colore della luce
    vec3 lighting = albedo.rgb * (ambient + gubo.lightColor.rgb * (0.9 * diffuse + 0.4 * specular));

    outColor = vec4(lighting, albedo.a); // Colore in uscita del vertice
}

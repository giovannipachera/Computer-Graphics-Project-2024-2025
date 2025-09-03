#version 450

// Elementi in ingresso dalla struttura Vertex
layout(location = 0) in vec3 inPos; // Posizione 3D del vertice nello spazio locale
layout(location = 1) in vec2 inUV; // Coordinate texture per il mapping delle immagini
layout(location = 2) in vec3 inNorm; // Vettore normale per il calcolo dell'illuminazione

// Elementi in uscita verso PhongFrag
layout(location = 0) out vec2 vUV; // Coordinate texture elaborate
layout(location = 1) out vec3 vWorldPos; // Posizione finale nello spazio mondo
layout(location = 2) out vec3 vWorldNorm; // Direzione "normale" del punto per l'illuminazione

// UBO: dati condivisi, specifici per ogni oggetto
layout(set = 0, binding = 0) uniform UBO {
    mat4 mvpMat; // Model-View-Projection: trasforma sullo schermo
    mat4 mMat; // Model: sposta/ruota/scala
    mat4 nMat;   // Matrice normale: per trasformare correttamente l'illuminazione
} ubo;

// GUBO: dati condivisi, specifici per tutti gli oggetti
layout(set = 0, binding = 2) uniform GlobalUBO {
    vec3 lightDir; // Da dove arriva la luce
    vec4 lightColor; // Che colore/intensità ha la luce
    vec3 eyePos; // Dove si trova la camera
    vec4 eyeDir; // In che direzione guarda la camera
} gubo;

void main() {
    vec4 worldPos = ubo.mMat * vec4(inPos, 1.0); // Posizione nel mondo 3D globale
    vec3 worldNrm = normalize((ubo.nMat * vec4(inNorm, 0.0)).xyz);

    vUV = inUV;
    vWorldPos = worldPos.xyz;
    vWorldNorm = worldNrm;

    gl_Position = ubo.mvpMat * vec4(inPos, 1.0); // Posizione finale sullo schermo
}


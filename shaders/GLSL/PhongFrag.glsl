#version 450

const int DEBUG_MODE = 0; // 0 = off, 1 = mostra N, 2 = mostra N·L (heat), 3 = front/back

vec3 heat(float t){ return mix(vec3(0.1,0.1,0.7), vec3(0.9,0.1,0.1), clamp(t,0.0,1.0)); }

layout(location = 0) in vec2 vUV;
layout(location = 1) in vec3 vWorldPos;
layout(location = 2) in vec3 vWorldNorm;

layout(location = 0) out vec4 outColor;

// set=0, binding=1: texture del materiale
layout(set = 0, binding = 1) uniform sampler2D uTex;

// set=0, binding=2: dati globali (luce/camera)
layout(set = 0, binding = 2) uniform GlobalUBO {
    vec3 lightDir;  float _pad0;   // direzione della luce in world (verso la scena)
    vec4 lightColor;               // colore/intensità luce
    vec3 eyePos;    float _pad1;   // posizione camera in world
    vec4 eyeDir;                    // non usato qui
} gubo;

void main() {
    vec3 N = normalize(vWorldNorm);
    vec3 L = normalize(-gubo.lightDir);            // trattiamo lightDir come direzione della luce verso l'oggetto

if (DEBUG_MODE == 1) { outColor = vec4(normalize(vWorldNorm)*0.5+0.5,1.0); return; }
if (DEBUG_MODE == 2) { float ndl = max(dot(normalize(vWorldNorm), normalize(-gubo.lightDir)), 0.0);
                       outColor = vec4(heat(ndl),1.0); return; }
if (DEBUG_MODE == 3) { outColor = gl_FrontFacing? vec4(0,1,0,1): vec4(1,0,0,1); return; }


    vec3 V = normalize(gubo.eyePos - vWorldPos);  // verso la camera
    vec3 H = normalize(L + V);                    // Blinn-Phong

    // componenti
    float NdotL = max(dot(N, L), 0.0);
    float NdotH = max(dot(N, H), 0.0);

    // fattori base (semplici, look "sereno" senza nebbia)
    float ambient  = 0.18;               // luce ambiente sobria
    float diffuse  = NdotL;
    float specular = (NdotL > 0.0) ? pow(NdotH, 32.0) : 0.0; // highlight non invadente

    vec4 albedo = texture(uTex, vUV);    // usa i colori vivaci della texture

    // combinazione: luce direzionale modulata dal suo colore
    vec3 lit = albedo.rgb * (ambient + gubo.lightColor.rgb * (0.9*diffuse + 0.4*specular));

    outColor = vec4(lit, albedo.a);
}


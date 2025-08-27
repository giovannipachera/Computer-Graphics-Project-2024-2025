#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec3 inNorm;

layout(location = 0) out vec2 vUV;
layout(location = 1) out vec3 vWorldPos;
layout(location = 2) out vec3 vWorldNorm;

// set=0, binding=0  (per-oggetto)
layout(set = 0, binding = 0) uniform UBO {
    mat4 mvpMat;
    mat4 mMat;
    mat4 nMat;   // inverse-transpose of mMat, in 4x4
} ubo;

// set=0, binding=2  (globale: luce/camera) - non usato qui ma lo dichiariamo già se serve in futuro
layout(set = 0, binding = 2) uniform GlobalUBO {
    vec3 lightDir;  float _pad0;
    vec4 lightColor;
    vec3 eyePos;    float _pad1;
    vec4 eyeDir;    // non usato
} gubo;

void main() {
    // posizione e normale in world space
    vec4 worldPos = ubo.mMat * vec4(inPos, 1.0);
    vec3 worldNrm = normalize((ubo.nMat * vec4(inNorm, 0.0)).xyz);

    vUV        = inUV;
    vWorldPos  = worldPos.xyz;
    vWorldNorm = worldNrm;

    gl_Position = ubo.mvpMat * vec4(inPos, 1.0);
}


// TO MOVE
#define JSON_DIAGNOSTICS 1
#include "modules/Starter.hpp"
#include "modules/TextMaker.hpp"
#include "AudioPlayer.hpp"

#include <filesystem>
#include <iostream>
#include <unordered_set>

#if defined(__APPLE__)
  #include <mach-o/dyld.h>
#elif defined(_WIN32)
  #include <windows.h>
#endif

// ===== Utility: directory dell'eseguibile =====
static std::filesystem::path getExecutableDir() {
#if defined(__APPLE__)
    uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);
    std::string buf(size, '\0');
    if (_NSGetExecutablePath(buf.data(), &size) == 0) {
        std::error_code ec;
        auto p = std::filesystem::weakly_canonical(std::filesystem::path(buf), ec);
        return p.parent_path();
    }
    return std::filesystem::current_path();
#elif defined(__linux__)
    std::error_code ec;
    auto p = std::filesystem::read_symlink("/proc/self/exe", ec);
    if (!ec) return p.parent_path();
    return std::filesystem::current_path();
#elif defined(_WIN32)
    char buf[MAX_PATH]{0};
    GetModuleFileNameA(nullptr, buf, MAX_PATH);
    return std::filesystem::path(buf).parent_path();
#else
    return std::filesystem::current_path();
#endif
}

std::vector<SingleText> outText = {
    {1, {"Ciao", "Take a walk in the wild side!", "", ""}, 0, 0}
};

struct UniformBufferObject {
    alignas(16) glm::mat4 mvpMat;
    alignas(16) glm::mat4 mMat;
    alignas(16) glm::mat4 nMat;
};

struct GlobalUniformBufferObject {
    alignas(16) glm::vec3 lightDir;
    alignas(16) glm::vec4 lightColor;
    alignas(16) glm::vec3 eyePos;
    alignas(16) glm::vec4 eyeDir;
};

struct Vertex {
    glm::vec3 pos;
    glm::vec2 UV;
    glm::vec3 norm;
};

#include "modules/Scene.hpp"
#include "WVP.hpp"

class SIMULATOR : public BaseProject {
protected:
    DescriptorSetLayout DSL;
    VertexDescriptor    VD;
    Pipeline            P;

    Scene SC;
    glm::vec3 **deltaP;
    float *deltaA;
    float *usePitch;

    TextMaker txt;

    int   currScene = 0;
    float Ar;
    glm::vec3 Pos;
    float Yaw;
    glm::vec3 InitialPos;

    std::vector<std::string> tractorBodies = {"tbc", "tbb"};
    int currentBody = 0;

    AudioPlayer music;

    // Percorso ASSOLUTO agli assets (iniettato da CMake)
    const std::filesystem::path ASSETS_ROOT = std::filesystem::path(PROJECT_ASSETS_DIR);

    // Nomi file (non percorsi!) delle canzoni associate ai body
    std::vector<std::string> bodyMusic = {
        "classic.mp3",
        "barbie.mp3"
    };

    std::vector<std::string> tractorAxes    = {"ax"};
    std::vector<std::string> tractorWheels  = {"flw","frw","blw","brw"};
    std::vector<std::string> tractorFenders = {"lf", "rf"};
    std::vector<std::string> tractorScene   = {"pln","prm"};

    std::unordered_set<int> tractorPartIdx;
    int tbIndex = -1;

    void setWindowParameters() {
        windowWidth = 800;
        windowHeight = 600;
        windowTitle = "Hey buddy, are you a real country boy?";
        windowResizable = GLFW_TRUE;
        initialBackgroundColor = {0.0f, 0.85f, 1.0f, 1.0f};
        uniformBlocksInPool = 19*2 + 2;
        texturesInPool      = 19 + 1;
        setsInPool          = 19 + 1;
        Ar = 4.0f/3.0f;
    }

    void onWindowResize(int w, int h) {
        Ar = (float)w/(float)h;
    }

    // Helper: stampa contenuto directory (per debug)
    static void printDir(const std::filesystem::path& p, const char* tag){
        std::error_code ec;
        if (!std::filesystem::exists(p, ec)) {
            std::cerr << "[DBG] Dir '" << tag << "' NOT FOUND: " << p << "\n";
            return;
        }
        std::cerr << "[DBG] Listing '" << tag << "': " << p << "\n";
        for (auto& e : std::filesystem::directory_iterator(p, ec)) {
            std::cerr << "  - " << e.path().filename().string() << "\n";
        }
    }

    // Ritorna un path valido alla canzone cercando in 3 posti:
    // 1) accanto all'eseguibile: <exeDir>/assets/audio/<fileName>
    // 2) dalla CWD:              assets/audio/<fileName>
    // 3) dal path assoluto CMake: PROJECT_ASSETS_DIR/audio/<fileName>
    std::optional<std::filesystem::path> resolveMusicPath(const std::string& fileName) {
        const auto exeDir = getExecutableDir();
        std::vector<std::filesystem::path> cands = {
            exeDir / "assets" / "audio" / fileName,
            std::filesystem::path("assets") / "audio" / fileName,
            ASSETS_ROOT / "audio" / fileName
        };
        std::cerr << "[AUDIO][DBG] exeDir=" << exeDir << "\n";
        std::cerr << "[AUDIO][DBG] CWD="    << std::filesystem::current_path() << "\n";
        for (auto& c : cands) {
            std::cerr << "[AUDIO][DBG] cand=" << c << " exists=" << (std::filesystem::exists(c) ? "YES":"NO") << "\n";
            if (std::filesystem::exists(c)) return c;
        }
        return std::nullopt;
    }

    bool loadAndPlay(const std::filesystem::path& p, bool loop=true) {
        const std::string s = p.string();
        bool okLoad = music.load(s, loop);
        std::cerr << "[AUDIO] load('" << s << "') -> " << (okLoad ? "OK" : "FAIL") << "\n";
        if (!okLoad) return false;
        bool okPlay = music.play();
        std::cerr << "[AUDIO] play() -> " << (okPlay ? "OK" : "FAIL") << "\n";
        return okPlay;
    }

    void localInit() {
        DSL.init(this, {
            {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_ALL_GRAPHICS},
            {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT},
            {2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_ALL_GRAPHICS}
        });

        VD.init(this, { {0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX} }, {
            {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex,pos),  sizeof(glm::vec3), POSITION},
            {0, 1, VK_FORMAT_R32G32_SFLOAT,    offsetof(Vertex,UV),   sizeof(glm::vec2),  UV},
            {0, 2, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex,norm), sizeof(glm::vec3),  NORMAL}
        });

        P.init(this, &VD, "shaders/PhongVert.spv", "shaders/PhongFrag.spv", {&DSL});
        P.setAdvancedFeatures(VK_COMPARE_OP_LESS_OR_EQUAL, VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, false);

        // Carica scena
        SC.init(this, &VD, DSL, P, "assets/models/scene.json");
        txt.init(this, &outText);

        // ==== DEBUG directory utili
        printDir(getExecutableDir(), "exeDir");
        printDir(std::filesystem::current_path(), "CWD");
        printDir(ASSETS_ROOT, "ASSETS_ROOT");
        printDir(ASSETS_ROOT / "audio", "ASSETS_ROOT/audio");
        printDir(getExecutableDir() / "assets" / "audio", "exeDir/assets/audio");
        printDir("assets", "CWD/assets");
        printDir("assets/audio", "CWD/assets/audio");

        // --- AUDIO: risoluzione robusta del path e avvio in loop
        const std::string fileName = bodyMusic[currentBody];
        auto p = resolveMusicPath(fileName);
        if (p) {
            (void)loadAndPlay(*p, /*loop=*/true);
        } else {
            std::cerr << "[AUDIO][ERR] File audio non trovato in nessuno dei percorsi candidati: "
                         << fileName << "\n";
        }

        // Indici utili
        tbIndex = SC.InstanceIds["tb"];
        tractorPartIdx.insert(tbIndex);
        tractorPartIdx.insert(SC.InstanceIds["ax"]);
        tractorPartIdx.insert(SC.InstanceIds["flw"]);
        tractorPartIdx.insert(SC.InstanceIds["frw"]);
        tractorPartIdx.insert(SC.InstanceIds["blw"]);
        tractorPartIdx.insert(SC.InstanceIds["brw"]);
        tractorPartIdx.insert(SC.InstanceIds["lf"]);
        tractorPartIdx.insert(SC.InstanceIds["rf"]);

        // Pos del corpo
        Pos = glm::vec3(0.0f, 3.25f, 0.0f);
        InitialPos = Pos;
        Yaw = 0.0f;

        // Offset letti dal JSON (relativi al corpo)
        deltaP   = (glm::vec3**)calloc(SC.InstanceCount, sizeof(glm::vec3*));
        deltaA   = (float*)calloc(SC.InstanceCount, sizeof(float));
        usePitch = (float*)calloc(SC.InstanceCount, sizeof(float));

        for (int i=0; i<SC.InstanceCount; ++i) {
            deltaP[i] = new glm::vec3(glm::vec3(SC.I[i].Wm[3])); // estrai solo la traduzione dal JSON
            deltaA[i] = 0.0f;
            usePitch[i] = 0.0f;
        }
    }

    void pipelinesAndDescriptorSetsInit() {
        P.create();
        SC.pipelinesAndDescriptorSetsInit(DSL);
        txt.pipelinesAndDescriptorSetsInit();
    }
    void pipelinesAndDescriptorSetsCleanup() {
        P.cleanup();
        SC.pipelinesAndDescriptorSetsCleanup();
        txt.pipelinesAndDescriptorSetsCleanup();
    }
    void localCleanup() {
        for (int i=0;i<SC.InstanceCount;++i) delete deltaP[i];
        free(deltaP); free(deltaA); free(usePitch);
        DSL.cleanup(); P.destroy(); SC.localCleanup(); txt.localCleanup();
    }
    void populateCommandBuffer(VkCommandBuffer cb, int currentImage) {
        P.bind(cb);
        SC.populateCommandBuffer(cb, currentImage, P);
        txt.populateCommandBuffer(cb, currentImage, currScene);
    }

    // Correzione SOLO per 'prm' (GLTF Z-up) → mondo Y-up. OBJ (pln) invariato.
    glm::mat4 baseFor(const std::string& id) {
        if (id == "prm") {
            return glm::rotate(glm::mat4(1.0f), glm::half_pi<float>(), glm::vec3(1,0,0)); // +90° su X
        }
        return glm::mat4(1.0f);
    }

    void updateUniformBuffer(uint32_t currentImage) {
        float deltaT; glm::vec3 m(0), r(0); bool fire=false; getSixAxis(deltaT,m,r,fire);

        // --- input O/P per switch body/colore (e quindi brano)
        static int prevP = GLFW_RELEASE, prevO = GLFW_RELEASE;
        int curP = glfwGetKey(window, GLFW_KEY_P);
        int curO = glfwGetKey(window, GLFW_KEY_O);

        bool bodyChanged = false;
        if (curP == GLFW_PRESS && prevP == GLFW_RELEASE) {
            currentBody = (currentBody + 1) % (int)tractorBodies.size();
            bodyChanged = true;
        }
        if (curO == GLFW_PRESS && prevO == GLFW_RELEASE) {
            currentBody = (currentBody - 1 + (int)tractorBodies.size()) % (int)tractorBodies.size();
            bodyChanged = true;
        }
        prevP = curP; prevO = curO;

        if (bodyChanged) {
            music.stop();
            const std::string fn = bodyMusic[currentBody];
            auto p = resolveMusicPath(fn);
            if (p) {
                bool ok = loadAndPlay(*p, /*loop=*/true);
                std::cerr << "[AUDIO][SWITCH] " << (ok ? "OK" : "FAIL") << " (" << p->string() << ")\n";
            } else {
                std::cerr << "[AUDIO][SWITCH][ERR] File non trovato: " << fn << "\n";
            }
        }

        static float CamPitch = glm::radians(20.0f);
        static float CamYaw   = M_PI;
        static float CamDist  = 10.0f;
        static float CamRoll  = 0.0f;
        const  glm::vec3 CamTargetDelta = glm::vec3(0,2,0);

        static float SteeringAng = 0.0f;
        static float wheelRoll   = 0.0f;
        static float dampedVel   = 0.0f;

        const float STEERING_SPEED = glm::radians(30.0f);
        const float ROT_SPEED      = glm::radians(120.0f);
        const float MOVE_SPEED     = 2.5f;

        // Input → sterzo/velocità
        SteeringAng += -m.x * STEERING_SPEED * deltaT;
        SteeringAng  = glm::clamp(SteeringAng, glm::radians(-35.0f), glm::radians(35.0f));

        double lambdaVel=8.0, eps=0.001;
        dampedVel = MOVE_SPEED*deltaT*m.z*(1-exp(-lambdaVel*deltaT)) + dampedVel*exp(-lambdaVel*deltaT);
        if (fabs(dampedVel)<eps) dampedVel=0.0f;

        wheelRoll = fmod(wheelRoll - dampedVel/0.4f + 2*M_PI, 2*M_PI);

        // Cinematica semplice
        if (dampedVel!=0.0f) {
            if (SteeringAng!=0.0f) {
                const float l=2.78f;
                float rturn = l/tan(SteeringAng);
                float cx = Pos.x + rturn * cos(Yaw);
                float cz = Pos.z - rturn * sin(Yaw);
                float Dbeta = dampedVel / rturn;
                Yaw -= Dbeta;
                Pos.x = cx - rturn * cos(Yaw);
                Pos.z = cz + rturn * sin(Yaw);
            } else {
                Pos.x -= sin(Yaw)*dampedVel;
                Pos.z -= cos(Yaw)*dampedVel;
            }
            if (m.x==0.0f) {
                if (SteeringAng> STEERING_SPEED*deltaT) SteeringAng -= STEERING_SPEED*deltaT;
                else if (SteeringAng<-STEERING_SPEED*deltaT) SteeringAng += STEERING_SPEED*deltaT;
                else SteeringAng=0.0f;
            }
        }

        // Camera
        CamYaw   += ROT_SPEED*deltaT*r.y;
        CamPitch -= ROT_SPEED*deltaT*r.x;
        CamRoll  -= ROT_SPEED*deltaT*r.z;
        CamDist  -= MOVE_SPEED*deltaT*m.y;

        CamYaw   = glm::clamp(CamYaw,0.0f,2.0f*float(M_PI));
        CamPitch = glm::clamp(CamPitch,0.0f,float(M_PI_2)-0.01f);
        CamRoll  = glm::clamp(CamRoll,float(-M_PI),float(M_PI));
        CamDist  = glm::clamp(CamDist,7.0f,15.0f);

        glm::vec3 CamTarget = Pos + glm::vec3(glm::rotate(glm::mat4(1), Yaw, glm::vec3(0,1,0)) * glm::vec4(CamTargetDelta,1));
        glm::vec3 CamPos = CamTarget + glm::vec3(
            glm::rotate(glm::mat4(1), Yaw+CamYaw, glm::vec3(0,1,0)) *
            glm::rotate(glm::mat4(1), -CamPitch,  glm::vec3(1,0,0)) * glm::vec4(0,0,CamDist,1)
        );

        static glm::vec3 dampedCamPos = CamPos;
        const float lambdaCam=10.0f;
        dampedCamPos = CamPos*(1-exp(-lambdaCam*deltaT)) + dampedCamPos*exp(-lambdaCam*deltaT);
        glm::mat4 ViewPrj = MakeViewProjectionLookAt(dampedCamPos, CamTarget, glm::vec3(0,1,0),
                                                     CamRoll, glm::radians(90.0f), Ar, 0.1f, 500.0f);

        UniformBufferObject ubo{};
        GlobalUniformBufferObject gubo{};
        gubo.lightDir   = glm::vec3(cos(glm::radians(135.0f)), sin(glm::radians(135.0f)), 0.0f);
        gubo.lightColor = glm::vec4(1.0f);
        gubo.eyePos     = dampedCamPos;
        gubo.eyeDir     = glm::vec4(0,0,0,1);

        // --- Corpo: mostra SOLO l'istanza selezionata; le altre sottoterra
        {
            auto HideMat = [](const glm::mat4& baseTr) {
                return glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -10000.0f, 0.0f)) * baseTr;
            };

            for (int k = 0; k < (int)tractorBodies.size(); ++k) {
                const std::string& name = tractorBodies[k];
                int i = SC.InstanceIds[name];

                glm::mat4 baseTr = baseFor(name);
                if (k == currentBody) {
                    ubo.mMat   = MakeWorld(Pos, Yaw, 0.0f, 0.0f) * baseTr;
                } else {
                    ubo.mMat   = HideMat(baseTr);
                }

                ubo.mvpMat = ViewPrj * ubo.mMat;
                ubo.nMat   = glm::inverse(glm::transpose(ubo.mMat));

                SC.DS[i]->map(currentImage, &ubo, sizeof(ubo), 0);
                SC.DS[i]->map(currentImage, &gubo, sizeof(gubo), 2);
            }
        }

        // Assi
        for (const std::string& name : tractorAxes) {
            int i = SC.InstanceIds[name];
            glm::mat4 bodyTr  = MakeWorld(Pos, Yaw, 0.0f, 0.0f);
            glm::mat4 localTr = glm::translate(glm::mat4(1.0f), *deltaP[i]);
            glm::mat4 baseTr  = baseFor(name);
            ubo.mMat   = bodyTr * localTr * baseTr;
            ubo.mvpMat = ViewPrj * ubo.mMat;
            ubo.nMat   = glm::inverse(glm::transpose(ubo.mMat));
            SC.DS[i]->map(currentImage, &ubo, sizeof(ubo), 0);
            SC.DS[i]->map(currentImage, &gubo, sizeof(gubo), 2);
        }

        // Fasi rilievi ruote
        static float PhaseFrontRight = 0.0f;
        static float PhaseFrontLeft  = glm::radians(20.0f);
        static float PhaseRearRight  = 0.0f;
        static float PhaseRearLeft   = glm::radians(20.0f);

        // Ruote
        for (const std::string& name : tractorWheels) {
            int i = SC.InstanceIds[name];

            glm::mat4 bodyTr  = MakeWorld(Pos, Yaw, 0.0f, 0.0f);
            glm::mat4 localTr = glm::translate(glm::mat4(1.0f), *deltaP[i]);

            const bool isFront = (name=="flw" || name=="frw");
            glm::mat4 steerTr  = isFront
                ? glm::rotate(glm::mat4(1.0f), SteeringAng, glm::vec3(0,1,0))
                : glm::mat4(1.0f);

            float phase = 0.0f;
            if      (name == "frw") phase = PhaseFrontRight;
            else if (name == "flw") phase = PhaseFrontLeft;
            else if (name == "brw") phase = PhaseRearRight;
            else if (name == "blw") phase = PhaseRearLeft;

            glm::mat4 rollTr = glm::rotate(glm::mat4(1.0f), wheelRoll + phase, glm::vec3(1,0,0));
            glm::mat4 mirrorTr = (name == "flw" || name == "blw")
                ? glm::scale(glm::mat4(1.0f), glm::vec3(-1.0f, 1.0f, 1.0f))
                : glm::mat4(1.0f);

            glm::mat4 baseTr  = baseFor(name);

            ubo.mMat   = bodyTr * localTr * steerTr * rollTr * mirrorTr * baseTr;
            ubo.mvpMat = ViewPrj * ubo.mMat;
            ubo.nMat   = glm::inverse(glm::transpose(ubo.mMat));

            SC.DS[i]->map(currentImage, &ubo, sizeof(ubo), 0);
            SC.DS[i]->map(currentImage, &gubo, sizeof(gubo), 2);
        }

        // Parafanghi
        for (const std::string& name : tractorFenders) {
            int iF = SC.InstanceIds[name];
            const std::string wheelName = (name == "lf") ? "flw" : "frw";
            int iW = SC.InstanceIds[wheelName];

            glm::mat4 bodyTr   = MakeWorld(Pos, Yaw, 0.0f, 0.0f);
            glm::vec3 wheelP   = *deltaP[iW];              // pivot = posizione ruota
            glm::vec3 fendOff  = *deltaP[iF] - wheelP;     // offset parafango rispetto ruota

            glm::mat4 toWheel   = glm::translate(glm::mat4(1.0f), wheelP);
            glm::mat4 steerTr   = glm::rotate(glm::mat4(1.0f), SteeringAng, glm::vec3(0,1,0));
            glm::mat4 fromWheel = glm::translate(glm::mat4(1.0f), fendOff);

            glm::mat4 baseTr    = baseFor(name);

            ubo.mMat   = bodyTr * toWheel * steerTr * fromWheel * baseTr;
            ubo.mvpMat = ViewPrj * ubo.mMat;
            ubo.nMat   = glm::inverse(glm::transpose(ubo.mMat));

            SC.DS[iF]->map(currentImage, &ubo, sizeof(ubo), 0);
            SC.DS[iF]->map(currentImage, &gubo, sizeof(gubo), 2);
        }

        // Scenario
        for (const std::string& name : tractorScene) {
            int i = SC.InstanceIds[name];
            glm::mat4 baseTr = baseFor(name);
            ubo.mMat  = SC.I[i].Wm * baseTr;
            ubo.mvpMat= ViewPrj * ubo.mMat;
            ubo.nMat  = glm::inverse(glm::transpose(ubo.mMat));
            SC.DS[i]->map(currentImage, &ubo, sizeof(ubo), 0);
            SC.DS[i]->map(currentImage, &gubo, sizeof(gubo), 2);
        }
    }
};

int main() {
    SIMULATOR app;
    try { app.run(); }
    catch (const std::exception& e) { std::cerr << e.what() << std::endl; return EXIT_FAILURE; }
    return EXIT_SUCCESS;
}

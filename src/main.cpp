
// TO MOVE
#define JSON_DIAGNOSTICS 1
#include "AudioPlayer.hpp"
#include "modules/Starter.hpp"
#include "modules/TextMaker.hpp"
#include "modules/LogoMaker.hpp"

std::vector<SingleText> outText = {
    {1, {" ", "", "", ""}, 0, 0},
    {2, {"  ", "", "", ""}, 0, 0}};

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

#include "WVP.hpp"
#include "modules/Scene.hpp"
#include <unordered_set>
#include <cmath>

class SIMULATOR : public BaseProject {
protected:
  DescriptorSetLayout DSL;
  VertexDescriptor VD;
  Pipeline P;

  Scene SC;
  glm::vec3 **deltaP;
  float *deltaA;
  float *usePitch;

  TextMaker txt;
  LogoMaker logo;

  int currScene = 0;
  float Ar;
  glm::vec3 Pos;
  float Yaw;
  glm::vec3 InitialPos;

  std::vector<std::string> tractorBodies = {"tbc", "tbb"};
  int currentBody = 0;
  int lastBody = -1;

  AudioPlayer classicAudio;
  AudioPlayer barbieAudio;
  AudioPlayer hornAudio;

  std::vector<std::string> tractorAxes = {"axc", "axb"};
  std::vector<std::string> tractorWheels = {"flwc", "frwc", "blwc", "brwc", "flwb", "frwb", "blwb", "brwb"};
  std::vector<std::string> tractorFenders = {"lfc", "rfc", "lfb", "rfb"};
  std::vector<std::string> tractorPlow = {"p"};
  std::vector<std::string> tractorScene = {"pln", "prm"};
  std::vector<std::string> horse1 = {"hrs1"};
  std::vector<std::string> horse2 = {"hrs2"};
  std::vector<std::string> horse3 = {"hrs3"};
  std::vector<std::string> horse4 = {"hrs4"};
  std::vector<std::string> horse5 = {"hrs5"};
  std::vector<std::string> horse6 = {"hrs6"};
  std::vector<std::string> horse7 = {"hrs7"};
  std::vector<std::string> horse8 = {"hrs8"};
  std::vector<std::string> horse9 = {"hrs9"};
  std::vector<std::string> horse10 = {"hrs10"};
  std::vector<std::string> horse11 = {"hrs11"};




  std::unordered_set<int> tractorPartIdx;
  int tbIndex = -1;
  int plowIndex = -1;
  bool plowAttached = false;
  glm::vec3 plowOffset = glm::vec3(0.0f);

  void setWindowParameters() {
    windowWidth = 800;
    windowHeight = 600;
    windowTitle = "Country Roads";
    windowResizable = GLFW_TRUE;
    initialBackgroundColor = {0.4f, 0.8f, 1.0f, 1.0f};
    uniformBlocksInPool = 64;
    texturesInPool = 64;
    setsInPool = 64;
    Ar = 4.0f / 3.0f;
  }

  void onWindowResize(int w, int h) { Ar = (float)w / (float)h; }

  void localInit() {
    DSL.init(
        this,
        {{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS},
         {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
          VK_SHADER_STAGE_FRAGMENT_BIT},
         {2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS}});

    VD.init(this, {{0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX}},
            {{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos),
              sizeof(glm::vec3), POSITION},
             {0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, UV),
              sizeof(glm::vec2), UV},
             {0, 2, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, norm),
              sizeof(glm::vec3), NORMAL}});

    P.init(this, &VD, "shaders/PhongVert.spv", "shaders/PhongFrag.spv", {&DSL});
    P.setAdvancedFeatures(VK_COMPARE_OP_LESS_OR_EQUAL, VK_POLYGON_MODE_FILL,
                          VK_CULL_MODE_NONE, false);

    // Carica scena
    SC.init(this, &VD, DSL, P, "assets/models/scene.json");
    txt.init(this, &outText);
    logo.init(this);

    #ifdef _WIN32
    classicAudio.load("C:\\Users\\admin\\Desktop\\Computer Graphic project\\Computer-Graphics-Project-2024-2025\\assets\\audio\\classic.wav", true);
    barbieAudio.load("C:\\Users\\admin\\Desktop\\Computer Graphic project\\Computer-Graphics-Project-2024-2025\\assets\\audio\\barbie.wav", true);
    hornAudio.load("C:\\Users\\admin\\Desktop\\Computer Graphic project\\Computer-Graphics-Project-2024-2025\\assets\\audio\\horn.wav", true);
    #else
    classicAudio.load("assets/audio/classic.wav", true);
    barbieAudio.load("assets/audio/barbie.wav", true);
    hornAudio.load("assets/audio/horn.wav", true);
    #endif

    classicAudio.play();
    lastBody = currentBody;

    // Indici utili
    tractorPartIdx.insert(SC.InstanceIds["tbc"]);
    tractorPartIdx.insert(SC.InstanceIds["tbb"]);
    tractorPartIdx.insert(SC.InstanceIds["axc"]);
    tractorPartIdx.insert(SC.InstanceIds["axb"]);
    tractorPartIdx.insert(SC.InstanceIds["flwc"]);
    tractorPartIdx.insert(SC.InstanceIds["frwc"]);
    tractorPartIdx.insert(SC.InstanceIds["blwc"]);
    tractorPartIdx.insert(SC.InstanceIds["brwc"]);
    tractorPartIdx.insert(SC.InstanceIds["flwb"]);
    tractorPartIdx.insert(SC.InstanceIds["frwb"]);
    tractorPartIdx.insert(SC.InstanceIds["blwb"]);
    tractorPartIdx.insert(SC.InstanceIds["brwb"]);
    tractorPartIdx.insert(SC.InstanceIds["lfc"]);
    tractorPartIdx.insert(SC.InstanceIds["rfc"]);
    tractorPartIdx.insert(SC.InstanceIds["lfb"]);
    tractorPartIdx.insert(SC.InstanceIds["rfb"]);
    tractorPartIdx.insert(SC.InstanceIds["p"]);
    plowIndex = SC.InstanceIds["p"];

    // Pos del corpo: gestita da codice (non dal JSON)
    Pos = glm::vec3(-50.0f, 3.45f, 0.0f);
    InitialPos = Pos;
    Yaw = 0.0f;

    // Offset letti dal JSON (RELATIVI al corpo)
    deltaP = (glm::vec3 **)calloc(SC.InstanceCount, sizeof(glm::vec3 *));
    deltaA = (float *)calloc(SC.InstanceCount, sizeof(float));
    usePitch = (float *)calloc(SC.InstanceCount, sizeof(float));

    for (int i = 0; i < SC.InstanceCount; ++i) {
      deltaP[i] = new glm::vec3(
          glm::vec3(SC.I[i].Wm[3])); // estrai solo la traduzione dal JSON
      deltaA[i] = 0.0f;
      usePitch[i] = 0.0f;
    }
  }

  void pipelinesAndDescriptorSetsInit() {
    P.create();
    SC.pipelinesAndDescriptorSetsInit(DSL);
    txt.pipelinesAndDescriptorSetsInit();
    logo.pipelinesAndDescriptorSetsInit();
  }
  void pipelinesAndDescriptorSetsCleanup() {
    P.cleanup();
    SC.pipelinesAndDescriptorSetsCleanup();
    txt.pipelinesAndDescriptorSetsCleanup();
    logo.pipelinesAndDescriptorSetsCleanup();
  }
  void localCleanup() {
    for (int i = 0; i < SC.InstanceCount; ++i)
      delete deltaP[i];
    free(deltaP);
    free(deltaA);
    free(usePitch);
    classicAudio.stop(true);
    barbieAudio.stop(true);
    hornAudio.stop(true);
    DSL.cleanup();
    P.destroy();
    SC.localCleanup();
    txt.localCleanup();
    logo.localCleanup();
  }
  void populateCommandBuffer(VkCommandBuffer cb, int currentImage) {
    P.bind(cb);
    SC.populateCommandBuffer(cb, currentImage, P);

    logo.populateCommandBuffer(cb, currentImage);

    txt.populateCommandBuffer(cb, currentImage, currScene);
  }

  void reRecordCommandBuffers() {
    vkDeviceWaitIdle(device);
    vkFreeCommandBuffers(device, commandPool,
                         static_cast<uint32_t>(commandBuffers.size()),
                         commandBuffers.data());
    createCommandBuffers();
  }

  void refreshUIResources() {
    logo.pipelinesAndDescriptorSetsCleanup();
    txt.pipelinesAndDescriptorSetsCleanup();
    vkDestroyDescriptorPool(device, descriptorPool, nullptr);
    createDescriptorPool();
    pipelinesAndDescriptorSetsInit();
    reRecordCommandBuffers();
  }

  // Correzione SOLO per 'prm' (GLTF Z-up) → mondo Y-up. OBJ (pln) invariato.
  glm::mat4 baseFor(const std::string &id) {
    if (id == "prm") {
      return glm::rotate(glm::mat4(1.0f), glm::half_pi<float>(),
                         glm::vec3(1, 0, 0)); // +90° su X
    }
    return glm::mat4(1.0f);
  }

  void updateUniformBuffer(uint32_t currentImage) {
    float deltaT;
    glm::vec3 m(0), r(0);
    bool fire = false, next = false, prev = false, horn = false;
    getSixAxis(deltaT, m, r, fire, next, prev, horn);

    // Clamp del timestep per stabilità
    const float MAX_DELTA_T = 0.05f;
    if (deltaT > MAX_DELTA_T)
      deltaT = MAX_DELTA_T;

    static bool prevP = false;
    bool pPressed = next && !prevP;
    prevP = next;

    if (pPressed) {
      currentBody = (currentBody + 1) % (int)tractorBodies.size();
    }

    static bool prevO = false;
    bool oPressed = prev && !prevO;
    prevO = prev;

    if (oPressed) {
      currentBody = (currentBody - 1 + (int)tractorBodies.size()) %
                    (int)tractorBodies.size();
    }

    static bool prevK = false;
    bool kPressed = horn && !prevK;
    prevK = horn;

    if (kPressed) {
      hornAudio.play();
      hornAudio.stop(false);
    }

    if (currentBody != lastBody) {
      if (lastBody == 0)
        classicAudio.stop(false);
      else if (lastBody == 1)
        barbieAudio.stop(false);

      if (currentBody == 0)
        classicAudio.play();
      else if (currentBody == 1)
        barbieAudio.play();

      logo.setCurrentLogo(currentBody);
      currScene = currentBody;
      refreshUIResources();

      lastBody = currentBody;
    }


    // Stato veicolo semplificato (invariato)
    static float SteeringAng = 0.0f;
    static float wheelRoll = 0.0f;
    static float dampedVel = 0.0f;

    const float STEERING_SPEED = glm::radians(30.0f);
    const float MOVE_SPEED = 5.5f;

    // Input → sterzo/velocità
    SteeringAng += -m.x * STEERING_SPEED * deltaT;
    SteeringAng =
        glm::clamp(SteeringAng, glm::radians(-35.0f), glm::radians(35.0f));

    double lambdaVel = 8.0, eps = 0.001;
    dampedVel = MOVE_SPEED * deltaT * m.z * (1 - exp(-lambdaVel * deltaT)) +
                dampedVel * exp(-lambdaVel * deltaT);
    if (fabs(dampedVel) < eps)
      dampedVel = 0.0f;

    wheelRoll = fmod(wheelRoll - dampedVel / 0.4f + 2 * M_PI, 2 * M_PI);

    // Cinematica semplice
    if (dampedVel != 0.0f) {
      if (SteeringAng != 0.0f) {
        const float l = 2.78f;
        float rturn = l / tan(SteeringAng);
        float cx = Pos.x + rturn * cos(Yaw);
        float cz = Pos.z - rturn * sin(Yaw);
        float Dbeta = dampedVel / rturn;
        Yaw -= Dbeta;
        Pos.x = cx - rturn * cos(Yaw);
        Pos.z = cz + rturn * sin(Yaw);
      } else {
        Pos.x -= sin(Yaw) * dampedVel;
        Pos.z -= cos(Yaw) * dampedVel;
      }
      if (m.x == 0.0f) {
        if (SteeringAng > STEERING_SPEED * deltaT)
          SteeringAng -= STEERING_SPEED * deltaT;
        else if (SteeringAng < -STEERING_SPEED * deltaT)
          SteeringAng += STEERING_SPEED * deltaT;
        else
          SteeringAng = 0.0f;
      }
    }

    // Attacca strozapaglia (plow) quando vicino e premi "fire"
    static bool prevFire = false;
    bool firePressed = fire && !prevFire;
    prevFire = fire;

    if (!plowAttached && firePressed) {
      glm::vec3 plowWorldPos = glm::vec3(SC.I[plowIndex].Wm[3]);
      if (glm::length(plowWorldPos - Pos) < 5.0f) {
        plowAttached = true;
        plowOffset =
            glm::vec3(glm::rotate(glm::mat4(1.0f), -Yaw, glm::vec3(0, 1, 0)) *
                      glm::vec4(plowWorldPos - Pos, 0.0f));
      }
    }

    // =========================
    //   CAMERA ORBITALE (Frecce + Right Stick PS)
    //   - ←/→: ruota attorno al trattore (azimuth)
    //   - ↑/↓: alza/abbassa l'elevazione
    //   - R/F : zoom in / zoom out
    //   - Right Stick (PS): stessa logica delle frecce (r.x -> elevazione, r.y -> azimuth)
    //   Nota: nessuno smoothing, nessun roll
    // =========================
    static float camDist = 13.0f;                          // distanza iniziale un po' maggiore
    static float camEl   = glm::radians(25.0f);            // elevazione rispetto all'orizzontale
    static float camAz   = glm::pi<float>();

    const float ORBIT_SPEED = glm::radians(90.0f);         // deg/s
    const float ZOOM_SPEED  = 18.0f;                       // unità/s
    const float MIN_DIST    = 8.0f;
    const float MAX_DIST    = 35.0f;

    camDist = glm::clamp(camDist - 0.2f * m.y * ZOOM_SPEED * deltaT, MIN_DIST, MAX_DIST);
    camAz -= 0.5*(r.y * ORBIT_SPEED * deltaT);
    camEl  = glm::clamp(camEl + 0.5f*(r.x * ORBIT_SPEED * deltaT), glm::radians(5.0f), glm::radians(80.0f));

    // Punto che vogliamo guardare (leggermente sopra il corpo)
    glm::vec3 CamTarget = Pos + glm::vec3(0, 2, 0);

    // Offset camera in sistema locale del trattore (sferico: distanza + elevazione + azimuth)
    glm::mat4 yawRot   = glm::rotate(glm::mat4(1.0f), Yaw + camAz, glm::vec3(0, 1, 0));
    glm::mat4 pitchRot = glm::rotate(glm::mat4(1.0f), -camEl,       glm::vec3(1, 0, 0));
    glm::vec3 camOff   = glm::vec3(yawRot * pitchRot * glm::vec4(0, 0, camDist, 1));

    glm::vec3 CamPos = CamTarget + camOff;

    // View-Projection senza roll e senza smoothing
    glm::mat4 ViewPrj = MakeViewProjectionLookAt(
        CamPos,                // posizione camera
        CamTarget,             // punto guardato
        glm::vec3(0, 1, 0),    // up
        0.0f,                  // roll eliminato
        glm::radians(70.0f),   // FOV più naturale
        Ar,
        0.1f,
        500.0f);

    // Global UBO
    GlobalUniformBufferObject gubo{};
    gubo.lightDir =
        glm::vec3(cos(glm::radians(135.0f)), sin(glm::radians(135.0f)), 0.0f);
    gubo.lightColor = glm::vec4(1.0f);
    gubo.eyePos = CamPos;
    gubo.eyeDir = glm::vec4(0, 0, 0, 1);

    UniformBufferObject ubo{};

    auto HideMat = [](const glm::mat4 &baseTr) {
      return glm::translate(glm::mat4(1.0f),
                            glm::vec3(0.0f, -10000.0f, 0.0f)) *
             baseTr;
    };

    // --- Corpo: mostra SOLO l'istanza selezionata; le altre le portiamo sottoterra ---
    for (int k = 0; k < (int)tractorBodies.size(); ++k) {
      const std::string &name = tractorBodies[k];
      int i = SC.InstanceIds[name];

      glm::mat4 baseTr = baseFor(name);
      if (k == currentBody) {
        // corpo visibile: segue Pos/Yaw come prima
        ubo.mMat = MakeWorld(Pos, Yaw, 0.0f, 0.0f) * baseTr;
      } else {
        // corpo nascosto: lo trasliamo molto sotto
        ubo.mMat = HideMat(baseTr);
      }

      ubo.mvpMat = ViewPrj * ubo.mMat;
      ubo.nMat = glm::inverse(glm::transpose(ubo.mMat));

      SC.DS[i]->map(currentImage, &ubo, sizeof(ubo), 0);
      SC.DS[i]->map(currentImage, &gubo, sizeof(gubo), 2);
    }

    // Assi: solo movimento globale + offset locale. NIENTE SteeringAng.
    for (const std::string &name : tractorAxes) {
      int i = SC.InstanceIds[name];
      glm::mat4 baseTr = baseFor(name);
      bool isBarbie = (name.back() == 'b');
      if ((isBarbie && currentBody == 1) || (!isBarbie && currentBody == 0)) {
        glm::mat4 bodyTr = MakeWorld(Pos, Yaw, 0.0f, 0.0f);
        glm::mat4 localTr = glm::translate(glm::mat4(1.0f), *deltaP[i]);
        ubo.mMat = bodyTr * localTr * baseTr;
      } else {
        ubo.mMat = HideMat(baseTr);
      }
      ubo.mvpMat = ViewPrj * ubo.mMat;
      ubo.nMat = glm::inverse(glm::transpose(ubo.mMat));
      SC.DS[i]->map(currentImage, &ubo, sizeof(ubo), 0);
      SC.DS[i]->map(currentImage, &gubo, sizeof(gubo), 2);
    }

    // --- Allineamento tasselli (fase iniziale ruote)
    static float PhaseFrontRight = 0.0f;               // riferimento
    static float PhaseFrontLeft = glm::radians(20.0f); // tweak
    static float PhaseRearRight = 0.0f;                // riferimento
    static float PhaseRearLeft = glm::radians(20.0f);  // tweak

    // Ruote
    for (const std::string &name : tractorWheels) {
      int i = SC.InstanceIds[name];
      glm::mat4 baseTr = baseFor(name);
      bool isBarbie = (name.back() == 'b');
      std::string baseName = name.substr(0, name.size() - 1);
      if ((isBarbie && currentBody == 1) || (!isBarbie && currentBody == 0)) {
        glm::mat4 bodyTr = MakeWorld(Pos, Yaw, 0.0f, 0.0f);
        glm::mat4 localTr = glm::translate(glm::mat4(1.0f), *deltaP[i]);

        const bool isFront = (baseName == "flw" || baseName == "frw");
        glm::mat4 steerTr =
            isFront ? glm::rotate(glm::mat4(1.0f), SteeringAng,
                                  glm::vec3(0, 1, 0))
                    : glm::mat4(1.0f);

        float phase = 0.0f;
        if (baseName == "frw")
          phase = PhaseFrontRight;
        else if (baseName == "flw")
          phase = PhaseFrontLeft;
        else if (baseName == "brw")
          phase = PhaseRearRight;
        else if (baseName == "blw")
          phase = PhaseRearLeft;

        glm::mat4 rollTr =
            glm::rotate(glm::mat4(1.0f), wheelRoll + phase, glm::vec3(1, 0, 0));

        glm::mat4 mirrorTr =
            (baseName == "flw" || baseName == "blw")
                ? glm::scale(glm::mat4(1.0f), glm::vec3(-1.0f, 1.0f, 1.0f))
                : glm::mat4(1.0f);

        ubo.mMat = bodyTr * localTr * steerTr * rollTr * mirrorTr * baseTr;
      } else {
        ubo.mMat = HideMat(baseTr);
      }
      ubo.mvpMat = ViewPrj * ubo.mMat;
      ubo.nMat = glm::inverse(glm::transpose(ubo.mMat));

      SC.DS[i]->map(currentImage, &ubo, sizeof(ubo), 0);
      SC.DS[i]->map(currentImage, &gubo, sizeof(gubo), 2);
    }

    // Parafanghi: sterzano attorno al centro RUOTA (nessun roll)
    for (const std::string &name : tractorFenders) {
      int iF = SC.InstanceIds[name];
      glm::mat4 baseTr = baseFor(name);
      bool isBarbie = (name.back() == 'b');
      std::string baseName = name.substr(0, name.size() - 1);
      if ((isBarbie && currentBody == 1) || (!isBarbie && currentBody == 0)) {
        std::string suffix(1, name.back());
        const std::string wheelName =
            (baseName == "lf") ? "flw" + suffix : "frw" + suffix;
        int iW = SC.InstanceIds[wheelName];

        glm::mat4 bodyTr = MakeWorld(Pos, Yaw, 0.0f, 0.0f);
        glm::vec3 wheelP = *deltaP[iW]; // pivot = posizione ruota dal JSON
        glm::vec3 fendOff =
            *deltaP[iF] - wheelP; // offset parafango rispetto alla ruota

        glm::mat4 toWheel = glm::translate(glm::mat4(1.0f), wheelP);
        glm::mat4 steerTr =
            glm::rotate(glm::mat4(1.0f), SteeringAng, glm::vec3(0, 1, 0));
        glm::mat4 fromWheel = glm::translate(glm::mat4(1.0f), fendOff);

        ubo.mMat = bodyTr * toWheel * steerTr * fromWheel * baseTr;
      } else {
        ubo.mMat = HideMat(baseTr);
      }
      ubo.mvpMat = ViewPrj * ubo.mMat;
      ubo.nMat = glm::inverse(glm::transpose(ubo.mMat));

      SC.DS[iF]->map(currentImage, &ubo, sizeof(ubo), 0);
      SC.DS[iF]->map(currentImage, &gubo, sizeof(gubo), 2);
    }

    // Aratro (plow) e scena
    for (const std::string &name : tractorPlow) {
      int i = SC.InstanceIds[name];
      glm::mat4 baseTr = baseFor(name);
      if (plowAttached) {
        glm::mat4 bodyTr = MakeWorld(Pos, Yaw, 0.0f, 0.0f);
        glm::mat4 localTr = glm::translate(glm::mat4(1.0f), plowOffset);
        ubo.mMat = bodyTr * localTr * baseTr;
      } else {
        ubo.mMat = SC.I[i].Wm * baseTr;
      }
      ubo.mvpMat = ViewPrj * ubo.mMat;
      ubo.nMat = glm::inverse(glm::transpose(ubo.mMat));
      SC.DS[i]->map(currentImage, &ubo, sizeof(ubo), 0);
      SC.DS[i]->map(currentImage, &gubo, sizeof(gubo), 2);
    }

    for (const std::string &name : tractorScene) {
      int i = SC.InstanceIds[name];
      glm::mat4 baseTr = baseFor(name);
      ubo.mMat = SC.I[i].Wm * baseTr;
      ubo.mvpMat = ViewPrj * ubo.mMat;
      ubo.nMat = glm::inverse(glm::transpose(ubo.mMat));
      SC.DS[i]->map(currentImage, &ubo, sizeof(ubo), 0);
      SC.DS[i]->map(currentImage, &gubo, sizeof(gubo), 2);
    }

    // Mostra il cavallo 'hrs1' al centro del campo
    for (const std::string &name : horse1) {
      int i = SC.InstanceIds[name];

      // Base rotation (se GLTF Z-up)
      glm::mat4 baseHr = baseFor(name);

      // Rotazione di 90° attorno a Z
      glm::mat4 rotHr = glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0,0,1));
      glm::mat4 rotHr2 = glm::rotate(glm::mat4(1.0f), glm::radians(270.0f), glm::vec3(1,0,0));
      glm::mat4 rotHr3 = glm::rotate(glm::mat4(1.0f), glm::radians(170.0f), glm::vec3(1,1,0));



      // Scala il cavallo (1.5x)
      glm::mat4 scaleHr = glm::scale(glm::mat4(1.0f), glm::vec3(3.0f));

      // Traslazione: posizione centrale del campo
      // Y = altezza del piano + offset del cavallo
      float yOffset = 0.5f; // regola in base all'altezza del cavallo
      glm::mat4 transHr = glm::translate(glm::mat4(1.0f),
                                         glm::vec3(6.0f, 0.5f , -95.0f));

      // Composizione finale: traslazione * scala * base
      ubo.mMat = transHr * rotHr3* rotHr * rotHr2 * scaleHr * baseHr;
      ubo.mvpMat = ViewPrj * ubo.mMat;
      ubo.nMat = glm::inverse(glm::transpose(ubo.mMat));

      SC.DS[i]->map(currentImage, &ubo, sizeof(ubo), 0);
      SC.DS[i]->map(currentImage, &gubo, sizeof(gubo), 2);

    }

    // Mostra il cavallo 'hrs1' al centro del campo
    for (const std::string &name : horse1) {
      int i = SC.InstanceIds[name];

      // Base rotation (se GLTF Z-up)
      glm::mat4 baseHr = baseFor(name);

      // Rotazione di 90° attorno a Z
      glm::mat4 rotHr = glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0,0,1));
      glm::mat4 rotHr2 = glm::rotate(glm::mat4(1.0f), glm::radians(270.0f), glm::vec3(1,0,0));


      // Scala il cavallo (1.5x)
      glm::mat4 scaleHr = glm::scale(glm::mat4(1.0f), glm::vec3(3.0f));

      // Traslazione: posizione centrale del campo
      // Y = altezza del piano + offset del cavallo
      float yOffset = 0.5f; // regola in base all'altezza del cavallo
      glm::mat4 transHr = glm::translate(glm::mat4(1.0f),
                                         glm::vec3(6.0f, 0.5f , -110.0f));

      // Composizione finale: traslazione * scala * base
      ubo.mMat = transHr * rotHr * rotHr2 * scaleHr * baseHr;
      ubo.mvpMat = ViewPrj * ubo.mMat;
      ubo.nMat = glm::inverse(glm::transpose(ubo.mMat));

      SC.DS[i]->map(currentImage, &ubo, sizeof(ubo), 0);
      SC.DS[i]->map(currentImage, &gubo, sizeof(gubo), 2);

    }

     // Mostra il cavallo 'hrs2' al centro del campo
    for (const std::string &name : horse2) {
      int i = SC.InstanceIds[name];

      // Base rotation (se GLTF Z-up)
      glm::mat4 baseHr = baseFor(name);

      // Rotazione di 90° attorno a Z
      glm::mat4 rotHr = glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0,0,1));
      glm::mat4 rotHr2 = glm::rotate(glm::mat4(1.0f), glm::radians(270.0f), glm::vec3(1,0,0));
      glm::mat4 rotHr3 = glm::rotate(glm::mat4(1.0f), glm::radians(-110.0f), glm::vec3(0,1,0));


      // Scala il cavallo (1.5x)
      glm::mat4 scaleHr = glm::scale(glm::mat4(1.0f), glm::vec3(3.0f));

      // Traslazione: posizione centrale del campo
      // Y = altezza del piano + offset del cavallo
      float yOffset = 0.5f; // regola in base all'altezza del cavallo
      glm::mat4 transHr = glm::translate(glm::mat4(1.0f),
                                         glm::vec3(20.0f, 0.5f , -100.0f));

      // Composizione finale: traslazione * scala * base
      ubo.mMat = transHr * rotHr3* rotHr * rotHr2 * scaleHr * baseHr;
      ubo.mvpMat = ViewPrj * ubo.mMat;
      ubo.nMat = glm::inverse(glm::transpose(ubo.mMat));

      SC.DS[i]->map(currentImage, &ubo, sizeof(ubo), 0);
      SC.DS[i]->map(currentImage, &gubo, sizeof(gubo), 2);





    }
    // Mostra il cavallo 'hrs3' al centro del campo
    for (const std::string &name : horse3) {
      int i = SC.InstanceIds[name];

      // Base rotation (se GLTF Z-up)
      glm::mat4 baseHr = baseFor(name);

      // Rotazione di 90° attorno a Z
      glm::mat4 rotHr = glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0,0,1));
      glm::mat4 rotHr2 = glm::rotate(glm::mat4(1.0f), glm::radians(270.0f), glm::vec3(1,0,0));
      glm::mat4 rotHr3 = glm::rotate(glm::mat4(1.0f), glm::radians(48.0f), glm::vec3(0,1,0));



      // Scala il cavallo (1.5x)
      glm::mat4 scaleHr = glm::scale(glm::mat4(1.0f), glm::vec3(3.0f));

      // Traslazione: posizione centrale del campo
      // Y = altezza del piano + offset del cavallo
      float yOffset = 0.5f; // regola in base all'altezza del cavallo
      glm::mat4 transHr = glm::translate(glm::mat4(1.0f),
                                         glm::vec3(-5.0f, 0.5f , -100.0f));

      // Composizione finale: traslazione * scala * base
      ubo.mMat = transHr * rotHr3* rotHr * rotHr2 * scaleHr * baseHr;
      ubo.mvpMat = ViewPrj * ubo.mMat;
      ubo.nMat = glm::inverse(glm::transpose(ubo.mMat));

      SC.DS[i]->map(currentImage, &ubo, sizeof(ubo), 0);
      SC.DS[i]->map(currentImage, &gubo, sizeof(gubo), 2);





    }
    // Mostra il cavallo 'hrs4' al centro del campo
    for (const std::string &name : horse4) {
      int i = SC.InstanceIds[name];

      // Base rotation (se GLTF Z-up)
      glm::mat4 baseHr = baseFor(name);

      // Rotazione di 90° attorno a Z
      glm::mat4 rotHr = glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0,0,1));
      glm::mat4 rotHr2 = glm::rotate(glm::mat4(1.0f), glm::radians(270.0f), glm::vec3(1,0,0));
      glm::mat4 rotHr3= glm::rotate(glm::mat4(1.0f), glm::radians(-60.0f), glm::vec3(0,1,0));



      // Scala il cavallo (1.5x)
      glm::mat4 scaleHr = glm::scale(glm::mat4(1.0f), glm::vec3(3.0f));

      // Traslazione: posizione centrale del campo
      // Y = altezza del piano + offset del cavallo
      float yOffset = 0.5f; // regola in base all'altezza del cavallo
      glm::mat4 transHr = glm::translate(glm::mat4(1.0f),
                                         glm::vec3(-6.0f, 0.5f , -120.0f));

      // Composizione finale: traslazione * scala * base
      ubo.mMat = transHr * rotHr3* rotHr * rotHr2 * scaleHr * baseHr;
      ubo.mvpMat = ViewPrj * ubo.mMat;
      ubo.nMat = glm::inverse(glm::transpose(ubo.mMat));

      SC.DS[i]->map(currentImage, &ubo, sizeof(ubo), 0);
      SC.DS[i]->map(currentImage, &gubo, sizeof(gubo), 2);





    }
    // Mostra il cavallo 'hrs5' al centro del campo
    for (const std::string &name : horse5) {
      int i = SC.InstanceIds[name];

      // Base rotation (se GLTF Z-up)
      glm::mat4 baseHr = baseFor(name);

      // Rotazione di 90° attorno a Z
      glm::mat4 rotHr = glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0,0,1));
      glm::mat4 rotHr2 = glm::rotate(glm::mat4(1.0f), glm::radians(270.0f), glm::vec3(1,0,0));
      glm::mat4 rotHr3 = glm::rotate(glm::mat4(1.0f), glm::radians(-26.0f), glm::vec3(0,1,0));


      // Scala il cavallo (1.5x)
      glm::mat4 scaleHr = glm::scale(glm::mat4(1.0f), glm::vec3(3.0f));

      // Traslazione: posizione centrale del campo
      // Y = altezza del piano + offset del cavallo
      float yOffset = 0.5f; // regola in base all'altezza del cavallo
      glm::mat4 transHr = glm::translate(glm::mat4(1.0f),
                                         glm::vec3(40.0f, 0.5f , -110.0f));

      // Composizione finale: traslazione * scala * base
      ubo.mMat = transHr* rotHr3 * rotHr * rotHr2 * scaleHr * baseHr;
      ubo.mvpMat = ViewPrj * ubo.mMat;
      ubo.nMat = glm::inverse(glm::transpose(ubo.mMat));

      SC.DS[i]->map(currentImage, &ubo, sizeof(ubo), 0);
      SC.DS[i]->map(currentImage, &gubo, sizeof(gubo), 2);





    }

    // Mostra il cavallo 'hrs6' al centro del campo
    for (const std::string &name : horse6) {
      int i = SC.InstanceIds[name];

      // Base rotation (se GLTF Z-up)
      glm::mat4 baseHr = baseFor(name);

      // Rotazione di 90° attorno a Z
      glm::mat4 rotHr = glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0,0,1));
      glm::mat4 rotHr2 = glm::rotate(glm::mat4(1.0f), glm::radians(270.0f), glm::vec3(1,0,0));
      glm::mat4 rotHr3 = glm::rotate(glm::mat4(1.0f), glm::radians(-120.0f), glm::vec3(0,1,0));



      // Scala il cavallo (1.5x)
      glm::mat4 scaleHr = glm::scale(glm::mat4(1.0f), glm::vec3(3.0f));

      // Traslazione: posizione centrale del campo
      // Y = altezza del piano + offset del cavallo
      float yOffset = 0.5f; // regola in base all'altezza del cavallo
      glm::mat4 transHr = glm::translate(glm::mat4(1.0f),
                                         glm::vec3(30.0f, 0.5f , -90.0f));

      // Composizione finale: traslazione * scala * base
      ubo.mMat = transHr * rotHr3* rotHr * rotHr2 * scaleHr * baseHr;
      ubo.mvpMat = ViewPrj * ubo.mMat;
      ubo.nMat = glm::inverse(glm::transpose(ubo.mMat));

      SC.DS[i]->map(currentImage, &ubo, sizeof(ubo), 0);
      SC.DS[i]->map(currentImage, &gubo, sizeof(gubo), 2);

    }

     // Mostra il cavallo 'hrs7' al centro del campo
    for (const std::string &name : horse7) {
      int i = SC.InstanceIds[name];

      // Base rotation (se GLTF Z-up)
      glm::mat4 baseHr = baseFor(name);

      // Rotazione di 90° attorno a Z
      glm::mat4 rotHr = glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0,0,1));
      glm::mat4 rotHr2 = glm::rotate(glm::mat4(1.0f), glm::radians(270.0f), glm::vec3(1,0,0));
      glm::mat4 rotHr3 = glm::rotate(glm::mat4(1.0f), glm::radians(110.0f), glm::vec3(0,1,0));


      // Scala il cavallo (1.5x)
      glm::mat4 scaleHr = glm::scale(glm::mat4(1.0f), glm::vec3(3.0f));

      // Traslazione: posizione centrale del campo
      // Y = altezza del piano + offset del cavallo
      float yOffset = 0.5f; // regola in base all'altezza del cavallo
      glm::mat4 transHr = glm::translate(glm::mat4(1.0f),
                                         glm::vec3(-20.0f, 0.5f , -120.0f));

      // Composizione finale: traslazione * scala * base
      ubo.mMat = transHr * rotHr3* rotHr * rotHr2 * scaleHr * baseHr;
      ubo.mvpMat = ViewPrj * ubo.mMat;
      ubo.nMat = glm::inverse(glm::transpose(ubo.mMat));

      SC.DS[i]->map(currentImage, &ubo, sizeof(ubo), 0);
      SC.DS[i]->map(currentImage, &gubo, sizeof(gubo), 2);





    }
    // Mostra il cavallo 'hrs8' al centro del campo
    for (const std::string &name : horse8) {
      int i = SC.InstanceIds[name];

      // Base rotation (se GLTF Z-up)
      glm::mat4 baseHr = baseFor(name);

      // Rotazione di 90° attorno a Z
      glm::mat4 rotHr = glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0,0,1));
      glm::mat4 rotHr2 = glm::rotate(glm::mat4(1.0f), glm::radians(270.0f), glm::vec3(1,0,0));
      glm::mat4 rotHr3 = glm::rotate(glm::mat4(1.0f), glm::radians(-48.0f), glm::vec3(0,1,0));



      // Scala il cavallo (1.5x)
      glm::mat4 scaleHr = glm::scale(glm::mat4(1.0f), glm::vec3(3.0f));

      // Traslazione: posizione centrale del campo
      // Y = altezza del piano + offset del cavallo
      float yOffset = 0.5f; // regola in base all'altezza del cavallo
      glm::mat4 transHr = glm::translate(glm::mat4(1.0f),
                                         glm::vec3(25.0f, 0.5f , -115.0f));

      // Composizione finale: traslazione * scala * base
      ubo.mMat = transHr * rotHr3* rotHr * rotHr2 * scaleHr * baseHr;
      ubo.mvpMat = ViewPrj * ubo.mMat;
      ubo.nMat = glm::inverse(glm::transpose(ubo.mMat));

      SC.DS[i]->map(currentImage, &ubo, sizeof(ubo), 0);
      SC.DS[i]->map(currentImage, &gubo, sizeof(gubo), 2);





    }
    // Mostra il cavallo 'hrs9' al centro del campo
    for (const std::string &name : horse9) {
      int i = SC.InstanceIds[name];

      // Base rotation (se GLTF Z-up)
      glm::mat4 baseHr = baseFor(name);

      // Rotazione di 90° attorno a Z
      glm::mat4 rotHr = glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0,0,1));
      glm::mat4 rotHr2 = glm::rotate(glm::mat4(1.0f), glm::radians(270.0f), glm::vec3(1,0,0));
      glm::mat4 rotHr3= glm::rotate(glm::mat4(1.0f), glm::radians(210.0f), glm::vec3(0,1,0));



      // Scala il cavallo (1.5x)
      glm::mat4 scaleHr = glm::scale(glm::mat4(1.0f), glm::vec3(3.0f));

      // Traslazione: posizione centrale del campo
      // Y = altezza del piano + offset del cavallo
      float yOffset = 0.5f; // regola in base all'altezza del cavallo
      glm::mat4 transHr = glm::translate(glm::mat4(1.0f),
                                         glm::vec3(10.0f, 0.5f , -95.0f));

      // Composizione finale: traslazione * scala * base
      ubo.mMat = transHr * rotHr3* rotHr * rotHr2 * scaleHr * baseHr;
      ubo.mvpMat = ViewPrj * ubo.mMat;
      ubo.nMat = glm::inverse(glm::transpose(ubo.mMat));

      SC.DS[i]->map(currentImage, &ubo, sizeof(ubo), 0);
      SC.DS[i]->map(currentImage, &gubo, sizeof(gubo), 2);





    }
    // Mostra il cavallo 'hrs10' al centro del campo
    for (const std::string &name : horse10) {
      int i = SC.InstanceIds[name];

      // Base rotation (se GLTF Z-up)
      glm::mat4 baseHr = baseFor(name);

      // Rotazione di 90° attorno a Z
      glm::mat4 rotHr = glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0,0,1));
      glm::mat4 rotHr2 = glm::rotate(glm::mat4(1.0f), glm::radians(270.0f), glm::vec3(1,0,0));
      glm::mat4 rotHr3 = glm::rotate(glm::mat4(1.0f), glm::radians(280.0f), glm::vec3(0,1,0));


      // Scala il cavallo (1.5x)
      glm::mat4 scaleHr = glm::scale(glm::mat4(1.0f), glm::vec3(3.0f));

      // Traslazione: posizione centrale del campo
      // Y = altezza del piano + offset del cavallo
      float yOffset = 0.5f; // regola in base all'altezza del cavallo
      glm::mat4 transHr = glm::translate(glm::mat4(1.0f),
                                         glm::vec3(-20.0f, 0.5f , -110.0f));

      // Composizione finale: traslazione * scala * base
      ubo.mMat = transHr* rotHr3 * rotHr * rotHr2 * scaleHr * baseHr;
      ubo.mvpMat = ViewPrj * ubo.mMat;
      ubo.nMat = glm::inverse(glm::transpose(ubo.mMat));

      SC.DS[i]->map(currentImage, &ubo, sizeof(ubo), 0);
      SC.DS[i]->map(currentImage, &gubo, sizeof(gubo), 2);





    }


    // Mostra il cavallo 'hrs10' al centro del campo
    for (const std::string &name : horse11) {
      int i = SC.InstanceIds[name];

      // Base rotation (se GLTF Z-up)
      glm::mat4 baseHr = baseFor(name);

      // Rotazione di 90° attorno a Z
      glm::mat4 rotHr = glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0,0,1));
      glm::mat4 rotHr2 = glm::rotate(glm::mat4(1.0f), glm::radians(270.0f), glm::vec3(1,0,0));
      glm::mat4 rotHr3 = glm::rotate(glm::mat4(1.0f), glm::radians(186.0f), glm::vec3(0,1,0));


      // Scala il cavallo (1.5x)
      glm::mat4 scaleHr = glm::scale(glm::mat4(1.0f), glm::vec3(3.0f));

      // Traslazione: posizione centrale del campo
      // Y = altezza del piano + offset del cavallo
      float yOffset = 0.5f; // regola in base all'altezza del cavallo
      glm::mat4 transHr = glm::translate(glm::mat4(1.0f),
                                         glm::vec3(-18.0f, 0.5f , -90.0f));

      // Composizione finale: traslazione * scala * base
      ubo.mMat = transHr* rotHr3 * rotHr * rotHr2 * scaleHr * baseHr;
      ubo.mvpMat = ViewPrj * ubo.mMat;
      ubo.nMat = glm::inverse(glm::transpose(ubo.mMat));

      SC.DS[i]->map(currentImage, &ubo, sizeof(ubo), 0);
      SC.DS[i]->map(currentImage, &gubo, sizeof(gubo), 2);





    }






  }
};

int main() {
  SIMULATOR app;
  try {
    app.run();
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

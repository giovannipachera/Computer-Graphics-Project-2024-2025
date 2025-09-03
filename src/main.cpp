#include "modules/Starter.hpp"
#include "modules/TextMaker.hpp"
#include "modules/LogoMaker.hpp"
#include "AudioPlayer.hpp"
#include "WVP.hpp"
#include <cmath>
#include <thread>

// Strutture dati per la comunicazione con gli shader Vulkan
// UBO che contiene le matrici di trasformazione, utilizzato per trasformare i vertici dallo spazio locale allo spazio schermo
struct UniformBufferObject {
  alignas(16) glm::mat4 mvpMat; // Matrice Model-View-Projection per la trasformazione finale
  alignas(16) glm::mat4 mMat; // Matrice Model per  posizionare l'oggetto nel mondo
  alignas(16) glm::mat4 nMat; // Matrice Normal per calcolare l'illuminazione corretta
};

// GUBO per parametri di illuminazione e camera, condiviso tra tutti gli oggetti della scena per mantenere consistenza visiva
struct GlobalUniformBufferObject {
  alignas(16) glm::vec3 lightDir; // Direzione della luce principale (sole)
  alignas(16) glm::vec4 lightColor; // Colore e intensità della luce
  alignas(16) glm::vec3 eyePos; // Posizione della camera nello spazio mondo
  alignas(16) glm::vec4 eyeDir; // Direzione di vista della camera
};

// Struttura vertex che che definisce la geometria di ogni punto del modello 3D, compatibilmente con il formato dei gile importati
struct Vertex {
  glm::vec3 pos; // Posizione 3D del vertice nello spazio locale
  glm::vec2 UV; // Coordinate texture per il mapping delle immagini
  glm::vec3 norm; // Vettore normale per il calcolo dell'illuminazione
};

// Struttura per rappresentare le pale eoliche nel mondo di gioco
struct Blade {
  std::string id; // Identificativo univoco della pala
};

// Struttura generica per rappresentare gli animali nella fattoria
struct Animal {
  std::string id; // Identificativo univoco dell'animale nella scena
  glm::vec3 position; // Posizione dell'animale nel mondo di gioco
  float rotY; // Orientamento calcolato ruotando in gradi sull'asse Y
};

// Obiettivi di gioco che il giocatore deve completare in ordine
int currTextIndex = 0; // Indice dell'obiettivo corrente
// Testi degli obiettivi
std::vector<SingleText> outText = {
  {1, {"1. Find the plow and attach it to the tractor", "", "", ""}, 0, 0},
  {1, {"2. Fuffy loves the horn! Find him and play it  ", "", "", ""}, 0, 0},
  {1, {"3. Last Goal!!! Find the bear", "", "", ""},0, 0},
  {1, {"4. You've reached all of your goals!", "", "", ""},0, 0}};

#include "modules/Scene.hpp"

class SIMULATOR : public BaseProject {
protected:
  // Componenti Vulkan per il rendering
  DescriptorSetLayout DSL; // Layout che descrive a quali risorse (texture, buffer) gli shader possono accedere
  VertexDescriptor VD; // Descrittore del formato dei vertici (posizioni, UV, normali)
  Pipeline P; // Pipeline grafica che definisce come vengono renderizzati i modelli
  Scene SC; // Gestore della scena che carica e organizza tutti i modelli

  // Componenenti dell'interfaccia utente
  TextMaker txt; // Sistema per renderizzare il testo sullo schermo
  LogoMaker logo; // Sistema per renderizzare il logo sullo schermo

  // Stato della camera e del mondo
  float Ar; // Aspect Ratio della finestra (larghezza/altezza)
  glm::vec3 Pos; // Posizione corrente del trattore nel mondo
  float Yaw; // Rotazione del trattore sull'asse Y

  // Stato del trattore
  glm::vec3 **deltaP; // Traslazione della istanza letta da JSON
  int currentBody = 0; // Modello di trattore attualmente selezionato (0: Classic, 1: Barbie)
  int lastBody = -1; // Ultimo modello selezionato (per gestire cambi audio)

  // Gestione della barra spaziatrice per cambiare obiettivo
  bool LockText = true;

  // Sistema audio
  AudioPlayer classicAudio; // Musichetta per il modello Classic
  AudioPlayer barbieAudio; // Musichetta per il modello Barbie
  AudioPlayer hornAudio; // Suono del clacson

  // Istanze raggruppate per categoria
  // Corpi dei due modelli
  std::vector<std::string> tractorBodies = {"tbc", "tbb"};
  // Assi delle ruote frontali dei due modelli
  std::vector<std::string> tractorAxes = {"axc", "axb"};
  // Ruote frontali e posteriori dei due modelli
  std::vector<std::string> tractorWheels = {"flwc", "frwc", "blwc", "brwc", "flwb", "frwb", "blwb", "brwb"};
  // Parafanghi dei due modelli
  std::vector<std::string> tractorFenders = {"lfc", "rfc", "lfb", "rfb"};
  // Aratro
  std::vector<std::string> tractorPlow = {"p"};
  // Eliche delle pale eoliche
  std::vector<Blade> blades = {
    {"bl1"},  // Prima pala eolica
    {"bl2"},  // Seconda pala eolica
    {"bl3"}  // Terza pala eolica
  };
  // Eliche del mulino
  std::vector<std::string> mill = {"m"};
  // Orso
  std::vector<std::string> bear = {"bear"};
  // Cane
  std::vector<std::string> dog = {"dog"};
  // Cavalli
  std::vector<Animal> horses = {
      {"hrs1", {6.0f, 0.5f, -110.0f}, 0.0f},
      {"hrs2", {20.0f, 0.5f, -100.0f}, -110.0f},
      {"hrs3", {-5.0f, 0.5f, -100.0f}, 48.0f},
      {"hrs4", {-6.0f, 0.5f, -120.0f}, -60.0f},
      {"hrs5", {40.0f, 0.5f, -110.0f}, -26.0f},
      {"hrs6", {30.0f, 0.5f, -90.0f}, -120.0f},
      {"hrs7", {-20.0f, 0.5f, -120.0f}, 110.0f},
      {"hrs8", {25.0f, 0.5f, -115.0f}, -48.0f},
      {"hrs9", {10.0f, 0.5f, -95.0f}, 210.0f},
      {"hrs10", {-20.0f, 0.5f, -110.0f}, 280.0f},
      {"hrs11", {-18.0f, 0.5f, -90.0f}, 186.0f}};
  // Galline
  std::vector<Animal> chickens = {
      {"chk1", {-108.0f, 0.5f, -35.0f}, 0.0f},
      {"chk2", {-106.0f, 0.5f, -47.0f}, -110.0f},
      {"chk3", {-80.0f, 0.5f, -55.0f}, 48.0f},
      {"chk4", {-93.0f, 0.5f, -33.0f}, -60.0f},
      {"chk5", {-99.0f, 0.5f, -46.0f}, -26.0f},
      {"chk6", {-110.0f, 0.5f, -34.0f}, -120.0f},
      {"chk7", {-104.0f, 0.5f, -52.0f}, 110.0f},
      {"chk8", {-100.0f, 0.5f, -30.0f}, -48.0f},
      {"chk9", {-83.0f, 0.5f, -35.0f}, 210.0f},
      {"chk10", {-108.0f, 0.5f, -56.0f}, 280.0f},
      {"chk11", {-92.0f, 0.5f, -37.0f}, 186.0f},
      {"chk12", {-95.0f, 0.5f, -50.0f}, 35.0f},
      {"chk13", {-120.0f, 0.5f, -40.0f}, -75.0f},
      {"chk14", {-87.0f, 0.5f, -25.0f}, 150.0f},
      {"chk15", {-115.0f, 0.5f, -55.0f}, -20.0f},
      {"chk16", {-102.0f, 0.5f, -60.0f}, 95.0f},
      {"chk17", {-90.0f, 0.5f, -62.0f}, 15.0f},
      {"chk18", {-125.0f, 0.5f, -48.0f}, -45.0f},
      {"chk19", {-100.0f, 0.5f, -45.0f}, 95.0f},
      {"chk20", {-115.0f, 0.5f, -32.0f}, -120.0f},
      {"chk21", {-85.0f, 0.5f, -50.0f}, 60.0f},
      {"chk22", {-105.0f, 0.5f, -25.0f}, -200.0f},
      {"chk23", {-97.0f, 0.5f, -62.0f}, 130.0f},
      {"chk24", {-118.0f, 0.5f, -60.0f}, -10.0f},
      {"chk25", {-112.0f, 0.5f, -42.0f}, -90.0f},
      {"chk26", {-90.0f, 0.5f, -40.0f}, 30.0f}};
  // Mucche
  std::vector<Animal> cows = {
      {"cow1", {156.0f, 0.5f, 110.0f}, 0.0f},
      {"cow2", {170.0f, 0.5f, 100.0f}, -45.0f},
      {"cow3", {145.0f, 0.5f, 100.0f}, 30.0f},
      {"cow4", {144.0f, 0.5f, 120.0f}, -60.0f},
      {"cow5", {190.0f, 0.5f, 110.0f}, -15.0f},
      {"cow6", {180.0f, 0.5f, 90.0f}, -120.0f},
      {"cow7", {130.0f, 0.5f, 120.0f}, 75.0f},
      {"cow8", {175.0f, 0.5f, 115.0f}, -30.0f},
      {"cow9", {160.0f, 0.5f, 95.0f}, 210.0f},
      {"cow10", {130.0f, 0.5f, 110.0f}, 280.0f},
      {"cow11", {156.0f, 0.5f, 80.0f}, 10.0f},
      {"cow12", {170.0f, 0.5f, 70.0f}, -90.0f},
      {"cow13", {145.0f, 0.5f, 70.0f}, 45.0f},
      {"cow14", {144.0f, 0.5f, 90.0f}, -75.0f},
      {"cow15", {190.0f, 0.5f, 80.0f}, -20.0f},
      {"cow16", {180.0f, 0.5f, 60.0f}, -150.0f},
      {"cow17", {130.0f, 0.5f, 90.0f}, 60.0f},
      {"cow18", {175.0f, 0.5f, 85.0f}, -10.0f},
      {"cow19", {160.0f, 0.5f, 65.0f}, 190.0f},
      {"cow20", {130.0f, 0.5f, 80.0f}, 250.0f},
      {"cow21", {156.0f, 0.5f, 50.0f}, 5.0f},
      {"cow22", {170.0f, 0.5f, 40.0f}, -100.0f},
      {"cow23", {145.0f, 0.5f, 40.0f}, 35.0f},
      {"cow24", {144.0f, 0.5f, 60.0f}, -50.0f},
      {"cow25", {190.0f, 0.5f, 50.0f}, -5.0f},
      {"cow26", {180.0f, 0.5f, 30.0f}, -130.0f},
      {"cow27", {130.0f, 0.5f, 60.0f}, 90.0f},
      {"cow28", {175.0f, 0.5f, 55.0f}, -35.0f},
      {"cow29", {160.0f, 0.5f, 35.0f}, 170.0f},
      {"cow30", {130.0f, 0.5f, 50.0f}, 240.0f}};
  // Papere
  std::vector<Animal> ducks = {
    {"duck1", {146.0f, -0.5f, -135.0f}, 0.0f},
    {"duck2", {160.0f, -0.5f, -135.0f}, -45.0f},
    {"duck3", {135.0f, -0.5f, -135.0f}, 30.0f},
    {"duck4", {134.0f, -0.5f, -150.0f}, -60.0f},
    {"duck5", {180.0f, -0.5f, -145.0f}, -15.0f},
    {"duck6", {170.0f, -0.5f, -125.0f}, -120.0f},
    {"duck7", {120.0f, -0.5f, -155.0f}, 75.0f},
    {"duck8", {165.0f, -0.5f, -150.0f}, -30.0f},
    {"duck9", {150.0f, -0.5f, -130.0f}, 210.0f},
    {"duck10", {120.0f, -0.5f, -145.0f}, 280.0f},
  };
  // Ambientazione
  std::vector<std::string> tractorScene = {"prm"};

  // Sistema di attacco dell'aratro
  int plowIndex = -1; // Indice dell'aratro nella scena
  bool plowAttached = false; // Flag che indica se l'aratro è attaccato al trattore
  glm::vec3 plowOffset = glm::vec3(0.0f); // Offset dell'aratro rispetto al centro del trattore

  // Condizione iniziale della finestra di gioco
  void setWindowParameters() {
    windowWidth = 800; // Larghezza
    windowHeight = 600; // Altezza
    windowTitle = "Country Roads"; // Titolo
    windowResizable = GLFW_TRUE; // Ridimensionabilità
    initialBackgroundColor = {0.4f, 0.8f, 1.0f, 1.0f}; // Colore del cielo
    uniformBlocksInPool = 64; // Numero massimo di Uniform Blocks nel pool
    texturesInPool = 64; // Numero massimo di Texture nel pool
    setsInPool = 64; // Numero massimo descriptor set nel pool
    Ar = 4.0f / 3.0f; // Aspect Ratio iniziale
  }

  // Ricalcolo di Aspect Ratio quando si cambia la dimensione della finestra
  void onWindowResize(int w, int h) { Ar = (float)w / (float)h; }

  // Inizializzazione completa di tutte le risorse Vulkan e degli oggetti di gioco
  void localInit() {
    //Inizializzazione del Descriptor Set Layout: quali tipi di risorse (buffer, texture) gli shader possono utilizzare
    DSL.init(
        this,
        {{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS}, // Binding 0: slot UBO
         {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // Binding 1: slot textures
          VK_SHADER_STAGE_FRAGMENT_BIT},
         {2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS}}); // Binding 2: slot GUBO (luce e camera)

    // Inizializzazione del Vertex Descriptor: definisce il formato dei vertici (come sono strutturati i dati geometrici)
    VD.init(this, {{0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX}}, // Binding 0: un Vertex per vertice
            {{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos), // Posizione 3D
              sizeof(glm::vec3), POSITION},
             {0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, UV), // Coordinate texture
              sizeof(glm::vec2), UV},
             {0, 2, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, norm), // Vettore normale
              sizeof(glm::vec3), NORMAL}});

    // Inizializzazione della Pipeline: carica e configura gli shader per il rendering con illuminazione Phong
    P.init(this, &VD, "shaders/PhongVert.spv", "shaders/PhongFrag.spv", {&DSL});
    P.setAdvancedFeatures(VK_COMPARE_OP_LESS_OR_EQUAL, // Test profondità less-or-equal
                          VK_POLYGON_MODE_FILL, // Riempie i poligoni
                          VK_CULL_MODE_NONE, // Non elimina le facce (mostra fronte e retro)
                          false); // Non abilita blending

    // Caricamento della scena e inizializzazione dell'UI
    SC.init(this, &VD, DSL, P, "assets/models/scene.json"); // Modelli 3D
    txt.init(this, &outText); // Sistema del testo
    logo.init(this); // Sistema del logo

    // Caricamento dei file di audio
    #ifdef _WIN32
    classicAudio.load("C:\\Users\\admin\\Desktop\\Computer Graphic project\\Computer-Graphics-Project-2024-2025\\assets\\audio\\classic.wav", true);
    barbieAudio.load("C:\\Users\\admin\\Desktop\\Computer Graphic project\\Computer-Graphics-Project-2024-2025\\assets\\audio\\barbie.wav", true);
    hornAudio.load("C:\\Users\\admin\\Desktop\\Computer Graphic project\\Computer-Graphics-Project-2024-2025\\assets\\audio\\horn.wav", false);
    #else
    classicAudio.load("assets/audio/classic.wav", true);
    barbieAudio.load("assets/audio/barbie.wav", true);
    hornAudio.load("assets/audio/horn.wav", false);
    #endif

    // Avvio della traccia audio all'inizio della simulazione
    classicAudio.play(); // Inizio con la musica del trattore classico
    lastBody = currentBody; // Sincronizza stato audio

    // Posizione iniziale del trattore
    Pos = glm::vec3(-50.0f, 3.45f, 130.0f); // Posizione
    Yaw = glm::radians(180.0f); // Orientamento

    // Offset letti dal JSON (RELATIVI al corpo)
    deltaP = (glm::vec3 **)calloc(SC.InstanceCount, sizeof(glm::vec3 *));

    // Ricerca dell'indice dell'aratro nella scena
    if (SC.InstanceIds.count("p")) {
      plowIndex = SC.InstanceIds["p"];
    }

    // Estrazione di deltaP per ogni istanza
    for (int i = 0; i < SC.InstanceCount; ++i) {
      deltaP[i] = new glm::vec3(
          glm::vec3(SC.I[i].Wm[3])); // estrai solo la traduzione dal JSON
    }
  }

  // Creazione di pipeline e descriptor set dopo l'inizializzazione del device
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
    txt.populateCommandBuffer(cb, currentImage, currTextIndex);
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
    bool fire = false, next = false, prev = false, horn = false, changeText = false;
    getSixAxis(deltaT, m, r, fire, next, prev, horn, changeText);

    // Clamp del timestep per stabilità
    const float MAX_DELTA_T = 0.05f;
    if (deltaT > MAX_DELTA_T)
      deltaT = MAX_DELTA_T;


    // Logic of last goal
    glm::vec3 bearPos = glm::vec3(205.0f, 0.5f, -95.0f);


    bool bearNear;

    float distToBear = glm::length(Pos - bearPos);

    if (distToBear < 20.0f) {  // soglia in metri/unità di mondo
      bearNear = true;
      LockText=false;


    } else {
      bearNear = false;
    }

    /////////////////////////////////

    // Logic of last goal
    glm::vec3 dogPos = glm::vec3(-95.0f, 0.5f, 95.0f);


    bool dogNear;

    float distToDog = glm::length(Pos - dogPos);

    if (distToDog < 15.0f) {  // soglia in metri/unità di mondo
      dogNear = true;
      LockText=false;



    } else {
      dogNear = false;
    }

    /////////////////////////////////


    static bool prevT = false;
    bool tPressed = changeText && !prevT;
    prevT = changeText;

    if (tPressed && !LockText) {
      LockText=true;


      currTextIndex = (currTextIndex + 1) % outText.size(); // cicla le scritte
      txt.setText(currTextIndex);
      refreshUIResources();
    }



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

    /*
    static bool prevK = false;
    bool kPressed = horn && !prevK;
    prevK = horn;

    if (kPressed) {
      hornAudio.play();
      hornAudio.stop(false);
    }

    */

    static bool prevK = false;
    bool kPressed = horn && !prevK;
    prevK = horn;

#ifdef _WIN32
      static bool hornCooldown = false;
      static float hornTimer = 0.0f;

      if (kPressed && !hornCooldown) {

      std::cout << "[DEBUG] Clacson - PowerShell ottimizzato" << std::endl;
      // Comando PowerShell più veloce (senza Start-Sleep)
      std::string hornPath = "C:\\Users\\admin\\Desktop\\Computer Graphic project\\Computer-Graphics-Project-2024-2025\\assets\\audio\\horn.wav";
      std::string cmd = "powershell -WindowStyle Hidden -NoProfile -Command \"[System.Media.SoundPlayer]::new('" + hornPath + "').PlaySync()\"";

      // Avvia il comando in background
      std::thread([cmd]() {
          std::system(cmd.c_str());
      }).detach();

      hornCooldown = true;
      hornTimer = 1.0f;
    }

    // Gestione cooldown
    if (hornCooldown) {
      hornTimer -= deltaT;
      if (hornTimer <= 0.0f) {
        hornCooldown = false;
      }
    }
#else
    if (kPressed) {
      hornAudio.play();
      hornAudio.stop(false);
    }
#endif


    if (currentBody != lastBody) {
      if (lastBody == 0)
        classicAudio.stop(true);
      else if (lastBody == 1)
        barbieAudio.stop(true);

      if (currentBody == 0)
        classicAudio.play();
      else if (currentBody == 1)
        barbieAudio.play();

      logo.setCurrentLogo(currentBody);
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
        LockText=false;

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
        1000.0f);

    // Global UBO
    GlobalUniformBufferObject gubo{};
    // DOPO: sole dall’alto, leggermente verso la camera
    gubo.lightDir = glm::normalize(glm::vec3(-1.0f, -1.0f, 0.0f));

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
      glm::mat4 R90 = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0,1,0));

      if (plowAttached) {
        glm::mat4 bodyTr = MakeWorld(Pos, Yaw, 0.0f, 0.0f);
        glm::mat4 localTr = glm::translate(glm::mat4(1.0f), plowOffset);

        ubo.mMat = bodyTr * localTr *  baseTr;
      } else {
        ubo.mMat = SC.I[i].Wm * R90 * baseTr;
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

    // --- PALE EOLICHE: rotazione pulita attorno al mozzo, senza wobble ---
static float bladeRot = 20.0f;
bladeRot -= deltaT * glm::radians(60.0f);

for (const auto &b : blades) {
    int i = SC.InstanceIds[b.id];

    // 1) Pose di base dall'istanza (pos + orientazione corretta dal JSON)
    glm::mat4 M0 = SC.I[i].Wm * baseFor(b.id);

    // 2) Elimina scala/shear dall'istanza (causa classica del "ballo")
    glm::vec3 T = glm::vec3(M0[3]);
    glm::vec3 Xw = glm::vec3(M0[0]), Yw = glm::vec3(M0[1]), Zw = glm::vec3(M0[2]);
    if (glm::length(Xw) > 0) Xw = glm::normalize(Xw);
    if (glm::length(Yw) > 0) Yw = glm::normalize(Yw);
    if (glm::length(Zw) > 0) Zw = glm::normalize(Zw);
    glm::mat4 R0(1.0f);
    R0[0] = glm::vec4(Xw, 0);
    R0[1] = glm::vec4(Yw, 0);
    R0[2] = glm::vec4(Zw, 0);
    glm::mat4 M_noScale = glm::translate(glm::mat4(1.0f), T) * R0;

    // 3) Asse LOCALE di rotazione: scegliamo quello più orizzontale
    glm::vec3 axes[3] = { {1,0,0}, {0,1,0}, {0,0,1} };
    glm::vec3 axesW[3] = {
        glm::vec3(R0 * glm::vec4(axes[0], 0)),
        glm::vec3(R0 * glm::vec4(axes[1], 0)),
        glm::vec3(R0 * glm::vec4(axes[2], 0))
    };
    int best = 0; float sc = fabs(axesW[0].y);
    for (int k = 1; k < 3; ++k) { float s = fabs(axesW[k].y); if (s < sc) { sc = s; best = k; } }
    glm::vec3 axisLocal = axes[best];  // se gira al contrario, usa -bladeRot

    // 4) Micro-offset del mozzo (se l'origin non è centrato al micron)
    glm::vec3 hubLocal = glm::vec3(0.0f); // es. {0.0f, -0.02f, 0.0f} se serve
    glm::mat4 toHub   = glm::translate(glm::mat4(1.0f),  hubLocal);
    glm::mat4 fromHub = glm::translate(glm::mat4(1.0f), -hubLocal);

    // 5) Rotazione locale attorno al mozzo
    glm::mat4 Rspin = glm::rotate(glm::mat4(1.0f), bladeRot, axisLocal);

    ubo.mMat  = M_noScale * toHub * Rspin * fromHub;
    ubo.mvpMat = ViewPrj * ubo.mMat;
    ubo.nMat   = glm::inverse(glm::transpose(ubo.mMat));

    SC.DS[i]->map(currentImage, &ubo, sizeof(ubo), 0);
    SC.DS[i]->map(currentImage, &gubo, sizeof(gubo), 2);
}


    // --- MULINO (id "m"): tilt fisso + spin lento attorno a X ---
    // velocità più lenta delle eoliche, stesso verso (usa -=)
    static float millRot = 0.0f;
    millRot -= deltaT * glm::radians(15.0f); // regola i 15°/s a gusto

    for (const std::string &name : mill) {
      int i = SC.InstanceIds[name];
      glm::mat4 baseTr = baseFor(name);

      // Matrice di partenza e base ortonormale (evita wobble)
      glm::mat4 M0 = SC.I[i].Wm * baseTr;
      glm::vec3 T  = glm::vec3(M0[3]);
      glm::vec3 Xw = glm::normalize(glm::vec3(M0[0]));
      glm::vec3 Yw = glm::normalize(glm::vec3(M0[1]));
      glm::vec3 Zw = glm::normalize(glm::vec3(M0[2]));
      glm::mat4 R0(1.0f);
      R0[0] = glm::vec4(Xw, 0.0f);
      R0[1] = glm::vec4(Yw, 0.0f);
      R0[2] = glm::vec4(Zw, 0.0f);
      glm::mat4 M_noScale = glm::translate(glm::mat4(1.0f), T) * R0;

      // tuoi tilt fissati (tienili come li hai trovati buoni)
      const float tiltX_deg = -15.0f;
      const float tiltY_deg = -20.0f;

      glm::mat4 RtiltX = glm::rotate(glm::mat4(1.0f), glm::radians(tiltX_deg), glm::vec3(1,0,0));
      glm::mat4 RtiltY = glm::rotate(glm::mat4(1.0f), glm::radians(tiltY_deg), glm::vec3(0,0,1));

      // SPIN attorno all’asse X locale (perché in Blender l’asse del mulino l’abbiamo allineato a +X)
      glm::mat4 RspinX = glm::rotate(glm::mat4(1.0f), millRot, glm::vec3(1,0,0));

      // Ordine: prima tilt (posa iniziale), poi spin (verso coerente con eoliche)
      glm::mat4 Rtotal = RtiltX * RtiltY * RspinX;

      ubo.mMat  = M_noScale * Rtotal;  // origin = centro mozzo → nessun toPivot/fromPivot
      ubo.mvpMat = ViewPrj * ubo.mMat;
      ubo.nMat   = glm::inverse(glm::transpose(ubo.mMat));

      SC.DS[i]->map(currentImage, &ubo, sizeof(ubo), 0);
      SC.DS[i]->map(currentImage, &gubo, sizeof(gubo), 2);
    }

    for (const auto &h : horses) {
      int i = SC.InstanceIds[h.id];
      glm::mat4 baseHr = baseFor(h.id);
      glm::mat4 rotZ = glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0, 0, 1));
      glm::mat4 rotX = glm::rotate(glm::mat4(1.0f), glm::radians(270.0f), glm::vec3(1, 0, 0));
      glm::mat4 rotY = glm::rotate(glm::mat4(1.0f), glm::radians(h.rotY), glm::vec3(0, 1, 0));
      glm::mat4 scaleHr = glm::scale(glm::mat4(1.0f), glm::vec3(3.0f));
      glm::mat4 transHr = glm::translate(glm::mat4(1.0f), h.position);
      ubo.mMat = transHr * rotY * rotZ * rotX * scaleHr * baseHr;
      ubo.mvpMat = ViewPrj * ubo.mMat;
      ubo.nMat = glm::inverse(glm::transpose(ubo.mMat));
      SC.DS[i]->map(currentImage, &ubo, sizeof(ubo), 0);
      SC.DS[i]->map(currentImage, &gubo, sizeof(gubo), 2);
    }

    // ---------- CHICKENS ----------

    for (const auto &c : chickens) {
      int i = SC.InstanceIds[c.id];
      glm::mat4 baseCh = baseFor(c.id);
      glm::mat4 rotZ = glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0, 0, 1));
      glm::mat4 rotX = glm::rotate(glm::mat4(1.0f), glm::radians(270.0f), glm::vec3(1, 0, 0));
      glm::mat4 rotY = glm::rotate(glm::mat4(1.0f), glm::radians(c.rotY), glm::vec3(0, 1, 0));
      glm::mat4 scaleCh = glm::scale(glm::mat4(1.0f), glm::vec3(3.0f));
      glm::mat4 transCh = glm::translate(glm::mat4(1.0f), c.position);
      ubo.mMat = transCh * rotY * rotZ * rotX * scaleCh * baseCh;
      ubo.mvpMat = ViewPrj * ubo.mMat;
      ubo.nMat = glm::inverse(glm::transpose(ubo.mMat));
      SC.DS[i]->map(currentImage, &ubo, sizeof(ubo), 0);
      SC.DS[i]->map(currentImage, &gubo, sizeof(gubo), 2);
    }

    // ---------- COWS ----------

    for (const auto &h : cows) {
      int i = SC.InstanceIds[h.id];
      glm::mat4 baseHr = baseFor(h.id);
      glm::mat4 rotZ = glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0, 0, 1));
      glm::mat4 rotX = glm::rotate(glm::mat4(1.0f), glm::radians(270.0f), glm::vec3(1, 0, 0));
      glm::mat4 rotY = glm::rotate(glm::mat4(1.0f), glm::radians(h.rotY), glm::vec3(0, 1, 0));
      glm::mat4 scaleHr = glm::scale(glm::mat4(1.0f), glm::vec3(3.0f));
      glm::mat4 transHr = glm::translate(glm::mat4(1.0f), h.position);
      ubo.mMat = transHr * rotY * rotZ * rotX * scaleHr * baseHr;
      ubo.mvpMat = ViewPrj * ubo.mMat;
      ubo.nMat = glm::inverse(glm::transpose(ubo.mMat));
      SC.DS[i]->map(currentImage, &ubo, sizeof(ubo), 0);
      SC.DS[i]->map(currentImage, &gubo, sizeof(gubo), 2);
    }

    // ---------- DOG ----------
    for (const auto &d : dog) {
      int i = SC.InstanceIds[d];        // ID dell'orso
      glm::mat4 baseB = baseFor(d);     // base dal JSON

      // Trasformazioni di orientamento e scala (simile agli altri animali)
      glm::mat4 rotZ = glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0,0,1));
      glm::mat4 rotX = glm::rotate(glm::mat4(1.0f), glm::radians(270.0f), glm::vec3(1,0,0));
      glm::mat4 rotY = glm::mat4(1.0f); // puoi regolare se vuoi un'orientazione iniziale diversa
      glm::mat4 scaleB = glm::scale(glm::mat4(1.0f), glm::vec3(4.0f)); // scala a piacere
      glm::mat4 transB = glm::translate(glm::mat4(1.0f), glm::vec3(-95.0f, 0.5f, 95.0f)); // posizione iniziale

      ubo.mMat = transB * rotY * rotZ * rotX * scaleB * baseB;
      ubo.mvpMat = ViewPrj * ubo.mMat;
      ubo.nMat = glm::inverse(glm::transpose(ubo.mMat));

      SC.DS[i]->map(currentImage, &ubo, sizeof(ubo), 0);
      SC.DS[i]->map(currentImage, &gubo, sizeof(gubo), 2);
    }

    // ---------- BEAR ----------
    for (const auto &b : bear) {
      int i = SC.InstanceIds[b];        // ID dell'orso
      glm::mat4 baseB = baseFor(b);     // base dal JSON

      // Trasformazioni di orientamento e scala (simile agli altri animali)
      glm::mat4 rotZ = glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0,0,1));
      glm::mat4 rotX = glm::rotate(glm::mat4(1.0f), glm::radians(270.0f), glm::vec3(1,0,0));
      glm::mat4 rotY = glm::mat4(1.0f); // puoi regolare se vuoi un'orientazione iniziale diversa
      glm::mat4 scaleB = glm::scale(glm::mat4(1.0f), glm::vec3(4.0f)); // scala a piacere
      glm::mat4 transB = glm::translate(glm::mat4(1.0f), glm::vec3(205.0f, 0.5f, -95.0f)); // posizione iniziale

      ubo.mMat = transB * rotY * rotZ * rotX * scaleB * baseB;
      ubo.mvpMat = ViewPrj * ubo.mMat;
      ubo.nMat = glm::inverse(glm::transpose(ubo.mMat));

      SC.DS[i]->map(currentImage, &ubo, sizeof(ubo), 0);
      SC.DS[i]->map(currentImage, &gubo, sizeof(gubo), 2);
    }


    // ---------- DUCKS: movimento sull'acqua ----------
    static float duckTime = 0.0f;
    duckTime += deltaT;

    const float swimRadiusX = 2.0f;
    const float swimRadiusZ = 3.5f;
    const float bobAmp = 0.12f;
    const float bobFreq = 2.5f;

    for (size_t idx = 0; idx < ducks.size(); ++idx) {
      int i = SC.InstanceIds[ducks[idx].id];
      glm::mat4 baseD = baseFor(ducks[idx].id);

      // centro di partenza
      glm::vec3 center = ducks[idx].position;

      // fase individuale (qui uso id per diversificare un po’)
      float phase = static_cast<float>(idx) * 0.7f;

      // calcola spostamento ellittico e bobbing
      float t = duckTime + phase;
      float dx = swimRadiusX * sin(t * 0.8f);
      float dz = swimRadiusZ * cos(t * 0.72f);
      float dy = bobAmp * sin(t * bobFreq);

      glm::vec3 pos = center + glm::vec3(dx, dy, dz);

      // direzione di movimento per rotazione
      float heading = glm::degrees(atan2(-0.8f * swimRadiusX * cos(t * 0.8f),
                                         -0.72f * swimRadiusZ * sin(t * 0.72f)));

      glm::mat4 rotZ = glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0, 0, 1));
      glm::mat4 rotX = glm::rotate(glm::mat4(1.0f), glm::radians(270.0f), glm::vec3(1, 0, 0));
      glm::mat4 rotY = glm::rotate(glm::mat4(1.0f), glm::radians(heading), glm::vec3(0, 1, 0));
      glm::mat4 scaleD = glm::scale(glm::mat4(1.0f), glm::vec3(6.0f));
      glm::mat4 transD = glm::translate(glm::mat4(1.0f), pos);

      ubo.mMat = transD * rotY * rotZ * rotX * scaleD * baseD;
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

// This has been adapted from the Vulkan tutorial

// TO MOVE
#define JSON_DIAGNOSTICS 1
#include "modules/Starter.hpp"
#include "modules/TextMaker.hpp"

std::vector<SingleText> outText = {
	{1, {"Ciao", "Take a walk in the wild side!", "", ""}, 0, 0}
};

// The uniform buffer object used in this example
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

// The vertices data structures
// Example
struct Vertex {
	glm::vec3 pos;
	glm::vec2 UV;
	glm::vec3 norm;
};

#include "modules/Scene.hpp"
#include "WVP.hpp"

// MAIN !
class SIMULATOR : public BaseProject {
	protected:

	// Descriptor Layouts ["classes" of what will be passed to the shaders]
	DescriptorSetLayout DSL;

	// Vertex formats
	VertexDescriptor VD;

	// Pipelines [Shader couples]
	Pipeline P;

	Scene SC;
	glm::vec3 **deltaP;
	float *deltaA;
	float *usePitch;

	TextMaker txt;

	// Other application parameters
	int currScene = 0;
	float Ar;
	glm::vec3 Pos;
	float Yaw;
	glm::vec3 InitialPos;

	std::vector<std::string> tractorBody = {"tb"};
	std::vector<std::string> tractorAxes = {"ax"};
	std::vector<std::string> tractorWheels =  {"flw", "frw", "blw", "brw"};
	std::vector<std::string> tractorScene =  {"pln", "prm"};

	// Here you set the main application parameters
	void setWindowParameters() {
		// window size, titile and initial background
		windowWidth = 800;
		windowHeight = 600;
		windowTitle = "Hey buddy, are you a real country boy?";
		windowResizable = GLFW_TRUE;
		initialBackgroundColor = {0.0f, 0.85f, 1.0f, 1.0f};

		// Descriptor pool sizes
		uniformBlocksInPool = 19 * 2 + 2;
		texturesInPool = 19 + 1;
		setsInPool = 19 + 1;

		Ar = 4.0f / 3.0f;
	}

	// What to do when the window changes size
	void onWindowResize(int w, int h) {
		std::cout << "Window resized to: " << w << " x " << h << "\n";
		Ar = (float)w / (float)h;
	}

	// Here you load and setup all your Vulkan Models and Texutures.
	// Here you also create your Descriptor set layouts and load the shaders for the pipelines
	void localInit() {
		// Descriptor Layouts [what will be passed to the shaders]
		DSL.init(this, {
					{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS},
					{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT},
					{2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS}
				});

		// Vertex descriptors
		VD.init(this, {
				  {0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX}
				}, {
				  {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos),
				         sizeof(glm::vec3), POSITION},
				  {0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, UV),
				         sizeof(glm::vec2), UV},
				  {0, 2, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, norm),
				         sizeof(glm::vec3), NORMAL}
				});

		// Pipelines [Shader couples]
		P.init(this, &VD, "shaders/PhongVert.spv", "shaders/PhongFrag.spv", {&DSL});
		P.setAdvancedFeatures(VK_COMPARE_OP_LESS_OR_EQUAL, VK_POLYGON_MODE_FILL,
 								    VK_CULL_MODE_NONE, false);

		// Models, textures and Descriptors (values assigned to the uniforms)
/*		std::vector<Vertex> vertices = {
					   {{-100.0,0.0f,-100.0}, {0.0f,0.0f}, {0.0f,1.0f,0.0f}},
					   {{-100.0,0.0f, 100.0}, {0.0f,1.0f}, {0.0f,1.0f,0.0f}},
					   {{ 100.0,0.0f,-100.0}, {1.0f,0.0f}, {0.0f,1.0f,0.0f}},
					   {{ 100.0,0.0f, 100.0}, {1.0f,1.0f}, {0.0f,1.0f,0.0f}}};
		M1.vertices = std::vector<unsigned char>(vertices.size()*sizeof(Vertex), 0);
		memcpy(&vertices[0], &M1.vertices[0], vertices.size()*sizeof(Vertex));
		M1.indices = {0, 1, 2,    1, 3, 2};
		M1.initMesh(this, &VD); */


		// Load Scene
		SC.init(this, &VD, DSL, P, "assets/models/scene.json");

		// updates the text
		txt.init(this, &outText);

		// Init local variables
		{
			int tbIndex = SC.InstanceIds["tb"];
			glm::mat4 tbMatrix = SC.I[tbIndex].Wm;

                        // La traslazione è nella **quarta colonna**, NON nella quarta riga!
                        Pos = glm::vec3(tbMatrix[0][3], tbMatrix[1][3], tbMatrix[2][3]);
			InitialPos = Pos;
			Yaw = 0.0f;
		}

		deltaP = (glm::vec3 **)calloc(SC.InstanceCount, sizeof(glm::vec3 *));
		deltaA = (float *)calloc(SC.InstanceCount, sizeof(float));
		usePitch = (float *)calloc(SC.InstanceCount, sizeof(float));
		for(int i = 0; i < SC.InstanceCount; i++) {
			glm::mat4 W = SC.I[i].Wm;
                        glm::vec3 translation(W[0][3], W[1][3], W[2][3]);  // COLONNA 4 della matrice

			deltaP[i] = new glm::vec3(translation);
			deltaA[i] = 0.0f;
			usePitch[i] = 0.0f;
		}

	}

	// Here you create your pipelines and Descriptor Sets!
	void pipelinesAndDescriptorSetsInit() {
		// This creates a new pipeline (with the current surface), using its shaders
		P.create();

		// Here you define the data set
		SC.pipelinesAndDescriptorSetsInit(DSL);
		txt.pipelinesAndDescriptorSetsInit();
	}

	// Here you destroy your pipelines and Descriptor Sets!
	// All the object classes defined in Starter.hpp have a method .cleanup() for this purpose
	void pipelinesAndDescriptorSetsCleanup() {
		// Cleanup pipelines
		P.cleanup();

		SC.pipelinesAndDescriptorSetsCleanup();
		txt.pipelinesAndDescriptorSetsCleanup();
	}

	// Here you destroy all the Models, Texture and Desc. Set Layouts you created!
	// All the object classes defined in Starter.hpp have a method .cleanup() for this purpose
	// You also have to destroy the pipelines: since they need to be rebuilt, they have two different
	// methods: .cleanup() recreates them, while .destroy() delete them completely
	void localCleanup() {
		for(int i=0; i < SC.InstanceCount; i++) {
			delete deltaP[i];
		}
		free(deltaP);
		free(deltaA);
		free(usePitch);

		// Cleanup descriptor set layouts
		DSL.cleanup();

		// Destroies the pipelines
		P.destroy();

		SC.localCleanup();
		txt.localCleanup();
	}

	// Here it is the creation of the command buffer:
	// You send to the GPU all the objects you want to draw,
	// with their buffers and textures
	void populateCommandBuffer(VkCommandBuffer commandBuffer, int currentImage) {
		// binds the pipeline
		P.bind(commandBuffer);

		/*		// binds the data set
				DS1.bind(commandBuffer, P, 0, currentImage);

				// binds the model
				M1.bind(commandBuffer);

				// record the drawing command in the command buffer
				vkCmdDrawIndexed(commandBuffer,
						static_cast<uint32_t>(M1.indices.size()), 1, 0, 0, 0);
		std::cout << M1.indices.size();
		*/
		SC.populateCommandBuffer(commandBuffer, currentImage, P);
		txt.populateCommandBuffer(commandBuffer, currentImage, currScene);
	}

	// Here is where you update the uniforms.
	// Very likely this will be where you will be writing the logic of your application.
	void updateUniformBuffer(uint32_t currentImage) {
	    float deltaT;
	    glm::vec3 m(0.0f), r(0.0f);
	    bool fire = false;
	    getSixAxis(deltaT, m, r, fire);

	    static float CamPitch = glm::radians(20.0f);
	    static float CamYaw   = M_PI;
	    static float CamDist  = 10.0f;
	    static float CamRoll  = 0.0f;
	    const glm::vec3 CamTargetDelta = glm::vec3(0,2,0);

	    static float SteeringAng = 0.0f;
	    static float wheelRoll = 0.0f;
	    static float dampedVel = 0.0f;

	    const float STEERING_SPEED = glm::radians(30.0f);
	    const float ROT_SPEED = glm::radians(120.0f);
	    const float MOVE_SPEED = 2.5f;

	    SteeringAng += -m.x * STEERING_SPEED * deltaT;
	    SteeringAng = glm::clamp(SteeringAng, glm::radians(-35.0f), glm::radians(35.0f));

	    glm::mat4 M;
	    glm::vec3 CamPos = Pos;
	    static glm::vec3 dampedCamPos = CamPos;

	    double lambdaVel = 8.0f;
	    double dampedVelEpsilon = 0.001f;
	    dampedVel =  MOVE_SPEED * deltaT * m.z * (1 - exp(-lambdaVel * deltaT)) +
	                 dampedVel * exp(-lambdaVel * deltaT);
	    dampedVel = (fabs(dampedVel) < dampedVelEpsilon) ? 0.0f : dampedVel;

	    wheelRoll -= dampedVel / 0.4f;
	    wheelRoll = fmod(wheelRoll + 2 * M_PI, 2 * M_PI);

	    if (dampedVel != 0.0f) {
	        glm::vec3 oldPos = Pos;
	        if (SteeringAng != 0.0f) {
	            const float l = 2.78f;
	            float r = l / tan(SteeringAng);
	            float cx = Pos.x + r * cos(Yaw);
	            float cz = Pos.z - r * sin(Yaw);
	            float Dbeta = dampedVel / r;
	            Yaw -= Dbeta;
	            Pos.x = cx - r * cos(Yaw);
	            Pos.z = cz + r * sin(Yaw);
	        } else {
	            Pos.x -= sin(Yaw) * dampedVel;
	            Pos.z -= cos(Yaw) * dampedVel;
	        }

	        if (m.x == 0.0f) {
	            if (SteeringAng > STEERING_SPEED * deltaT) SteeringAng -= STEERING_SPEED * deltaT;
	            else if (SteeringAng < -STEERING_SPEED * deltaT) SteeringAng += STEERING_SPEED * deltaT;
	            else SteeringAng = 0.0f;
	        }
	    }

	    CamYaw += ROT_SPEED * deltaT * r.y;
	    CamPitch -= ROT_SPEED * deltaT * r.x;
	    CamRoll -= ROT_SPEED * deltaT * r.z;
	    CamDist -= MOVE_SPEED * deltaT * m.y;

	    CamYaw = glm::clamp(CamYaw, 0.0f, 2.0f * float(M_PI));
	    CamPitch = glm::clamp(CamPitch, 0.0f, float(M_PI_2) - 0.01f);
	    CamRoll = glm::clamp(CamRoll, float(-M_PI), float(M_PI));
	    CamDist = glm::clamp(CamDist, 7.0f, 15.0f);

	    glm::vec3 CamTarget = Pos + glm::vec3(glm::rotate(glm::mat4(1), Yaw, glm::vec3(0,1,0)) * glm::vec4(CamTargetDelta,1));
	    CamPos = CamTarget + glm::vec3(glm::rotate(glm::mat4(1), Yaw + CamYaw, glm::vec3(0,1,0)) *
	                                    glm::rotate(glm::mat4(1), -CamPitch, glm::vec3(1,0,0)) * glm::vec4(0,0,CamDist,1));

	    const float lambdaCam = 10.0f;
	    dampedCamPos = CamPos * (1 - exp(-lambdaCam * deltaT)) + dampedCamPos * exp(-lambdaCam * deltaT);
	    glm::mat4 ViewPrj = MakeViewProjectionLookAt(dampedCamPos, CamTarget, glm::vec3(0,1,0), CamRoll, glm::radians(90.0f), Ar, 0.1f, 500.0f);

	    UniformBufferObject ubo{};
	    GlobalUniformBufferObject gubo{};
	    gubo.lightDir = glm::vec3(cos(glm::radians(135.0f)), sin(glm::radians(135.0f)), 0.0f);
	    gubo.lightColor = glm::vec4(1.0f);
	    gubo.eyePos = dampedCamPos;
	    gubo.eyeDir = glm::vec4(0, 0, 0, 1);

	    glm::mat4 baseTr = glm::mat4(1.0f);

	    // Corpo principale
	    for (const std::string& name : tractorBody) {
	        int i = SC.InstanceIds[name.c_str()];
	        glm::vec3 dP = glm::vec3(glm::rotate(glm::mat4(1), Yaw, glm::vec3(0,1,0)) * glm::vec4(*deltaP[i],1));
	        ubo.mMat = MakeWorld(Pos + dP, Yaw + deltaA[i], 0.0f, 0.0f) * baseTr;
	        ubo.mvpMat = ViewPrj * ubo.mMat;
	        ubo.nMat = glm::inverse(glm::transpose(ubo.mMat));
	        SC.DS[i]->map(currentImage, &ubo, sizeof(ubo), 0);
	        SC.DS[i]->map(currentImage, &gubo, sizeof(gubo), 2);
	    }

	    // Asse anteriore (rotazione orizzontale)
	    for (const std::string& name : tractorAxes) {
	        int i = SC.InstanceIds[name.c_str()];
	        glm::vec3 dP = glm::vec3(glm::rotate(glm::mat4(1), Yaw, glm::vec3(0,1,0)) * glm::vec4(*deltaP[i],1));
	        ubo.mMat = MakeWorld(Pos + dP, Yaw + deltaA[i] + SteeringAng, 0.0f, 0.0f) * baseTr;
	        ubo.mvpMat = ViewPrj * ubo.mMat;
	        ubo.nMat = glm::inverse(glm::transpose(ubo.mMat));
	        SC.DS[i]->map(currentImage, &ubo, sizeof(ubo), 0);
	        SC.DS[i]->map(currentImage, &gubo, sizeof(gubo), 2);
	    }

            // Ruote (rotazione verticale)
            for (const std::string& name : tractorWheels) {
                int i = SC.InstanceIds[name.c_str()];
                glm::vec3 dP = glm::vec3(glm::rotate(glm::mat4(1), Yaw, glm::vec3(0,1,0)) * glm::vec4(*deltaP[i],1));

                // front wheels steer with the axle, back wheels keep the body yaw
                bool isFront = (name == "flw" || name == "frw");
                float yawSteer = isFront ? SteeringAng : 0.0f;

                // right wheels are mirrored in the model and need opposite roll
                bool isRight = (name == "frw" || name == "brw");
                float pitchRoll = isRight ? -wheelRoll : wheelRoll;

                ubo.mMat = MakeWorld(Pos + dP, Yaw + deltaA[i] + yawSteer, pitchRoll, 0.0f) * baseTr;
                ubo.mvpMat = ViewPrj * ubo.mMat;
                ubo.nMat = glm::inverse(glm::transpose(ubo.mMat));
                SC.DS[i]->map(currentImage, &ubo, sizeof(ubo), 0);
                SC.DS[i]->map(currentImage, &gubo, sizeof(gubo), 2);
            }

	    // Scenario
	    for (const std::string& name : tractorScene) {
	        int i = SC.InstanceIds[name.c_str()];
	        ubo.mMat = SC.I[i].Wm * baseTr;
	        ubo.mvpMat = ViewPrj * ubo.mMat;
	        ubo.nMat = glm::inverse(glm::transpose(ubo.mMat));
	        SC.DS[i]->map(currentImage, &ubo, sizeof(ubo), 0);
	        SC.DS[i]->map(currentImage, &gubo, sizeof(gubo), 2);
	    }
	}
};

// This is the main: probably you do not need to touch this!
int main() {
    SIMULATOR app;

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
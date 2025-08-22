#pragma once

#include "Starter.hpp"

struct LogoVertex {
    glm::vec2 pos;
    glm::vec2 texCoord;
};

struct LogoMaker {
    VertexDescriptor VD;
    BaseProject *BP;
    DescriptorSetLayout DSL;
    Pipeline P;
    Model M;
    Texture logos[2];
    DescriptorSet DS[2];

    int currentLogo = 0;


    void init(BaseProject *_BP) {
        BP = _BP;
        createDescriptorSetAndVertexLayout();
        createPipeline();
        createModel();
        createTextures();
    }

    void createDescriptorSetAndVertexLayout() {
        VD.init(BP,
                {{0, sizeof(LogoVertex), VK_VERTEX_INPUT_RATE_VERTEX}},
                {{0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(LogoVertex, pos), sizeof(glm::vec2), OTHER},
                 {0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(LogoVertex, texCoord), sizeof(glm::vec2), UV}});
        DSL.init(BP, {{0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}});
    }

    void createPipeline() {
        P.init(BP, &VD, "shaders/TextVert.spv", "shaders/TextFrag.spv", {&DSL});
        P.setAdvancedFeatures(VK_COMPARE_OP_LESS_OR_EQUAL, VK_POLYGON_MODE_FILL,
                              VK_CULL_MODE_NONE, true);
    }

    void createModel() {
        createQuadMesh();
        M.initMesh(BP, &VD);
    }

    void createQuadMesh() {
        float PtoTdx = -0.95f;
        float PtoTdy = -0.95f;
        float PtoTsx = 2.0f / 800.0f;
        float PtoTsy = 2.0f / 600.0f;
        float w = 150.0f;
        float h = 150.0f;

        int stride = VD.Bindings[0].stride;
        std::vector<unsigned char> vertex(stride, 0);
        LogoVertex *v = (LogoVertex *)&vertex[0];

        auto push = [&](float x, float y, float u, float vcoord) {
            v->pos = {x, y};
            v->texCoord = {u, vcoord};
            M.vertices.insert(M.vertices.end(), vertex.begin(), vertex.end());
        };

        push(PtoTdx, PtoTdy, 0.0f, 0.0f);
        push(PtoTdx + w * PtoTsx, PtoTdy, 1.0f, 0.0f);
        push(PtoTdx, PtoTdy + h * PtoTsy, 0.0f, 1.0f);
        push(PtoTdx + w * PtoTsx, PtoTdy + h * PtoTsy, 1.0f, 1.0f);

        M.indices = {0, 1, 2, 1, 2, 3};
    }

    void createTextures() {
        logos[0].init(BP, "assets/textures/classic_logo.png", VK_FORMAT_R8G8B8A8_SRGB, true);
        logos[1].init(BP, "assets/textures/barbie_logo.png", VK_FORMAT_R8G8B8A8_SRGB, true);
    }

    void createDescriptorSets() {
        for (int i = 0; i < 2; ++i) {
            DS[i].init(BP, &DSL, {{0, TEXTURE, 0, &logos[i]}});
        }
    }

    void pipelinesAndDescriptorSetsInit() {
        P.create();
        createDescriptorSets();
    }

    void pipelinesAndDescriptorSetsCleanup() {
        P.cleanup();
        for (int i = 0; i < 2; ++i) {
            DS[i].cleanup();
        }
    }

    void localCleanup() {
        for (int i = 0; i < 2; ++i) {
            logos[i].cleanup();
        }
        M.cleanup();
        DSL.cleanup();
        P.destroy();
    }

    void setCurrentLogo(int idx) { currentLogo = idx; }

    void populateCommandBuffer(VkCommandBuffer cb, int currentImage) {
        P.bind(cb);
        M.bind(cb);
        DS[currentLogo].bind(cb, P, 0, currentImage);

        vkCmdDrawIndexed(cb, 6, 1, 0, 0, 0);
    }
};


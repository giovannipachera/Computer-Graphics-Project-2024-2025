#pragma once
#include <vector>
#include <string>

struct LogoVertex {
    glm::vec2 pos;
    glm::vec3 color;
    glm::vec2 texCoord;
};

struct LogoMaker {
    VertexDescriptor VD;
    BaseProject *BP;
    DescriptorSetLayout DSL;
    Pipeline P;
    Model M;
    std::vector<Texture> logos;
    std::vector<DescriptorSet> DS;

    void init(BaseProject *bp, const std::vector<std::string> &logoFiles) {
        BP = bp;
        createDescriptorSetAndVertexLayout();
        createPipeline();
        createModelAndTextures(logoFiles);
    }

    void createDescriptorSetAndVertexLayout() {
        VD.init(BP, {{0, sizeof(LogoVertex), VK_VERTEX_INPUT_RATE_VERTEX}},
                {{0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(LogoVertex, pos), sizeof(glm::vec2), OTHER},
                 {0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(LogoVertex, color), sizeof(glm::vec3), COLOR},
                 {0, 2, VK_FORMAT_R32G32_SFLOAT, offsetof(LogoVertex, texCoord), sizeof(glm::vec2), UV}});
        DSL.init(BP, {{0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}});
    }

    void createPipeline() {
        P.init(BP, &VD, "shaders/ShaderVert.spv", "shaders/ShaderFrag.spv", {&DSL});
        P.setAdvancedFeatures(VK_COMPARE_OP_LESS_OR_EQUAL, VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, true);
    }

    void createModelAndTextures(const std::vector<std::string> &logoFiles) {
        createMesh();
        M.initMesh(BP, &VD);
        logos.resize(logoFiles.size());
        for (size_t i = 0; i < logoFiles.size(); ++i) {
            logos[i].init(BP, logoFiles[i], VK_FORMAT_R8G8B8A8_SRGB, true);
        }
        DS.resize(logoFiles.size());
    }

    void createDescriptorSets() {
        for (size_t i = 0; i < logos.size(); ++i) {
            DS[i].init(BP, &DSL, {{0, TEXTURE, 0, &logos[i]}});
        }
    }

    void createMesh() {
        float margin = 0.05f;
        float size = 0.25f;
        float left = -1.0f + margin;
        float top = -1.0f + margin;
        float right = left + size;
        float bottom = top + size;

        M.indices = {0, 1, 2, 1, 2, 3};

        int stride = VD.Bindings[0].stride;
        std::vector<unsigned char> vertex(stride, 0);
        LogoVertex *v = reinterpret_cast<LogoVertex *>(&vertex[0]);

        // Top-left
        v->pos = {left, top};
        v->color = {1.0f, 1.0f, 1.0f};
        v->texCoord = {0.0f, 0.0f};
        M.vertices.insert(M.vertices.end(), vertex.begin(), vertex.end());

        // Top-right
        v->pos = {right, top};
        v->color = {1.0f, 1.0f, 1.0f};
        v->texCoord = {1.0f, 0.0f};
        M.vertices.insert(M.vertices.end(), vertex.begin(), vertex.end());

        // Bottom-left
        v->pos = {left, bottom};
        v->color = {1.0f, 1.0f, 1.0f};
        v->texCoord = {0.0f, 1.0f};
        M.vertices.insert(M.vertices.end(), vertex.begin(), vertex.end());

        // Bottom-right
        v->pos = {right, bottom};
        v->color = {1.0f, 1.0f, 1.0f};
        v->texCoord = {1.0f, 1.0f};
        M.vertices.insert(M.vertices.end(), vertex.begin(), vertex.end());
    }

    void pipelinesAndDescriptorSetsInit() {
        P.create();
        createDescriptorSets();
    }

    void pipelinesAndDescriptorSetsCleanup() {
        P.cleanup();
        for (auto &ds : DS) {
            ds.cleanup();
        }
    }

    void localCleanup() {
        for (auto &tex : logos) {
            tex.cleanup();
        }
        M.cleanup();
        DSL.cleanup();
        P.destroy();
    }

    void populateCommandBuffer(VkCommandBuffer cb, int currentImage, int curLogo) {
        if (logos.empty()) return;
        int idx = curLogo % logos.size();
        P.bind(cb);
        M.bind(cb);
        DS[idx].bind(cb, P, 0, currentImage);
        vkCmdDrawIndexed(cb, static_cast<uint32_t>(M.indices.size()), 1, 0, 0, 0);
    }
};


#pragma once

/*
* This class implements deferred shadow mapping techniques from the paper
*  "Shadow Techniques from Final Fantasy XVI", Square Enix 2023.
* 
* The Technique can be split into 3 main sections:
*  - Gather Lights (Approximately, then Accurately)
*  - Deferred Shadowing (Per pixel visibility of each light)
*  - High Quality Shadows (Composite high quality shadow maps on top)
* 
* The technique comes at the expense of memory, so we can choose to apply
*  the high quality shadow map step to only important objects.
* 
* TODO: More / refined info here.
*/ 

#include <sumire/core/render_systems/high_quality_shadow_mapping/push_constant_structs.hpp>
#include <sumire/core/render_systems/high_quality_shadow_mapping/zbin.hpp>
#include <sumire/core/render_systems/high_quality_shadow_mapping/light_mask.hpp>
#include <sumire/core/render_systems/high_quality_shadow_mapping/view_space_light.hpp>

#include <sumire/core/graphics_pipeline/sumi_buffer.hpp>
#include <sumire/core/graphics_pipeline/sumi_compute_pipeline.hpp>
#include <sumire/core/rendering/lighting/sumi_light.hpp>
#include <sumire/core/rendering/general/sumi_frame_info.hpp>
#include <sumire/core/rendering/general/sumi_camera.hpp>
#include <sumire/core/rendering/geometry/sumi_hzb.hpp>
#include <sumire/core/rendering/geometry/sumi_gbuffer.hpp>

#include <memory>

namespace sumire {

    class HighQualityShadowMapper {
    public:
        HighQualityShadowMapper(
            SumiDevice& device,
            uint32_t screenWidth, 
            uint32_t screenHeight,
            SumiHZB* hzb,
            SumiAttachment* zbuffer,
            SumiAttachment* gWorldPos,
            VkDescriptorSetLayout globalDescriptorSetLayout
        );
        ~HighQualityShadowMapper();

        static constexpr uint32_t NUM_SLICES = 1024u;

        static std::vector<structs::viewSpaceLight> sortLightsByViewSpaceDepth(
            SumiLight::Map& lights,
            glm::mat4 view,
            float near
        );

        void updateScreenBounds(
            uint32_t width, uint32_t height, 
            SumiHZB* hzb,
            SumiAttachment* zbuffer,
            SumiAttachment* gWorldPos
        );

        glm::uvec2 getResolution() const { return glm::uvec2{ screenWidth, screenHeight }; }
        glm::uvec2 getShadowTileResolution() const { return glm::uvec2{ numShadowTilesX, numShadowTilesY }; }
        glm::uvec2 getTileGroupResolution() const { return glm::uvec2{ numTileGroupsX, numTileGroupsY }; }

        // ---- Phase 1: Prepare ---------------------------------------------------------------------------------
        void prepare(
            const std::vector<structs::viewSpaceLight>& lights,
            const SumiCamera& camera
        );

        // ---- Phase 2: Find Lights Approx ----------------------------------------------------------------------
        void findLightsApproximate(
            VkCommandBuffer commandBuffer,
            float near, float far
        );
        SumiBuffer* getLightCountEarlyBuffer() const { return tileLightCountEarlyBuffer.get(); }

        // ---- Phase 3: Find Lights Accurate --------------------------------------------------------------------
        void findLightsAccurate(VkCommandBuffer commandBuffer);
        SumiBuffer* getLightCountFinalBuffer() const { return tileLightCountFinalBuffer.get(); }

        // ---- Phase 4: Generate Deferred Shadows ---------------------------------------------------------------
        void generateDeferredShadows(VkCommandBuffer commandBuffer, FrameInfo& frameInfo);

        // ---- Phase 5: High Quality Shadows --------------------------------------------------------------------
        void compositeHighQualityShadows(VkCommandBuffer commandBuffer);

        // TODO: debug views in composite renderpass.

        const structs::zBin& getZbin() { return zBin; }
        structs::lightMask* getLightMask() { return lightMask.get(); }

    private:
        // ---- (CPU) Phase 1: Prepare ---------------------------------------------------------------------------
        void initPreparePhase();

        void createZbinBuffer();
        void generateZbin(
            const std::vector<structs::viewSpaceLight>& lights,
            const SumiCamera& camera
        );
        void writeZbinBuffer();

        void createLightMaskBuffer();
        void generateLightMask(
            const std::vector<structs::viewSpaceLight>& lights,
            const SumiCamera& camera
        );
        void writeLightMaskBuffer();

        structs::zBin zBin;
        std::unique_ptr<SumiBuffer> zBinBuffer;
        std::unique_ptr<structs::lightMask> lightMask;
        std::unique_ptr<SumiBuffer> lightMaskBuffer;

        // ---- (GPU) Phases 2+ ----------------------------------------------------------------------------------
        void initDescriptorLayouts();
        void createAttachmentSampler();

        VkSampler attachmentSampler = VK_NULL_HANDLE;

        // ---- Phase 2: Find Lights Approx ----------------------------------------------------------------------
        void initLightsApproxPhase(SumiHZB* hzb);
        void createTileGroupLightMaskBuffer();
        void createTileShadowSlotIDsBuffer();
        void createSlotCountersBuffer();
        void initLightsApproxDescriptorSet(SumiHZB* hzb);
        void updateLightsApproxDescriptorSet(SumiHZB* hzb);
        void initLightsApproxPipeline();
        void cleanupLightsApproxPhase();

        std::unique_ptr<SumiBuffer> tileGroupLightMaskBuffer;
        std::unique_ptr<SumiBuffer> tileShadowSlotIDsBuffer;
        std::unique_ptr<SumiBuffer> slotCountersBuffer;

        std::unique_ptr<SumiDescriptorSetLayout> lightsApproxDescriptorLayout;
        VkDescriptorSet lightsApproxDescriptorSet = VK_NULL_HANDLE;

        VkPipelineLayout findLightsApproxPipelineLayout = VK_NULL_HANDLE;
        std::unique_ptr<SumiComputePipeline> findLightsApproxPipeline;

        // ---- Phase 3: Find Lights Accurate --------------------------------------------------------------------
        void initLightsAccuratePhase(SumiAttachment* zbuffer, SumiAttachment* gWorldPos);
        void createTileLightListEarlyBuffer();
        void createTileLightCountEarlyBuffer();
        void initLightsAccurateDescriptorSet(SumiAttachment* zbuffer, SumiAttachment* gWorldPos);
        void updateLightsAccurateDescriptorSet(SumiAttachment* zbuffer, SumiAttachment* gWorldPos);
        void initLightsAccuratePipeline();
        void cleanupLightsAccuratePhase();

        std::unique_ptr<SumiBuffer> tileLightListEarlyBuffer;
        std::unique_ptr<SumiBuffer> tileLightCountEarlyBuffer;

        std::unique_ptr<SumiDescriptorSetLayout> lightsAccurateDescriptorLayout;
        VkDescriptorSet lightsAccurateDescriptorSet = VK_NULL_HANDLE;

        VkPipelineLayout findLightsAccuratePipelineLayout = VK_NULL_HANDLE;
        std::unique_ptr<SumiComputePipeline> findLightsAccuratePipeline;

        // ---- Phase 4: Generate Deferred Shadows ---------------------------------------------------------------
        void initDeferredShadowsPhase(
            SumiAttachment* zbuffer, SumiAttachment* gWorldPos,
            VkDescriptorSetLayout globalDescriptorSetLayout
        );
        void createTileLightListFinalBuffer();
        void createTileLightCountFinalBuffer();
        void createTileLightVisibilityBuffer();
        void initDeferredShadowsDescriptorSet(SumiAttachment* zbuffer, SumiAttachment* gWorldPos);
        void updateDeferredShadowsDescriptorSet(SumiAttachment* zbuffer, SumiAttachment* gWorldPos);
        void initDeferredShadowsPipeline(VkDescriptorSetLayout globalDescriptorSetLayout);
        void cleanupDeferredShadowsPhase();

        std::unique_ptr<SumiBuffer> tileLightListFinalBuffer;;
        std::unique_ptr<SumiBuffer> tileLightCountFinalBuffer;
        std::unique_ptr<SumiBuffer> tileLightVisibilityBuffer;

        std::unique_ptr<SumiDescriptorSetLayout> deferredShadowsDescriptorLayout;
        VkDescriptorSet deferredShadowsDescriptorSet = VK_NULL_HANDLE;

        VkPipelineLayout genDeferredShadowsPipelineLayout = VK_NULL_HANDLE;
        std::unique_ptr<SumiComputePipeline> genDeferredShadowsPipeline;

        // -------------------------------------------------------------------------------------------------------
        
        uint32_t screenWidth;
        uint32_t screenHeight;

        uint32_t numShadowTilesX = 0;
        uint32_t numShadowTilesY = 0;
        uint32_t numShadowTiles  = 0;
        uint32_t numTileGroupsX  = 0;
        uint32_t numTileGroupsY  = 0;
        uint32_t numTileGroups   = 0;
        void calculateTileResolutions();

        SumiDevice& sumiDevice;

        std::unique_ptr<SumiDescriptorPool> descriptorPool;


    };

}
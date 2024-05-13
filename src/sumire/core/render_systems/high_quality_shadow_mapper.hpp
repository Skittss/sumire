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

#include <sumire/core/render_systems/data_structs/high_quality_shadow_mapper_structs.hpp>

#include <sumire/core/graphics_pipeline/sumi_buffer.hpp>
#include <sumire/core/graphics_pipeline/sumi_compute_pipeline.hpp>
#include <sumire/core/rendering/sumi_light.hpp>
#include <sumire/core/rendering/sumi_camera.hpp>
#include <sumire/core/rendering/sumi_hzb.hpp>
#include <sumire/core/rendering/sumi_gbuffer.hpp>

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
            SumiAttachment* gWorldPos
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

        // ---- Phase 3: Find Lights Accurate --------------------------------------------------------------------
        void findLightsAccurate(VkCommandBuffer commandBuffer);

        // ---- Phase 4: Generate Deferred Shadows ---------------------------------------------------------------
        void generateDeferredShadowMaps(VkCommandBuffer commandBuffer);

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
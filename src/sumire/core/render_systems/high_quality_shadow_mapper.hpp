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

#include <memory>

namespace sumire {

    class HighQualityShadowMapper {
    public:
        HighQualityShadowMapper(
            SumiDevice& device,
            uint32_t screenWidth, 
            uint32_t screenHeight,
            SumiHZB* hzb
        );
        ~HighQualityShadowMapper();

        static constexpr uint32_t NUM_SLICES = 1024u;

        static std::vector<structs::viewSpaceLight> sortLightsByViewSpaceDepth(
            SumiLight::Map& lights,
            glm::mat4 view,
            float near
        );

        void updateScreenBounds(
            uint32_t width, uint32_t height, SumiHZB* hzb
        );

        // Phase 1
        void prepare(
            const std::vector<structs::viewSpaceLight>& lights,
            float near, float far,
            const glm::mat4& view,
            const glm::mat4& projection
        );
        // Phase 2
        void findLightsApproximate(
            VkCommandBuffer commandBuffer,
            float near, float far
        );
        // Phase 3
        void findLightsAccurate(VkCommandBuffer commandBuffer);
        // Phase 4
        void generateDeferredShadowMaps(VkCommandBuffer commandBuffer);
        // Phase 5
        void compositeHighQualityShadows(VkCommandBuffer commandBuffer);


        const structs::zBin& getZbin() { return zBin; }
        structs::lightMask* getLightMask() { return lightMask.get(); }

    private:
        // CPU (Phase 1)
        void initPreparePhase();

        void createZbinBuffer();
        void generateZbin(
            const std::vector<structs::viewSpaceLight>& lights,
            float near, float far,
            const glm::mat4& view
        );
        void writeZbinBuffer();

        void createLightMaskBuffer();
        void generateLightMask(
            const std::vector<structs::viewSpaceLight>& lights,
            const glm::mat4& projection
        );
        void writeLightMaskBuffer();

        // GPU (Phases 2+)
        void initDescriptorLayouts();

        // Phase 2
        void initLightsApproxPhase(SumiHZB* hzb);
        void createLightsApproxHZBsampler();
        //void createLightsApproxUniformBuffer();
        void initLightsApproxDescriptorSet(SumiHZB* hzb);
        void updateLightsApproxDescriptorSet(SumiHZB* hzb);
        void initLightsApproxPipeline();
        void cleanupLightsApproxPhase();

        SumiDevice& sumiDevice;
        uint32_t screenWidth;
        uint32_t screenHeight;

        VkSampler HZBsampler = VK_NULL_HANDLE;

        structs::zBin zBin;
        std::unique_ptr<SumiBuffer> zBinBuffer;

        std::unique_ptr<structs::lightMask> lightMask;
        std::unique_ptr<SumiBuffer> lightMaskBuffer;
        std::unique_ptr<SumiBuffer> findLightsApproxUniformBuffer;

        std::unique_ptr<SumiDescriptorPool> descriptorPool;
        std::unique_ptr<SumiDescriptorSetLayout> lightsApproxDescriptorLayout;
        VkDescriptorSet lightsApproxDescriptorSet = VK_NULL_HANDLE;

        VkPipelineLayout findLightsApproxPipelineLayout = VK_NULL_HANDLE;
        std::unique_ptr<SumiComputePipeline> findLightsApproxPipeline;
    };

}
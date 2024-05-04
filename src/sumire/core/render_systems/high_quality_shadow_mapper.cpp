#include <sumire/core/render_systems/high_quality_shadow_mapper.hpp>

#include <sumire/math/view_space_depth.hpp>
#include <sumire/util/vk_check_success.hpp>
#include <sumire/util/sumire_engine_path.hpp>

#include <algorithm>
#include <array>

// debug
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>
#include <iostream>

namespace sumire {

    HighQualityShadowMapper::HighQualityShadowMapper(
        SumiDevice &device,
        const uint32_t screenWidth, 
        const uint32_t screenHeight,
        SumiHZB* hzb
    ) : sumiDevice{ device },
        screenWidth{ screenWidth }, 
        screenHeight{ screenHeight }, 
        zBin{ NUM_SLICES }
    {
        lightMask = std::make_unique<structs::lightMask>(screenWidth, screenHeight);

        initDescriptorLayouts();

        initPreparePhase();
        initLightsApproxPhase(hzb);
    }

    HighQualityShadowMapper::~HighQualityShadowMapper() {
        cleanupLightsApproxPhase();
    }

    std::vector<structs::viewSpaceLight> HighQualityShadowMapper::sortLightsByViewSpaceDepth(
        SumiLight::Map& lights,
        glm::mat4 view,
        float near
    ) {
        // Create view space lights & calculate view space depths
        auto viewSpaceLights = std::vector<structs::viewSpaceLight>();
        for (auto& kv : lights) {
            auto& light = kv.second;

            structs::viewSpaceLight viewSpaceLight{};
            viewSpaceLight.lightPtr = &light;

            viewSpaceLight.viewSpaceDepth = calculateOrthogonalViewSpaceDepth(
                light.transform.getTranslation(),
                view,
                &viewSpaceLight.viewSpacePosition
            );
            viewSpaceLight.minDepth = viewSpaceLight.viewSpaceDepth - light.range;
            viewSpaceLight.maxDepth = viewSpaceLight.viewSpaceDepth + light.range;

            viewSpaceLights.push_back(std::move(viewSpaceLight));
        }

        // Sort by (min) view space depth
        std::sort(viewSpaceLights.begin(), viewSpaceLights.end(), 
            [](const structs::viewSpaceLight& a, const structs::viewSpaceLight& b) {
                return a.minDepth < b.minDepth;
            }
        );

        return viewSpaceLights;
    }

    void HighQualityShadowMapper::updateScreenBounds(
        uint32_t width, uint32_t height, SumiHZB* hzb
    ) {
        screenWidth = width;
        screenHeight = height;

        lightMask = std::make_unique<structs::lightMask>(screenWidth, screenHeight);
        createLightMaskBuffer();
        updateLightsApproxDescriptorSet(hzb);
    }

    void HighQualityShadowMapper::prepare(
        const std::vector<structs::viewSpaceLight>& lights,
        float near, float far,
        const glm::mat4& view,
        const glm::mat4& projection
    ) {
        // We end up doing this preparation step on the CPU as the light list needs
        //  to be view-depth sorted prior to zBin and light mask generation for memory reduction.
        generateZbin(lights, near, far, view);
        writeZbinBuffer();
        generateLightMask(lights, projection);
        writeLightMaskBuffer();
    }

    void HighQualityShadowMapper::findLightsApproximate(
        VkCommandBuffer commandBuffer,
        float near, float far
    ) {
        findLightsApproxPipeline->bind(commandBuffer);

        structs::findLightsApproxPush push{};
        push.tileResolution = glm::uvec2(lightMask->numTilesX, lightMask->numTilesY);
        push.numZbinSlices = NUM_SLICES;
        push.cameraNear = glm::float32(near);
        push.cameraFar = glm::float32(far);

        vkCmdPushConstants(
            commandBuffer,
            findLightsApproxPipelineLayout,
            VK_SHADER_STAGE_COMPUTE_BIT,
            0,
            sizeof(structs::findLightsApproxPush),
            &push
        );

        std::array<VkDescriptorSet, 1> descriptors{
            lightsApproxDescriptorSet
        };

        vkCmdBindDescriptorSets(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_COMPUTE,
            findLightsApproxPipelineLayout,
            0, static_cast<uint32_t>(descriptors.size()),
            descriptors.data(),
            0, nullptr
        );

        uint32_t groupCountX = glm::ceil<uint32_t>(screenWidth / 8.0f);
        uint32_t groupCountY = glm::ceil<uint32_t>(screenHeight / 8.0f);
        vkCmdDispatch(commandBuffer, groupCountX, groupCountY, 1u);
    }

    void HighQualityShadowMapper::initPreparePhase() {
        createZbinBuffer();
        createLightMaskBuffer();
    }

    void HighQualityShadowMapper::createZbinBuffer() {
        zBinBuffer = std::make_unique<SumiBuffer>(
            sumiDevice,
            NUM_SLICES * sizeof(structs::zBinData),
            1,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
        );
        zBinBuffer->map();
    }

    void HighQualityShadowMapper::createLightMaskBuffer() {
        uint32_t nTiles = lightMask->numTilesX * lightMask->numTilesY;
        lightMaskBuffer = std::make_unique<SumiBuffer>(
            sumiDevice,
            nTiles * sizeof(structs::lightMaskTile),
            1,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
        );
        lightMaskBuffer->map();
    }

    void HighQualityShadowMapper::generateZbin(
        const std::vector<structs::viewSpaceLight>& lights,
        float near, float far,
        const glm::mat4 &view
    ) {
        // Bin lights into discrete z intervals between the near and far camera plane.
        // Note: lights MUST BE PRE-SORTED BY VIEWSPACE DISTANCE. ( see sortLightsByViewSpaceDepth() ).
        
        zBin.reset();

        if (lights.size() == 0)  return;

        const float logFarNear = glm::log(far / near);
        const float sliceFrac1 = static_cast<float>(NUM_SLICES) / logFarNear;
        const float sliceFrac2 = static_cast<float>(NUM_SLICES) * glm::log(near) / logFarNear;

        // Fill standard zBin
        uint32_t lastLightIdx = static_cast<uint32_t>(lights.size()) - 1u;
        for (size_t i = 0; i <= lastLightIdx; i++) {
            float minZ = lights[i].minDepth;
            float maxZ = lights[i].maxDepth;

            // Slice calculation from Tiago Sous' DOOM 2016 Siggraph presentation.
            //   Interactive graph: https://www.desmos.com/calculator/bf0g6n0hqp
            int minSlice = glm::floor<int>(glm::log(minZ) * sliceFrac1 - sliceFrac2);
            int maxSlice = glm::floor<int>(glm::log(maxZ) * sliceFrac1 - sliceFrac2);

            // Clamp to valid index range, leaving -1 and NUM_SLICES for out-of-range flags.
            minSlice = glm::clamp<int>(minSlice, -1, NUM_SLICES);
            maxSlice = glm::clamp<int>(maxSlice, -1, NUM_SLICES);

            // Set min and max light indices (zBin metadata) when lights are visible
            if (minSlice < static_cast<int>(NUM_SLICES) && maxSlice >= 0) {
                if (zBin.minLight == -1) zBin.minLight = static_cast<int>(i);
                zBin.maxLight = static_cast<int>(i);
            }

            for (int j = minSlice; j <= maxSlice; j++) {
                // Disregard lights out of zBin range (either behind near plane or beyond far plane)
                if (j < 0 || j >= NUM_SLICES) continue;

                //   min
                if (zBin.data[j].minLightIdx == -1) zBin.data[j].minLightIdx = i;
                else zBin.data[j].minLightIdx = glm::min<int>(zBin.data[j].minLightIdx, i);
                //   max
                zBin.data[j].maxLightIdx = glm::max<int>(zBin.data[j].maxLightIdx, i);

                // Fill first / last zBin idx (zBin metadata)
                //  first
                if (zBin.firstFullIdx == -1 && j != -1) 
                    zBin.firstFullIdx = j;
                else 
                    zBin.firstFullIdx = glm::min(zBin.firstFullIdx, j);
                //  last
                zBin.lastFullIdx = glm::max(zBin.lastFullIdx, j);
            }
        }

        // Fill ranged zBin
        const uint32_t lastZbinIdx = static_cast<uint32_t>(zBin.data.size()) - 1u;
        int nextIdx;
        int currIdx;
        int prevIdx;

        int nextRangedMin = zBin.minLight;

        // TODO: Here be some voodoo. There is likely a more straight forward way to fill this
        //         ranged buffer, but the paper isn't very explicit about how they do it so I made my own
        //		   solution. 
        //       We fill explicit ranges first, then fill in the gaps with a reversed two-pointer type approach.

        // First pass - fill in explicit ranges
        for (uint32_t j = 0; j <= lastZbinIdx; j++) {
            // rMin
            if (j != lastZbinIdx) {
                currIdx = zBin.data[j].minLightIdx;
                nextIdx = zBin.data[j + 1].minLightIdx;

                if (nextIdx == -1) 
                    nextIdx = zBin.data[j].minLightIdx;
                if (currIdx != -1)
                    zBin.data[j].rangedMinLightIdx = glm::min(currIdx, nextIdx);
            }

            // rMax
            if (j != 0) {
                currIdx = zBin.data[j].maxLightIdx;
                prevIdx = zBin.data[j - 1].maxLightIdx;

                if (prevIdx == -1) 
                    prevIdx = zBin.data[j].maxLightIdx;
                if (currIdx != -1)
                    zBin.data[j].rangedMaxLightIdx = glm::max(currIdx, prevIdx);
            }
        }

        // Second pass - fill in implicit ranges (gaps) where indices are still undefined (-1)
        //   but have prior (in the case of max) or following (in the case of min) lights.
        // TODO: There may be a way to combine this into the pass above for O(2n) -> O(n).
        int minIdxCache = zBin.maxLight;
        int maxIdxCache = zBin.minLight;

        uint32_t minPtr;
        for (uint32_t maxPtr = 0; maxPtr <= lastZbinIdx; maxPtr++) {
            // rMin
            minPtr = lastZbinIdx - maxPtr;
            if (minPtr <= zBin.lastFullIdx) {
                int currMinIdx = zBin.data[minPtr].rangedMinLightIdx;
                if (currMinIdx == -1) {
                    zBin.data[minPtr].rangedMinLightIdx = minIdxCache;
                }
                else if (minPtr == 0 || zBin.data[minPtr - 1].rangedMinLightIdx == -1) {
                    minIdxCache = currMinIdx;
                }
            }

            // rMax
            if (maxPtr >= zBin.firstFullIdx) {
                int currMaxIdx = zBin.data[maxPtr].rangedMaxLightIdx;
                if (currMaxIdx == -1) {
                    zBin.data[maxPtr].rangedMaxLightIdx = maxIdxCache;
                }
                else if (maxPtr == lastZbinIdx || zBin.data[maxPtr + 1].rangedMaxLightIdx == -1) {
                    maxIdxCache = currMaxIdx;
                }
            }
        }
    }

    void HighQualityShadowMapper::writeZbinBuffer() {
        zBinBuffer->writeToBuffer(zBin.data.data());
        zBinBuffer->flush();
    }

    void HighQualityShadowMapper::generateLightMask(
        const std::vector<structs::viewSpaceLight>& lights,
        const glm::mat4& projection
    ) {
        assert(lights.size() < 1025);

        lightMask->clear();

        for (uint32_t i = 0; i < lights.size(); i++) {
            // TODO: should we be culling all lights off-screen?
            //         there are some details on this at the end of the paper.
            //if (light.viewSpaceDepth < near)

            auto& light = lights[i];

            // light -> raster space
            glm::vec4 screenPos = projection * glm::vec4(light.viewSpacePosition, 1.0);
            screenPos /= screenPos.w;
            
            glm::vec2 ndcPos = 0.5f + 0.5f * glm::vec2(screenPos);
            glm::vec2 rasterPos = glm::floor(glm::vec2{
                ndcPos.x * screenWidth,
                (1.0f - ndcPos.y) * screenHeight
                });

            if (rasterPos.x < 0 || rasterPos.y < 0 ||
                rasterPos.x >= screenWidth || rasterPos.y >= screenHeight) { continue; }

            // Set light validity for tile
            uint32_t tileIdx_x = rasterPos.x / 32u;
            uint32_t tileIdx_y = rasterPos.y / 32u;

            structs::lightMaskTile& tile = lightMask->tileAtIdx(tileIdx_x, tileIdx_y);
            tile.setLightBit(i);
        }
    }

    void HighQualityShadowMapper::writeLightMaskBuffer() {
        lightMaskBuffer->writeToBuffer((void *)lightMask->tiles.data());
        lightMaskBuffer->flush();
    }

    void HighQualityShadowMapper::initDescriptorLayouts() {
        descriptorPool = SumiDescriptorPool::Builder(sumiDevice)
            .setMaxSets(3)
            .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1)
            .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2)
            .build();

        lightsApproxDescriptorLayout = SumiDescriptorSetLayout::Builder(sumiDevice)
            .addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
            .addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
            .addBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
            .build();
    }

    void HighQualityShadowMapper::initLightsApproxPhase(SumiHZB* hzb) {
        //createLightsApproxUniformBuffer();
        //createLightsApproxHZBsampler();
        initLightsApproxDescriptorSet(hzb);
        initLightsApproxPipeline();
    }

    //void HighQualityShadowMapper::createLightsApproxUniformBuffer() {
    //	zBinBuffer = std::make_unique<SumiBuffer>(
    //		sumiDevice,
    //		sizeof(structs::findLightsApproxUniforms),
    //		1,
    //		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
    //		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
    //	);
    //	zBinBuffer->map();
    //}

    //void HighQualityShadowMapper::createLightsApproxHZBsampler() {
    //    VkSamplerCreateInfo samplerCreateInfo{};
    //    samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    //    samplerCreateInfo.magFilter = VK_FILTER_NEAREST;
    //    samplerCreateInfo.minFilter = VK_FILTER_NEAREST;
    //    samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE; // Clamp to edge to not affect downsample min 
    //    samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    //    samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    //    samplerCreateInfo.anisotropyEnable = VK_FALSE;
    //    samplerCreateInfo.maxAnisotropy = 0.0f;
    //    samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    //    samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;
    //    samplerCreateInfo.compareEnable = VK_FALSE;
    //    samplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    //    samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    //    samplerCreateInfo.mipLodBias = 0.0f;
    //    samplerCreateInfo.minLod = 0.0f;
    //    samplerCreateInfo.maxLod = 0.0f;

    //    VK_CHECK_SUCCESS(
    //        vkCreateSampler(sumiDevice.device(), &samplerCreateInfo, nullptr, &HZBsampler),
    //        "[Sumire::HighQualityShadowMapper] Failed to create HZB sampler."
    //    );
    //}

    void HighQualityShadowMapper::initLightsApproxDescriptorSet(SumiHZB* hzb) {
        assert(zBinBuffer != nullptr && "Cannot instantiate descriptor set with null zbin buffer");
        assert(lightMaskBuffer != nullptr && "Cannot instantiate descriptor set with null light mask buffer");

        VkDescriptorBufferInfo zbinInfo = zBinBuffer->descriptorInfo();
        VkDescriptorBufferInfo lightMaskInfo = lightMaskBuffer->descriptorInfo();

        VkDescriptorImageInfo hzbInfo{};
        hzbInfo.sampler     = VK_NULL_HANDLE;
        hzbInfo.imageView   = hzb->getBaseImageView();
        hzbInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

        SumiDescriptorWriter(*lightsApproxDescriptorLayout, *descriptorPool)
            .writeImage(0, &hzbInfo)
            .writeBuffer(1, &zbinInfo)
            .writeBuffer(2, &lightMaskInfo)
            .build(lightsApproxDescriptorSet);
    }

    void HighQualityShadowMapper::updateLightsApproxDescriptorSet(SumiHZB* hzb) {
        assert(lightMaskBuffer != nullptr && "Cannot update descriptor set with null light mask buffer");

        VkDescriptorBufferInfo zbinInfo = zBinBuffer->descriptorInfo();
        VkDescriptorBufferInfo lightMaskInfo = lightMaskBuffer->descriptorInfo();

        VkDescriptorImageInfo hzbInfo{};
        hzbInfo.sampler = VK_NULL_HANDLE;
        hzbInfo.imageView = hzb->getBaseImageView();
        hzbInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

        SumiDescriptorWriter(*lightsApproxDescriptorLayout, *descriptorPool)
            .writeImage(0, &hzbInfo)
            .writeBuffer(1, &zbinInfo)
            .writeBuffer(2, &lightMaskInfo)
            .overwrite(lightsApproxDescriptorSet);
    }

    void HighQualityShadowMapper::initLightsApproxPipeline() {

        VkPushConstantRange pushRange{};
        pushRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        pushRange.offset = 0;
        pushRange.size = sizeof(structs::findLightsApproxPush);

        std::vector<VkDescriptorSetLayout> descriptorSetLayouts{
            lightsApproxDescriptorLayout->getDescriptorSetLayout()
        };

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
        pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushRange;

        VK_CHECK_SUCCESS(
            vkCreatePipelineLayout(
                sumiDevice.device(), &pipelineLayoutInfo, nullptr, &findLightsApproxPipelineLayout),
            "[Sumire::HighQualityShadowMapper] Failed to create lights approx pipeline layout (Phase 1)."
        );

        findLightsApproxPipeline = std::make_unique<SumiComputePipeline>(
            sumiDevice,
            SUMIRE_ENGINE_PATH(
                "shaders/high_quality_shadow_mapping/find_lights_approx/find_lights_approximate.comp.spv"),
            findLightsApproxPipelineLayout
        );
    }

    void HighQualityShadowMapper::cleanupLightsApproxPhase() {
        vkDestroyPipelineLayout(sumiDevice.device(), findLightsApproxPipelineLayout, nullptr);
        //vkDestroySampler(sumiDevice.device(), , nullptr);
    }
}
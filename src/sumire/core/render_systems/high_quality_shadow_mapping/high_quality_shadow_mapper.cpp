#include <sumire/core/render_systems/high_quality_shadow_mapping/high_quality_shadow_mapper.hpp>

#include <sumire/math/view_space_depth.hpp>
#include <sumire/math/frustum_culling.hpp>
#include <sumire/math/coord_space_converters.hpp>
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
        SumiHZB* hzb,
        SumiAttachment* zbuffer,
        SumiAttachment* gWorldPos,
        VkDescriptorSetLayout globalDescriptorSetLayout
    ) : sumiDevice{ device },
        screenWidth{ screenWidth }, 
        screenHeight{ screenHeight }, 
        zBin{ NUM_SLICES }
    {
        lightMask = std::make_unique<structs::lightMask>(screenWidth, screenHeight);
        calculateTileResolutions();

        initDescriptorLayouts();

        initPreparePhase();                           // Phase 1
        initLightsApproxPhase(hzb);                   // Phase 2
        initLightsAccuratePhase(zbuffer, gWorldPos);  // Phase 3
        initDeferredShadowsPhase(                     // Phase 4
            zbuffer, gWorldPos, 
            globalDescriptorSetLayout
        ); 
    }

    HighQualityShadowMapper::~HighQualityShadowMapper() {
        cleanupLightsApproxPhase();
        cleanupLightsAccuratePhase();
        cleanupDeferredShadowsPhase();
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
        uint32_t width, uint32_t height, 
        SumiHZB* hzb,
        SumiAttachment* zbuffer,
        SumiAttachment* gWorldPos
    ) {
        screenWidth = width;
        screenHeight = height;

        lightMask = std::make_unique<structs::lightMask>(screenWidth, screenHeight);
        calculateTileResolutions();

        // ---- Recreate Lights Approx Buffers -------------------------------------------------------------------
        lightMaskBuffer = nullptr;
        createLightMaskBuffer();

        tileGroupLightMaskBuffer = nullptr;
        createTileGroupLightMaskBuffer();

        tileShadowSlotIDsBuffer = nullptr;
        createTileShadowSlotIDsBuffer();

        slotCountersBuffer = nullptr;
        createSlotCountersBuffer();

        updateLightsApproxDescriptorSet(hzb);

        // ---- Recreate Lights Accurate Buffers -----------------------------------------------------------------
        tileLightListEarlyBuffer = nullptr;
        createTileLightListEarlyBuffer();

        tileLightCountEarlyBuffer = nullptr;
        createTileLightCountEarlyBuffer();

        updateLightsAccurateDescriptorSet(zbuffer, gWorldPos);

        // ---- Recreate Deferred Shadows Buffers ----------------------------------------------------------------
        tileLightListFinalBuffer = nullptr;
        createTileLightListFinalBuffer();

        tileLightCountFinalBuffer = nullptr;
        createTileLightCountFinalBuffer();

        tileLightVisibilityBuffer = nullptr;
        createTileLightVisibilityBuffer();

        updateDeferredShadowsDescriptorSet(zbuffer, gWorldPos);
    }

    void HighQualityShadowMapper::prepare(
        const std::vector<structs::viewSpaceLight>& lights,
        const SumiCamera& camera
    ) {
        // We end up doing this preparation step on the CPU as the light list needs
        //  to be view-depth sorted prior to zBin and light mask generation for memory reduction.
        generateZbin(lights, camera);
        writeZbinBuffer();
        generateLightMask(lights, camera);
        writeLightMaskBuffer();
    }

    void HighQualityShadowMapper::findLightsApproximate(
        VkCommandBuffer commandBuffer,
        float near, float far
    ) {
        findLightsApproxPipeline->bind(commandBuffer);

        structs::findLightsApproxPush push{};
        push.screenResolution     = glm::uvec2(screenWidth, screenHeight);
        push.shadowTileResolution = glm::uvec2(numShadowTilesX, numShadowTilesY);
        push.tileGroupResolution  = glm::uvec2(numTileGroupsX, numTileGroupsY);
        push.lightMaskResolution  = glm::uvec2(lightMask->numTilesX, lightMask->numTilesY);
        push.numZbinSlices        = glm::uint(NUM_SLICES);
        push.cameraNear           = glm::float32(near);
        push.cameraFar            = glm::float32(far);

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

        // Execute one work group per tile group
        vkCmdDispatch(commandBuffer, numTileGroupsX, numTileGroupsY, 1);
    }

    void HighQualityShadowMapper::findLightsAccurate(
        VkCommandBuffer commandBuffer
    ) {
        findLightsAccuratePipeline->bind(commandBuffer);

        structs::findLightsAccuratePush push{};
        push.screenResolution     = glm::uvec2(screenWidth, screenHeight);
        push.shadowTileResolution = glm::uvec2(numShadowTilesX, numShadowTilesY);
        push.tileGroupResolution  = glm::uvec2(numTileGroupsX, numTileGroupsY);
        push.lightMaskResolution  = glm::uvec2(lightMask->numTilesX, lightMask->numTilesY);
        push.numZbinSlices        = NUM_SLICES;

        vkCmdPushConstants(
            commandBuffer,
            findLightsAccuratePipelineLayout,
            VK_SHADER_STAGE_COMPUTE_BIT,
            0,
            sizeof(structs::findLightsAccuratePush),
            &push
        );

        std::array<VkDescriptorSet, 1> descriptors{
            lightsAccurateDescriptorSet
        };

        vkCmdBindDescriptorSets(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_COMPUTE,
            findLightsAccuratePipelineLayout,
            0, static_cast<uint32_t>(descriptors.size()),
            descriptors.data(),
            0, nullptr
        );

        // Execute one work group per shadow tile
        vkCmdDispatch(commandBuffer, numShadowTilesX, numShadowTilesY, 1);
    }

    void HighQualityShadowMapper::generateDeferredShadows(
        VkCommandBuffer commandBuffer, FrameInfo& frameInfo
    ) {
        genDeferredShadowsPipeline->bind(commandBuffer);

        structs::genDeferredShadowsPush push{};
        push.screenResolution     = glm::uvec2(screenWidth, screenHeight);
        push.shadowTileResolution = glm::uvec2(numShadowTilesX, numShadowTilesY);
        push.tileGroupResolution  = glm::uvec2(numTileGroupsX, numTileGroupsY);
        push.lightMaskResolution  = glm::uvec2(lightMask->numTilesX, lightMask->numTilesY);
        push.numZbinSlices        = NUM_SLICES;

        vkCmdPushConstants(
            commandBuffer,
            genDeferredShadowsPipelineLayout,
            VK_SHADER_STAGE_COMPUTE_BIT,
            0,
            sizeof(structs::genDeferredShadowsPush),
            &push
        );

        std::array<VkDescriptorSet, 2> descriptors{
            frameInfo.globalDescriptorSet,
            deferredShadowsDescriptorSet
        };

        vkCmdBindDescriptorSets(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_COMPUTE,
            genDeferredShadowsPipelineLayout,
            0, static_cast<uint32_t>(descriptors.size()),
            descriptors.data(),
            0, nullptr
        );

        // Execute one work group per shadow tile
        vkCmdDispatch(commandBuffer, numShadowTilesX, numShadowTilesY, 1);
    }

    // ---- (CPU) Phase 1: Prepare -------------------------------------------------------------------------------
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
        const SumiCamera& camera
    ) {
        // Bin lights into discrete z intervals between the near and far camera plane.
        // Note: lights MUST BE PRE-SORTED BY VIEWSPACE DISTANCE. ( see sortLightsByViewSpaceDepth() ).
        
        zBin.reset();

        if (lights.size() == 0)  return;

        const glm::mat4 view = camera.getViewMatrix();
        const float near     = camera.getNear();
        const float far      = camera.getFar();

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
        const SumiCamera& camera
    ) {
        assert(lights.size() < 1025);

        lightMask->clear();

        // Frustum calculations are done in view space as we have access
        //   to all light positions in view space from the prior sort.
        constexpr glm::vec3 origin = glm::vec3(0.0f);
        const float near = camera.getNear();
        const float far  = camera.getFar();

        const glm::vec2 screenDim{ screenWidth, screenHeight };
        const glm::mat4 invProjection = glm::inverse(camera.getProjectionMatrix());

        // Cull lights against frusta with sphere intersection
        for (uint32_t frustumX = 0; frustumX < lightMask->numTilesX; frustumX++) {
            for (uint32_t frustumY = 0; frustumY < lightMask->numTilesY; frustumY++) {
                structs::lightMaskTile& tile = lightMask->tileAtIdx(frustumX, frustumY);

                constexpr float lightMaskTileSize = 32.0f;

                // Screen space coords of frusta points
                const glm::vec4 screenTopL = { lightMaskTileSize * glm::vec2{  frustumX  , frustumY  }, -1.0f, 1.0f };
                const glm::vec4 screenTopR = { lightMaskTileSize * glm::vec2{  frustumX+1, frustumY  }, -1.0f, 1.0f };
                const glm::vec4 screenBotL = { lightMaskTileSize * glm::vec2{  frustumX  , frustumY+1}, -1.0f, 1.0f };
                const glm::vec4 screenBotR = { lightMaskTileSize * glm::vec2{  frustumX+1, frustumY+1}, -1.0f, 1.0f };

                // View space coords of frusta points
                // TODO: Can probably calculate these view space pos from camera's intrinsic to save 
                //       On the inv projection calculation + four multiplications per tile
                const glm::vec3 viewTopL = glm::vec3(screenToView(screenTopL, screenDim, invProjection));
                const glm::vec3 viewTopR = glm::vec3(screenToView(screenTopR, screenDim, invProjection));
                const glm::vec3 viewBotL = glm::vec3(screenToView(screenBotL, screenDim, invProjection));
                const glm::vec3 viewBotR = glm::vec3(screenToView(screenBotR, screenDim, invProjection));

                // Frusta bounding planes from points
                const FrustumPlane tileFrustumT = computeFrustumPlane(origin, viewTopR, viewTopL);
                const FrustumPlane tileFrustumB = computeFrustumPlane(origin, viewBotL, viewBotR);
                const FrustumPlane tileFrustumR = computeFrustumPlane(origin, viewBotR, viewTopR);
                const FrustumPlane tileFrustumL = computeFrustumPlane(origin, viewTopL, viewBotL);

                for (uint32_t i = 0; i < lights.size(); i++) {
                    const float r = lights[i].lightPtr->range;
                    const glm::vec3 p = lights[i].viewSpacePosition;

                    const bool intersectionIntT = tileFrustumT.intersectSphere(p, r);
                    const bool intersectionIntB = tileFrustumB.intersectSphere(p, r);
                    const bool intersectionIntL = tileFrustumL.intersectSphere(p, r);
                    const bool intersectionIntR = tileFrustumR.intersectSphere(p, r);
                    const bool intersectionIntN = lights[i].viewSpaceDepth + r > near;
                    const bool intersectionIntF = lights[i].viewSpaceDepth - r < far;

                    const bool intersects = (
                        intersectionIntT &&
                        intersectionIntB &&
                        intersectionIntL &&
                        intersectionIntR &&
                        intersectionIntN &&
                        intersectionIntF
                    );

                    if (intersects) tile.setLightBit(i);
                }
            }
        }
    }

    void HighQualityShadowMapper::writeLightMaskBuffer() {
        lightMaskBuffer->writeToBuffer((void *)lightMask->tiles.data());
        lightMaskBuffer->flush();
    }

    // ---- (GPU) Phases 2+ --------------------------------------------------------------------------------------
    void HighQualityShadowMapper::initDescriptorLayouts() {
        descriptorPool = SumiDescriptorPool::Builder(sumiDevice)
            .setMaxSets(
                6 + // Lights Approx
                6 + // Lights Accurate
                9   // Deferred Shadows
            )
            // ---- Lights Approx -----------------------------------------
            .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1)
            .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 5)
            // ---- Lights Accurate ---------------------------------------
            .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2)
            .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 4)
            // ---- Deferred Shadows --------------------------------------
            .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2)
            .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 7)
            // ------------------------------------------------------------
            .build();

        // TODO: Consider splitting these descriptor sets into multiple sets so that 
        //       shared buffers do not have to be re-bound (e.g. tileGroupLightMaskBuffer).
        lightsApproxDescriptorLayout = SumiDescriptorSetLayout::Builder(sumiDevice)
            .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT) // HZB
            .addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT) // TileGroupLightMaskBuffer
            .addBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT) // TileShadowSlotIDsBuffer
            .addBinding(3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT) // SlotCountersBuffer
            .addBinding(4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT) // zBin
            .addBinding(5, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT) // LightMaskBuffer
            .build();

        lightsAccurateDescriptorLayout = SumiDescriptorSetLayout::Builder(sumiDevice)
            .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT) // Z buffer
            .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT) // G world pos
            .addBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT) // TileGroupLightMaskBuffer
            .addBinding(3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT) // TileShadowSlotIDsBuffer
            .addBinding(4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT) // TileLightListEarly
            .addBinding(5, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT) // TileLightCountEarly
            .build();

        deferredShadowsDescriptorLayout = SumiDescriptorSetLayout::Builder(sumiDevice)
            .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT) // Z buffer
            .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT) // G world pos
            .addBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT) // TileShadowSlotIDsBuffer
            .addBinding(3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT) // SlotCountersBuffer
            .addBinding(4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT) // TileLightListEarly
            .addBinding(5, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT) // TileLightCountEarly
            .addBinding(6, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT) // TileLightListFinal
            .addBinding(7, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT) // TileLightCountFinal
            .addBinding(8, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT) // TileLightVisibility
            .build();
    }

    void HighQualityShadowMapper::createAttachmentSampler() {
        VkSamplerCreateInfo samplerCreateInfo{};
        samplerCreateInfo.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerCreateInfo.magFilter               = VK_FILTER_NEAREST;
        samplerCreateInfo.minFilter               = VK_FILTER_NEAREST;
        samplerCreateInfo.addressModeU            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerCreateInfo.addressModeV            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerCreateInfo.addressModeW            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerCreateInfo.anisotropyEnable        = VK_FALSE;
        samplerCreateInfo.maxAnisotropy           = 0.0f;
        samplerCreateInfo.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;
        samplerCreateInfo.compareEnable           = VK_FALSE;
        samplerCreateInfo.compareOp               = VK_COMPARE_OP_ALWAYS;
        samplerCreateInfo.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        samplerCreateInfo.mipLodBias              = 0.0f;
        samplerCreateInfo.minLod                  = 0.0f;
        samplerCreateInfo.maxLod                  = 0.0f;

        VK_CHECK_SUCCESS(
            vkCreateSampler(sumiDevice.device(), &samplerCreateInfo, nullptr, &attachmentSampler),
            "[Sumire::HighQualityShadowMapper] Failed to create attachment sampler."
        );
    }

    // ---- Phase 2: Find Lights Approx --------------------------------------------------------------------------
    void HighQualityShadowMapper::initLightsApproxPhase(SumiHZB* hzb) {
        createAttachmentSampler();
        createTileGroupLightMaskBuffer();
        createTileShadowSlotIDsBuffer();
        createSlotCountersBuffer();
        initLightsApproxDescriptorSet(hzb);
        initLightsApproxPipeline();
    }

    void HighQualityShadowMapper::createTileGroupLightMaskBuffer() {
        // 1 Light mask entry per 8x8 shadow tile group
        tileGroupLightMaskBuffer = std::make_unique<SumiBuffer>(
            sumiDevice,
            numTileGroups * sizeof(structs::lightMaskTile),
            1,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        );
    }

    void HighQualityShadowMapper::createTileShadowSlotIDsBuffer() {
        // 1 ID per shadow tile
        tileShadowSlotIDsBuffer = std::make_unique<SumiBuffer>(
            sumiDevice,
            numShadowTiles * sizeof(uint32_t),
            1,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        );
    }

    void HighQualityShadowMapper::createSlotCountersBuffer() {
        // 1 Counter per tile group
        slotCountersBuffer = std::make_unique<SumiBuffer>(
            sumiDevice,
            numTileGroups * sizeof(uint32_t),
            1,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        );
    }

    void HighQualityShadowMapper::initLightsApproxDescriptorSet(SumiHZB* hzb) {
        assert(zBinBuffer != nullptr 
            && "Cannot instantiate descriptor set with null zbin buffer");
        assert(lightMaskBuffer != nullptr 
            && "Cannot instantiate descriptor set with null light mask buffer");
        assert(tileGroupLightMaskBuffer != nullptr 
            && "Cannot instantiate descriptor set with null tile group light mask buffer");
        assert(tileShadowSlotIDsBuffer != nullptr
            && "Cannot instantiate descriptor set with null tile shadow slot ids buffer");
        assert(slotCountersBuffer != nullptr
            && "Cannot instantiate descriptor set with null slot counters buffer");

        VkDescriptorBufferInfo zbinInfo               = zBinBuffer->descriptorInfo();
        VkDescriptorBufferInfo lightMaskInfo          = lightMaskBuffer->descriptorInfo();
        VkDescriptorBufferInfo tileGroupLightMaskInfo = tileGroupLightMaskBuffer->descriptorInfo();
        VkDescriptorBufferInfo tileShadowSlotIDsInfo  = tileShadowSlotIDsBuffer->descriptorInfo();
        VkDescriptorBufferInfo slotCountersInfo       = slotCountersBuffer->descriptorInfo();

        VkDescriptorImageInfo hzbInfo{};
        hzbInfo.sampler     = attachmentSampler;
        hzbInfo.imageView   = hzb->getBaseImageView();
        hzbInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        SumiDescriptorWriter(*lightsApproxDescriptorLayout, *descriptorPool)
            .writeImage(0, &hzbInfo)
            .writeBuffer(1, &tileGroupLightMaskInfo)
            .writeBuffer(2, &tileShadowSlotIDsInfo)
            .writeBuffer(3, &slotCountersInfo)
            .writeBuffer(4, &zbinInfo)
            .writeBuffer(5, &lightMaskInfo)
            .build(lightsApproxDescriptorSet);
    }

    void HighQualityShadowMapper::updateLightsApproxDescriptorSet(SumiHZB* hzb) {
        assert(zBinBuffer != nullptr
            && "Cannot update descriptor set with null zbin buffer");
        assert(lightMaskBuffer != nullptr 
            && "Cannot update descriptor set with null light mask buffer");
        assert(tileGroupLightMaskBuffer != nullptr
            && "Cannot update descriptor set with null tile group light mask buffer");
        assert(tileShadowSlotIDsBuffer != nullptr
            && "Cannot update descriptor set with null tile shadow slot ids buffer");
        assert(slotCountersBuffer != nullptr
            && "Cannot update descriptor set with null slot counters buffer");

        VkDescriptorBufferInfo zbinInfo               = zBinBuffer->descriptorInfo();
        VkDescriptorBufferInfo lightMaskInfo          = lightMaskBuffer->descriptorInfo();
        VkDescriptorBufferInfo tileGroupLightMaskInfo = tileGroupLightMaskBuffer->descriptorInfo();
        VkDescriptorBufferInfo tileShadowSlotIDsInfo  = tileShadowSlotIDsBuffer->descriptorInfo();
        VkDescriptorBufferInfo slotCountersInfo       = slotCountersBuffer->descriptorInfo();

        VkDescriptorImageInfo hzbInfo{};
        hzbInfo.sampler = attachmentSampler;
        hzbInfo.imageView = hzb->getBaseImageView();
        hzbInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        SumiDescriptorWriter(*lightsApproxDescriptorLayout, *descriptorPool)
            .writeImage(0, &hzbInfo)
            .writeBuffer(1, &tileGroupLightMaskInfo)
            .writeBuffer(2, &tileShadowSlotIDsInfo)
            .writeBuffer(3, &slotCountersInfo)
            .writeBuffer(4, &zbinInfo)
            .writeBuffer(5, &lightMaskInfo)
            .overwrite(lightsApproxDescriptorSet);
    }

    void HighQualityShadowMapper::initLightsApproxPipeline() {

        VkPushConstantRange pushRange{};
        pushRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        pushRange.offset     = 0;
        pushRange.size       = sizeof(structs::findLightsApproxPush);

        std::vector<VkDescriptorSetLayout> descriptorSetLayouts{
            lightsApproxDescriptorLayout->getDescriptorSetLayout()
        };

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount         = static_cast<uint32_t>(descriptorSetLayouts.size());
        pipelineLayoutInfo.pSetLayouts            = descriptorSetLayouts.data();
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges    = &pushRange;

        VK_CHECK_SUCCESS(
            vkCreatePipelineLayout(
                sumiDevice.device(), &pipelineLayoutInfo, nullptr, &findLightsApproxPipelineLayout),
            "[Sumire::HighQualityShadowMapper] Failed to create lights approx pipeline layout (Phase 1)."
        );

        findLightsApproxPipeline = std::make_unique<SumiComputePipeline>(
            sumiDevice,
            SUMIRE_ENGINE_PATH(
                "shaders/high_quality_shadow_mapping/find_lights_approximate.comp"),
            findLightsApproxPipelineLayout
        );
    }

    void HighQualityShadowMapper::cleanupLightsApproxPhase() {
        vkDestroyPipelineLayout(sumiDevice.device(), findLightsApproxPipelineLayout, nullptr);

        // This cleanup func also handles cleaning up shared resources for phases 2+
        vkDestroySampler(sumiDevice.device(), attachmentSampler, nullptr);  
    }
    
    // ---- Phase 3: Find Lights Accurate ------------------------------------------------------------------------
    void HighQualityShadowMapper::initLightsAccuratePhase(
        SumiAttachment* zbuffer, SumiAttachment* gWorldPos
    ) {
        createTileLightListEarlyBuffer();
        createTileLightCountEarlyBuffer();
        initLightsAccurateDescriptorSet(zbuffer, gWorldPos);
        initLightsAccuratePipeline();
    }

    void HighQualityShadowMapper::createTileLightListEarlyBuffer() {
        // 1 List per shadow tile
        // TODO: What is the list size here?
        tileLightListEarlyBuffer = std::make_unique<SumiBuffer>(
            sumiDevice,
            numShadowTiles * sizeof(uint32_t),
            1,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        );
    }

    void HighQualityShadowMapper::createTileLightCountEarlyBuffer() {
        // 1 entry per shadow tile
        tileLightCountEarlyBuffer = std::make_unique<SumiBuffer>(
            sumiDevice,
            numShadowTiles * sizeof(uint32_t),
            1,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        );
    }

    void HighQualityShadowMapper::initLightsAccurateDescriptorSet(
        SumiAttachment* zbuffer,
        SumiAttachment* gWorldPos
    ) {
        assert(zbuffer != nullptr
            && "Cannot instantiate descriptor set with null zbuffer");
        assert(gWorldPos != nullptr
            && "Cannot instantiate descriptor set with null world pos attachment");
        assert(tileLightListEarlyBuffer != nullptr
            && "Cannot instantiate descriptor set with null tile light list early buffer");
        assert(tileLightCountEarlyBuffer != nullptr
            && "Cannot instantiate descriptor set with null tile light count early buffer");

        VkDescriptorImageInfo zbufferInfo{};
        zbufferInfo.sampler     = attachmentSampler;
        zbufferInfo.imageView   = zbuffer->getImageView();
        zbufferInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

        VkDescriptorImageInfo gWorldPosInfo{};
        gWorldPosInfo.sampler     = attachmentSampler;
        gWorldPosInfo.imageView   = gWorldPos->getImageView();
        gWorldPosInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkDescriptorBufferInfo tileGroupLightMaskInfo = tileGroupLightMaskBuffer->descriptorInfo();
        VkDescriptorBufferInfo tileShadowSlotIDsInfo  = tileShadowSlotIDsBuffer->descriptorInfo();
        VkDescriptorBufferInfo lightListEarlyInfo     = tileLightListEarlyBuffer->descriptorInfo();
        VkDescriptorBufferInfo lightCountEarlyInfo    = tileLightCountEarlyBuffer->descriptorInfo();

        SumiDescriptorWriter(*lightsAccurateDescriptorLayout, *descriptorPool)
            .writeImage(0, &zbufferInfo)
            .writeImage(1, &gWorldPosInfo)
            .writeBuffer(2, &tileGroupLightMaskInfo)
            .writeBuffer(3, &tileShadowSlotIDsInfo)
            .writeBuffer(4, &lightListEarlyInfo)
            .writeBuffer(5, &lightCountEarlyInfo)
            .build(lightsAccurateDescriptorSet);
    }

    void HighQualityShadowMapper::updateLightsAccurateDescriptorSet(
        SumiAttachment* zbuffer, 
        SumiAttachment* gWorldPos
    ) {
        assert(zbuffer != nullptr
            && "Cannot update descriptor set with null zbuffer");
        assert(gWorldPos != nullptr
            && "Cannot update descriptor set with null world pos attachment");
        assert(tileLightListEarlyBuffer != nullptr
            && "Cannot update descriptor set with null tile light list early buffer");
        assert(tileLightCountEarlyBuffer != nullptr
            && "Cannot update descriptor set with null tile light count early buffer");

        VkDescriptorImageInfo zbufferInfo{};
        zbufferInfo.sampler     = attachmentSampler;
        zbufferInfo.imageView   = zbuffer->getImageView();
        zbufferInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

        VkDescriptorImageInfo gWorldPosInfo{};
        gWorldPosInfo.sampler     = attachmentSampler;
        gWorldPosInfo.imageView   = gWorldPos->getImageView();
        gWorldPosInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkDescriptorBufferInfo tileGroupLightMaskInfo = tileGroupLightMaskBuffer->descriptorInfo();
        VkDescriptorBufferInfo tileShadowSlotIDsInfo  = tileShadowSlotIDsBuffer->descriptorInfo();
        VkDescriptorBufferInfo lightListEarlyInfo     = tileLightListEarlyBuffer->descriptorInfo();
        VkDescriptorBufferInfo lightCountEarlyInfo    = tileLightCountEarlyBuffer->descriptorInfo();

        SumiDescriptorWriter(*lightsAccurateDescriptorLayout, *descriptorPool)
            .writeImage(0, &zbufferInfo)
            .writeImage(1, &gWorldPosInfo)
            .writeBuffer(2, &tileGroupLightMaskInfo)
            .writeBuffer(3, &tileShadowSlotIDsInfo)
            .writeBuffer(4, &lightListEarlyInfo)
            .writeBuffer(5, &lightCountEarlyInfo)
            .overwrite(lightsAccurateDescriptorSet);
    }

    void HighQualityShadowMapper::initLightsAccuratePipeline() {

        VkPushConstantRange pushRange{};
        pushRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        pushRange.offset     = 0;
        pushRange.size       = sizeof(structs::findLightsAccuratePush);

        std::vector<VkDescriptorSetLayout> descriptorSetLayouts{
            lightsAccurateDescriptorLayout->getDescriptorSetLayout()
        };

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount         = static_cast<uint32_t>(descriptorSetLayouts.size());
        pipelineLayoutInfo.pSetLayouts            = descriptorSetLayouts.data();
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges    = &pushRange;

        VK_CHECK_SUCCESS(
            vkCreatePipelineLayout(
                sumiDevice.device(), &pipelineLayoutInfo, nullptr, &findLightsAccuratePipelineLayout),
            "[Sumire::HighQualityShadowMapper] Failed to create lights accurate pipeline layout (Phase 3)."
        );

        findLightsAccuratePipeline = std::make_unique<SumiComputePipeline>(
            sumiDevice,
            SUMIRE_ENGINE_PATH(
                "shaders/high_quality_shadow_mapping/find_lights_accurate.comp"),
            findLightsAccuratePipelineLayout
        );
    }

    void HighQualityShadowMapper::cleanupLightsAccuratePhase() {
        vkDestroyPipelineLayout(sumiDevice.device(), findLightsAccuratePipelineLayout, nullptr);
    }

    // ---- Phase 4: Generate Deferred Shadows -------------------------------------------------------------------
    void HighQualityShadowMapper::initDeferredShadowsPhase(
        SumiAttachment* zbuffer, SumiAttachment* gWorldPos,
        VkDescriptorSetLayout globalDescriptorSetLayout
    ) {
        createTileLightListFinalBuffer();
        createTileLightCountFinalBuffer();
        createTileLightVisibilityBuffer();
        initDeferredShadowsDescriptorSet(zbuffer, gWorldPos);
        initDeferredShadowsPipeline(globalDescriptorSetLayout);
    }

    void HighQualityShadowMapper::createTileLightListFinalBuffer() {
        // 1 List per shadow tile
        // TODO: What is the list size here?
        tileLightListFinalBuffer = std::make_unique<SumiBuffer>(
            sumiDevice,
            numShadowTiles * sizeof(uint32_t),
            1,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        );
    }

    void HighQualityShadowMapper::createTileLightCountFinalBuffer() {
        // 1 entry per shadow tile
        tileLightCountFinalBuffer = std::make_unique<SumiBuffer>(
            sumiDevice,
            numShadowTiles * sizeof(uint32_t),
            1,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        );
    }

    void HighQualityShadowMapper::createTileLightVisibilityBuffer() {
        // 8 bits per pixel
        uint32_t numPixels = screenWidth * screenHeight;
        tileLightVisibilityBuffer = std::make_unique<SumiBuffer>(
            sumiDevice,
            numPixels * sizeof(uint8_t),
            1,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        );
    }

    void HighQualityShadowMapper::initDeferredShadowsDescriptorSet(
        SumiAttachment* zbuffer,
        SumiAttachment* gWorldPos
    ) {
        assert(zbuffer != nullptr
            && "Cannot instantiate descriptor set with null zbuffer");
        assert(gWorldPos != nullptr
            && "Cannot instantiate descriptor set with null world pos attachment");
        assert(tileLightListEarlyBuffer != nullptr
            && "Cannot instantiate descriptor set with null tile light list early buffer");
        assert(tileLightCountEarlyBuffer != nullptr
            && "Cannot instantiate descriptor set with null tile light count early buffer");
        assert(tileLightListFinalBuffer != nullptr
            && "Cannot instantiate descriptor set with null tile light list final buffer");
        assert(tileLightCountFinalBuffer != nullptr
            && "Cannot instantiate descriptor set with null tile light count final buffer");

        VkDescriptorImageInfo zbufferInfo{};
        zbufferInfo.sampler     = attachmentSampler;
        zbufferInfo.imageView   = zbuffer->getImageView();
        zbufferInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

        VkDescriptorImageInfo gWorldPosInfo{};
        gWorldPosInfo.sampler     = attachmentSampler;
        gWorldPosInfo.imageView   = gWorldPos->getImageView();
        gWorldPosInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkDescriptorBufferInfo tileShadowSlotIDsInfo = tileShadowSlotIDsBuffer->descriptorInfo();
        VkDescriptorBufferInfo lightListEarlyInfo    = tileLightListEarlyBuffer->descriptorInfo();
        VkDescriptorBufferInfo lightCountEarlyInfo   = tileLightCountEarlyBuffer->descriptorInfo();
        VkDescriptorBufferInfo lightListFinalInfo    = tileLightListFinalBuffer->descriptorInfo();
        VkDescriptorBufferInfo lightCountFinalInfo   = tileLightCountFinalBuffer->descriptorInfo();
        VkDescriptorBufferInfo lightVisibilityInfo   = tileLightVisibilityBuffer->descriptorInfo();

        SumiDescriptorWriter(*deferredShadowsDescriptorLayout, *descriptorPool)
            .writeImage(0, &zbufferInfo)
            .writeImage(1, &gWorldPosInfo)
            .writeBuffer(2, &tileShadowSlotIDsInfo)
            .writeBuffer(3, &lightListEarlyInfo)
            .writeBuffer(4, &lightCountEarlyInfo)
            .writeBuffer(5, &lightListFinalInfo)
            .writeBuffer(6, &lightCountFinalInfo)
            .writeBuffer(7, &lightVisibilityInfo)
            .build(deferredShadowsDescriptorSet);
    }

    void HighQualityShadowMapper::updateDeferredShadowsDescriptorSet(
        SumiAttachment* zbuffer,
        SumiAttachment* gWorldPos
    ) {
        assert(zbuffer != nullptr
            && "Cannot update descriptor set with null zbuffer");
        assert(gWorldPos != nullptr
            && "Cannot update descriptor set with null world pos attachment");
        assert(tileLightListEarlyBuffer != nullptr
            && "Cannot update descriptor set with null tile light list early buffer");
        assert(tileLightCountEarlyBuffer != nullptr
            && "Cannot update descriptor set with null tile light count early buffer");
        assert(tileLightListFinalBuffer != nullptr
            && "Cannot update descriptor set with null tile light list final buffer");
        assert(tileLightCountFinalBuffer != nullptr
            && "Cannot update descriptor set with null tile light count final buffer");

        VkDescriptorImageInfo zbufferInfo{};
        zbufferInfo.sampler = attachmentSampler;
        zbufferInfo.imageView = zbuffer->getImageView();
        zbufferInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

        VkDescriptorImageInfo gWorldPosInfo{};
        gWorldPosInfo.sampler = attachmentSampler;
        gWorldPosInfo.imageView = gWorldPos->getImageView();
        gWorldPosInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkDescriptorBufferInfo tileShadowSlotIDsInfo = tileShadowSlotIDsBuffer->descriptorInfo();
        VkDescriptorBufferInfo lightListEarlyInfo = tileLightListEarlyBuffer->descriptorInfo();
        VkDescriptorBufferInfo lightCountEarlyInfo = tileLightCountEarlyBuffer->descriptorInfo();
        VkDescriptorBufferInfo lightListFinalInfo = tileLightListFinalBuffer->descriptorInfo();
        VkDescriptorBufferInfo lightCountFinalInfo = tileLightCountFinalBuffer->descriptorInfo();
        VkDescriptorBufferInfo lightVisibilityInfo = tileLightVisibilityBuffer->descriptorInfo();

        SumiDescriptorWriter(*deferredShadowsDescriptorLayout, *descriptorPool)
            .writeImage(0, &zbufferInfo)
            .writeImage(1, &gWorldPosInfo)
            .writeBuffer(2, &tileShadowSlotIDsInfo)
            .writeBuffer(3, &lightListEarlyInfo)
            .writeBuffer(4, &lightCountEarlyInfo)
            .writeBuffer(5, &lightListFinalInfo)
            .writeBuffer(6, &lightCountFinalInfo)
            .writeBuffer(7, &lightVisibilityInfo)
            .overwrite(deferredShadowsDescriptorSet);
    }

    void HighQualityShadowMapper::initDeferredShadowsPipeline(
        VkDescriptorSetLayout globalDescriptorSetLayout
    ) {
        VkPushConstantRange pushRange{};
        pushRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        pushRange.offset = 0;
        pushRange.size = sizeof(structs::genDeferredShadowsPush);

        std::vector<VkDescriptorSetLayout> descriptorSetLayouts{
            globalDescriptorSetLayout,
            deferredShadowsDescriptorLayout->getDescriptorSetLayout()
        };

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
        pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushRange;

        VK_CHECK_SUCCESS(
            vkCreatePipelineLayout(
                sumiDevice.device(), &pipelineLayoutInfo, nullptr, &genDeferredShadowsPipelineLayout),
            "[Sumire::HighQualityShadowMapper] Failed to create lights accurate pipeline layout (Phase 4)."
        );

        genDeferredShadowsPipeline = std::make_unique<SumiComputePipeline>(
            sumiDevice,
            SUMIRE_ENGINE_PATH(
                "shaders/high_quality_shadow_mapping/generate_deferred_shadows.comp"),
            genDeferredShadowsPipelineLayout
        );
    }

    void HighQualityShadowMapper::cleanupDeferredShadowsPhase() {
        vkDestroyPipelineLayout(sumiDevice.device(), genDeferredShadowsPipelineLayout, nullptr);
    }
    
    // -----------------------------------------------------------------------------------------------------------
    void HighQualityShadowMapper::calculateTileResolutions() {
        numShadowTilesX = glm::ceil(static_cast<float>(screenWidth) / 8.0f);
        numShadowTilesY = glm::ceil(static_cast<float>(screenHeight) / 8.0f);
        numShadowTiles = numShadowTilesX * numShadowTilesY;

        numTileGroupsX = glm::ceil(static_cast<float>(screenWidth) / 64.0f);
        numTileGroupsY = glm::ceil(static_cast<float>(screenHeight) / 64.0f);
        numTileGroups = numTileGroupsX * numTileGroupsY;
    }
}
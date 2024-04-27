#include <sumire/core/render_systems/point_light_rendersys.hpp>
#include <sumire/core/render_systems/data_structs/point_light_rendersys_structs.hpp>

#include <sumire/util/sumire_engine_path.hpp>
#include <sumire/util/vk_check_success.hpp>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <stdexcept>
#include <array>
#include <cassert>

namespace sumire {

    PointLightRenderSys::PointLightRenderSys(
        SumiDevice& device,
        VkRenderPass renderPass,
        uint32_t subpassIdx,
        VkDescriptorSetLayout globalDescriptorSetLayout
    ) : sumiDevice{device} {
            
        createPipelineLayout(globalDescriptorSetLayout);
        createPipeline(renderPass, subpassIdx);
    }

    PointLightRenderSys::~PointLightRenderSys() {
        // TODO: Destroy pipeline at this line??
        vkDestroyPipelineLayout(sumiDevice.device(), pipelineLayout, nullptr);
    }

    void PointLightRenderSys::createPipelineLayout(VkDescriptorSetLayout globalDescriptorSetLayout) {

        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(structs::PointLightPushConstantData);
        
        std::vector<VkDescriptorSetLayout> descriptorSetLayouts{globalDescriptorSetLayout};

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
        pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

        VK_CHECK_SUCCESS(
            vkCreatePipelineLayout(
                sumiDevice.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout),
            "[Sumire::PointLightRenderSys] Failed to create point light rendering pipeline layout."
        );
    }

    void PointLightRenderSys::createPipeline(VkRenderPass renderPass, uint32_t subpassIdx) {
        assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout.");

        PipelineConfigInfo pipelineConfig{};
        SumiPipeline::defaultPipelineConfigInfo(pipelineConfig);
        pipelineConfig.rasterizationInfo.cullMode = VK_CULL_MODE_NONE; // no culling
        pipelineConfig.attributeDescriptions.clear(); // do not use vertex attributes or bindings
        pipelineConfig.bindingDescriptions.clear();
        pipelineConfig.renderPass = renderPass;
        pipelineConfig.subpass = subpassIdx;
        pipelineConfig.pipelineLayout = pipelineLayout;
        sumiPipeline = std::make_unique<SumiPipeline>(
            sumiDevice,
            SUMIRE_ENGINE_PATH("shaders/world_ui/point_light.vert.spv"),
            SUMIRE_ENGINE_PATH("shaders/world_ui/point_light.frag.spv"),
            pipelineConfig);
    }

    void PointLightRenderSys::render(VkCommandBuffer commandBuffer, FrameInfo &frameInfo) {
        sumiPipeline->bind(commandBuffer);

        vkCmdBindDescriptorSets(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipelineLayout,
            0, 1,
            &frameInfo.globalDescriptorSet,
            0, nullptr
        );

        uint32_t nLights = static_cast<uint32_t>(frameInfo.lights.size());

        vkCmdDraw(commandBuffer, 6, nLights, 0, 0);

    }
}
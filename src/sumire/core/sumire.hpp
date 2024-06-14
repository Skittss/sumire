#pragma once

#include <sumire/config/sumi_config.hpp>

#include <sumire/core/windowing/sumi_window.hpp>
#include <sumire/core/graphics_pipeline/sumi_device.hpp>
#include <sumire/core/graphics_pipeline/sumi_descriptors.hpp>
#include <sumire/core/models/sumi_model.hpp>
#include <sumire/core/rendering/general/sumi_object.hpp>
#include <sumire/core/rendering/lighting/sumi_light.hpp>
#include <sumire/core/rendering/sumi_renderer.hpp>

// Render Systems
#include <sumire/core/render_systems/forward/mesh_rendersys.hpp>
#include <sumire/core/render_systems/deferred/deferred_mesh_rendersys.hpp>
#include <sumire/core/render_systems/depth_buffers/hzb_generator.hpp>
#include <sumire/core/render_systems/high_quality_shadow_mapping/high_quality_shadow_mapper.hpp>
#include <sumire/core/render_systems/post/post_processor.hpp>
#include <sumire/core/render_systems/world_ui/point_light_rendersys.hpp>
#include <sumire/core/render_systems/world_ui/grid_rendersys.hpp>

// Debug Render Systems
#include <sumire/core/render_systems/high_quality_shadow_mapping/hqsm_debugger.hpp>

#include <memory>
#include <vector>

namespace sumire {

    class Sumire {
    public:
        Sumire();
        ~Sumire() = default;

        Sumire(const Sumire&) = delete;
        Sumire& operator=(const Sumire&) = delete;

        uint32_t screenWidth = 0;
        uint32_t screenHeight = 0;

        void init();
        void run();

    private:
        void initBuffers();
        void initDescriptors();
        void initRenderSystems();
        void initDebugRenderSystems();
        void loadObjects();
        void loadLights(); 

        SumiConfig sumiConfig{};
        SumiWindow sumiWindow{ 
            static_cast<int>(sumiConfig.startupData.RESOLUTION.WIDTH),
            static_cast<int>(sumiConfig.startupData.RESOLUTION.HEIGHT),
            "Sumire" 
        };
        SumiDevice sumiDevice{ sumiWindow, &sumiConfig };
        SumiRenderer sumiRenderer{ sumiWindow, sumiDevice, sumiConfig };
        
        // Render Systems
        std::unique_ptr<MeshRenderSys>           meshRenderSystem;
        std::unique_ptr<DeferredMeshRenderSys>   deferredMeshRenderSystem;
        std::unique_ptr<HzbGenerator>            hzbGenerator;
        std::unique_ptr<HighQualityShadowMapper> shadowMapper;
        std::unique_ptr<PostProcessor>           postProcessor;
        std::unique_ptr<PointLightRenderSys>     pointLightSystem;
        std::unique_ptr<GridRendersys>           gridRenderSystem;

        // Debug Render Systems
        std::unique_ptr<HQSMdebugger>            hqsmDebugger;

        // Global descriptors and buffers
        std::unique_ptr<SumiDescriptorPool>      globalDescriptorPool;
        std::unique_ptr<SumiDescriptorSetLayout> globalDescriptorSetLayout;
        std::vector<VkDescriptorSet>             globalDescriptorSets{};

        std::vector<std::unique_ptr<SumiBuffer>> globalUniformBuffers;
        std::vector<std::unique_ptr<SumiBuffer>> cameraUniformBuffers;
        std::vector<std::unique_ptr<SumiBuffer>> lightSSBOs;

        SumiObject::Map objects;
        SumiLight::Map lights;
    };
}
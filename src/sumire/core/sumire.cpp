#include <sumire/core/sumire.hpp>
#include <sumire/core/graphics_pipeline/sumi_buffer.hpp>
#include <sumire/util/sumire_engine_path.hpp>

// Asset loaders
#include <sumire/loaders/gltf_loader.hpp>
#include <sumire/loaders/obj_loader.hpp>

// input
#include <sumire/input/sumi_kbm_controller.hpp>

// GUI layer
#include <sumire/gui/sumi_imgui.hpp>

// Profiling
#include <sumire/core/profiling/gpu_profiler.hpp>
#define BEGIN_GPU_PROFILING_BLOCK(profiler, cmdBuffer, blockName)    \
   if ((profiler)) (profiler)->beginBlock((cmdBuffer), (blockName)); \

#define END_GPU_PROFILING_BLOCK(profiler, cmdBuffer, blockName)    \
   if ((profiler)) (profiler)->endBlock((cmdBuffer), (blockName)); \

#include <sumire/core/profiling/cpu_profiler.hpp>
#define BEGIN_CPU_PROFILING_BLOCK(profiler, blockName)  \
   if ((profiler)) (profiler)->beginBlock((blockName)); \

#define END_CPU_PROFILING_BLOCK(profiler, blockName)  \
   if ((profiler)) (profiler)->endBlock((blockName)); \

// glmw
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/color_space.hpp>

#include <stdexcept>
#include <array>
#include <cassert>
#include <chrono>
#include <iostream>

namespace sumire {

    struct GlobalUBO {
        int nLights = 0;
    };

    struct CameraUBO {
        glm::mat4 projectionMatrix{ 1.0f };
        glm::mat4 viewMatrix{ 1.0f };
        glm::mat4 projectionViewMatrix{ 1.0f };
        glm::vec3 cameraPosition{ 0.0f };
    };

    Sumire::Sumire() {
        screenWidth = sumiConfig.runtimeData.graphics.user.RESOLUTION.WIDTH;
        screenHeight = sumiConfig.runtimeData.graphics.user.RESOLUTION.HEIGHT;

        init();

        loadObjects();
        loadLights();
    }

    void Sumire::init() {
        initBuffers();
        initDescriptors();
        initRenderSystems();

        if (sumiConfig.runtimeData.shaders.DEBUG_SHADERS) {
            initDebugRenderSystems();
        }
    }

    void Sumire::initBuffers() {
        // ---- Global Descriptor Set ----------------------------------------------------------------------------
        // Uniform Buffers
        globalUniformBuffers = std::vector<std::unique_ptr<SumiBuffer>>(SumiSwapChain::MAX_FRAMES_IN_FLIGHT);
        cameraUniformBuffers = std::vector<std::unique_ptr<SumiBuffer>>(SumiSwapChain::MAX_FRAMES_IN_FLIGHT);
        for (int i = 0; i < SumiSwapChain::MAX_FRAMES_IN_FLIGHT; i++) {

            // Global uniforms
            globalUniformBuffers[i] = std::make_unique<SumiBuffer>(
                sumiDevice,
                sizeof(GlobalUBO),
                1,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
            );
            globalUniformBuffers[i]->map(); //enable writing to buffer memory

            // Camera uniforms
            cameraUniformBuffers[i] = std::make_unique<SumiBuffer>(
                sumiDevice,
                sizeof(CameraUBO),
                1,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
            );
            cameraUniformBuffers[i]->map(); //enable writing to buffer memory
        }

        // ---- Light SSBOs --------------------------------------------------------------------------------------
        lightSSBOs = std::vector<std::unique_ptr<SumiBuffer>>(SumiSwapChain::MAX_FRAMES_IN_FLIGHT);
        for (int i = 0; i < SumiSwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
            lightSSBOs[i] = std::make_unique<SumiBuffer>(
                sumiDevice,
                sumiConfig.runtimeData.graphics.internal.MAX_N_LIGHTS * sizeof(SumiLight::LightShaderData),
                1,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
            );
            lightSSBOs[i]->map();
        }
    }

    void Sumire::initDescriptors() {
        globalDescriptorPool = SumiDescriptorPool::Builder(sumiDevice)
            .setMaxSets((3 * SumiSwapChain::MAX_FRAMES_IN_FLIGHT))
            .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, SumiSwapChain::MAX_FRAMES_IN_FLIGHT) // global
            .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, SumiSwapChain::MAX_FRAMES_IN_FLIGHT) // camera
            .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, SumiSwapChain::MAX_FRAMES_IN_FLIGHT) // Light SSBO
            .build();

        globalDescriptorSetLayout = SumiDescriptorSetLayout::Builder(sumiDevice)
            .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS | VK_SHADER_STAGE_COMPUTE_BIT)
            .addBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS | VK_SHADER_STAGE_COMPUTE_BIT)
            .addBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS | VK_SHADER_STAGE_COMPUTE_BIT)
            .build();

        // Write Descriptor Sets
        globalDescriptorSets = std::vector<VkDescriptorSet>(SumiSwapChain::MAX_FRAMES_IN_FLIGHT);
        for (int i = 0; i < globalDescriptorSets.size(); i++) {
            auto globalBufferInfo = globalUniformBuffers[i]->descriptorInfo();
            auto cameraBufferInfo = cameraUniformBuffers[i]->descriptorInfo();
            auto lightSSBOinfo = lightSSBOs[i]->descriptorInfo();
            SumiDescriptorWriter(*globalDescriptorSetLayout, *globalDescriptorPool)
                .writeBuffer(0, &globalBufferInfo)
                .writeBuffer(1, &cameraBufferInfo)
                .writeBuffer(2, &lightSSBOinfo)
                .build(globalDescriptorSets[i]);
        }
    }

    void Sumire::initRenderSystems() {

        //meshRenderSystem = std::make_unique<MeshRenderSys>(
        //    sumiDevice,
        //    sumiRenderer.getLateGraphicsRenderPass(),
        //    sumiRenderer.forwardRenderSubpassIdx(),
        //    globalDescriptorSetLayout->getDescriptorSetLayout()
        //);

        deferredMeshRenderSystem = std::make_unique<DeferredMeshRenderSys>(
            sumiDevice,
            sumiRenderer.getGbuffer(),
            sumiRenderer.getEarlyGraphicsRenderPass(),
            sumiRenderer.gbufferFillSubpassIdx(),
            sumiRenderer.getLateGraphicsRenderPass(),
            sumiRenderer.gbufferResolveSubpassIdx(),
            globalDescriptorSetLayout->getDescriptorSetLayout()
        );

        hzbGenerator = std::make_unique<HzbGenerator>(
            sumiDevice,
            sumiRenderer.getSwapChain()->getDepthAttachment(),
            sumiRenderer.getHZB()
        );

        shadowMapper = std::make_unique<HighQualityShadowMapper>(
            sumiDevice,
            screenWidth, screenHeight,
            sumiRenderer.getHZB(),
            sumiRenderer.getSwapChain()->getDepthAttachment(),
            sumiRenderer.getGbuffer()->positionAttachment(),
            globalDescriptorSetLayout->getDescriptorSetLayout()
        );

        postProcessor = std::make_unique<PostProcessor>(
            sumiDevice,
            sumiRenderer.getIntermediateColorAttachments(),
            sumiRenderer.getCompositionRenderPass()
        );

        pointLightSystem = std::make_unique<PointLightRenderSys>(
            sumiDevice,
            sumiRenderer.getLateGraphicsRenderPass(),
            sumiRenderer.forwardRenderSubpassIdx(),
            globalDescriptorSetLayout->getDescriptorSetLayout()
        );

        gridRenderSystem = std::make_unique<GridRendersys>(
            sumiDevice,
            sumiRenderer.getLateGraphicsRenderPass(),
            sumiRenderer.forwardRenderSubpassIdx(),
            globalDescriptorSetLayout->getDescriptorSetLayout()
        );
    }

    void Sumire::initDebugRenderSystems() {
        hqsmDebugger = std::make_unique<HQSMdebugger>(
            sumiDevice,
            shadowMapper.get(),
            sumiRenderer.getHZB(),
            sumiRenderer.getCompositionRenderPass()
        );
    }

    void Sumire::run() {

        sumiWindow.setMousePollMode(SumiWindow::MousePollMode::MANUAL);

        // Camera Control
        SumiCamera camera{ glm::radians(50.0f), sumiRenderer.getAspect() };
        camera.transform.setTranslation(glm::vec3{ 0.0f, 1.0f, 3.0f });
        SumiKBMcontroller cameraController{
            sumiConfig,
            sumiWindow,
            SumiKBMcontroller::ControllerType::FPS
        };

        // GUI
        SumiImgui gui = SumiImgui{
            sumiDevice,
            sumiConfig,
            sumiRenderer,
            sumiRenderer.getCompositionRenderPass(),
            sumiRenderer.compositionSubpassIdx(),
            sumiDevice.presentQueue()
        };

        // Profiling, if enabled
        std::unique_ptr<GpuProfiler> gpuProfiler = nullptr;
        if (sumiConfig.startupData.profiling.GPU_PROFILING) {
            if (!sumiDevice.properties.limits.timestampComputeAndGraphics) {
                std::cout << "[Sumire::Sumire] WARNING: Physical device does not support profiling." << std::endl;
            }
            else {
                // Note: any blocks we add here *must be written* with beginBlock() & endBlock() 
                //       or else we will never have query results available.
                gpuProfiler = GpuProfiler::Builder(sumiDevice)
                    .addBlock("0-- Predraw Compute")
                    .addBlock("1-- Early Graphics")
                    .addBlock("2-- Early Compute")
                    .addBlock("2-0: HZB building")
                    .addBlock("2-1: Find Lights Approx")
                    .addBlock("2-2: Find Lights Accurate")
                    .addBlock("2-3: Generate Deferred Shadows")
                    .addBlock("3-- Late Graphics")
                    .addBlock("4-- Late Compute")
                    .addBlock("5-- Composite")
                    .build();
            }
        }

        std::unique_ptr<CpuProfiler> cpuProfiler = nullptr;
        if (sumiConfig.startupData.profiling.CPU_PROFILING) {
            cpuProfiler = CpuProfiler::Builder()
                .addBlock("0: Shadow Map Prepare")
                .build();
        }

        auto currentTime = std::chrono::high_resolution_clock::now();
        float cumulativeFrameTime = 0.0f;

        // Poll Mouse pos once directly before starting loop to prevent an incorrect
        //  Mouse update on start.
        sumiWindow.pollMousePos();

        // Draw loop
        while (!sumiWindow.shouldClose()) {
            sumiWindow.pollMousePos(); // if manual polling mouse pos
            // sumiWindow.clearMouseDelta(); // if using event-based polling
            sumiWindow.clearKeypressEvents();
            SumiWindow::pollEvents();

            auto newTime = std::chrono::high_resolution_clock::now();
            float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
            currentTime = newTime;

            const float MAX_FRAME_TIME = 0.2f; // 5fps
            frameTime = glm::min(frameTime, MAX_FRAME_TIME);

            cumulativeFrameTime += frameTime;

            // Handle input.
            ImGuiIO& guiIO = gui.getIO();

            if (sumiWindow.isCursorHidden()) gui.ignoreMouse();
            else gui.enableMouse();

            if (!(guiIO.WantCaptureKeyboard && guiIO.WantTextInput)) {

                glm::vec2 mouseDelta = (!sumiWindow.isCursorHidden() && guiIO.WantCaptureMouse)
                    ? glm::tvec2<double>{0.0f}
                    : sumiWindow.mouseDelta;

                cameraController.move(
                    frameTime, 
                    sumiWindow.keypressEvents,
                    mouseDelta,
                    camera.getOrthonormalBasis(),
                    camera.transform
                );
            }

            camera.setViewYXZ(camera.transform.getTranslation(), camera.transform.getRotation());

            // TODO: Move these to separate func
            if (sumiRenderer.wasSwapChainRecreated()) {
                // Update main application viewport size tracking vars
                VkExtent2D newExtent = sumiRenderer.getWindow().getExtent();
                screenWidth = newExtent.width;
                screenHeight = newExtent.height;

                float aspect = sumiRenderer.getAspect();
                camera.setAspect(aspect, true);

                postProcessor->updateDescriptors(
                    sumiRenderer.getIntermediateColorAttachments()
                );
                hzbGenerator->updateDescriptors(
                    sumiRenderer.getSwapChain()->getDepthAttachment(), sumiRenderer.getHZB()
                );
                shadowMapper->updateScreenBounds(
                    screenWidth, screenHeight,
                    sumiRenderer.getHZB(),
                    sumiRenderer.getSwapChain()->getDepthAttachment(),
                    sumiRenderer.getGbuffer()->positionAttachment()
                );
                if (hqsmDebugger) hqsmDebugger->updateScreenBounds(sumiRenderer.getHZB());

                sumiRenderer.resetScRecreatedFlag();
            }

            if (sumiRenderer.wasGbufferRecreated()) {
                deferredMeshRenderSystem->updateResolveDescriptors(sumiRenderer.getGbuffer());
                sumiRenderer.resetGbufferRecreatedFlag();
            }

            // Set up FrameInfo for GUI, and add remaining props later;
            FrameInfo frameInfo{
                -1,        // frameIdx
                frameTime,
                cumulativeFrameTime,
                camera,
                nullptr,   // globalDescriptorSet
                objects,
                lights
            };

            SumiRenderer::FrameCommandBuffers frameCommandBuffers = sumiRenderer.beginFrame();

            // If all cmd buffers are null, then the frame was not started.
            if (frameCommandBuffers.validFrame()) {

                int frameIdx = sumiRenderer.getFrameIdx();
                // Reset bound pipeline caches
                SumiComputePipeline::resetBoundPipelineCache();
                SumiPipeline::resetBoundPipelineCache();

                // Fill in rest of frame-specific frameinfo props
                frameInfo.frameIdx = frameIdx;
                //frameInfo.commandBuffer = frameCommandBuffers.graphics;
                frameInfo.globalDescriptorSet = globalDescriptorSets[frameIdx];

                // Populate uniform buffers with data
                CameraUBO cameraUbo{};
                cameraUbo.projectionMatrix     = camera.getProjectionMatrix();
                cameraUbo.viewMatrix           = camera.getViewMatrix();
                cameraUbo.projectionViewMatrix = cameraUbo.projectionMatrix * cameraUbo.viewMatrix;
                cameraUbo.cameraPosition = camera.transform.getTranslation();
                cameraUniformBuffers[frameIdx]->writeToBuffer(&cameraUbo);
                cameraUniformBuffers[frameIdx]->flush();

                const int nLights = static_cast<int>(lights.size());
                GlobalUBO globalUbo{};
                globalUbo.nLights = nLights;
                globalUniformBuffers[frameIdx]->writeToBuffer(&globalUbo);
                globalUniformBuffers[frameIdx]->flush();

                // Prepare Lights
                //   Sort lights by view space depth for shadow mapping pass
                auto sortedLights = HighQualityShadowMapper::sortLightsByViewSpaceDepth(
                    lights, 
                    cameraUbo.viewMatrix, 
                    camera.getNear()
                );

                //   Write lights SSBO
                //   TODO: This will be very slow when the number of lights increases.
                //          We should only write to the buffer when absolutely necessary, i.e. on light change.
                //   TODO: This also needs ring buffering as the sort will make in progress frames flicker.
                auto lightData = std::vector<SumiLight::LightShaderData>{};
                for (auto& viewSpaceLight : sortedLights) {
                    lightData.push_back(viewSpaceLight.lightPtr->getShaderData());
                }
                lightSSBOs[frameIdx]->writeToBuffer(lightData.data(), nLights * sizeof(SumiLight::LightShaderData));
                lightSSBOs[frameIdx]->flush();

                // ---- Shadow mapping preparation on the CPU ----------------------------------------------------
                //  TODO: Only re-prepare if lights / camera view have changed.
                BEGIN_CPU_PROFILING_BLOCK(cpuProfiler, "0: Shadow Map Prepare");
                shadowMapper->prepare(
                    sortedLights,
                    camera
                );
                END_CPU_PROFILING_BLOCK(cpuProfiler, "0: Shadow Map Prepare");
                
                if (gpuProfiler) gpuProfiler->beginFrame(frameCommandBuffers.predrawCompute);

                // ---- Pre-draw compute dispatches --------------------------------------------------------------
                // TODO: Compute based culling and skinning.
                BEGIN_GPU_PROFILING_BLOCK(gpuProfiler, frameCommandBuffers.predrawCompute, "0-- Predraw Compute");

                END_GPU_PROFILING_BLOCK(gpuProfiler, frameCommandBuffers.predrawCompute, "0-- Predraw Compute");

                // ---- Early Graphics ---------------------------------------------------------------------------
                // Fill gbuffer in place of a z-prepass

                BEGIN_GPU_PROFILING_BLOCK(gpuProfiler, frameCommandBuffers.earlyGraphics, "1-- Early Graphics");
                sumiRenderer.beginEarlyGraphicsRenderPass(frameCommandBuffers.earlyGraphics);

                deferredMeshRenderSystem->fillGbuffer(frameCommandBuffers.earlyGraphics, frameInfo);

                sumiRenderer.endEarlyGraphicsRenderPass(frameCommandBuffers.earlyGraphics);
                END_GPU_PROFILING_BLOCK(gpuProfiler, frameCommandBuffers.earlyGraphics, "1-- Early Graphics");

                // ---- Early Compute ----------------------------------------------------------------------------
                // Shadow mapping resolve which gbuffer resolve relies on

                BEGIN_GPU_PROFILING_BLOCK(gpuProfiler, frameCommandBuffers.earlyCompute, "2-- Early Compute");

                sumiRenderer.beginEarlyCompute(frameCommandBuffers.earlyCompute);

                //   Generate HZB
                BEGIN_GPU_PROFILING_BLOCK(gpuProfiler, frameCommandBuffers.earlyCompute, "2-0: HZB building");
                hzbGenerator->generateShadowTileHzb(frameCommandBuffers.earlyCompute);
                END_GPU_PROFILING_BLOCK(gpuProfiler, frameCommandBuffers.earlyCompute, "2-0: HZB building");

                //   Transition HZB for combined image sampler use
                sumiDevice.imageMemoryBarrier(
                    sumiRenderer.getHZB()->getImage(),
                    VK_IMAGE_LAYOUT_GENERAL,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    0,
                    0,
                    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                    sumiRenderer.getHZB()->getBaseImageViewCreateInfo().subresourceRange,
                    VK_QUEUE_FAMILY_IGNORED,
                    VK_QUEUE_FAMILY_IGNORED,
                    frameCommandBuffers.earlyCompute
                );

                //   Shadow mapping
                BEGIN_GPU_PROFILING_BLOCK(gpuProfiler, frameCommandBuffers.earlyCompute, "2-1: Find Lights Approx");
                shadowMapper->findLightsApproximate(
                    frameCommandBuffers.earlyCompute,
                    camera.getNear(), camera.getFar()
                );
                END_GPU_PROFILING_BLOCK(gpuProfiler, frameCommandBuffers.earlyCompute, "2-1: Find Lights Approx");

                BEGIN_GPU_PROFILING_BLOCK(gpuProfiler, frameCommandBuffers.earlyCompute, "2-2: Find Lights Accurate");
                shadowMapper->findLightsAccurate(frameCommandBuffers.earlyCompute);
                END_GPU_PROFILING_BLOCK(gpuProfiler, frameCommandBuffers.earlyCompute, "2-2: Find Lights Accurate");

                BEGIN_GPU_PROFILING_BLOCK(gpuProfiler, frameCommandBuffers.earlyCompute, "2-3: Generate Deferred Shadows");
                shadowMapper->generateDeferredShadows(frameCommandBuffers.earlyCompute, frameInfo);
                END_GPU_PROFILING_BLOCK(gpuProfiler, frameCommandBuffers.earlyCompute, "2-3: Generate Deferred Shadows");

                //shadowMapper.compositeHighQualityShadows(frameCommandBuffers.earlyCompute);

                sumiRenderer.endEarlyCompute(frameCommandBuffers.earlyCompute);

                END_GPU_PROFILING_BLOCK(gpuProfiler, frameCommandBuffers.earlyCompute, "2-- Early Compute");

                // ---- Late Graphics ----------------------------------------------------------------------------
                // Lighting resolve and forward subpass for world UI + OIT
                BEGIN_GPU_PROFILING_BLOCK(gpuProfiler, frameCommandBuffers.lateGraphics, "3-- Late Graphics");
                sumiRenderer.beginLateGraphicsRenderPass(frameCommandBuffers.lateGraphics);

                //   Deferred resolve subpass
                deferredMeshRenderSystem->resolveGbuffer(frameCommandBuffers.lateGraphics, frameInfo);

                //   Forward rendering subpass
                sumiRenderer.nextLateGraphicsSubpass(frameCommandBuffers.lateGraphics);

                pointLightSystem->render(frameCommandBuffers.lateGraphics, frameInfo);
                
                if (gui.showGrid && gui.gridOpacity > 0.0f) {
                    auto gridUbo = gui.getGridUboData();
                    gridRenderSystem->render(frameCommandBuffers.lateGraphics, frameInfo, gridUbo);
                }
                
                sumiRenderer.endLateGraphicsRenderPass(frameCommandBuffers.lateGraphics);
                END_GPU_PROFILING_BLOCK(gpuProfiler, frameCommandBuffers.lateGraphics, "3-- Late Graphics");

                // ---- Late Compute -----------------------------------------------------------------------------
                // Post effects which can be interleaved with generation of next frame via async compute queue
                BEGIN_GPU_PROFILING_BLOCK(gpuProfiler, frameCommandBuffers.lateCompute, "4-- Late Compute");

                postProcessor->tonemap(frameCommandBuffers.lateCompute, frameInfo.frameIdx);

                sumiRenderer.endLateCompute(frameCommandBuffers.lateCompute);
                END_GPU_PROFILING_BLOCK(gpuProfiler, frameCommandBuffers.lateCompute, "4-- Late Compute");

                // ---- Final Composite --------------------------------------------------------------------------
                // Copy everything out to the swap chain image and render UI
                BEGIN_GPU_PROFILING_BLOCK(gpuProfiler, frameCommandBuffers.present, "5-- Composite");
                sumiRenderer.beginCompositeRenderPass(frameCommandBuffers.present);

                postProcessor->compositeFrame(frameCommandBuffers.present, frameInfo.frameIdx);

                if (hqsmDebugger) {
                    hqsmDebugger->renderDebugView(frameCommandBuffers.present, gui.HQSMdebugView);
                }

                gui.beginFrame();
                gui.drawSceneViewer(
                    frameInfo,
                    cameraController,
                    shadowMapper->getZbin(),
                    shadowMapper->getLightMask(),
                    hqsmDebugger.get(),
                    gpuProfiler.get(),
                    cpuProfiler.get()
                );
                gui.endFrame();

                gui.render(frameCommandBuffers.present);

                sumiRenderer.endCompositeRenderPass(frameCommandBuffers.present);
                END_GPU_PROFILING_BLOCK(gpuProfiler, frameCommandBuffers.present, "5-- Composite");

                sumiRenderer.endFrame();
                if (gpuProfiler) gpuProfiler->endFrame();
            }
        }

        // Prevent cleanup from happening while GPU resources are in use on close.
        //  TODO: can this error?
        vkDeviceWaitIdle(sumiDevice.device());
    }

    void Sumire::loadObjects() {
        // TODO: Load objects in asynchronously
        // std::shared_ptr<SumiModel> modelObj1 = loaders::OBJloader::createModelFromFile(sumiDevice, SUMIRE_ENGINE_PATH("assets/models/obj/clorinde.obj"));
        //std::shared_ptr<SumiModel> modelObj1 = loaders::GLTFloader::createModelFromFile(sumiDevice, SUMIRE_ENGINE_PATH("assets/models/gltf/test/NormalTangentMirrorTest.glb"));
        //std::shared_ptr<SumiModel> modelObj1 = loaders::GLTFloader::createModelFromFile(sumiDevice, SUMIRE_ENGINE_PATH("assets/models/gltf/clorinde.glb"));
        //auto obj1 = SumiObject::createObject();
        //obj1.model = modelObj1;
        //obj1.transform.setTranslation(glm::vec3{-8.0f, 0.0f, 0.0f});
        //obj1.transform.setScale(glm::vec3{1.0f});
        //objects.emplace(obj1.getId(), std::move(obj1));

        //std::shared_ptr<SumiModel> modelGlb1 = loaders::GLTFloader::createModelFromFile(sumiDevice, SUMIRE_ENGINE_PATH("assets/models/gltf/test/MetalRoughSpheres.glb"));
        //auto glb1 = SumiObject::createObject();
        //glb1.model = modelGlb1;
        //glb1.transform.setTranslation(glm::vec3{-4.0f, 0.0f, 0.0f});
        //glb1.transform.setScale(glm::vec3{0.2f});
        //objects.emplace(glb1.getId(), std::move(glb1));

        // std::shared_ptr<SumiModel> modelGlb2 = loaders::GLTFloader::createModelFromFile(sumiDevice, SUMIRE_ENGINE_PATH("assets/models/gltf/doomslayer.glb"));
        // auto glb2 = SumiObject::createObject();
        // glb2.model = modelGlb2;
        // glb2.transform.setTranslation(glm::vec3{0.0f, 0.0f, 0.0f});
        // glb2.transform.setScale(glm::vec3{1.0f});
        // objects.emplace(glb2.getId(), std::move(glb2));

        std::shared_ptr<SumiModel> modelGlb3 = loaders::GLTFloader::createModelFromFile(sumiDevice, SUMIRE_ENGINE_PATH("assets/models/gltf/2b.glb"));
        auto glb3 = SumiObject::createObject();
        glb3.model = modelGlb3;
        glb3.transform.setTranslation(glm::vec3{0.0f, 0.0f, 0.0f});
        glb3.transform.setScale(glm::vec3{1.0f});
        objects.emplace(glb3.getId(), std::move(glb3));
    }

    void Sumire::loadLights() {
        constexpr float radial_n_lights = 20.0;
        for (float i = 0; i < radial_n_lights; i++) {
            float rads = i * glm::two_pi<float>() / radial_n_lights;
            auto light = SumiLight::createPointLight(glm::vec3{1.5f * glm::sin(rads), 3.0f, 1.5f * glm::cos(rads)});
            float hue = i * 360.0f / radial_n_lights;
            light.color = glm::vec4(glm::rgbColor(glm::vec3{ hue, 1.0, 1.0 }), 1.0);
            light.range = 4.0f;
            lights.emplace(light.getId(), std::move(light));
        }

        // zBin Light Tests
        //auto light1 = SumiLight::createPointLight({ 0.0f, 1.0f, 0.0f });
        //light1.range = 1.0f;
        //lights.emplace(light1.getId(), std::move(light1));

        //auto light2 = SumiLight::createPointLight({ -1.0f, 1.0f, -5.5f });
        //light2.range = 2.0f;
        //lights.emplace(light2.getId(), std::move(light2));

        //auto light3 = SumiLight::createPointLight({ 1.0f, 1.0f, -5.0f });
        //light3.range = 1.0f;
        //lights.emplace(light3.getId(), std::move(light3));

        //auto light4 = SumiLight::createPointLight({ -2.0f, 1.0f, -10.0f });
        //light4.range = 1.0f;
        //lights.emplace(light4.getId(), std::move(light4));

        //auto light5 = SumiLight::createPointLight({ 0.5f, 1.0f, -11.0f });
        //light5.range = 1.0f;
        //lights.emplace(light5.getId(), std::move(light5));

        //auto light6 = SumiLight::createPointLight({ 0.5f, 1.0f, -100.0f });
        //light6.range = 1.0f;
        //lights.emplace(light6.getId(), std::move(light6));

        // Light Mask Buffer Tests
        //auto light1 = SumiLight::createPointLight({ 0.0f, 1.0f, 0.0f });
        //light1.range = 1.0f;
        //lights.emplace(light1.getId(), std::move(light1));

    }
}
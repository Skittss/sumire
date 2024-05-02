#pragma once

#include <sumire/core/graphics_pipeline/sumi_device.hpp>
#include <sumire/core/profiling/profiling_block.hpp>

namespace sumire {

    class GpuProfiler {
    public:

        class Builder {
        public:
            Builder(SumiDevice& device) : sumiDevice{ device } {}

            Builder& addBlock(std::string name);
            GpuProfiler build() const;

        private:
            SumiDevice& sumiDevice;
            uint32_t currentBlockIdx = 0u;
            std::unordered_map<std::string, ProfilingBlock> profilingBlocks;
        };

        GpuProfiler(
            SumiDevice& device,
            std::unordered_map<std::string, ProfilingBlock> profilingBlocks
        );
        ~GpuProfiler();

        void beginFrame();
        void endFrame();
        void beginBlock(VkCommandBuffer commandBuffer, const std::string& name);
        void endBlock(VkCommandBuffer commandBuffer, const std::string& name);

    private:
        void createQueryPool();

        SumiDevice& sumiDevice;
        bool frameStarted = false;

        std::unordered_map<std::string, ProfilingBlock> namedProfilingBlocks;

        VkQueryPool timestampQueryPool = VK_NULL_HANDLE;

    };

}
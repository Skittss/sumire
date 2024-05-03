#pragma once

#include <sumire/core/graphics_pipeline/sumi_device.hpp>
#include <sumire/core/profiling/profiling_block.hpp>

#include <memory>
#include <map>

namespace sumire {

    class GpuProfiler {
    public:

        typedef std::map<std::string, ProfilingBlock> NamedProfilingBlockMap;

        class Builder {
        public:
            Builder(SumiDevice& device) : sumiDevice{ device } {}

            Builder& addBlock(std::string name);
            std::unique_ptr<GpuProfiler> build() const;

        private:
            SumiDevice& sumiDevice;
            uint32_t currentBlockIdx = 0u;
            NamedProfilingBlockMap profilingBlocks;
        };

        GpuProfiler(
            SumiDevice& device,
            NamedProfilingBlockMap profilingBlocks
        );
        ~GpuProfiler();

        void beginFrame(VkCommandBuffer commandBuffer);
        void endFrame();
        void beginBlock(VkCommandBuffer commandBuffer, const std::string& name);
        void endBlock(VkCommandBuffer commandBuffer, const std::string& name);

        const NamedProfilingBlockMap& getNamedBlocks() const {
            return namedProfilingBlocks;
        }

    private:
        void createQueryPool();

        void updateBlockMillis();

        SumiDevice& sumiDevice;
        bool frameStarted = false;

        NamedProfilingBlockMap namedProfilingBlocks;

        enum QueryPoolStatus {
            RESET,
            ISSUED,
            AVAILABLE
        };

        uint32_t queryCount = 0u;
        VkQueryPool timestampQueryPool = VK_NULL_HANDLE;
        std::vector<uint64_t> timestampQueryResults;
        QueryPoolStatus timestampQueryStatus = QueryPoolStatus::AVAILABLE;

    };

}
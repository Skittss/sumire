#include <sumire/core/profiling/gpu_profiler.hpp>

#include <sumire/util/vk_check_success.hpp>

namespace sumire {

    // ---- Builder ----------------------------------------------------------------------------------------------
    GpuProfiler::Builder& GpuProfiler::Builder::addBlock(std::string name) {
        profilingBlocks.emplace(name, ProfilingBlock{ currentBlockIdx } );
        currentBlockIdx += 2u;

        return *this;
    };

    GpuProfiler GpuProfiler::Builder::build() const {
        return GpuProfiler{ sumiDevice, profilingBlocks };
    }

    // ---- Profiler ---------------------------------------------------------------------------------------------
    GpuProfiler::GpuProfiler(
        SumiDevice& device,
        std::unordered_map<std::string, ProfilingBlock> profilingBlocks
    ) : sumiDevice{ device }, namedProfilingBlocks{ profilingBlocks } {

        createQueryPool();
    }

    GpuProfiler::~GpuProfiler() {
        vkDestroyQueryPool(sumiDevice.device(), timestampQueryPool, nullptr);
    }

    void GpuProfiler::createQueryPool() {

        const uint32_t queryCount = static_cast<uint32_t>(namedProfilingBlocks.size()) * 2u;

        VkQueryPoolCreateInfo timestampPoolInfo{};
        timestampPoolInfo.sType              = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
        timestampPoolInfo.queryType          = VK_QUERY_TYPE_TIMESTAMP;
        timestampPoolInfo.queryCount         = queryCount;

        VK_CHECK_SUCCESS(
            vkCreateQueryPool(sumiDevice.device(), &timestampPoolInfo, nullptr, &timestampQueryPool),
            "[Sumire::GpuProfiler] Failed to create timestamp query pool."
        );
    }

    void GpuProfiler::beginFrame() {
        assert(!frameStarted && "Tried to begin a profiling frame before ending a previous frame.");
        namedProfilingBlocks.clear();

        frameStarted = true;
    }

    void GpuProfiler::endFrame() {
        assert(frameStarted && "Tried to end a profiling frame before starting a frame.");

        frameStarted = false;
    }

    void GpuProfiler::beginBlock(VkCommandBuffer commandBuffer, const std::string& name) {
        assert(namedProfilingBlocks.find(name) != namedProfilingBlocks.end() 
            && "Tried to begin a block not specified when building the GpuProfiler.");
        ProfilingBlock& block = namedProfilingBlocks[name];

        vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, timestampQueryPool, block.idx);
    }

    void GpuProfiler::endBlock(VkCommandBuffer commandBuffer, const std::string& name) {
        assert(namedProfilingBlocks.find(name) != namedProfilingBlocks.end()
            && "Tried to end a block not specified when building the GpuProfiler.");
        ProfilingBlock& block = namedProfilingBlocks[name];

        vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, timestampQueryPool, block.idx + 1u);
    }

}
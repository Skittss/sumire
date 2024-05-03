#include <sumire/core/profiling/gpu_profiler.hpp>

#include <sumire/util/vk_check_success.hpp>

#include <iostream>
#include <vulkan/vk_enum_string_helper.h>

namespace sumire {

    // ---- Builder ----------------------------------------------------------------------------------------------
    GpuProfiler::Builder& GpuProfiler::Builder::addBlock(std::string name) {
        profilingBlocks.emplace(name, ProfilingBlock{ currentBlockIdx } );
        currentBlockIdx += 2u;

        return *this;
    };

    std::unique_ptr<GpuProfiler> GpuProfiler::Builder::build() const {
        return std::make_unique<GpuProfiler>( sumiDevice, profilingBlocks );
    }

    // ---- Profiler ---------------------------------------------------------------------------------------------
    GpuProfiler::GpuProfiler(
        SumiDevice& device,
        NamedProfilingBlockMap profilingBlocks
    ) : sumiDevice{ device }, namedProfilingBlocks{ profilingBlocks } {

        createQueryPool();
    }

    GpuProfiler::~GpuProfiler() {
        vkDestroyQueryPool(sumiDevice.device(), timestampQueryPool, nullptr);
    }

    void GpuProfiler::createQueryPool() {

        queryCount = static_cast<uint32_t>(namedProfilingBlocks.size()) * 2u;
        uint32_t queryResultSize = 2u * queryCount;
        timestampQueryResults.resize(queryResultSize, 0u);

        VkQueryPoolCreateInfo timestampPoolInfo{};
        timestampPoolInfo.sType              = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
        timestampPoolInfo.queryType          = VK_QUERY_TYPE_TIMESTAMP;
        timestampPoolInfo.queryCount         = queryCount;

        VK_CHECK_SUCCESS(
            vkCreateQueryPool(sumiDevice.device(), &timestampPoolInfo, nullptr, &timestampQueryPool),
            "[Sumire::GpuProfiler] Failed to create timestamp query pool."
        );
    }

    void GpuProfiler::beginFrame(VkCommandBuffer commandBuffer) {
        assert(!frameStarted && "Tried to begin a profiling frame before ending a previous frame.");

        if (timestampQueryStatus == QueryPoolStatus::AVAILABLE) {
            vkCmdResetQueryPool(commandBuffer, timestampQueryPool, 0u, queryCount);
            timestampQueryStatus = QueryPoolStatus::RESET;
        }

        frameStarted = true;
    }

    void GpuProfiler::endFrame() {
        assert(frameStarted && "Tried to end a profiling frame before starting a frame.");

        // Timestamp results
        VkResult res = vkGetQueryPoolResults(
            sumiDevice.device(), timestampQueryPool,
            0u, queryCount,
            timestampQueryResults.size() * sizeof(uint64_t),
            timestampQueryResults.data(),
            2u * sizeof(uint64_t),
            VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WITH_AVAILABILITY_BIT
        );
        timestampQueryStatus = res == VK_SUCCESS ? QueryPoolStatus::AVAILABLE : QueryPoolStatus::ISSUED;

        if (res == VK_SUCCESS) updateBlockMillis();

        frameStarted = false;
    }

    void GpuProfiler::beginBlock(VkCommandBuffer commandBuffer, const std::string& name) {
        assert(namedProfilingBlocks.find(name) != namedProfilingBlocks.end() 
            && "Tried to begin a block not specified when building the GpuProfiler.");
        ProfilingBlock& block = namedProfilingBlocks[name];

        if (timestampQueryStatus == QueryPoolStatus::RESET) {
            vkCmdWriteTimestamp(
                commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, timestampQueryPool, block.idx);
        }
    }

    void GpuProfiler::endBlock(VkCommandBuffer commandBuffer, const std::string& name) {
        assert(namedProfilingBlocks.find(name) != namedProfilingBlocks.end()
            && "Tried to end a block not specified when building the GpuProfiler.");
        ProfilingBlock& block = namedProfilingBlocks[name];

        if (timestampQueryStatus == QueryPoolStatus::RESET) {
            vkCmdWriteTimestamp(
                commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, timestampQueryPool, block.idx + 1u);
        }
    }

    void GpuProfiler::updateBlockMillis() {
        for (auto& kv : namedProfilingBlocks) {
            uint32_t startIdx             = kv.second.idx * 2u;
            uint32_t startAvailabilityIdx = startIdx + 1u;
            uint32_t endIdx               = startIdx + 2u;
            uint32_t endAvailabilityIdx   = startIdx + 3u;

            bool startAvailability = timestampQueryResults[startAvailabilityIdx] == 1u;
            bool endAvailability   = timestampQueryResults[ endAvailabilityIdx ] == 1u;

            if (startAvailability && endAvailability) {
                uint64_t diff = timestampQueryResults[endIdx] - timestampQueryResults[startIdx];
                double ns = diff * sumiDevice.properties.limits.timestampPeriod;

                kv.second.ms = ns / 1e6;
            }
        }
    }

}
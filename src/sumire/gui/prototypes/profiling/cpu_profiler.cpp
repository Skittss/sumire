#include <sumire/gui/prototypes/profiling/cpu_profiler.hpp>

#include <chrono>
#include <cassert>

namespace kbf {

    // Global Profiler instance
    std::unique_ptr<CpuProfiler> CpuProfiler::GlobalProfiler = nullptr;

    // ---- Builder ----------------------------------------------------------------------------------------------
    CpuProfiler::Builder& CpuProfiler::Builder::addBlock(std::string name) {
        profilingBlocks.emplace(name, ProfilingBlock{ currentBlockIdx });
        currentBlockIdx += 1u;

        return *this;
    };

    std::unique_ptr<CpuProfiler> CpuProfiler::Builder::build() const {
        return std::make_unique<CpuProfiler>(profilingBlocks);
    }

    // ---- Profiler ---------------------------------------------------------------------------------------------
    CpuProfiler::CpuProfiler(
        NamedProfilingBlockMap profilingBlocks
    ) : namedProfilingBlocks{ profilingBlocks } {
        for (auto& kv : namedProfilingBlocks) {
            recordedTimestamps.emplace(kv.first, ProfilingBlockTimestamp{});
        }
    }

    void CpuProfiler::setBlockMillis(const std::string& name, double ms) {
        namedProfilingBlocks[name].ms = ms;
    }

    void CpuProfiler::beginBlock(const std::string& name) {
        assert(namedProfilingBlocks.find(name) != namedProfilingBlocks.end()
            && "Tried to begin a block not specified when building the CpuProfiler.");
        ProfilingBlock& block = namedProfilingBlocks[name];
        recordedTimestamps[name].start = std::chrono::high_resolution_clock::now();
    }

    void CpuProfiler::endBlock(const std::string& name) {
        assert(namedProfilingBlocks.find(name) != namedProfilingBlocks.end()
            && "Tried to end a block not specified when building the CpuProfiler.");
        ProfilingBlock& block = namedProfilingBlocks[name];
        recordedTimestamps[name].end = std::chrono::high_resolution_clock::now();

        block.ms = std::chrono::duration<float, std::chrono::seconds::period>(
            recordedTimestamps[name].end - recordedTimestamps[name].start).count();
    }

}
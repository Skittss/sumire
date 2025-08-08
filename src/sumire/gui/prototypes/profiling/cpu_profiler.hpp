#pragma once

#include <sumire/gui/prototypes/profiling/profiling_block.hpp>
#include <sumire/gui/prototypes/profiling/profiling_block_timestamp.hpp>

#include <memory>
#include <map>

#define BEGIN_CPU_PROFILING_BLOCK(profiler, blockName)  \
   if ((profiler)) (profiler)->beginBlock((blockName)); \

#define END_CPU_PROFILING_BLOCK(profiler, blockName)  \
   if ((profiler)) (profiler)->endBlock((blockName)); \

namespace kbf {

    class CpuProfiler {
    public:

        static std::unique_ptr<CpuProfiler> GlobalProfiler;
        typedef std::map<std::string, ProfilingBlock> NamedProfilingBlockMap;

        class Builder {
        public:
            Builder() {}

            Builder& addBlock(std::string name);
            std::unique_ptr<CpuProfiler> build() const;

        private:
            uint32_t currentBlockIdx = 0u;
            NamedProfilingBlockMap profilingBlocks;
        };

        CpuProfiler(
            NamedProfilingBlockMap profilingBlocks
        );
        ~CpuProfiler() = default;

        void setBlockMillis(const std::string& name, double ms);
        void beginBlock(const std::string& name);
        void endBlock(const std::string& name);

        const NamedProfilingBlockMap& getNamedBlocks() const {
            return namedProfilingBlocks;
        }

    private:
        NamedProfilingBlockMap namedProfilingBlocks;
        std::map<std::string, ProfilingBlockTimestamp> recordedTimestamps;
    };

}
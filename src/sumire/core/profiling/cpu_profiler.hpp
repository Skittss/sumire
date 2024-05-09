#pragma once

#include <sumire/core/profiling/profiling_block.hpp>
#include <sumire/core/profiling/profiling_block_timestamp.hpp>

#include <memory>
#include <map>

namespace sumire {

    class CpuProfiler {
    public:

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
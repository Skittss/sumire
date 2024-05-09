#pragma once

#include <sumire/core/rendering/sumi_light.hpp>

#include <sumire/util/bit_field_operators.hpp>

#include <glm/glm.hpp>

#include <cstdint>
#include <vector>

namespace sumire::structs {

    struct viewSpaceLight {
        SumiLight* lightPtr = nullptr;
        glm::vec3 viewSpacePosition{ 0.0f };
        float viewSpaceDepth = 0.0f;
        float minDepth = 0.0f;
        float maxDepth = 0.0f;
    };

    struct findLightsApproxPush {
        glm::uvec2   shadowTileResolution;
        glm::uvec2   tileGroupResolution;
        glm::uvec2   lightMaskResolution;
        glm::uint    numZbinSlices;
        glm::float32 cameraNear;
        glm::float32 cameraFar;
    };

    struct findLightsAccuratePush {
        glm::uvec2   screenResolution;
        glm::uvec2   shadowTileResolution;
        glm::uvec2   tileGroupResolution;
        glm::uvec2   lightMaskResolution;
        glm::uint    numZbinSlices;
        glm::float32 cameraNear;
        glm::float32 cameraFar;
    };

    // We send light mask bits to a shader as vectors of uint32_t
    struct lightMaskBitField {
        long bits : 32;
    };

    struct lightMaskTile {
        // Idx 0: Reserved for light groups
        // Idx 1-32: Light mask bits
        lightMaskBitField bitFields[33] { 0b0 };

        void clear() {
            for (auto& field : bitFields)
                field.bits = 0b0;
        }

        bool isBitSet(uint32_t x, uint32_t y) const {
            assert(x < 32 && y < 33 && "queried bit index is out of range");
            return BIT_IS_SET(bitFields[y].bits, x);
        }

        void setLightBit(uint32_t lightIdx) {
            uint32_t lightGroupIdx = lightIdx / 32u;
            uint32_t bitIdx = lightIdx - 32u * lightGroupIdx;

            // Set light group bit and light bit
            BIT_SET(bitFields[0].bits, lightGroupIdx);
            BIT_SET(bitFields[lightGroupIdx + 1].bits, bitIdx);
        }
    };

    struct lightMask {
        // Idx 0 is reserved for light *groups* for speeding up iteration
        //  of this buffer.

        const uint32_t width;
        const uint32_t height;
        const uint32_t numTilesX;
        const uint32_t numTilesY;

        std::vector<lightMaskTile> tiles;

        void clear() {
            // clear bits of all tiles
            for (auto& tile : tiles)
                tile.clear();
        }

        const lightMaskTile& readTileAtIdx(const uint32_t x, const uint32_t y) const {
            return tiles[x + y * numTilesX];
        }

        lightMaskTile& tileAtIdx(const uint32_t x, const uint32_t y) { 
            return tiles[x + y * numTilesX];
        }

        lightMaskTile& tileAtRasterPos(const uint32_t x, const uint32_t y) {
            uint32_t xIdx = x - (x / 32u);
            uint32_t yIdx = y - (y / 32u);

            return tiles[x + y * numTilesX];
        }

        lightMask(
            const uint32_t width, const uint32_t height
        ) : width{ width }, 
            height{ height }, 
            numTilesX{ static_cast<uint32_t>(glm::ceil(static_cast<float>(width)  / 32.0f)) }, 
            numTilesY{ static_cast<uint32_t>(glm::ceil(static_cast<float>(height) / 32.0f)) }
        {

            tiles = std::vector<lightMaskTile>(numTilesX * numTilesY);
        }
    };

    // Combination of Zbin and Ranged Zbin.
    //   Ranged Zbin values offer more conservative estimates for 
    //   light queries over a range (>2) of multiple contiguous Zbins.
    struct zBinData {
        int minLightIdx = -1;
        int maxLightIdx = -1;
        int rangedMinLightIdx = -1;
        int rangedMaxLightIdx = -1;
    };

    struct zBin {
        int minLight = -1;
        int maxLight = -1;
        int firstFullIdx = -1;
        int lastFullIdx = -1;

        std::vector<zBinData> data;

        zBin(const uint32_t size) {
            data = std::vector<zBinData>(size);
        }

        void reset() {
            std::fill(data.begin(), data.end(), zBinData{});
            firstFullIdx = -1;
            lastFullIdx = -1;
            minLight = -1;
            maxLight = -1;
        }
    };

}
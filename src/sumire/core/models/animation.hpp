#pragma once

#include <sumire/core/models/node.hpp>
#include <sumire/util/gltf_interpolators.hpp>

namespace sumire {

    struct AnimationChannel {
        enum PathType { TRANSLATION, ROTATION, SCALE, WEIGHTS };
        PathType path;
        Node *node;
        uint32_t samplerIdx;
        
        ~AnimationChannel() {
            node = nullptr;
        };
    };

    struct AnimationSampler {
        util::GLTFinterpolationType interpolation;
        std::vector<float> inputs;
        std::vector<glm::vec4> outputs;
    };

    struct Animation {
        std::string name;
        std::vector<AnimationSampler> samplers;
        std::vector<AnimationChannel> channels;
        float start = std::numeric_limits<float>::max();
        float end = std::numeric_limits<float>::min();

        ~Animation() {
            channels.clear();
        }
    };

}
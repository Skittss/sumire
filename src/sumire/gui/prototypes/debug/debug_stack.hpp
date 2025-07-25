#pragma once

#include <sumire/gui/prototypes/debug/log_data.hpp>

#include <deque>
#include <string>
#include <chrono>

namespace kbf {

    class DebugStack {
    public:
        DebugStack(size_t limit) : limit{ limit } {}

        void push(LogData logData) {
            stack.push_back(logData);
            if (stack.size() > limit) pop();
        }

        void push(const std::string& message, glm::vec3 colour) {
            push(LogData{ message, colour, DebugStack::now() });
        }

        void pop() {
            if (!stack.empty()) {
                stack.pop_front();
            }
        }

        const LogData& peek() const {
            return stack.back();
        }

        void clear() {
            while (!stack.empty()) {
                stack.pop_back();
            }
        }

        bool empty() { return stack.empty(); }

        auto begin() { return stack.begin(); }
        auto end() { return stack.end(); }

        auto begin() const { return stack.begin(); }
        auto end() const { return stack.end(); }

        struct Color {
            static constexpr glm::vec3 ERROR   = glm::vec3(0.839f, 0.365f, 0.365f);  // #D65D5D
            static constexpr glm::vec3 WARNING = glm::vec3(0.902f, 0.635f, 0.235f);  // #E6A23C
            static constexpr glm::vec3 INFO    = glm::vec3(0.753f, 0.753f, 0.753f);  // #C0C0C0
            static constexpr glm::vec3 DEBUG   = glm::vec3(0.365f, 0.678f, 0.886f);  // #5DADE2
            static constexpr glm::vec3 SUCCESS = glm::vec3(0.451f, 0.776f, 0.424f);  // #73C66C		
        };

        static inline std::chrono::system_clock ::time_point now() noexcept { return std::chrono::system_clock::now(); }

    private:
        size_t limit;
        std::deque<LogData> stack{};
    };

    inline DebugStack DEBUG_STACK{ 1000 };

}
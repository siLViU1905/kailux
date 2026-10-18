#pragma once
#include <bitset>

#include "window/InputSource.h"

namespace kailux
{
    class Input
    {
    public:
        bool Enabled() const;
        bool IsKeyDown(Key key) const;
        bool IsKeyPressed(Key key) const;
        bool IsKeyReleased(Key key) const;
        bool IsMouseDown(MouseButton button) const;
        glm::vec2 MousePosition() const;
        glm::vec2 MouseDelta() const;

    private:
        static constexpr size_t kKeyCount{GLFW_KEY_LAST + 1};

        static bool valid_key(Key key);

        void BeginFrame(const InputSource& source);
        void Reset();
        void Track(Key key) const;

        InputSource mSource;

        mutable std::bitset<kKeyCount> mTracked;
        mutable std::bitset<kKeyCount> mPrevDown;
        mutable std::bitset<kKeyCount> mCurrDown;

        glm::vec2 mMousePosition{0.f};
        glm::vec2 mMouseDelta{0.f};
        bool      mHasMousePosition{};

        friend class ScriptSystem;
    };
}
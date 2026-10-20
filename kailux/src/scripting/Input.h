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
        bool IsMousePressed(MouseButton button) const;
        bool IsMouseReleased(MouseButton button) const;
        glm::vec2 MousePosition() const;
        glm::vec2 MouseDelta() const;
        glm::vec2 ScrollDelta() const;

        void       SetCursorMode(CursorMode mode);
        CursorMode GetCursorMode() const;

    private:
        static constexpr size_t kKeyCount{GLFW_KEY_LAST + 1};
        static constexpr size_t kButtonCount{GLFW_MOUSE_BUTTON_LAST + 1};

        static bool valid_key(Key key);
        static bool valid_button(MouseButton button);

        void BeginFrame(const InputSource& source, glm::vec2 scrollDelta);
        void Reset();
        void Track(Key key) const;

        InputSource mSource;

        mutable std::bitset<kKeyCount> mTracked;
        mutable std::bitset<kKeyCount> mPrevDown;
        mutable std::bitset<kKeyCount> mCurrDown;

        std::bitset<kButtonCount> mPrevButtons;
        std::bitset<kButtonCount> mCurrButtons;
        bool                      mHasButtons{};

        glm::vec2 mMousePosition{0.f};
        glm::vec2 mMouseDelta{0.f};
        glm::vec2 mScrollDelta{0.f};
        bool      mHasMousePosition{};
        bool      mCursorChanged{};

        friend class ScriptSystem;
    };
}
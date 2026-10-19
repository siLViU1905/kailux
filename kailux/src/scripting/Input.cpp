#include "Input.h"

namespace kailux
{
    bool Input::Enabled() const
    {
        return mSource.Valid();
    }

    bool Input::IsKeyDown(Key key) const
    {
        return Enabled() && mSource.IsKeyPressed(key);
    }

    bool Input::IsKeyPressed(Key key) const
    {
        if (!Enabled() || !valid_key(key))
            return false;

        Track(key);
        const auto index{static_cast<size_t>(key)};
        return mCurrDown[index] && !mPrevDown[index];
    }

    bool Input::IsKeyReleased(Key key) const
    {
        if (!Enabled() || !valid_key(key))
            return false;

        Track(key);
        const auto index{static_cast<size_t>(key)};
        return !mCurrDown[index] && mPrevDown[index];
    }

    bool Input::IsMouseDown(MouseButton button) const
    {
        return Enabled() && mSource.IsButtonClicked(button);
    }

    bool Input::IsMousePressed(MouseButton button) const
    {
        if (!Enabled() || !valid_button(button))
            return false;

        const auto index{static_cast<size_t>(button)};
        return mCurrButtons[index] && !mPrevButtons[index];
    }

    bool Input::IsMouseReleased(MouseButton button) const
    {
        if (!Enabled() || !valid_button(button))
            return false;

        const auto index{static_cast<size_t>(button)};
        return !mCurrButtons[index] && mPrevButtons[index];
    }

    glm::vec2 Input::MousePosition() const
    {
        return mMousePosition;
    }
    glm::vec2 Input::MouseDelta() const
    {
        return mMouseDelta;
    }

    glm::vec2 Input::ScrollDelta() const
    {
        return mScrollDelta;
    }

    void Input::SetCursorMode(CursorMode mode)
    {
        if (!Enabled())
            return;

        mSource.SetCursorMode(mode);
        mCursorChanged = mode != CursorMode::Normal;
    }

    CursorMode Input::GetCursorMode() const
    {
        return Enabled() ? mSource.GetCursorMode() : CursorMode::Normal;
    }

    bool Input::valid_key(Key key)
    {
        const auto index{static_cast<int>(key)};
        return index >= 0 && static_cast<size_t>(index) < kKeyCount;
    }

    bool Input::valid_button(MouseButton button)
    {
        const auto index{static_cast<int>(button)};
        return index >= 0 && static_cast<size_t>(index) < kButtonCount;
    }

    void Input::BeginFrame(const InputSource &source, glm::vec2 scrollDelta)
    {
        if (source != mSource)
            Reset();

        mSource = source;
        if (!mSource.Valid())
            return;

        mScrollDelta = scrollDelta;

        mPrevButtons = mCurrButtons;
        for (size_t index{}; index < kButtonCount; ++index)
            mCurrButtons[index] = mSource.IsButtonClicked(static_cast<MouseButton>(index));
        if (!mHasButtons)
        {
            mPrevButtons = mCurrButtons;
            mHasButtons  = true;
        }

        for (size_t index{}; index < kKeyCount; ++index)
        {
            if (!mTracked[index])
                continue;

            mPrevDown[index] = mCurrDown[index];
            mCurrDown[index] = mSource.IsKeyPressed(static_cast<Key>(index));
        }

        const glm::vec2 position{mSource.GetMousePos()};
        mMouseDelta = mHasMousePosition ? position - mMousePosition : glm::vec2{0.f};
        mMousePosition = position;
        mHasMousePosition = true;
    }

    void Input::Reset()
    {
        if (mCursorChanged && mSource.Valid())
            mSource.SetCursorMode(CursorMode::Normal);
        mCursorChanged = false;

        mSource = {};
        mPrevButtons.reset();
        mCurrButtons.reset();
        mHasButtons  = false;
        mScrollDelta = glm::vec2{0.f};
        mTracked.reset();
        mPrevDown.reset();
        mCurrDown.reset();
        mMousePosition    = glm::vec2{0.f};
        mMouseDelta       = glm::vec2{0.f};
        mHasMousePosition = false;
    }

    void Input::Track(Key key) const
    {
        const auto index{static_cast<size_t>(key)};
        if (mTracked[index])
            return;

        const bool down{mSource.IsKeyPressed(key)};
        mTracked[index]  = true;
        mPrevDown[index] = down;
        mCurrDown[index] = down;
    }
}

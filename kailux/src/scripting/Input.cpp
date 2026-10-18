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
    glm::vec2 Input::MousePosition() const
    {
        return mMousePosition;
    }
    glm::vec2 Input::MouseDelta() const
    {
        return mMouseDelta;
    }
    bool Input::valid_key(Key key)
    {
        const auto index{static_cast<int>(key)};
        return index >= 0 && static_cast<size_t>(index) < kKeyCount;
    }

    void Input::BeginFrame(const InputSource &source)
    {
        if (source != mSource)
            Reset();

        mSource = source;
        if (!mSource.Valid())
            return;

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
        mSource = {};
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

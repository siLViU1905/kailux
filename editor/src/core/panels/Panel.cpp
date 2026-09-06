#include "Panel.h"

namespace kailux
{
    Panel::Panel() = default;

    Panel::Panel(std::string_view name, ImVec4 backgroundColor, bool open, bool focused) : mName(name),
                                                                                           mBackgroundColor(backgroundColor),
                                                                                           mOpen(open),
                                                                                           mFocused(focused)
    {
    }

    void Panel::SetName(std::string_view name)
    {
        mName = name;
    }

    void Panel::SetBackgroundColor(ImVec4 backgroundColor)
    {
        mBackgroundColor = backgroundColor;
    }

    void Panel::Open()
    {
        mOpen = true;
    }

    void Panel::Close()
    {
        mOpen = false;
    }

    void Panel::Toggle()
    {
        mOpen = !mOpen;
    }

    bool Panel::IsOpen() const
    {
        return mOpen;
    }

    bool Panel::IsFocused() const
    {
        return mFocused;
    }

    InputSource Panel::GetInputSource() const
    {
        return mPlatformWindow;
    }

    bool Panel::ConsumeToggleMouseLook()
    {
        return std::exchange(mToggleMouseLook, false);
    }
}

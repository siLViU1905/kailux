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

    void Panel::setName(std::string_view name)
    {
        mName = name;
    }

    void Panel::setBackgroundColor(ImVec4 backgroundColor)
    {
        mBackgroundColor = backgroundColor;
    }

    void Panel::open()
    {
        mOpen = true;
    }

    void Panel::close()
    {
        mOpen = false;
    }

    void Panel::toggle()
    {
        mOpen = !mOpen;
    }

    bool Panel::isOpen() const
    {
        return mOpen;
    }

    bool Panel::isFocused() const
    {
        return mFocused;
    }
}

#include "Panel.h"

namespace kailux
{
    Panel::Panel()
        : mBackgroundColor(0.f, 0.f, 0.f, 0.f),
          mOpen(true)
    {
    }

    Panel::Panel(std::string_view name, ImVec4 backgroundColor, bool open) : mName(name),
                                                                             mBackgroundColor(backgroundColor),
                                                                             mOpen(open)
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

    bool Panel::isOpen() const
    {
        return mOpen;
    }
}

#include "Panel.h"

namespace kailux
{
    Panel::Panel()
        : m_BackgroundColor(0.f, 0.f, 0.f, 0.f),
          m_Open(true)
    {
    }

    Panel::Panel(std::string_view name, ImVec4 backgroundColor, bool open) : m_Name(name),
                                                                             m_BackgroundColor(backgroundColor),
                                                                             m_Open(open)
    {
    }

    void Panel::setName(std::string_view name)
    {
        m_Name = name;
    }

    void Panel::setBackgroundColor(ImVec4 backgroundColor)
    {
        m_BackgroundColor = backgroundColor;
    }

    void Panel::open()
    {
        m_Open = true;
    }

    void Panel::close()
    {
        m_Open = false;
    }

    bool Panel::isOpen() const
    {
        return m_Open;
    }
}

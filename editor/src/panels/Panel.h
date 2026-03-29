#pragma once
#include "core/Scene.h"

namespace kailux
{
    class Panel
    {
    public:
        Panel();
        Panel(std::string_view name, ImVec2 position, ImVec2 size, ImVec4 backgroundColor);
        virtual ~Panel() = default;

        virtual void render(Scene& scene) = 0;

        void setName(std::string_view name);
        void setPosition(ImVec2 position);
        void setSize(ImVec2 size);
        void setBackgroundColor(ImVec4 backgroundColor);

    protected:
        std::string  m_Name;
        ImVec2       m_Position;
        ImVec2       m_Size;
        ImVec4       m_BackgroundColor;
    };
}

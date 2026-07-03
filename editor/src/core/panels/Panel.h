#pragma once
#include "core/Scene.h"

namespace kailux
{
    class Panel
    {
    public:
        Panel();
        Panel(std::string_view name, ImVec4 backgroundColor, bool open = true);
        virtual ~Panel() = default;

        virtual void render(Scene& scene) = 0;

        void setName(std::string_view name);
        void setBackgroundColor(ImVec4 backgroundColor);

        void open();
        void close();

        bool isOpen() const;

    protected:
        std::string  mName;
        ImVec4       mBackgroundColor;
        bool         mOpen;
    };
}

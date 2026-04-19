#pragma once
#include "../panels/Panel.h"

namespace kailux
{
    template<class... Panels>
    class Layer
    {
    public:
        Layer() : m_Panels(create_scoped<std::tuple<Panels...>>())
        {
        }
        Layer(const Layer&) = delete;
        Layer& operator=(const Layer&) = delete;
        Layer(Layer&& other) noexcept : m_Panels(std::move(other.m_Panels))
        {
        }
        Layer& operator=(Layer&& other) noexcept
        {
            if (this != &other)
            {
                m_Panels = std::move(other.m_Panels);
            }
            return *this;
        }
        virtual ~Layer() = default;

        void render(Scene& scene)
        {
            std::apply([&scene](auto&&... panel)
            {
                (panel.render(scene), ...);
            }, *m_Panels);
        }

        auto       &getPanels() {return *m_Panels;}
        const auto &getPanels() const {return *m_Panels;}

        template<typename Panel>
        auto& getPanel()
        {
            return std::get<Panel>(*m_Panels);
        }

        template<typename Panel>
        const auto& getPanel() const
        {
            return std::get<Panel>(*m_Panels);
        }

    private:
        Scoped<std::tuple<Panels...>> m_Panels;
    };
}

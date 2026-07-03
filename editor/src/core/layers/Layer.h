#pragma once
#include "../panels/Panel.h"

namespace kailux
{
    template<class... Panels>
    class Layer
    {
    public:
        Layer() : mPanels(create_scoped<std::tuple<Panels...>>())
        {
        }
        Layer(const Layer&) = delete;
        Layer& operator=(const Layer&) = delete;
        Layer(Layer&& other) noexcept : mPanels(std::move(other.mPanels))
        {
        }
        Layer& operator=(Layer&& other) noexcept
        {
            if (this != &other)
            {
                mPanels = std::move(other.mPanels);
            }
            return *this;
        }
        virtual ~Layer() = default;

        void render(Scene& scene)
        {
            std::apply([&scene](auto&&... panel)
            {
                (panel.render(scene), ...);
            }, *mPanels);
        }

        auto       &getPanels() {return *mPanels;}
        const auto &getPanels() const {return *mPanels;}

        template<typename _Panel>
        auto& getPanel()
        {
            return std::get<_Panel>(*mPanels);
        }

        template<typename _Panel>
        const auto& getPanel() const
        {
            return std::get<_Panel>(*mPanels);
        }

    private:
        Scoped<std::tuple<Panels...>> mPanels;
    };
}

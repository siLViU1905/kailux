#pragma once
#include "../panels/Panel.h"

namespace kailux
{
    class Layer
    {
    public:
        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(Layer)

        template<std::derived_from<Panel> TPanel, typename... Args>
        TPanel& EmplacePanel(Args&&... args)
        {
            auto& panel = mPanels.emplace_back(create_scoped<TPanel>(std::forward<Args>(args)...));
            return static_cast<TPanel &>(*panel);
        }

        template<std::derived_from<Panel> TPanel>
        TPanel& GetPanel()
        {
            auto* panel = TryGetPanel<TPanel>();
            assert(panel != nullptr && "Panel not found");
            return *panel;
        }

        template<std::derived_from<Panel> TPanel>
        const TPanel& GetPanel() const
        {
            return const_cast<Layer*>(this)->GetPanel<TPanel>();
        }

    protected:
        ~Layer() = default;

        void RenderPanels(Scene& scene) const;

    private:
        template<std::derived_from<Panel> TPanel>
        TPanel* TryGetPanel()
        {
            for (auto& panel : mPanels)
                if (typeid(*panel) == typeid(TPanel))
                    return static_cast<TPanel *>(panel.get());
            return nullptr;
        }

        std::vector<Scoped<Panel>> mPanels;
    };
}

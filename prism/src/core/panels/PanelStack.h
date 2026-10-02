#pragma once
#include "Panel.h"

namespace kailux
{
    class PanelStack
    {
    public:
        PanelStack();
        PanelStack(const PanelStack&) = delete;
        PanelStack& operator=(const PanelStack&) = delete;
        PanelStack(PanelStack&&) noexcept;
        PanelStack& operator=(PanelStack&&) noexcept;

        template<std::derived_from<Panel> TPanel, typename... Args>
        TPanel& EmplacePanel(Args&&... args)
        {
            auto& panel{mPanels.emplace_back(create_scoped<TPanel>(std::forward<Args>(args)...))};
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
            return const_cast<PanelStack*>(this)->GetPanel<TPanel>();
        }

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
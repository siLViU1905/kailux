#pragma once
#include "../panels/Panel.h"

namespace kailux
{
    class Layer
    {
    public:
        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(Layer)

        template<std::derived_from<Panel> TPanel, typename... Args>
        TPanel& emplacePanel(Args&&... args)
        {
            auto& panel = mPanels.emplace_back(create_scoped<TPanel>(std::forward<Args>(args)...));
            return static_cast<TPanel &>(*panel);
        }

        template<std::derived_from<Panel> TPanel>
        TPanel& getPanel()
        {
            auto* panel = tryGetPanel<TPanel>();
            assert(panel != nullptr && "Panel not found");
            return *panel;
        }

        template<std::derived_from<Panel> TPanel>
        const TPanel& getPanel() const
        {
            return const_cast<Layer*>(this)->getPanel<TPanel>();
        }

    protected:
        ~Layer() = default;

        void renderPanels(Scene& scene) const;

    private:
        template<std::derived_from<Panel> TPanel>
        TPanel* tryGetPanel()
        {
            for (auto& panel : mPanels)
                if (typeid(*panel) == typeid(TPanel))
                    return static_cast<TPanel *>(panel.get());
            return nullptr;
        }

        std::vector<Scoped<Panel>> mPanels;
    };
}

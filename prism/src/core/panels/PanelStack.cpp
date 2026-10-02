#include "PanelStack.h"

kailux::PanelStack::PanelStack() = default;

kailux::PanelStack::PanelStack(PanelStack &&) noexcept = default;

kailux::PanelStack & kailux::PanelStack::operator=(PanelStack &&) noexcept = default;

void kailux::PanelStack::RenderPanels(Scene &scene) const
{
    for (auto& panel : mPanels)
        if (panel->IsOpen())
            panel->Render(scene);
}

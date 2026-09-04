#pragma once
#include "Panel.h"

namespace kailux
{
    class SimulationPanel final : public Panel
    {
    public:
        SimulationPanel(std::string_view name);

        void Render(Scene& scene) override;

        void SetTextureId(ImTextureID id);

        vk::Extent2D GetExtent() const;

        bool IsVisible() const;

    private:
        ImTextureID mTextureId{};
        ImVec2      mExtent{};
        bool        mVisible{};
    };
}

#pragma once
#include "Panel.h"

namespace kailux
{
    class SimulationPanel final : public Panel
    {
    public:
        SimulationPanel(std::string_view name);

        void render(Scene& scene) override;

        void setTextureId(ImTextureID id);

        vk::Extent2D getExtent() const;

        bool isVisible() const;

    private:
        ImTextureID mTextureId{};
        ImVec2      mExtent{};
        bool        mVisible{};
    };
}

#pragma once
#include "Panel.h"

namespace kailux
{
    class SimulationPanel final : public Panel
    {
    public:
        void render(Scene& scene) override;

        void setTextureId(ImTextureID id);
        void setAspectRatio(float ratio);
        bool isVisible() const;

    private:
        ImTextureID mTextureId{};
        float       mAspectRatio{};
        bool        mVisible{};
    };
}

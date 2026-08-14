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

        using OnStop = std::move_only_function<void()>;
        void setOnStop(OnStop&& callback);

    private:
        ImTextureID mTextureId{};
        float       mAspectRatio{};
        bool        mVisible{};
        OnStop      mOnStop;
    };
}

#pragma once
#include "Panel.h"

namespace kailux
{
    class ViewportPanel : public Panel
    {
    public:
        ViewportPanel();

        void render(Scene &scene) override;

        void setSceneTextureId(ImTextureID id);

        struct MousePosition
        {
            uint32_t x{};
            uint32_t y{};
        };
        MousePosition getScaledMousePos() const;

        using OnClick = std::move_only_function<void()>;
        void setOnClick(OnClick&& callback);

        SimulationState getSimulationState() const;

        using OnSimulationStart = std::move_only_function<void()>;
        void setOnSimulationStart(OnSimulationStart&& callback);
        using OnSimulationPause = std::move_only_function<void()>;
        void setOnSimulationPause(OnSimulationPause&& callback);

    private:
        static MousePosition compute_relative_mouse_pos(ImVec2 minBound, ImVec2 viewportSize);

        void renderSimulationIndicator(ImVec2 minBound, ImVec2 viewportSize);

        ImTextureID   mSceneTextureId;
        MousePosition mMousePos;
        OnClick       mOnClick;

        SimulationState mSimulationState;
        OnSimulationStart mOnSimulationStart;
        OnSimulationPause mOnSimulationPause;
    };
}

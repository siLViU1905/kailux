#pragma once
#include "Panel.h"

namespace kailux
{
    class ViewportPanel : public Panel
    {
    public:
        ViewportPanel();
        ViewportPanel(std::string_view name);

        void Render(Scene &scene) override;

        void SetSceneTextureId(ImTextureID id);

        struct MousePosition
        {
            uint32_t x{};
            uint32_t y{};
        };
        MousePosition GetScaledMousePos() const;

        using OnClick = std::move_only_function<void()>;
        void SetOnClick(OnClick&& callback);

        SimulationState GetSimulationState() const;

        void RequestSimulationState(SimulationState state);

        using OnSimulationStart = std::move_only_function<void()>;
        void SetOnSimulationStart(OnSimulationStart&& callback);
        using OnSimulationPause = std::move_only_function<void()>;
        void SetOnSimulationPause(OnSimulationPause&& callback);

    private:
        static MousePosition compute_relative_mouse_pos(ImVec2 minBound, ImVec2 viewportSize);

        void RenderSimulationIndicator(ImVec2 minBound, ImVec2 viewportSize);

        ImTextureID   mSceneTextureId;
        MousePosition mMousePos;
        OnClick       mOnClick;

        SimulationState mSimulationState;
        OnSimulationStart mOnSimulationStart;
        OnSimulationPause mOnSimulationPause;
    };
}

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

        void SetSceneTexture(ImTextureID id, glm::ivec2 extent);

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

        using OnSimulationStart = std::move_only_function<bool()>;
        void SetOnSimulationStart(OnSimulationStart&& callback);
        using OnSimulationPause = std::move_only_function<void()>;
        void SetOnSimulationPause(OnSimulationPause&& callback);

    private:
        static MousePosition compute_relative_mouse_pos(ImVec2 minBound, ImVec2 viewportSize, glm::ivec2 textureExtent);

        void RenderSimulationIndicator(ImVec2 minBound, ImVec2 viewportSize);

        ImTextureID   mSceneTextureId;
        glm::ivec2    mSceneTextureExtent;
        MousePosition mMousePos;
        OnClick       mOnClick;

        SimulationState mSimulationState;
        OnSimulationStart mOnSimulationStart;
        OnSimulationPause mOnSimulationPause;
    };
}

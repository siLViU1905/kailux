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

    private:
        static MousePosition compute_relative_mouse_pos(ImVec2 minBound, ImVec2 viewportSize);

        ImTextureID   m_SceneTextureId;
        MousePosition m_MousePos;
        OnClick       m_OnClick;
    };
}

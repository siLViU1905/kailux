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

    private:
        ImTextureID m_SceneTextureId;
    };
}

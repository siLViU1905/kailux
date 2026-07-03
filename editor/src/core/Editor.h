#pragma once
#include "layers/EditorLayer.h"

namespace kailux
{
    class Editor
    {
    public:
        KAILUX_DECLARE_NON_COPYABLE_MOVABLE(Editor)

        static Editor create(ImTextureID directoryTextureId, ImTextureID fileTextureId);

        template<typename Layer>
        auto& getLayer()
        {
            return std::get<Layer>(*mActiveLayer);
        }

        template<typename Layer>
        const auto& getLayer() const
        {
            return std::get<Layer>(*mActiveLayer);
        }

        void render(Scene& scene);

        void update();

    private:
        void createLayers(ImTextureID directoryTextureId, ImTextureID fileTextureId);

        using LayerTypes = std::variant<EditorLayer>;
        Scoped<LayerTypes> mActiveLayer;
    };
}

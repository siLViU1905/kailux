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
        auto& GetLayer()
        {
            return std::get<Layer>(*mActiveLayer);
        }

        template<typename Layer>
        const auto& GetLayer() const
        {
            return std::get<Layer>(*mActiveLayer);
        }

        void Render(Scene& scene) const;

        void OnEvent(const Event& event);
        void Update() const;

    private:
        void CreateLayers(ImTextureID directoryTextureId, ImTextureID fileTextureId);

        using LayerTypes = std::variant<EditorLayer>;
        Scoped<LayerTypes> mActiveLayer;
    };
}

#pragma once
#include "Panel.h"
#include "core/components/entt/MaterialComponent.h"
#include "core/components/entt/MeshComponent.h"

namespace kailux
{
    class HierarchyPanel : public Panel
    {
    public:
        HierarchyPanel();
        HierarchyPanel(std::string_view name, ImVec2 position, ImVec2 size, ImVec4 backgroundColor);

        void render(Scene &scene) override;

        using OnEntitySelected = std::move_only_function<void(entt::entity, const Scene&)>;
        void  setOnEntitySelected(OnEntitySelected&& callback);

        using OnEntityDeleted = std::move_only_function<void(MeshComponent meshComponent, MaterialComponent materialComponent)>;
        void  setOnEntityDeleted(OnEntityDeleted&& callback);

        using OnDragDrop = std::move_only_function<void(std::string_view path)>;
        void  setOnDragDrop(OnDragDrop&& callback);

    private:
        static bool on_entity_rename(entt::entity entity, entt::registry &registry);
        static bool on_entity_delete(entt::entity entity, Scene &scene);

        OnEntitySelected m_OnEntitySelected;
        OnEntityDeleted  m_OnEntityDeleted;
        OnDragDrop       m_OnDragDrop;
        entt::entity     m_SelectedEntity;
    };
}

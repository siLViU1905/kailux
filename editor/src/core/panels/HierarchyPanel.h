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
        HierarchyPanel(std::string_view name, ImVec4 backgroundColor);

        void render(Scene &scene) override;

        using OnEntitySelected = std::move_only_function<void(entt::entity, const Scene&)>;
        void  setOnEntitySelected(OnEntitySelected&& callback);

        using OnEntityDeleted = std::move_only_function<void(MeshComponent meshComponent, MaterialComponent materialComponent)>;
        void  setOnEntityDeleted(OnEntityDeleted&& callback);

        using OnDragDrop = std::move_only_function<void(std::string_view path)>;
        void  setOnDragDrop(OnDragDrop&& callback);

        using OnNewMesh = std::move_only_function<void(MeshType)>;
        void  setOnNewMesh(OnNewMesh&& callback);

        void         selectEntity(entt::entity entity);
        entt::entity getSelectedEntity() const;

    private:
        static bool on_entity_rename(entt::registry &registry, entt::entity entity);
        static bool on_entity_delete(Scene &scene, entt::entity entity);
        static void on_hierarchy_delete(entt::registry &registry, entt::entity entity);

        void renderEntityNode(entt::entity entity, Scene& scene);

        OnEntitySelected m_OnEntitySelected;
        OnEntityDeleted  m_OnEntityDeleted;
        OnDragDrop       m_OnDragDrop;
        OnNewMesh        m_OnNewMesh;
        entt::entity     m_SelectedEntity;
        entt::entity     m_LastSelectedEntity{entt::null};
    };
}

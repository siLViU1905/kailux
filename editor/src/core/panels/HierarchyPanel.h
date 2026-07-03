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

        using OnEntityDeleted = std::move_only_function<void(MeshComponent, MaterialComponent, std::string_view)>;
        void  setOnEntityDeleted(OnEntityDeleted&& callback);

        using OnDragDrop = std::move_only_function<void(std::string_view)>;
        void  setOnDragDrop(OnDragDrop&& callback);

        using OnNewMesh = std::move_only_function<void(MeshType)>;
        void  setOnNewMesh(OnNewMesh&& callback);

        using OnAddPhysics = std::move_only_function<void(entt::entity, PhysicsBodyType, bool)>;
        void setOnAddPhysics(OnAddPhysics&& callback);

        void         selectEntity(entt::entity entity);
        entt::entity getSelectedEntity() const;

        friend class EntityEditorPanel;

    private:
        static constexpr std::string_view s_BodyTypeOptions = {
            "Static\0"
            "Kinematic\0"
            "Dynamic\0",
            sizeof("Static\0Dynamic\0Kinematic\0")
        };

        static bool on_entity_rename(entt::registry &registry, entt::entity entity);
        static bool on_entity_delete(const Scene &scene, entt::entity entity);

        static bool can_attach_physics(const entt::registry &registry, entt::entity entity);

        void notifyAndDestroyHierarchy(entt::registry& registry, entt::entity entity);

        void renderEntityNode(Scene& scene, entt::entity entity);
        void renderAddPhysicsPopup(const Scene &scene);

        OnEntitySelected m_OnEntitySelected;
        OnEntityDeleted  m_OnEntityDeleted;
        OnDragDrop       m_OnDragDrop;
        OnNewMesh        m_OnNewMesh;
        OnAddPhysics     m_OnAddPhysics;
        entt::entity     m_SelectedEntity;
        entt::entity     m_LastSelectedEntity{entt::null};

        bool         m_OpenPhysicsPopup{};
        entt::entity m_PhysicsTargetEntity{entt::null};

        int          m_PhysicsBodyType{};
        bool         m_PhysicsCanBecomeDynamic{true};
    };
}

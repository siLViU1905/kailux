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

        void Render(Scene &scene) override;

        using OnEntitySelected = std::move_only_function<void(entt::entity, const Scene&)>;
        void  SetOnEntitySelected(OnEntitySelected&& callback);

        using OnMeshDeleted = std::move_only_function<void(MeshComponent, std::string_view)>;
        void  SetOnMeshDeleted(OnMeshDeleted&& callback);

        using OnDragDrop = std::move_only_function<void(std::string_view)>;
        void  SetOnDragDrop(OnDragDrop&& callback);

        using OnNewMesh = std::move_only_function<void(MeshType)>;
        void  SetOnNewMesh(OnNewMesh&& callback);

        using OnNewLight = std::move_only_function<void(LightType)>;
        void  SetOnNewLight(OnNewLight&& callback);

        using OnAddPhysics = std::move_only_function<void(entt::entity, PhysicsBodyType, bool)>;
        void SetOnAddPhysics(OnAddPhysics&& callback);

        void         SelectEntity(entt::entity entity);
        entt::entity GetSelectedEntity() const;

        void DeleteSelectedEntity();

        friend class EntityEditorPanel;

    private:
        static constexpr std::string_view s_BodyTypeOptions = {
            "Static\0"
            "Kinematic\0"
            "Dynamic\0",
            sizeof("Static\0Dynamic\0Kinematic\0")
        };

        static bool on_entity_rename(entt::registry &registry, entt::entity entity);

        static bool can_delete_entity(const Scene& scene, entt::entity entity);

        static bool can_attach_physics(const entt::registry &registry, entt::entity entity);

        void OnEntityDelete(Scene &scene, entt::entity entity);

        void NotifyAndDestroyHierarchy(entt::registry& registry, entt::entity entity);

        void RenderEntityNode(Scene& scene, entt::entity entity);
        void RenderAddPhysicsPopup(const Scene &scene);

        OnEntitySelected mOnEntitySelected;
        OnMeshDeleted  mOnEntityDeleted;
        OnDragDrop       mOnDragDrop;
        OnNewMesh        mOnNewMesh;
        OnNewLight       mOnNewLight;
        OnAddPhysics     mOnAddPhysics;
        entt::entity     mSelectedEntity;
        entt::entity     mLastSelectedEntity{entt::null};
        entt::entity     mPendingDeleteEntity{entt::null};

        bool         mOpenPhysicsPopup{};
        entt::entity mPhysicsTargetEntity{entt::null};

        int          mPhysicsBodyType{};
        bool         mPhysicsCanBecomeDynamic{true};
    };
}

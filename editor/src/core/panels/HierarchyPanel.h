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

        using OnMeshDeleted = std::move_only_function<void(MeshComponent, std::string_view)>;
        void  setOnMeshDeleted(OnMeshDeleted&& callback);

        using OnDragDrop = std::move_only_function<void(std::string_view)>;
        void  setOnDragDrop(OnDragDrop&& callback);

        using OnNewMesh = std::move_only_function<void(MeshType)>;
        void  setOnNewMesh(OnNewMesh&& callback);

        using OnNewLight = std::move_only_function<void(LightType)>;
        void  setOnNewLight(OnNewLight&& callback);

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

        OnEntitySelected mOnEntitySelected;
        OnMeshDeleted  mOnEntityDeleted;
        OnDragDrop       mOnDragDrop;
        OnNewMesh        mOnNewMesh;
        OnNewLight       mOnNewLight;
        OnAddPhysics     mOnAddPhysics;
        entt::entity     mSelectedEntity;
        entt::entity     mLastSelectedEntity{entt::null};

        bool         mOpenPhysicsPopup{};
        entt::entity mPhysicsTargetEntity{entt::null};

        int          mPhysicsBodyType{};
        bool         mPhysicsCanBecomeDynamic{true};
    };
}

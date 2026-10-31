#pragma once
#include "Panel.h"
#include "components/entt/MaterialComponent.h"
#include "components/entt/MeshComponent.h"

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

        using OnDeleteEntity = std::move_only_function<void(entt::entity)>;
        void  SetOnDeleteEntity(OnDeleteEntity&& callback);

        using OnRenameEntity = std::move_only_function<void(entt::entity, std::string_view)>;
        void  SetOnRenameEntity(OnRenameEntity&& callback);

        using OnDragDrop = std::move_only_function<void(const std::filesystem::path&)>;
        void  SetOnDragDrop(OnDragDrop&& callback);

        using OnNewMesh = std::move_only_function<void(MeshType)>;
        void  SetOnNewMesh(OnNewMesh&& callback);

        using OnNewLight = std::move_only_function<void(LightType)>;
        void  SetOnNewLight(OnNewLight&& callback);

        using OnNewCamera = std::move_only_function<void()>;
        void  SetOnNewCamera(OnNewCamera&& callback);

        using OnAddPhysics = std::move_only_function<void(entt::entity, PhysicsBodyType, bool, float)>;
        void SetOnAddPhysics(OnAddPhysics&& callback);

        using OnNewAudioSource = std::move_only_function<void()>;
        void  SetOnNewAudioSource(OnNewAudioSource&& callback);

        using OnAddAudioSource = std::move_only_function<void(entt::entity)>;
        void  SetOnAddAudioSource(OnAddAudioSource&& callback);

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

        static bool can_delete_entity(const Scene& scene, entt::entity entity);

        static bool can_attach_physics(const entt::registry &registry, entt::entity entity);
        static bool can_attach_audio(const Scene& scene, entt::entity entity);

        void ValidateSelection(const Scene &scene);
        void RenderRenamePopup(const entt::registry &registry, entt::entity entity);

        void RenderEntityNode(Scene& scene, entt::entity entity);
        void RenderAddPhysicsPopup(const Scene &scene);

        OnEntitySelected mOnEntitySelected;
        OnDeleteEntity   mOnDeleteEntity;
        OnRenameEntity   mOnRenameEntity;
        OnDragDrop       mOnDragDrop;
        OnNewMesh        mOnNewMesh;
        OnNewLight       mOnNewLight;
        OnNewCamera      mOnNewCamera;
        OnAddPhysics     mOnAddPhysics;
        OnNewAudioSource mOnNewAudioSource;
        OnAddAudioSource mOnAddAudioSource;
        entt::entity     mSelectedEntity;
        entt::entity     mLastSelectedEntity{entt::null};
        entt::entity     mPendingDeleteEntity{entt::null};

        std::string      mRenameBuffer;
        entt::entity     mRenameTarget{entt::null};
        bool             mRenameNameExists{};

        bool         mOpenPhysicsPopup{};
        entt::entity mPhysicsTargetEntity{entt::null};

        int          mPhysicsBodyType{};
        bool         mPhysicsCanBecomeDynamic{true};
        float        mPhysicsMass{};
    };
}

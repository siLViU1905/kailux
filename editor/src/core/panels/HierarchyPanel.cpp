#include "HierarchyPanel.h"

#include <imgui_internal.h>

#include "core/components/entt/CachedPhysicsData.h"
#include "core/components/entt/HierarchyComponent.h"
#include "core/components/entt/MeshComponent.h"
#include "core/components/entt/PhysicsComponent.h"
#include "core/components/entt/TagComponent.h"
#include "core/components/gpu/TransformComponent.h"
#include "project_panel/ProjectPanel.h"

namespace kailux
{
    HierarchyPanel::HierarchyPanel() : mSelectedEntity(entt::null)
    {
    }

    HierarchyPanel::HierarchyPanel(std::string_view name, ImVec4 backgroundColor)
        : Panel(name, backgroundColor), mSelectedEntity(entt::null)
    {
    }

    void HierarchyPanel::Render(Scene &scene)
    {
        if (mSelectedEntity != mLastSelectedEntity)
        {
            mLastSelectedEntity = mSelectedEntity;
            mOnEntitySelected(mSelectedEntity, scene);
        }

        ImGui::PushStyleColor(ImGuiCol_WindowBg, mBackgroundColor);

        const bool visible{ImGui::Begin(mName.c_str(), &mOpen)};
        mFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
        if (visible)
        {
            auto &registry = scene.GetEntityRegistry();

            auto view = registry.view<TagComponent>();
            for (auto entity: view)
            {
                auto *hierarchy = registry.try_get<HierarchyComponent>(entity);

                if (!hierarchy || hierarchy->parent == entt::null)
                    RenderEntityNode(scene, entity);
            }

            if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered())
            {
                mSelectedEntity = entt::null;
                if (mOnEntitySelected)
                    mOnEntitySelected(mSelectedEntity, scene);
            }

            if (ImGui::BeginPopupContextWindow("##hierarchy_options",
                                               ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
            {
                if (ImGui::BeginMenu("New"))
                {
                    if (ImGui::BeginMenu("Mesh"))
                    {
                        if (ImGui::MenuItem("Cube"))
                            mOnNewMesh(MeshType::Cube);
                        if (ImGui::MenuItem("Sphere"))
                            mOnNewMesh(MeshType::Sphere);

                        ImGui::EndMenu();
                    }

                    if (ImGui::BeginMenu("Light"))
                    {
                        if (ImGui::MenuItem("Point"))
                            mOnNewLight(LightType::Point);
                        ImGui::EndMenu();
                    }

                    if (ImGui::MenuItem("Camera"))
                        mOnNewCamera();

                    ImGui::EndMenu();
                }

                ImGui::EndPopup();
            }
        }


        if (ImGui::BeginDragDropTargetCustom(
            ImGui::GetCurrentWindow()->InnerRect,
            ImGui::GetID(mName.c_str())
        ))
        {
            if (const auto *payload = ImGui::AcceptDragDropPayload(AssetBrowser::s_DragDropPayloadType.data()))
            {
                std::string path = static_cast<const char *>(payload->Data);
                mOnDragDrop(path);
            }
            ImGui::EndDragDropTarget();
        }

        if (mOpenPhysicsPopup)
        {
            ImGui::OpenPopup("Add Physics##add_physics_popup");
            mOpenPhysicsPopup = false;
        }
        RenderAddPhysicsPopup(scene);

        ImGui::End();
        ImGui::PopStyleColor();

        if (can_delete_entity(scene, mPendingDeleteEntity))
            OnEntityDelete(scene, mPendingDeleteEntity);
        mPendingDeleteEntity = entt::null;
    }

    void HierarchyPanel::SetOnEntitySelected(OnEntitySelected &&callback)
    {
        mOnEntitySelected = std::move(callback);
    }

    void HierarchyPanel::SetOnMeshDeleted(OnMeshDeleted &&callback)
    {
        mOnEntityDeleted = std::move(callback);
    }

    void HierarchyPanel::SetOnDragDrop(OnDragDrop &&callback)
    {
        mOnDragDrop = std::move(callback);
    }

    void HierarchyPanel::SetOnNewMesh(OnNewMesh &&callback)
    {
        mOnNewMesh = std::move(callback);
    }

    void HierarchyPanel::SetOnNewLight(OnNewLight &&callback)
    {
        mOnNewLight = std::move(callback);
    }

    void HierarchyPanel::SetOnNewCamera(OnNewCamera &&callback)
    {
        mOnNewCamera = std::move(callback);
    }

    void HierarchyPanel::SetOnAddPhysics(OnAddPhysics &&callback)
    {
        mOnAddPhysics = std::move(callback);
    }

    void HierarchyPanel::SelectEntity(entt::entity entity)
    {
        mSelectedEntity = entity;
    }

    entt::entity HierarchyPanel::GetSelectedEntity() const
    {
        return mSelectedEntity;
    }

    void HierarchyPanel::DeleteSelectedEntity()
    {
        mPendingDeleteEntity = mSelectedEntity;
    }

    bool HierarchyPanel::on_entity_rename(entt::registry &registry, entt::entity entity)
    {
        static char nameBuffer[64]{};
        static bool nameExistsError = false;

        if (ImGui::IsWindowAppearing())
        {
            nameExistsError = false;
            std::strncpy(nameBuffer, registry.get<TagComponent>(entity).name.c_str(), sizeof(nameBuffer) - 1);
            nameBuffer[sizeof(nameBuffer) - 1] = 0;
        }

        ImGui::Text("Rename Entity");

        if (ImGui::InputText("##rename", nameBuffer, sizeof(nameBuffer),
                             ImGuiInputTextFlags_EnterReturnsTrue))
        {
            std::string newName(nameBuffer);

            bool foundDuplicate = false;
            auto view = registry.view<TagComponent>();
            for (auto otherEntity: view)
            {
                if (otherEntity != entity &&
                    registry.get<TagComponent>(otherEntity).name == newName)
                {
                    foundDuplicate = true;
                    break;
                }
            }

            if (!newName.empty() && !foundDuplicate)
            {
                registry.get<TagComponent>(entity).name = newName;
                nameExistsError = false;
                ImGui::CloseCurrentPopup();
            } else
                nameExistsError = foundDuplicate;
        }
        return nameExistsError;
    }

    bool HierarchyPanel::can_delete_entity(const Scene &scene, entt::entity entity)
    {
        return scene.GetEntityRegistry().valid(entity) &&
               entity != scene.GetSun() &&
               entity != scene.GetSceneCamera() &&
               entity != scene.GetSimulationCamera();
    }

    void HierarchyPanel::OnEntityDelete(Scene &scene, entt::entity entity)
    {
        NotifyAndDestroyHierarchy(scene.GetEntityRegistry(), entity);

        if (mSelectedEntity == entity)
        {
            mSelectedEntity = entt::null;
            mLastSelectedEntity = entt::null;
            mPendingDeleteEntity = entt::null;
            mOnEntitySelected(mSelectedEntity, scene);
        }
    }

    bool HierarchyPanel::can_attach_physics(const entt::registry &registry, entt::entity entity)
    {
        if (registry.all_of<PhysicsComponent>(entity))
            return false;
        if (!registry.all_of<MeshComponent>(entity) && !registry.all_of<CachedPhysicsData>(entity))
            return false;

        const auto *h = registry.try_get<HierarchyComponent>(entity);
        return !h || h->parent == entt::null;
    }

    void HierarchyPanel::NotifyAndDestroyHierarchy(entt::registry &registry, entt::entity entity)
    {
        const auto *hierarchy = registry.try_get<HierarchyComponent>(entity);

        if (const auto *meshComponent = registry.try_get<MeshComponent>(entity))
        {
            std::string_view path;
            uint32_t submeshIndex{~0u};

            if (const auto *source = registry.try_get<MeshSourceComponent>(entity))
                path = source->path;
            else if (hierarchy && hierarchy->parent != entt::null)
            {
                if (const auto *parentSource = registry.try_get<MeshSourceComponent>(hierarchy->parent))
                    path = parentSource->path;

                if (auto *parentHierarchy = registry.try_get<HierarchyComponent>(hierarchy->parent))
                {
                    const auto it = std::ranges::find(parentHierarchy->children, entity);
                    if (it != parentHierarchy->children.end())
                        submeshIndex = static_cast<uint32_t>(
                            std::distance(parentHierarchy->children.begin(), it));
                }
            }

            if (!path.empty() && submeshIndex != ~0u)
                mOnEntityDeleted(*meshComponent, std::format("{}_sub{}", path, submeshIndex));
        }

        if (hierarchy)
            for (auto child: hierarchy->children)
                if (registry.valid(child))
                    NotifyAndDestroyHierarchy(registry, child);

        registry.destroy(entity);
    }

    void HierarchyPanel::RenderEntityNode(Scene &scene, entt::entity entity)
    {
        auto &registry = scene.GetEntityRegistry();
        const auto &tag = registry.get<TagComponent>(entity);
        auto *hierarchy = registry.try_get<HierarchyComponent>(entity);

        ImGuiTreeNodeFlags flags = ((mSelectedEntity == entity) ? ImGuiTreeNodeFlags_Selected : 0);
        flags |= ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

        if (!hierarchy || hierarchy->children.empty())
            flags |= ImGuiTreeNodeFlags_Leaf;

        bool opened = ImGui::TreeNodeEx(reinterpret_cast<void *>(static_cast<uintptr_t>(entity)), flags, "%s",
                                        tag.name.c_str());

        if (ImGui::IsItemClicked())
            mSelectedEntity = entity;

        if (ImGui::BeginPopupContextItem())
        {
            static entt::entity lastEntity = entt::null;
            static bool nameExistsError = false;

            if (ImGui::IsWindowAppearing() || lastEntity != entity)
            {
                lastEntity = entity;
                nameExistsError = false;
            }

            nameExistsError = on_entity_rename(registry, entity);

            if (nameExistsError)
                ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Name already exists!");

            ImGui::Separator();

            if (ImGui::MenuItem("Delete", "Del"))
            {
                mPendingDeleteEntity = entity;
                ImGui::CloseCurrentPopup();
            }

            if (can_attach_physics(registry, entity))
            {
                ImGui::Separator();
                if (ImGui::MenuItem("Add physics"))
                {
                    mOpenPhysicsPopup = true;
                    mPhysicsTargetEntity = entity;
                    mPhysicsBodyType = 0;
                    mPhysicsCanBecomeDynamic = true;
                }
            }

            ImGui::EndPopup();
        }

        if (opened)
        {
            if (hierarchy)
                for (auto child: hierarchy->children)
                    if (registry.valid(child))
                        RenderEntityNode(scene, child);

            ImGui::TreePop();
        }
    }

    void HierarchyPanel::RenderAddPhysicsPopup(const Scene &scene)
    {
        ImGui::SetNextWindowSize(ImVec2(300, 0), ImGuiCond_Appearing);
        if (ImGui::BeginPopupModal("Add Physics##add_physics_popup", nullptr,
                                   ImGuiWindowFlags_AlwaysAutoResize))
        {
            const auto &registry = scene.GetEntityRegistry();

            if (mPhysicsTargetEntity == entt::null)
            {
                ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
                return;
            }

            const auto &tag = registry.get<TagComponent>(mPhysicsTargetEntity);
            ImGui::Text("Entity: %s", tag.name.c_str());
            ImGui::Separator();

            ImGui::Combo("Body type", &mPhysicsBodyType, s_BodyTypeOptions.data());

            bool isStatic = (static_cast<PhysicsBodyType>(mPhysicsBodyType) == PhysicsBodyType::Static);
            ImGui::BeginDisabled(!isStatic);
            ImGui::Checkbox("Can become dynamic", &mPhysicsCanBecomeDynamic);
            ImGui::EndDisabled();
            if (ImGui::IsItemHovered() && isStatic)
                ImGui::SetTooltip("If unchecked, a loaded mesh uses a non-convex\n"
                    "collision shape and can't switch to Dynamic later.");

            ImGui::Separator();

            if (ImGui::Button("Add"))
            {
                auto type = static_cast<PhysicsBodyType>(mPhysicsBodyType);
                bool canDyn = isStatic ? mPhysicsCanBecomeDynamic : true;
                mOnAddPhysics(mPhysicsTargetEntity, type, canDyn);
                mPhysicsTargetEntity = entt::null;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel"))
            {
                mPhysicsTargetEntity = entt::null;
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }
}

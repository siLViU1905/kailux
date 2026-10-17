#include "ViewportPanel.h"
#include <ImGuizmo.h>
#include <glm/gtc/type_ptr.hpp>

namespace kailux
{
    ViewportPanel::ViewportPanel() : mSceneTextureId(0), mSimulationState(SimulationState::Paused)
    {
    }

    ViewportPanel::ViewportPanel(std::string_view name) : Panel(name, {}), mSceneTextureId(0), mSimulationState(SimulationState::Paused)
    {
    }

    void ViewportPanel::Render(Scene &scene)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

        const bool wasOpen{mOpen};
        const bool visible{ImGui::Begin(mName.c_str(), &mOpen)};
        if (wasOpen && !mOpen)
        {
            ImGui::End();
            ImGui::PopStyleVar();
            Close();
            return;
        }

        mFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
        mPlatformWindow = static_cast<GLFWwindow*>(ImGui::GetWindowViewport()->PlatformHandle);

        if (visible)
        {
            auto viewportMinRegion = ImGui::GetWindowContentRegionMin();
            auto viewportOffset = ImGui::GetWindowPos();

            ImVec2 minBound = {viewportMinRegion.x + viewportOffset.x, viewportMinRegion.y + viewportOffset.y};
            auto viewportSize = ImGui::GetContentRegionAvail();

            ImGui::Image(mSceneTextureId, viewportSize);

            const bool hovered{ImGui::IsItemHovered()};

            const auto mousePos{ImGui::GetMousePos()};
            const bool overViewGizmo{
                    mousePos.x >= minBound.x + viewportSize.x - kViewGizmoSize &&
                    mousePos.y <= minBound.y + kViewGizmoSize
                };
            if (hovered)
            {
                mMousePos = compute_relative_mouse_pos(minBound, viewportSize, mSceneTextureExtent);
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)
                    && !overViewGizmo)
                    mOnClick();
                else if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
                    mToggleMouseLook = true;
            }

            UpdateEditorCamera(scene, hovered);

            RenderSimulationIndicator(minBound, viewportSize);

            ImGuizmo::SetDrawlist();
            ImGuizmo::SetRect(minBound.x, minBound.y, viewportSize.x, viewportSize.y);

            RenderViewGizmo(scene, minBound, viewportSize);
        }
        ImGui::End();
        ImGui::PopStyleVar();
    }

    void ViewportPanel::SetSceneTexture(ImTextureID id, glm::ivec2 extent)
    {
        mSceneTextureId = id;
        mSceneTextureExtent = extent;
    }

    ViewportPanel::MousePosition ViewportPanel::GetScaledMousePos() const
    {
        return mMousePos;
    }

    void ViewportPanel::SetOnClick(OnClick &&callback)
    {
        mOnClick = std::move(callback);
    }

    SimulationState ViewportPanel::GetSimulationState() const
    {
        return mSimulationState;
    }

    void ViewportPanel::RequestSimulationState(SimulationState state)
    {
        if (mSimulationState == state)
            return;

        if (state == SimulationState::Running)
        {
            if (!mOnSimulationStart || !mOnSimulationStart())
                return;
        }
        else if (mOnSimulationPause)
            mOnSimulationPause();

        mSimulationState = state;
    }

    void ViewportPanel::SetOnSimulationStart(OnSimulationStart &&callback)
    {
        mOnSimulationStart = std::move(callback);
    }

    void ViewportPanel::SetOnSimulationPause(OnSimulationPause &&callback)
    {
        mOnSimulationPause = std::move(callback);
    }

    ViewportPanel::MousePosition ViewportPanel::compute_relative_mouse_pos(ImVec2 minBound, ImVec2 viewportSize, glm::ivec2 textureExtent)
    {
        const auto globalPos{ImGui::GetMousePos()};

        const float relX{globalPos.x - minBound.x};
        const float relY{globalPos.y - minBound.y};

        if (viewportSize.x <= 0.f || viewportSize.y <= 0.f)
            return {};

        const float u{std::clamp(relX / viewportSize.x, 0.f, 1.f)};
        const float v{std::clamp(relY / viewportSize.y, 0.f, 1.f)};

        return {
            static_cast<uint32_t>(u * static_cast<float>(textureExtent.x)),
            static_cast<uint32_t>(v * static_cast<float>(textureExtent.y))
        };
    }

    void ViewportPanel::RenderSimulationIndicator(ImVec2 minBound, ImVec2 viewportSize)
    {
        constexpr float padding = 8.f;
        constexpr ImVec2 size{26.f, 26.f};
        ImVec2 pos = {minBound.x + viewportSize.x * 0.5f - size.x * 0.5f, minBound.y + padding};
        ImDrawList *dl = ImGui::GetWindowDrawList();

        dl->AddRectFilled(
            pos,
            ImVec2(pos.x + size.x, pos.y + size.y),
            IM_COL32(20, 20, 20, 120),
            4.f
        );

        if (mSimulationState == SimulationState::Paused)
        {
            float cx = pos.x + size.x * 0.5f + 1.f;
            float cy = pos.y + size.y * 0.5f;
            constexpr float r = 7.f;
            dl->AddTriangleFilled(
                ImVec2(cx - r * 0.6f, cy - r),
                ImVec2(cx - r * 0.6f, cy + r),
                ImVec2(cx + r, cy),
                IM_COL32(60, 220, 80, 255)
            );
        } else
        {
            float cx = pos.x + size.x * 0.5f;
            float cy = pos.y + size.y * 0.5f;
            constexpr float bw = 3.5f;
            constexpr float bh = 9.f;
            constexpr float gap = 3.f;

            dl->AddRectFilled(
                ImVec2(cx - gap * 0.5f - bw, cy - bh),
                ImVec2(cx - gap * 0.5f, cy + bh),
                IM_COL32(255, 255, 255, 220)
            );
            dl->AddRectFilled(
                ImVec2(cx + gap * 0.5f, cy - bh),
                ImVec2(cx + gap * 0.5f + bw, cy + bh),
                IM_COL32(255, 255, 255, 220)
            );
        }

        ImGui::SetCursorScreenPos(pos);
        if (ImGui::InvisibleButton("##sim_indicator", size))
            RequestSimulationState(mSimulationState == SimulationState::Paused
                                       ? SimulationState::Running
                                       : SimulationState::Paused);

        if (ImGui::IsItemHovered())
        {
            dl->AddRectFilled(
                pos,
                ImVec2(pos.x + size.x, pos.y + size.y),
                IM_COL32(255, 255, 255, 25),
                4.f
            );
            dl->AddRect(
                pos,
                ImVec2(pos.x + size.x, pos.y + size.y),
                IM_COL32(255, 255, 255, 120),
                4.f,
                1.f
            );
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        }
    }

    void ViewportPanel::UpdateEditorCamera(Scene &scene, bool hovered)
    {
        const auto entity{scene.GetSceneCamera()};
        auto* camera{scene.GetEntityRegistry().try_get<CameraComponent>(entity)};
        if (!camera)
            return;

        auto target{camera->position + camera->forward * mOrbitDistance};

        const auto& io{ImGui::GetIO()};
        bool changed{};

        if (hovered && io.MouseWheel != 0.f)
        {
            mOrbitDistance = std::clamp(
                mOrbitDistance * std::pow(0.88f, io.MouseWheel),
                kMinOrbitDistance,
                kMaxOrbitDistance
            );
            changed = true;
        }

        if ((hovered || mFocused) && ImGui::IsMouseDragging(ImGuiMouseButton_Middle, 0.f))
        {
            const auto delta{io.MouseDelta};
            if (io.KeyShift)
            {
                const auto up{glm::cross(camera->right, camera->forward)};
                const float k{mOrbitDistance * 0.0015f};
                target += (-camera->right * delta.x + up * delta.y) * k;
            }
            else
            {
                camera->yaw += delta.x * 0.3f;
                camera->pitch = std::clamp(
                    camera->pitch - delta.y * 0.3f,
                    -89.9f,
                    89.9f
                );
            }
            changed = true;
        }

        if (!changed)
            return;

        apply_orbit(*camera, target, mOrbitDistance);
        scene.SyncCameraTransform(entity);
    }

    void ViewportPanel::RenderViewGizmo(Scene &scene, ImVec2 minBound, ImVec2 viewportSize) const
    {
        const auto entity{scene.GetSceneCamera()};
        auto *camera{scene.GetEntityRegistry().try_get<CameraComponent>(entity)};
        if (!camera)
            return;

        auto view{glm::lookAt(camera->position, camera->position + camera->forward, camera->up)};
        ImGuizmo::ViewManipulate(
            glm::value_ptr(view),
            mOrbitDistance,
            {minBound.x + viewportSize.x - kViewGizmoSize, minBound.y},
            {kViewGizmoSize, kViewGizmoSize},
            0x10101010
        );

        const auto inverse{glm::inverse(view)};
        const glm::vec3 position{inverse[3]};
        const auto forward{-glm::normalize(glm::vec3{inverse[2]})};

        camera->pitch = std::clamp(
            glm::degrees(
                std::asin(forward.y)
            ),
            -89.9f,
            89.9f
        );
        camera->yaw = glm::degrees(std::atan2(forward.z, forward.x));

        apply_orbit(*camera, position + forward * mOrbitDistance, mOrbitDistance);
        scene.SyncCameraTransform(entity);
    }

    void ViewportPanel::apply_orbit(CameraComponent &camera, glm::vec3 target, float distance)
    {
        const float yaw{glm::radians(camera.yaw)};
        const float pitch{glm::radians(camera.pitch)};

        camera.forward = glm::normalize(glm::vec3{
            std::cos(yaw) * std::cos(pitch),
            std::sin(pitch),
            std::sin(yaw) * std::cos(pitch)
        });
        camera.right = glm::normalize(glm::cross(camera.forward, camera.up));
        camera.position = target - camera.forward * distance;
    }
}

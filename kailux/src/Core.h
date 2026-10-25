#pragma once

#define KAILUX_DECLARE_NON_COPYABLE_MOVABLE(ClassName) \
    ClassName(); \
    ClassName(const ClassName&) = delete; \
    ClassName& operator=(const ClassName&) = delete; \
    ClassName(ClassName&& other) noexcept; \
    ClassName& operator=(ClassName&& other) noexcept;

#define KAILUX_CHECK_DATA_STRUCTURE_SIZE(ClassName) \
    static_assert(sizeof(ClassName) % 16 == 0);

namespace kailux
{
    enum class MeshType : uint8_t
    {
        Cube,
        Sphere,
        Loaded,
        Unknown
    };

    enum class LightType : uint8_t
    {
        Point,
        Unknown
    };

    enum class SimulationState : uint8_t
    {
        Paused,
        Running
    };

    enum class CullingPreset : uint8_t
    {
        SceneView,
        SimulationView
    };

    enum class PhysicsBodyType : uint8_t;
}

namespace kailux
{
    struct Handle
    {
        static constexpr uint32_t kInvalidIndex = ~0u;

        uint32_t index = kInvalidIndex;

        constexpr bool Valid() const { return index != kInvalidIndex; }
    };
}

namespace kailux
{
    template<typename T>
    using Scoped = std::unique_ptr<T>;
    template<typename T>
    using Shared = std::shared_ptr<T>;

    template<typename T, typename... Args>
    constexpr Scoped<T> create_scoped(Args&&... args)
    {
        return std::make_unique<T>(std::forward<Args>(args)...);
    }
    template<typename T, typename... Args>
    constexpr Shared<T> create_shared(Args&&... args)
    {
        return std::make_shared<T>(std::forward<Args>(args)...);
    }

    template<class... Overloads>
    struct VisitOverloads : Overloads...
    {
        using Overloads::operator()...;
    };
}

namespace kailux::details
{
    enum class CompileLevel : uint8_t
    {
        Debug,
        Release
    };

#ifndef NDEBUG
    constexpr auto kCompiledLevel{CompileLevel::Debug};
#else
    constexpr auto kCompiledLevel{CompileLevel::Release};
#endif
}

namespace kailux::details
{
    constexpr uint32_t kFramesInFlight{2};

    constexpr uint32_t kMaxMeshes{10'000};
    constexpr uint32_t kMaxGeometryLods{5};
    constexpr uint32_t kMinLodIndices{1500};
    constexpr uint32_t kMaxTextures{4'096};
    constexpr uint32_t kMaxMaterials{256};
    constexpr uint32_t kMaxPointLights{16};
    constexpr uint32_t kMaxCameras{4};

    constexpr uint32_t kSceneViewCameraIndex{0};
    constexpr uint32_t kSimulationViewCameraIndex{1};
    constexpr uint32_t kMaxCameraViews{2};

    constexpr uint32_t kShadowCascadeCount{4};
    constexpr uint32_t kShadowMapResolution{2048};

    constexpr uint32_t kPointShadowResolution{512};
    constexpr uint32_t kPointShadowFaceCount{6};
    constexpr uint32_t kMaxPointShadows{4};
    constexpr uint32_t kPointShadowDrawLists{kMaxCameraViews * kMaxPointShadows * kPointShadowFaceCount};

    constexpr uint32_t point_shadow_draw_list(uint32_t view, uint32_t slot, uint32_t face)
    {
        return (view * kMaxPointShadows + slot) * kPointShadowFaceCount + face;
    }

    constexpr uint32_t kMaxTextureDimension{512};

    //512x512 RGBA8, a full mip chain adds ~1/3 on top of mip 0 -> ~1.33 MiB
    constexpr uint64_t kTextureBytesPerPixel{4};
    constexpr uint64_t kMaxTextureBytes{
        static_cast<uint64_t>(kMaxTextureDimension) * kMaxTextureDimension * kTextureBytesPerPixel * 4 / 3
    };

    //worst case: every material has all 5 maps
    constexpr uint64_t kTexturesPerMaterial{5};
    constexpr uint64_t kTexturePoolMaterialBudget{128};

    constexpr uint64_t kTexturePoolSize{kMaxTextureBytes * kTexturesPerMaterial * kTexturePoolMaterialBudget};
}

namespace kailux::details
{
    static constexpr std::string_view kWorkspaceDefaultPath{"workspace"};
}

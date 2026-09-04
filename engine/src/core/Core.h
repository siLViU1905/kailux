#pragma once

#define KAILUX_DECLARE_NON_COPYABLE_MOVABLE(ClassName) \
    ClassName(); \
    ClassName(const ClassName&) = delete; \
    ClassName& operator=(const ClassName&) = delete; \
    ClassName(ClassName&& other) noexcept; \
    ClassName& operator=(ClassName&& other) noexcept;

#define KAILUX_DECLARE_SINGLETON(ClassName) \
    ClassName() = delete; \
    ClassName(const ClassName&) = delete; \
    ClassName& operator=(const ClassName&) = delete; \
    ClassName(ClassName&& other) noexcept = delete; \
    ClassName& operator=(ClassName&& other) noexcept = delete; \
    static Shared<ClassName> get();

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

    constexpr uint32_t kMaxMeshes{1'000};
    constexpr uint32_t kMaxTextures{4'096};
    constexpr uint32_t kMaxMaterials{kMaxMeshes};
    constexpr uint32_t kMaxPointLights{16};
    constexpr uint32_t kMaxCameras{4};

    constexpr uint32_t kSceneCameraIndex{0};
    constexpr uint32_t kSimulationCameraIndex{1};
}

namespace kailux::details
{
    static constexpr std::string_view kWorkspaceDefaultPath{"workspace"};
}

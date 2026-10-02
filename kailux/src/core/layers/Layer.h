#pragma once
#include <string>
#include <string_view>

#include "core/window/Event.h"

namespace kailux
{
    class Scene;

    class Layer
    {
    public:
        explicit Layer(std::string_view name = "Layer");

        Layer(const Layer&) = delete;
        Layer& operator=(const Layer&) = delete;
        Layer(Layer&&) = delete;
        Layer& operator=(Layer&&) = delete;

        virtual void OnAttach();
        virtual void OnDetach();
        virtual void OnUpdate(float deltaTime);

        virtual void OnImGuiRender(Scene& scene);

        virtual bool OnEvent(const Event& event);

        std::string_view GetName() const;

        virtual ~Layer() = default;

    private:
        std::string mName;
    };
}
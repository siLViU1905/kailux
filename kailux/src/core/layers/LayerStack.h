#pragma once
#include <vector>

#include "Layer.h"
#include "core/Core.h"

namespace kailux
{
    class LayerStack
    {
    public:
        LayerStack();

        LayerStack(const LayerStack&) = delete;
        LayerStack& operator=(const LayerStack&) = delete;

        Layer& PushLayer(Scoped<Layer>&& layer);
        Layer& PushOverlay(Scoped<Layer>&& layer);

        bool PopLayer(const Layer &layer);
        bool PopOverlay(const Layer &layer);

        void Clear();

        auto begin()        { return mLayers.begin(); }
        auto end()          { return mLayers.end(); }
        auto rbegin()       { return mLayers.rbegin(); }
        auto rend()         { return mLayers.rend(); }
        auto begin()  const { return mLayers.begin(); }
        auto end()    const { return mLayers.end(); }

        ~LayerStack();

    private:
        std::vector<Scoped<Layer>> mLayers;
        size_t                     mInsertIndex{};
    };
}
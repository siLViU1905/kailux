#include "LayerStack.h"

namespace kailux
{
    LayerStack::LayerStack() = default;

    Layer & LayerStack::PushLayer(Scoped<Layer> &&layer)
    {
        auto& ref{**mLayers.emplace(mLayers.begin() + mInsertIndex, std::move(layer))};
        ++mInsertIndex;
        ref.OnAttach();
        return ref;
    }

    Layer & LayerStack::PushOverlay(Scoped<Layer> &&layer)
    {
        auto& ref{*mLayers.emplace_back(std::move(layer))};
        ref.OnAttach();
        return ref;
    }

    bool LayerStack::PopLayer(const Layer &layer)
    {
        const auto last{mLayers.begin() + mInsertIndex};
        const auto it{std::find_if(mLayers.begin(), last, [&layer](const auto& l)
        {
            return l.get() == &layer;
        })};

        if (it == last)
            return false;

        (*it)->OnDetach();
        mLayers.erase(it);
        --mInsertIndex;
        return true;
    }

    bool LayerStack::PopOverlay(const Layer &layer)
    {
        const auto first{mLayers.begin() + mInsertIndex};
        const auto it{std::find_if(first, mLayers.end(), [&layer](const auto& l)
        {
            return l.get() == &layer;
        })};

        if (it == mLayers.end())
            return false;

        (*it)->OnDetach();
        mLayers.erase(it);
        return true;
    }

    void LayerStack::Clear()
    {
        while (!mLayers.empty())
        {
            mLayers.back()->OnDetach();
            mLayers.pop_back();
        }
        mInsertIndex = 0;
    }

    LayerStack::~LayerStack()
    {
        Clear();
    }
}

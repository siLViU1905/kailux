#include "Script.h"

#include "ScriptContext.h"

namespace kailux
{
    Script::Script() = default;


    Script::Script(Script &&other) noexcept : pContext(std::exchange(other.pContext, nullptr)),
                                              mSelf(std::exchange(other.mSelf, {}))
    {
    }

    Script & Script::operator=(Script &&other) noexcept
    {
        if (this != &other)
        {
            pContext = std::exchange(other.pContext, nullptr);
            mSelf    = std::exchange(other.mSelf, {});
        }
        return *this;
    }

    Script::~Script() = default;

    void Script::OnCreate()
    {
    }

    void Script::OnUpdate(float deltaTime)
    {
    }

    void Script::OnDestroy()
    {
    }

    Entity Script::GetSelf() const
    {
        return mSelf;
    }

    World & Script::GetWorld() const
    {
        return pContext->world;
    }

    const Input & Script::GetInput() const
    {
        return pContext->input;
    }

    const Time & Script::GetTime() const
    {
        return pContext->time;
    }

    void Script::Attach(ScriptContext &context, Entity self)
    {
        pContext = &context;
        mSelf = self;
    }
}

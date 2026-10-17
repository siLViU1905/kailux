#pragma once

namespace kailux
{
    struct HierarchyComponent
    {
        entt::entity              parent{entt::null};
        std::vector<entt::entity> children;
    };
}

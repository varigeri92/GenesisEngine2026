#pragma once

#include "entt/entt.hpp"

namespace gns
{
    using entityHandle = entt::entity;
    inline constexpr entityHandle NullEntity = entt::null;
}

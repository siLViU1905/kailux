#pragma once
#include <glm/detail/type_quat.hpp>

namespace glm
{
    inline void to_json(nlohmann::json &j, const vec3 &v)
    {
        j = {v.x, v.y, v.z};
    }
    inline void to_json(nlohmann::json &j, const vec4 &v)
    {
        j = {v.x, v.y, v.z, v.w};
    }
    inline void to_json(nlohmann::json &j, const quat &q)
    {
        j = {q.x, q.y, q.z, q.w};
    }

    inline void from_json(const nlohmann::json& j, vec3& v)
    {
        j.at(0).get_to(v.x);
        j.at(1).get_to(v.y);
        j.at(2).get_to(v.z);
    }
    inline void from_json(const nlohmann::json& j, vec4& v)
    {
        j.at(0).get_to(v.x);
        j.at(1).get_to(v.y);
        j.at(2).get_to(v.z);
        j.at(3).get_to(v.w);
    }
    inline void from_json(const nlohmann::json& j, quat& q)
    {
        j.at(0).get_to(q.x);
        j.at(1).get_to(q.y);
        j.at(2).get_to(q.z);
        j.at(3).get_to(q.w);
    }
}

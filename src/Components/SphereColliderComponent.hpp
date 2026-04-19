#pragma once

#include <Freyr/Freyr.hpp>
#include <glm/glm.hpp>

struct SphereColliderComponent : fr::Component
{
    float     radius;
    glm::vec3 offset;
};

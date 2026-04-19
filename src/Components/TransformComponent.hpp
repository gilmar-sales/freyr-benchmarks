#pragma once

#include <Freyr/Freyr.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

struct TransformComponent : fr::Component
{
    glm::vec3 position;
    glm::quat rotation;
    glm::vec3 scale;

    glm::vec3 GetForwardDirection() const
    {
        return glm::vec3(0, 0, 1) * rotation;
    }

    glm::vec3 GetRightDirection() const
    {
        return glm::vec3(1, 0, 0) * rotation;
    }

    glm::vec3 GetUpDirection() const { return glm::vec3(0, 1, 0) * rotation; }

    glm::mat4 GetModel() const
    {
        const auto translateM = glm::translate(glm::mat4(1), position);
        const auto rotateM    = glm::mat4(glm::inverse(rotation));
        const auto scaleM     = glm::scale(glm::mat4(1), scale);

        return translateM * rotateM * scaleM;
    }
};

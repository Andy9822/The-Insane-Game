#ifndef COLLISION_HPP_INCLUDED
#define COLLISION_HPP_INCLUDED

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

bool isPointInsideBBOX(glm::vec4 &point, glm::vec4 &bbox_min, glm::vec4 &bbox_max);

#endif // COLLISION_HPP_INCLUDED

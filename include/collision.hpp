#ifndef COLLISION_HPP_INCLUDED
#define COLLISION_HPP_INCLUDED

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

bool isPointInsideBBOX(glm::vec4 &point, glm::vec4 &bbox_min, glm::vec4 &bbox_max);
bool areBBOXintersecting(glm::vec4 &bbox_min1, glm::vec4 &bbox_max1, glm::vec4 &bbox_min2, glm::vec4 &bbox_max2);

#endif // COLLISION_HPP_INCLUDED

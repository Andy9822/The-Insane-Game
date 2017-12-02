#include "collision.hpp"

bool isPointInsideBBOX(glm::vec4 &point, glm::vec4 &bbox_min, glm::vec4 &bbox_max){

    return (point.x >= bbox_min.x && point.x <= bbox_max.x) &&
           (point.y >= bbox_min.y && point.y <= bbox_max.y) &&
           (point.z >= bbox_min.z && point.z <= bbox_max.z);

}

#include "collision.hpp"


bool isPointInsideBBOX(glm::vec4 &point, glm::vec4 &bbox_min, glm::vec4 &bbox_max){

    return (point.x >= bbox_min.x && point.x <= bbox_max.x) &&
           (point.y >= bbox_min.y && point.y <= bbox_max.y) &&
           (point.z >= bbox_min.z && point.z <= bbox_max.z);

}

bool areBBOXintersecting(glm::vec4 &bbox_min1, glm::vec4 &bbox_max1, glm::vec4 &bbox_min2, glm::vec4 &bbox_max2){

  return (bbox_min1.x <= bbox_max2.x && bbox_max1.x >= bbox_min2.x) &&
         (bbox_min1.y <= bbox_max2.y && bbox_max1.y >= bbox_min2.y) &&
         (bbox_min1.z <= bbox_max2.z && bbox_max1.z >= bbox_min2.z);

}

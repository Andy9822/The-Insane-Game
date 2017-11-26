#include "arrow.h"
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>


void updateArrow(Arrow *arrow, float timeElapsed){

    arrow->pos += arrow->speed * timeElapsed;
    arrow->speed += glm::vec4(0.0f, -0.02f, 0.0f, 0.0f) * timeElapsed;

}

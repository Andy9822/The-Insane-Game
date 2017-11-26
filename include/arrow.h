#ifndef ARROW_H_INCLUDED
#define ARROW_H_INCLUDED

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
struct Arrow {

    glm::vec4 pos;
    glm::vec4 speed;

    Arrow(glm::vec4 initialPos, glm::vec4 speedI){
        pos = initialPos;
        speed = 3.0f*speedI;
    }

};

void updateArrow(Arrow *arrow, float timeElapsed);


#endif // ARROW_H_INCLUDED

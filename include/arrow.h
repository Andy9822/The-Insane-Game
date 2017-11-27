#ifndef ARROW_H_INCLUDED
#define ARROW_H_INCLUDED

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
struct Arrow {

    glm::vec4 pos;
    glm::vec4 speed;
    float thetaAngle;
    float phiAngle;

    Arrow(glm::vec4 initialPos, glm::vec4 speedI, float theta, float phi){
        pos = initialPos;
        speed = speedI;
        thetaAngle = theta;
        phiAngle = phi;
    }

};

void updateArrow(Arrow *arrow, float timeElapsed);


#endif // ARROW_H_INCLUDED

#ifndef ARROW_H_INCLUDED
#define ARROW_H_INCLUDED

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

enum ArrowType { normal = 0, teleport = 1, plataform = 2};

struct Arrow {

    glm::vec4 pos;
    glm::vec4 speed;
    glm::vec4 w;
    float thetaAngle;
    float phiAngle;
    ArrowType type;

    Arrow(glm::vec4 initialPos, glm::vec4 speedI, float theta, float phi){
        pos = initialPos;
        speed = 0.8f*speedI;
        thetaAngle = theta;
        type = normal;
        phiAngle = phi;
    }

};

void updateArrow(Arrow *arrow, float timeElapsed);


#endif // ARROW_H_INCLUDED

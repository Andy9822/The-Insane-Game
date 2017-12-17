#include "arrow.h"
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

///calcula norma
float norma(glm::vec4 v)
{
    float vx = v.x;
    float vy = v.y;
    float vz = v.z;

    return sqrt( vx*vx + vy*vy + vz*vz );
}

///calcula produto vetorial
glm::vec4 produtovetorial(glm::vec4 u, glm::vec4 v)
{
    float u1 = u.x;
    float u2 = u.y;
    float u3 = u.z;
    float v1 = v.x;
    float v2 = v.y;
    float v3 = v.z;

    return glm::vec4(
        u2*v3 - u3*v2, // Primeiro coeficiente
        u3*v1 - u1*v3, // Segundo coeficiente
        u1*v2 - u2*v1, // Terceiro coeficiente
        0.0f // w = 0 para vetores.
    );
}

///Atualiza posicao da flecha conforme as leis da gravidade
void updateArrow(Arrow *arrow, float timeElapsed){

    arrow->pos += arrow->speed * timeElapsed;
    arrow->speed += glm::vec4(0.0f, -0.1f, 0.0f, 0.0f) * timeElapsed;

}

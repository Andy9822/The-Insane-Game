#ifndef ENEMY_H_INCLUDED
#define ENEMY_H_INCLUDED

#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <iostream>

struct Enemy{
    glm::vec4 pos;
    glm::vec3 scale;
    bool fixed;
    float Y_deviation;
    float rotation_Y;
    bool Y_deviationLimit;
    std::string name;

    Enemy(glm::vec4 p, bool isFixed, std::string enemyName){
        fixed = isFixed;
        rotation_Y = 0;
        Y_deviation = 0;
        Y_deviationLimit = false;
        pos = p;
        scale = glm::vec3(0.25f, 0.25f, 0.25f);
        name = enemyName;
    }
};

void updateEnemy(Enemy *e, glm::vec4 camera_pos, float timeElapsed);


#endif // ENEMY_H_INCLUDED

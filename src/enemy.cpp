#include "enemy.h"
#include <glm/vec4.hpp>
#include <cmath>

void updateEnemy(Enemy *e, glm::vec4 camera_pos, float timeElapsed)
{
    if(!e->fixed)
    {
        glm::vec4 d = camera_pos - e->pos;
        d = d/(float)(sqrt(d.x*d.x + d.y*d.y + d.z*d.z));

        e->pos += d * 0.4f * timeElapsed;
    }

    float angle = atan2((e->pos.x - camera_pos.x), (e->pos.z - camera_pos.z));

    if(e->Y_deviation > 0.25f){
      e->Y_deviationLimit = true;
    }else if(e->Y_deviation < 0.0f){
      e->Y_deviationLimit = false;
    }

    if(!e->Y_deviationLimit){
        e->Y_deviation+=0.05f;
    }else{
        e->Y_deviation-=0.02f;
    }
    e->rotation_Y = angle + 3.14;

}

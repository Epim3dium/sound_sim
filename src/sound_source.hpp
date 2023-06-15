#ifndef EPI_SOUNDSOURCE_H
#define EPI_SOUNDSOURCE_H
#include "sound_grid.hpp"
namespace epi {

struct SoundSource {
    sf::Vector2u pos;
    float initial_mag = 2.f;
    size_t spawn_time = 0;

    float omega = 0.07f;

    #define MIN_ACCEPTABLE 0.05f
    void update(Simulation& sim) {
        AABBu affected;
        float range = std::log2(MIN_ACCEPTABLE/initial_mag)/log2(Simulation::damp_pressure);
        affected.min.x = pos.x - range; 
        affected.min.y = pos.y - range; 
        affected.max.x = pos.x + range; 
        affected.max.y = pos.y + range; 

        sim.affectArea(affected);
        sim.pressure[pos.x + pos.y * sim.size_x] = initial_mag * sin(omega * spawn_time);
        spawn_time ++;
    }
};
}
#endif

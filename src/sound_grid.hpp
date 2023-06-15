#ifndef EPI_SOUNDGRID_H
#define EPI_SOUNDGRID_H
#include "utils.hpp"

#include <cmath>
#include <cstddef>
#include <stack>
#include <thread>
#include <iostream>
#include <algorithm>
#include <vector>
#include <array>
#include <numeric>
//#include <omp.h>

namespace epi {
class Simulation {
public:
    static constexpr float damp_pressure = 0.965f;

    size_t size_x, size_y;
    size_t frame;
    bool* updated;
    std::array<float, 4U>* velocities;
    float* pressure;
    float* wall;

    std::vector<AABBu> regions_to_update;

    Simulation(size_t w, size_t h) : 
        size_x(w), size_y(h),
        pressure(new float[size_y * size_x]), 
        velocities(new std::array<float, 4U>[size_y * size_x]) ,
        wall(new float[size_y * size_x]),
        updated(new bool[size_x * size_y])
    {
        memset(wall, 0, sizeof(float) * size_x * size_y);
        memset(velocities, 0, sizeof(float) * 4 * size_x * size_y);
        memset(pressure, 0, sizeof(float) * size_x * size_y);
        memset(updated, 0, size_x * size_y);

        for(int i = 0; i < size_y; i++) { 
            wall[i * size_x] = 1.f;
            wall[i * size_x + size_x - 1] = 1.f;
        }
        for(int j = 0; j < size_x; j++) { 
            wall[j] = 1.f;
            wall[j + (size_y - 1) * size_x] = 1.f;
        }
        frame = 0;
    }

    void updateV(vec2u min, vec2u max) {
        auto V = velocities;
        auto P = pressure;
#pragma omp parallel for collapse(2)
        for (int i = min.y; i < max.y; i++) {
            for (int j = min.x; j < max.x; j++) {
                if(updated[i * size_x + j])
                    continue;
                updated[i * size_x + j] = true;

                if(wall[i * size_x + j] == 1.f) {
                    V[i*size_x + j][0] = V[i*size_x + j][1] = V[i*size_x + j][2] = V[i*size_x + j][3] = 0.0;
                    continue;
                }
                auto cell_pressure = P[i*size_x + j];
                V[i*size_x + j][0] += cell_pressure - P[(i - 1) * size_x + j] ;
                V[i*size_x + j][1] += cell_pressure - P[(i) * size_x + j + 1] ;

                V[(i - 1) * size_x + j][2] += P[(i - 1) * size_x + j] - cell_pressure;
                V[(i) * size_x + j + 1][3] += P[(i) * size_x + j + 1] - cell_pressure;
            }
        }
    }

    void updateP() {
#pragma omp parallel for
        for (size_t i = 0; i < size_x * size_y; i++) {
            if(!updated[i]) {
                pressure[i] = 0;
                velocities[i][0] = velocities[i][1] = velocities[i][2] = velocities[i][3] = 0.0;
                continue;
            }

            pressure[i] -= 0.5 * (velocities[i][0] + velocities[i][1] + velocities[i][2] + velocities[i][3]);
            pressure[i] *= damp_pressure;
        }
    }

    void clampRegions() {
        for(auto& r : regions_to_update) {
            r.min.x = std::max(r.min.x, 0U);
            r.min.y = std::max(r.min.y, 0U);

            r.max.x = std::min((size_t)r.max.x, size_x);
            r.max.y = std::min((size_t)r.max.y, size_y);
        }
    }
    void step() {
        clampRegions();
        memset(updated, false, size_x * size_y);
        for(auto& r : regions_to_update) {
            updateV(r.min, r.max);
        }
        updateP();
        regions_to_update.clear();
        frame += 1;
    }
    float getMinDev() {
        float r = 0xffffff;
        for (int i = 0; i < size_y; i++) 
            for (int j = 0; j < size_x; j++) 
                r = std::min(r, pressure[i*size_x + j]);
        return r;
    }
    #define AFFECT_AREA_THRESHOLD 0.5
    void affectArea(AABBu aabb) {
        for(auto& r : regions_to_update) {
            if(isIntersecting(r, aabb)) {
                auto area = calcIntersectionArea(r, aabb);
                if(area.x * area.y > aabb.size().x * aabb.size().y * AFFECT_AREA_THRESHOLD) {
                    r.merge(aabb);
                    return;
                }
            }
        }
        regions_to_update.push_back(aabb);
    }
    float getMaxDev() {
        float r = 0.f;
        for (int i = 0; i < size_y; i++) 
            for (int j = 0; j < size_x; j++) 
                r = std::max(r, pressure[i*size_x + j]);
        return r;
    }
    sf::Vector2f getScale(sf::RenderTarget& rw) const {
        return sf::Vector2f(static_cast<float>(rw.getSize().x) / size_x, static_cast<float>(rw.getSize().y) / size_y);
    }
    void draw(sf::RenderTarget& rw, sf::Image& img, float dev_min = -1.f, float dev_max = 1.f) {
        for (int i = 0; i < size_y; i++) {
            for (int j = 0; j < size_x; j++) {
                if(wall[i*size_x + j] == 1.f) {
                    img.setPixel(j, i, sf::Color(0x04522bff));
                }else {
                    img.setPixel(j, i, getSciColor(pressure[i*size_x + j], dev_min, dev_max));
                }
            }
        }
//        for(auto& r : regions_to_update) {
//            for (int i = r.min.y; i < r.max.y; i++) {
//                for (int j = r.min.x; j < r.max.x; j++) {
//                    if(wall[i*size_x + j] == 1.f) {
//                        img.setPixel(sf::Vector2u(j, i), sf::Color(0x04522bff));
//                    }else {
//                        img.setPixel(sf::Vector2u(j, i), getSciColor(pressure[i*size_x + j], dev_min, dev_max));
//                    }
//                }
//            }
//        }
        sf::Texture tex;
        if(!tex.loadFromImage(img)) return;

        sf::Sprite spr(tex);
        sf::Vector2f scale = getScale(rw);
        spr.setScale(scale);

        rw.draw(spr);
    }
};
}

#endif

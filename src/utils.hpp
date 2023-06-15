#ifndef EPI_UTILS_H
#define EPI_UTILS_H
#include "SFML/Graphics.hpp"
#include "SFML/System/Vector2.hpp"
#include "imgui.h"

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

typedef sf::Vector2u vec2u;

sf::Color getSciColor(float val, float minVal, float maxVal) {
    val = std::fminf(std::fmaxf(val, minVal), maxVal- 0.0001);
    float d = maxVal - minVal;
    val =( d == 0.0 ? 0.5 : (val - minVal) / d);
    float m = 0.25;
    int num = floorf(val / m);
    float s = (val - num * m) / m;
    float r, g, b;

    switch (num) {
        case 0 : r = 0.0; g = s; b = 1.0; break;
        case 1 : r = 0.0; g = 0.0; b = 1.0-s; break;
        case 2 : r = s; g = 0.0; b = 0.0; break;
        case 3 : r = 1.0; g = s; b = 0.0; break;
    }

    return sf::Color(255.f*r,255*g,255*b, 255);
}
struct AABBu {
    vec2u min;
    vec2u max;
    vec2u size() const {
        return max - min;
    }
    void merge(AABBu& aabb) {
        min.x = std::min(min.x, aabb.min.x);
        max.x = std::max(max.x, aabb.max.x);
        min.y = std::min(min.y, aabb.min.y);
        max.y = std::max(max.y, aabb.max.y);
    }
};
bool isIntersecting(AABBu a, AABBu b) {
  return
    a.min.x <= b.max.x &&
    a.max.x >= b.min.x &&
    a.min.y <= b.max.y &&
    a.max.y >= b.min.y;
}
vec2u calcIntersectionArea(AABBu a, AABBu b) {
    vec2u r;
    r.x = std::min(b.max.x - a.min.x, a.max.x - b.min.x);
    r.y = std::min(b.max.y - a.min.y, a.max.y - b.min.y);
    return r;
}
};
#endif


#include "SFML/Graphics.hpp"
#include "SFML/Graphics/Image.hpp"
#include "SFML/Graphics/RectangleShape.hpp"
#include "SFML/Graphics/RenderTarget.hpp"
#include "SFML/Graphics/Sprite.hpp"
#include "SFML/Graphics/Texture.hpp"
#include "SFML/System/Vector2.hpp"
#include "SFML/Window/Event.hpp"
#include "SFML/Window/Mouse.hpp"
#include "imgui-SFML.h"
#include "imgui.h"
#include <cstddef>
#include <thread>
#include <iostream>
#include <algorithm>
#include <vector>
#include <array>
#include <numeric>
#include <omp.h>

#define SCALE 1.f
float pixel_size = 10.f;
int size_x = 350.f * SCALE;
int size_y = 200.f * SCALE;
float damping = 1.00;
float damp_pressure = 0.97f;

float initial_P = 200.f;
float max_pressure = initial_P / 2.f;
float min_presure = -initial_P / 2.f;

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

class Simulation {
public:
    size_t n = size_x;
    size_t frame;
    float* pressure;
    std::array<float, 4U>* velocities;
    float* wall;


    Simulation() : 
        pressure(new float[size_y * size_x]), 
        velocities(new std::array<float, 4U>[size_y * size_x]) ,
        wall(new float[size_y * size_x]) 
    {
        memset(wall, 0, sizeof(float) * size_x * size_y);
        memset(velocities, 0, sizeof(float) * 4 * size_x * size_y);
        memset(pressure, 0, sizeof(float) * size_x * size_y);

        for(int i = 0; i < size_y; i++) { 
            wall[i * n] = 1.f;
            wall[i * n + size_x - 1] = 1.f;
        }
        for(int j = 0; j < size_x; j++) { 
            wall[j] = 1.f;
            wall[j + (size_y - 1) * n] = 1.f;
        }
        frame = 0;
    }

    void updateV() {
        auto& V = velocities;
        const auto& P = pressure;
#pragma omp teams distribute parallel for collapse(2)
        for (int i = 0; i < size_y; i++) {
            for (int j = 0; j < size_x; j++) {
                if(wall[i * n + j] == 1.f) {
                    V[i*n + j][0] = V[i*n + j][1] = V[i*n + j][2] = V[i*n + j][3] = 0.0;
                    continue;
                }
                auto cell_pressure = P[i*n + j];
                V[i*n + j][0] += cell_pressure - P[(i - 1) * n + j] ;
                V[i*n + j][1] += cell_pressure - P[(i) * n + j + 1] ;

                V[(i - 1) * n + j][2] += P[(i - 1) * n + j] - cell_pressure;
                V[(i) * n + j + 1][3] += P[(i) * n + j + 1] - cell_pressure;
            }
        }
    }

    void updateP() {
        for (int i = 0; i < size_y; i++) {
            for (int j = 0; j < size_x; j++) {
                pressure[i*n + j] -= 0.5 * (velocities[i*n + j][0] + velocities[i*n + j][1] + velocities[i*n + j][2] + velocities[i*n + j][3]);
                pressure[i*n + j] *= damp_pressure;
            }
        }
    }

    void step() {
        updateV();
        updateP();
        frame += 1;
    }
    float getMinDev() {
        float r = 0xffffff;
        for (int i = 0; i < size_y; i++) 
            for (int j = 0; j < size_x; j++) 
                r = std::min(r, pressure[i*n + j]);
        return r;
    }
    float getMaxDev() {
        float r = 0.f;
        for (int i = 0; i < size_y; i++) 
            for (int j = 0; j < size_x; j++) 
                r = std::max(r, pressure[i*n + j]);
        return r;
    }
    sf::Vector2f getScale(sf::RenderTarget& rw) const {
        return sf::Vector2f(static_cast<float>(rw.getSize().x) / size_x, static_cast<float>(rw.getSize().y) / size_y);
    }
    void draw(sf::RenderTarget& rw, sf::Image& img, float dev_min = -1.f, float dev_max = 1.f) {
        for (int i = 0; i < size_y; i++) {
            for (int j = 0; j < size_x; j++) {
                if(wall[i*n + j] == 1.f) {
                    img.setPixel(sf::Vector2u(j, i), sf::Color::Green);
                }else {
                    img.setPixel(sf::Vector2u(j, i), getSciColor(pressure[i*n + j], dev_min, dev_max));
                }
            }
        }
        sf::Texture tex;
        if(!tex.loadFromImage(img)) return;
        sf::Sprite spr(tex);
        sf::Vector2f scale = getScale(rw);
        spr.setScale(scale);

        rw.draw(spr);
    }
};

struct SoundSource {
    sf::Vector2i pos;
    float initial_mag = 2.f;
    size_t spawn_time = 0;

    float omega = 0.07f;

    void update(Simulation& sim) {
        sim.pressure[pos.x + pos.y * sim.n] = initial_mag * sin(omega * spawn_time);
        spawn_time ++;
    }
};

int main() {
    // Create the main window
    sf::RenderWindow window(sf::VideoMode(sf::Vector2u(size_x * pixel_size, size_y * pixel_size)), "SFML window");
    std::vector<SoundSource> sources;
    Simulation sim;
    sf::Image sim_img;
    sim_img.create(sf::Vector2u(size_x, size_y));
    float default_freq = 0.15f;
    float default_mag = 1.f;
    bool isBuilding = false;
    int step_count = 1;

    if(!ImGui::SFML::Init(window))
        exit(1);
    // Start the game loop
    sf::Clock deltaClock;
    sf::Time delT;
    while (window.isOpen())
    {
        // Process events
        sf::Event event;

        sf::Vector2f mpos( sf::Mouse::getPosition(window));
        mpos.x /= sim.getScale(window).x;
        mpos.y /= sim.getScale(window).y;
        while (window.pollEvent(event))
        {
            ImGui::SFML::ProcessEvent(event);
            // Close window: exit
            if (event.type == sf::Event::Closed)
                window.close();
            if(event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Right) {
                sources.push_back({sf::Vector2i(mpos)});
                sources.back().omega = default_freq;
                sources.back().initial_mag = default_mag;
            }
            if(event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::R) {
                sources.clear();
            }
        }
        delT = deltaClock.restart();
        ImGui::SFML::Update(window, delT);
        if(isBuilding && sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
            sim.wall[(size_t)mpos.x + (size_t)mpos.y * sim.n] = 1.f;
        }
        {
            ImGui::Begin("options");
            ImGui::SliderFloat("change default frquency", &default_freq, 0.f, 0.5f);
            ImGui::SliderInt("change default step_count", &step_count, 1, 5);
            ImGui::SliderFloat("change default magnitude", &default_mag, 0.f, 10.f);
            if(ImGui::Button("build_mode")) {
                isBuilding = !isBuilding;
            }
                ImGui::Text("%f", 1.f / delT.asSeconds());
            ImGui::End();
        }
    
        for(int i = 0; i < step_count; i++) {
            for(auto& s : sources) {
                s.update(sim);
            }
            sim.step();
        }

        window.clear();
        sim.draw(window, sim_img);
        ImGui::SFML::Render(window);
        window.display();
    }
    return EXIT_SUCCESS;
}

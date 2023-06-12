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

float pixel_size = 10.f;
float size_x = 300.f;
float size_y = 200.f;
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
    size_t frame;
    std::vector<std::vector<float>> pressure;
    std::vector<std::vector<std::array<float, 4U> > > velocities;
    std::vector<std::vector<float>> wall;


    Simulation() : 
        pressure(size_y, std::vector<float>(size_x, 0.f)), 
        velocities(size_y, std::vector<std::array<float, 4U> > (size_x) ) ,
        wall(size_y, std::vector<float>(size_x, 0.f)) 
    {
        frame = 0;
    }

    void updateVthreaded(size_t count = 2) {
        sf::Vector2i seg_size(size_x / count, size_y / count);
        std::vector<std::thread> workes;
        for(int i = 0; i < count; i++) {
            for(int j = 0; j < count; j++) {
                auto from = sf::Vector2i(j * seg_size.x, i * seg_size.y);
                auto to = from + seg_size;
                workes.push_back(std::thread(&Simulation::updateV, std::ref(*this), from, to));
            }
        }
        for(auto& w : workes) {
            w.join();
        }
    }
    void updateV(sf::Vector2i from = {0, 0}, sf::Vector2i to = {static_cast<int>(size_x), static_cast<int>(size_y)}) {
        auto& V = velocities;
        const auto& P = pressure;
        for (int i = from.y; i < to.y && i < size_y; i++) {
            for (int j = from.x; j < to.x && j < size_x; j++) {
                if(wall[i][j] == 1.f) {
                    V[i][j][0] = V[i][j][1] = V[i][j][2] = V[i][j][3] = 0.0;
                    continue;
                }
                auto cell_pressure = P[i][j];
                V[i][j][0] = i > 0 ? V[i][j][0] + cell_pressure - P[i - 1][j] : cell_pressure;
                V[i][j][1] = j < size_x - 1 ? V[i][j][1] + cell_pressure - P[i][j + 1] : cell_pressure;
                V[i][j][2] = i < size_y - 1 ? V[i][j][2] + cell_pressure - P[i + 1][j] : cell_pressure;
                V[i][j][3] = j > 0 ? V[i][j][3] + cell_pressure - P[i][j - 1] : cell_pressure;
            }
        }
    }

    void updateP() {
        for (int i = 0; i < size_y; i++) {
            for (int j = 0; j < size_x; j++) {
                pressure[i][j] -= 0.5 * std::reduce(velocities[i][j].begin(), velocities[i][j].end());
                pressure[i][j] *= damp_pressure;
            }
        }
    }

    void step() {
        updateVthreaded(2U);
        updateP();
        frame += 1;
    }
    float getMinDev() {
        float r = 0xffffff;
        for (int i = 0; i < size_y; i++) 
            for (int j = 0; j < size_x; j++) 
                r = std::min(r, pressure[i][j]);
        return r;
    }
    float getMaxDev() {
        float r = 0.f;
        for (int i = 0; i < size_y; i++) 
            for (int j = 0; j < size_x; j++) 
                r = std::max(r, pressure[i][j]);
        return r;
    }
    void draw(sf::RenderTarget& rw, sf::Image& img, float dev_min = -1.f, float dev_max = 1.f) {
        for (int i = 0; i < size_y; i++) {
            for (int j = 0; j < size_x; j++) {
                if(wall[i][j] == 1.f) {
                    img.setPixel(i, j, sf::Color::Green);
                }else {
                    img.setPixel(i, j, getSciColor(pressure[i][j], dev_min, dev_max));
                }
            }
        }
        sf::Texture tex;
        tex.loadFromImage(img);
        sf::Sprite spr;
        spr.setTexture(tex);
        sf::Vector2f scale(rw.getSize().x / size_x, rw.getSize().y / size_y);
        spr.setScale(scale);
        rw.draw(spr);
    }
    sf::Vector2f getScale(sf::RenderTarget& rw) {
        return sf::Vector2f(rw.getSize().x / size_x, rw.getSize().y / size_y);
    }
};

struct SoundSource {
    sf::Vector2i pos;
    float initial_mag = 2.f;
    size_t spawn_time = 0;

    float omega = 0.07f;

    void update(Simulation& sim) {
        sim.pressure[pos.x][pos.y] = initial_mag * sin(omega * spawn_time);
        spawn_time ++;
    }
};

int main() {
    // Create the main window
    sf::RenderWindow window(sf::VideoMode(size_x * pixel_size, size_y * pixel_size), "SFML window");
    std::vector<SoundSource> sources;
    Simulation sim;
    sf::Image sim_img;
    sim_img.create(size_x, size_y);
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
        }
        delT = deltaClock.restart();
        ImGui::SFML::Update(window, delT);
        if(isBuilding && sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
            sim.wall[mpos.x][mpos.y] = 1.f;
        }
        {
            ImGui::Begin("options");
            ImGui::SliderFloat("change default frquency", &default_freq, 0.f, 1.f);
            ImGui::SliderInt("change default step_count", &step_count, 1, 5);
            ImGui::SliderFloat("change default magnitude", &default_mag, 0.f, 2.f);
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

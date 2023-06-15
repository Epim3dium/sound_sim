#include "SFML/Audio/SoundBuffer.hpp"
#include "SFML/Graphics.hpp"
#include "SFML/Audio/Sound.hpp"
#include "SFML/Graphics/BlendMode.hpp"
#include "SFML/Graphics/Image.hpp"
#include "SFML/Graphics/RectangleShape.hpp"
#include "SFML/Graphics/RenderTarget.hpp"
#include "SFML/Graphics/Sprite.hpp"
#include "SFML/Graphics/Texture.hpp"
#include "SFML/System/Vector2.hpp"
#include "SFML/Window/Event.hpp"
#include "SFML/Window/Mouse.hpp"
#include "implot/implot.h"
#include "imgui-SFML.h"
#include "imgui.h"

#include <cmath>
#include <cstddef>
#include <stack>
#include <thread>
#include <iostream>
#include <algorithm>
#include <vector>
#include <stdlib.h>
#include <array>
#include <numeric>
#include "implot/implot.h"
#include "sound_grid.hpp"
#include "sound_source.hpp"
//#include <omp.h>

using namespace epi;

#define SCALE 1.f
#define GW 300
#define GH 170
float pixel_size = 10.f;
float damp_pressure = 0.97f;

float initial_P = 200.f;
float max_pressure = initial_P / 2.f;
float min_presure = -initial_P / 2.f;


struct ScrollingBuffer {
    int MaxSize;
    int Offset;
    ImVector<ImVec2> Data;
    ScrollingBuffer(int max_size = 10000) {
        MaxSize = max_size;
        Offset  = 0;
        Data.reserve(MaxSize);
    }
    void AddPoint(float x, float y) {
        if (Data.size() < MaxSize)
            Data.push_back(ImVec2(x,y));
        else {
            Data[Offset] = ImVec2(x,y);
            Offset =  (Offset + 1) % MaxSize;
        }
    }
    void Erase() {
        if (Data.size() > 0) {
            Data.shrink(0);
            Offset  = 0;
        }
    }
};
void RealtimePlot(std::string title, float data, float minVal = -1.f, float maxVal = 1.f) {
    ImGui::BulletText("Move your mouse to change the data!");
    ImGui::BulletText("This example assumes 60 FPS. Higher FPS requires larger buffer size.");
    static ScrollingBuffer sdata1;
    ImVec2 mouse = ImGui::GetMousePos();
    static float t = 0;
    t += ImGui::GetIO().DeltaTime;
    sdata1.AddPoint(t, data);

    static float history = 10.0f;
    ImGui::SliderFloat("History",&history,1,30,"%.1f s");

    static ImPlotAxisFlags flags = ImPlotAxisFlags_NoTickLabels;

    if (ImPlot::BeginPlot(("##" + title).c_str(), ImVec2(-1,350))) {
        ImPlot::SetupAxes(nullptr, nullptr, flags, flags);
        ImPlot::SetupAxisLimits(ImAxis_X1,t - history, t, ImGuiCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1,minVal,maxVal);
        ImPlot::PlotLine(title.c_str(), &sdata1.Data[0].x, &sdata1.Data[0].y, sdata1.Data.size(), 0, sdata1.Offset, 2*sizeof(float));
        ImPlot::EndPlot();
    }
}
static void setupImGuiFont() {
    ImGuiIO& io = ImGui::GetIO();
    io.FontGlobalScale = 2.f;
}
int main() {
    Simulation sim(GW, GH);
    // Create the main window
    sf::RenderWindow window(sf::VideoMode(sim.size_x * pixel_size, sim.size_y * pixel_size), "SFML window");
    window.setFramerateLimit(60U);
    std::vector<SoundSource> sources;
    sf::Image sim_img;
    sim_img.create(sim.size_x, sim.size_y);
    float default_freq = 0.15f;
    float default_mag = 1.f;
    bool isBuilding = false;
    bool isDrawing = true;

    struct {
        std::vector<float> samples;
        float samples_time = 0.f;
        size_t cur_buf = 0;
        sf::SoundBuffer sndbuf[2];
        sf::Sound emitter;
        float pitch = 1.f;
        float volume = 0.f;
    }sound;

    if(!ImGui::SFML::Init(window))
        exit(1);
    ImPlot::CreateContext();
    setupImGuiFont();
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
                sources.push_back({sf::Vector2u(mpos)});
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
            sim.wall[(size_t)mpos.x + (size_t)mpos.y * sim.size_x] = 1.f;
        }
        {
            ImGui::Begin("options");
            ImGui::Text("%f", 1.f / delT.asSeconds());
            ImGui::Text("sources options:");
            ImGui::SliderFloat("change default frquency", &default_freq, 0.1f, 3.0f);
            ImGui::SliderFloat("change default magnitude", &default_mag, 0.f, 10.f);
            ImGui::Text("sound options:");
            ImGui::SliderFloat("change default pitch", &sound.pitch, 1.f, 10.f);
            ImGui::SliderFloat("change default volume", &sound.volume, 0.f, 100.f);
            if(ImGui::Button("build_mode")) {
                isBuilding = !isBuilding;
            }
            if(ImGui::Button("draw_visualisation")) {
                isDrawing = !isDrawing;
            }
            RealtimePlot("frequency", sim.pressure[(size_t)mpos.x + (size_t)mpos.y * sim.size_x]);


            ImGui::End();
        }
        sound.samples.push_back(sim.pressure[(size_t)mpos.x + (size_t)mpos.y * sim.size_x]);
        sound.samples_time += delT.asSeconds();
        if(sound.samples_time > 0.5f) {
            std::vector<int16_t> sample_bytes;
            for(auto& s : sound.samples) {
                sample_bytes.push_back(s * 30766);
            }
            sound.samples.clear();

            auto sample_rate = static_cast<float>(sample_bytes.size()) / sound.samples_time;
            sound.sndbuf[sound.cur_buf].loadFromSamples(&sample_bytes[0], sample_bytes.size(), 1U , sample_rate);
            sound.emitter.setVolume(0.f);
            sound.emitter.pause();
            sound.emitter.setBuffer(sound.sndbuf[sound.cur_buf]);

            if(sound.cur_buf == 1) 
                sound.cur_buf = 0;
            else
                sound.cur_buf = 1;

            sound.emitter.setLoop(true);
            sound.emitter.setPitch(sound.pitch);
            sound.emitter.setVolume(sound.volume);

            sound.emitter.play();
            sound.samples_time = 0.f;
            
        }
    
        for(auto& s : sources)
            s.update(sim);
        if(isDrawing)
            sim.draw(window, sim_img);
        sim.step();

        ImGui::SFML::Render(window);
        window.display();
    }
    return EXIT_SUCCESS;
}

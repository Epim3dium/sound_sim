#include "SFML/Audio/SoundBuffer.hpp"
#include "SFML/Graphics.hpp"
#include "SFML/Audio/Sound.hpp"
#include "SFML/Audio/SoundStream.hpp"
#include "SFML/Graphics/BlendMode.hpp"
#include "SFML/Graphics/Image.hpp"
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
float damp_pressure = 0.99f;

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
class SoundGenerator {
    std::vector<float> pressure_samples;
    std::vector<int16_t> samples;
    float time = 0.f;
    sf::SoundBuffer buffer;
public:
    float time_thresh = 0.1f;
    sf::Sound sound;


    bool normalize = false;
    void Update(float sample, float delT) {
        if(sound.getVolume() == 0.f)
            return;
        pressure_samples.push_back(sample);
        time += delT;
        if(time > time_thresh) {
            samples.clear();
            if(normalize) {
                float abs_max = 0.f;
                for(auto p : pressure_samples)
                    abs_max = std::max(abs_max, std::abs(p));
                for(auto& p : pressure_samples)
                    p /= abs_max;
            }
            for(auto p : pressure_samples)
                samples.push_back(p * 30000);
            pressure_samples.clear();
            float sample_rate = static_cast<float>(samples.size()) / time;
            sound.pause();
            buffer.loadFromSamples(&samples[0], samples.size(), 1U, sample_rate);
            sound.play();
            time = 0.f;
        }
    }
    SoundGenerator() {
        sound.setBuffer(buffer);
    }
};
struct DemoOptions {
    float default_freq = 0.15f;
    float default_mag = 1.f;
    bool isBuilding = false;
    bool isDrawing = true;
};
static void setupImGuiFont() {
    ImGuiIO& io = ImGui::GetIO();
    io.FontGlobalScale = 2.f;
}
int main() {
    Simulation sim(GW, GH);
    // Create the main window
    sf::RenderWindow window(sf::VideoMode(sim.size_x * pixel_size, sim.size_y * pixel_size), "SFML window");
    std::vector<SoundSource> sources;
    sf::Image sim_img; sim_img.create(sim.size_x, sim.size_y);
    DemoOptions options;
    SoundGenerator generator;

    float pitch = 1.f;
    float volume = 0.f;
    int iters = 1;

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
                sources.back().omega = options.default_freq;
                sources.back().initial_mag = options.default_mag;
            }
            if(event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::R) {
                sources.clear();
            }
        }
        delT = deltaClock.restart();
        ImGui::SFML::Update(window, delT);
        if(options.isBuilding && sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
            sim.wall[(size_t)mpos.x + (size_t)mpos.y * sim.size_x] = 1.f;
        }
        {
            ImGui::Begin("options");
            ImGui::Text("%f", 1.f / delT.asSeconds());
            ImGui::Text("sources options:");
            ImGui::SliderFloat("change default frquency", &options.default_freq, 0.1f, 1.0f);
            ImGui::SliderFloat("change default magnitude", &options.default_mag, 0.f, 10.f);
            ImGui::Text("sound options:");
            ImGui::SliderFloat("change default pitch", &pitch, 1.f, 10.f);
            ImGui::SliderFloat("change default volume", &volume, 0.f, 1000.f);
            if(ImGui::Button("build_mode")) {
                options.isBuilding = !options.isBuilding;
            }
            if(ImGui::Button("draw_visualisation")) {
                options.isDrawing = !options.isDrawing;
            }
            RealtimePlot("frequency", sim.pressure[(size_t)mpos.x + (size_t)mpos.y * sim.size_x]);
            ImGui::SliderInt("change iterations", &iters, 0, 5);


            ImGui::End();
        }
    
        for(int i = 0; i < iters; i++) {
            for(auto& s : sources)
                s.update(sim);
            sim.step();
            generator.Update(sim.pressure[(size_t)mpos.x + (size_t)mpos.y * sim.size_x], delT.asSeconds() / 2.f);
        }
        if(options.isDrawing)
            sim.draw(window, sim_img);
        generator.sound.setPitch(pitch);
        generator.sound.setVolume(volume);

        ImGui::SFML::Render(window);
        window.display();
    }
    return EXIT_SUCCESS;
}

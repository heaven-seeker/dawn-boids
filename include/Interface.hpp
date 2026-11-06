#pragma once

#include <CoreData.hpp>

#include "GLFW/glfw3.h"
#include "glm/glm.hpp"
#include "webgpu/webgpu_cpp.h"

namespace WGPUBoids {

class Camera;

class Interface {
   public:
    Interface() = default;
    ~Interface();

    bool init(GLFWwindow* window, Camera* pCamera, wgpu::Device device,
              wgpu::TextureFormat targetFormat);
    void destroy();
    void beginFrame();
    void buildUI();
    void draw(wgpu::RenderPassEncoder renderPass);

   private:
    void buildControlPanel();
    void buildSceneTab();
    void buildBehaviorTab();
    void buildViewTab();
    void buildNavigationHint();

    float currentScale = 1.0f;
    float fontScale = 1.0f;
    SimulationParams params;
    int targetFPS = 60;
    int pendingTargetFPS = 60;
    bool showVelocity = false;
    bool showCoM = false;
    bool divideFlocks = false;

    bool requestFullscreenToggle = false;

    GLFWwindow* glfwWindow = nullptr;
    Camera* camera = nullptr;
    bool showControlPanel = true;

   public:
    const SimulationParams& getParams() const { return params; }
    void setDeltaTime(float deltaTime) { params.deltaTime = deltaTime; }

    bool getShowVelocity() const { return showVelocity; }
    bool getShowCoM() const { return showCoM; }

    int getTargetFPS() const { return targetFPS; }

    bool consumeFullscreenToggle() {
        if (requestFullscreenToggle) {
            requestFullscreenToggle = false;
            return true;
        }
        return false;
    }
};

}  // namespace WGPUBoids

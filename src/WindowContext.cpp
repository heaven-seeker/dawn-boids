#include "WindowContext.hpp"

#include <algorithm>
#include <iostream>

#include "std_image.h"

namespace WGPUBoids {
namespace {

GLFWmonitor* monitorForRectangle(int x, int y, int width, int height) {
    int count = 0;
    GLFWmonitor** monitors = glfwGetMonitors(&count);
    GLFWmonitor* bestMonitor = glfwGetPrimaryMonitor();
    int largestOverlap = 0;
    for (int index = 0; index < count; ++index) {
        const GLFWvidmode* mode = glfwGetVideoMode(monitors[index]);
        if (mode == nullptr) continue;

        int monitorX, monitorY;
        glfwGetMonitorPos(monitors[index], &monitorX, &monitorY);
        const int overlapWidth =
            std::max(0, std::min(x + width, monitorX + mode->width) -
                            std::max(x, monitorX));
        const int overlapHeight =
            std::max(0, std::min(y + height, monitorY + mode->height) -
                            std::max(y, monitorY));
        const int overlap = overlapWidth * overlapHeight;
        if (overlap > largestOverlap) {
            largestOverlap = overlap;
            bestMonitor = monitors[index];
        }
    }
    return bestMonitor;
}

}  // namespace

bool WindowContext::init(int width, int height, const char* title) {
    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
    window = glfwCreateWindow(width, height, title, primaryMonitor, nullptr);
    this->name = title;

    if (!window) {
        std::cerr << "Can't create window" << std::endl;
        return false;
    }

    glfwGetFramebufferSize(window, &this->width, &this->height);
    glfwSetWindowSizeLimits(window, 640, 480, GLFW_DONT_CARE, GLFW_DONT_CARE);
    int workX = 0, workY = 0, workWidth = width, workHeight = height;
    if (primaryMonitor != nullptr) {
        glfwGetMonitorWorkarea(primaryMonitor, &workX, &workY, &workWidth,
                               &workHeight);
    }
    savedWidth = std::min(1280, std::max(640, workWidth - 80));
    savedHeight = std::min(800, std::max(480, workHeight - 80));
    savedX = workX + (workWidth - savedWidth) / 2;
    savedY = workY + (workHeight - savedHeight) / 2;

#ifdef _WIN32  // Set app icon on windows
    GLFWimage images[1];
    images[0].pixels = stbi_load("assets/app_icon.png", &images[0].width,
                                 &images[0].height, 0, 4);
    if (images[0].pixels) {
        glfwSetWindowIcon(window, 1, images);
        stbi_image_free(images[0].pixels);
    }
#endif

    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, [](GLFWwindow* window, int width,
                                              int height) {
        auto that =
            reinterpret_cast<WindowContext*>(glfwGetWindowUserPointer(window));
        if (that != nullptr) {
            that->width = width;
            that->height = height;
            that->setResized(true);
        }
    });
    glfwSetScrollCallback(window, [](GLFWwindow* window, double xoffset,
                                     double yoffset) {
        auto that =
            reinterpret_cast<WindowContext*>(glfwGetWindowUserPointer(window));
        if (that != nullptr) {
            that->setScrollDelta(static_cast<float>(yoffset));
        }
    });
    return true;
}

void WindowContext::pollEvents() { glfwPollEvents(); }

bool WindowContext::shouldClose() { return glfwWindowShouldClose(window); }

std::pair<int, int> WindowContext::getFramebufferSize() {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    return std::pair<int, int>(width, height);
}

WindowContext::~WindowContext() { glfwDestroyWindow(window); }

float WindowContext::getAndResetScrollDelta() {
    float d = scrollDelta;
    scrollDelta = 0.f;
    return d;
}

void WindowContext::toggleFullscreen() {
    if (glfwGetWindowMonitor(window) != nullptr) {
        GLFWmonitor* monitor =
            monitorForRectangle(savedX, savedY, savedWidth, savedHeight);
        if (monitor != nullptr) {
            int workX, workY, workWidth, workHeight;
            glfwGetMonitorWorkarea(monitor, &workX, &workY, &workWidth,
                                   &workHeight);
            savedWidth = std::min(savedWidth, workWidth);
            savedHeight = std::min(savedHeight, workHeight);
            savedX = std::clamp(savedX, workX, workX + workWidth - savedWidth);
            savedY =
                std::clamp(savedY, workY, workY + workHeight - savedHeight);
        }
        glfwSetWindowMonitor(window, nullptr, savedX, savedY, savedWidth,
                             savedHeight, GLFW_DONT_CARE);
        if (savedMaximized) glfwMaximizeWindow(window);
    } else {
        glfwGetWindowPos(window, &savedX, &savedY);
        glfwGetWindowSize(window, &savedWidth, &savedHeight);
        savedMaximized =
            glfwGetWindowAttrib(window, GLFW_MAXIMIZED) == GLFW_TRUE;

        GLFWmonitor* monitor =
            monitorForRectangle(savedX, savedY, savedWidth, savedHeight);
        if (monitor == nullptr) return;
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        if (mode == nullptr) return;

        glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height,
                             mode->refreshRate);
    }

    glfwGetFramebufferSize(window, &width, &height);
    setResized(true);
}

}  // namespace WGPUBoids

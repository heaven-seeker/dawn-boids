#include "Interface.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <string>

#include "Camera.hpp"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_wgpu.h"
#include "imgui.h"

namespace WGPUBoids {
namespace {

const ImVec4 accent(0.38f, 0.86f, 0.75f, 1.0f);

constexpr ImGuiWindowFlags fixedWindowFlags =
    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
    ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
    ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoFocusOnAppearing;

void applyStyle(float scale) {
    ImGuiStyle& style = ImGui::GetStyle();
    style = ImGuiStyle();
    ImGui::StyleColorsDark();
    style.WindowPadding = ImVec2(18.0f, 14.0f);
    style.FramePadding = ImVec2(10.0f, 6.0f);
    style.ItemSpacing = ImVec2(8.0f, 7.0f);
    style.ItemInnerSpacing = ImVec2(8.0f, 6.0f);
    style.WindowRounding = 14.0f;
    style.ChildRounding = 8.0f;
    style.FrameRounding = 6.0f;
    style.PopupRounding = 8.0f;
    style.ScrollbarRounding = 8.0f;
    style.GrabRounding = 4.0f;
    style.TabRounding = 6.0f;
    style.ScrollbarSize = 6.0f;
    style.GrabMinSize = 12.0f;
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.TabBorderSize = 0.0f;
    style.SeparatorTextBorderSize = 1.0f;
    style.ButtonTextAlign = ImVec2(0.5f, 0.5f);

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_Text] = ImVec4(0.91f, 0.94f, 0.97f, 1.0f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.57f, 0.64f, 0.71f, 1.0f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.055f, 0.075f, 0.10f, 0.98f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.11f, 0.14f, 0.99f);
    colors[ImGuiCol_Border] = ImVec4(0.20f, 0.27f, 0.32f, 0.65f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.11f, 0.15f, 0.19f, 1.0f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.15f, 0.22f, 0.26f, 1.0f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.17f, 0.29f, 0.31f, 1.0f);
    colors[ImGuiCol_Button] = ImVec4(0.12f, 0.18f, 0.22f, 1.0f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.19f, 0.30f, 0.33f, 1.0f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.23f, 0.39f, 0.40f, 1.0f);
    colors[ImGuiCol_Header] = ImVec4(0.10f, 0.17f, 0.20f, 1.0f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.15f, 0.26f, 0.29f, 1.0f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.18f, 0.33f, 0.34f, 1.0f);
    colors[ImGuiCol_Tab] = ImVec4(0.08f, 0.12f, 0.16f, 1.0f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.18f, 0.32f, 0.34f, 1.0f);
    colors[ImGuiCol_TabActive] = ImVec4(0.15f, 0.28f, 0.30f, 1.0f);
    colors[ImGuiCol_CheckMark] = accent;
    colors[ImGuiCol_SliderGrab] = accent;
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.62f, 0.96f, 0.85f, 1.0f);
    colors[ImGuiCol_Separator] = ImVec4(0.20f, 0.29f, 0.34f, 0.65f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.26f, 0.36f, 0.41f, 0.8f);
    colors[ImGuiCol_ScrollbarGrabHovered] = accent;
    colors[ImGuiCol_ScrollbarGrabActive] = accent;
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.25f, 0.65f, 0.57f, 0.4f);
    colors[ImGuiCol_NavHighlight] = accent;
    style.ScaleAllSizes(scale);
}

std::string findSystemFont() {
#ifdef _WIN32
    const char* windowsDirectory = std::getenv("WINDIR");
    const std::filesystem::path fontPath =
        std::filesystem::path(windowsDirectory ? windowsDirectory
                                               : "C:/Windows") /
        "Fonts/segoeui.ttf";
#elif defined(__APPLE__)
    const std::filesystem::path fontPath = "/System/Library/Fonts/SFNS.ttf";
#else
    const std::filesystem::path fontPath =
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf";
#endif
    std::error_code error;
    return std::filesystem::is_regular_file(fontPath, error) ? fontPath.string()
                                                             : std::string();
}

ImFont* addFont(const std::string& path, float size) {
    ImFontAtlas* fonts = ImGui::GetIO().Fonts;
    if (!path.empty()) {
        if (ImFont* font = fonts->AddFontFromFileTTF(path.c_str(), size)) {
            return font;
        }
    }
    ImFontConfig config;
    config.SizePixels = size;
    return fonts->AddFontDefault(&config);
}

void showTooltip(const char* text) {
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 23.0f);
        ImGui::TextUnformatted(text);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

void sectionHeading(const char* title, const char* description) {
    ImGui::Spacing();
    ImGui::TextColored(accent, "%s", title);
    if (description != nullptr &&
        ImGui::GetIO().DisplaySize.y > ImGui::GetFontSize() * 38.0f) {
        ImGui::PushStyleColor(ImGuiCol_Text,
                              ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
        ImGui::TextWrapped("%s", description);
        ImGui::PopStyleColor();
    }
    ImGui::Separator();
}

bool parameterSlider(const char* label, float* value, float minimum,
                     float maximum, const char* format, const char* description,
                     ImGuiSliderFlags flags = ImGuiSliderFlags_None) {
    ImGui::PushID(label);
    ImGui::TextUnformatted(label);
    showTooltip(description);
    ImGui::SetNextItemWidth(-1.0f);
    bool changed =
        ImGui::SliderFloat("##value", value, minimum, maximum, format, flags);
    showTooltip(description);
    ImGui::PopID();
    return changed;
}

}  // namespace

Interface::~Interface() { destroy(); }

bool Interface::init(GLFWwindow* window, Camera* pCamera, wgpu::Device device,
                     wgpu::TextureFormat targetFormat) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    bool success =
        ImGui_ImplGlfw_InitForOther(window, true) &&
        ImGui_ImplWGPU_Init(device.Get(), 3,
                            static_cast<WGPUTextureFormat>(targetFormat),
                            WGPUTextureFormat_Undefined);
    if (!success) return false;

    glfwWindow = window;
    camera = pCamera;

    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    float xScale, yScale;
    glfwGetWindowContentScale(window, &xScale, &yScale);
    currentScale = std::max(xScale, yScale);
    fontScale = currentScale;
    const std::string fontPath = findSystemFont();
    io.FontDefault = addFont(fontPath, 17.0f * fontScale);
    applyStyle(currentScale);
    return true;
}

void Interface::destroy() {
    if (ImGui::GetCurrentContext()) {
        ImGui_ImplWGPU_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }
}

void Interface::beginFrame() {
    float xScale, yScale;
    glfwGetWindowContentScale(glfwWindow, &xScale, &yScale);
    const float scale = std::max(xScale, yScale);
    if (std::abs(scale - currentScale) > 0.01f) {
        currentScale = scale;
        applyStyle(currentScale);
        ImGui::GetIO().FontGlobalScale = currentScale / fontScale;
    }

    ImGui_ImplWGPU_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void Interface::buildUI() {
    if (!ImGui::GetIO().WantTextInput) {
        if (ImGui::IsKeyPressed(ImGuiKey_F1, false)) {
            showControlPanel = !showControlPanel;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_F11, false)) {
            requestFullscreenToggle = true;
        }
    }

    if (showControlPanel) buildControlPanel();
    buildNavigationHint();
}

void Interface::buildControlPanel() {
    const ImVec2 display = ImGui::GetIO().DisplaySize;
    const float inset = 16.0f * currentScale;
    const bool compact = display.y < 650.0f * currentScale;
    ImGui::SetNextWindowPos(ImVec2(inset, inset));
    ImGui::SetNextWindowSize(ImVec2(
        std::max(1.0f,
                 std::min(350.0f * currentScale, display.x - inset * 2.0f)),
        std::max(1.0f, display.y - inset - 58.0f * currentScale)));

    if (ImGui::Begin("##Controls", nullptr,
                     fixedWindowFlags | ImGuiWindowFlags_NoScrollbar)) {
        ImGui::TextUnformatted("Simulation controls");
        if (!compact) ImGui::TextDisabled("Make the flock your own.");
        ImGui::Spacing();

        const float footerHeight =
            ImGui::GetFrameHeight() + ImGui::GetStyle().ItemSpacing.y * 3.0f +
            (compact ? 0.0f : ImGui::GetTextLineHeightWithSpacing());

        if (ImGui::BeginTabBar("Settings",
                               ImGuiTabBarFlags_NoCloseWithMiddleMouseButton)) {
            if (ImGui::BeginTabItem("Scene")) {
                if (ImGui::BeginChild("SceneContent",
                                      ImVec2(0.0f, -footerHeight))) {
                    buildSceneTab();
                }
                ImGui::EndChild();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Behavior")) {
                if (ImGui::BeginChild("BehaviorContent",
                                      ImVec2(0.0f, -footerHeight))) {
                    buildBehaviorTab();
                }
                ImGui::EndChild();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("View")) {
                if (ImGui::BeginChild("ViewContent",
                                      ImVec2(0.0f, -footerHeight))) {
                    buildViewTab();
                }
                ImGui::EndChild();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }

        ImGui::Separator();
        if (!compact) ImGui::TextDisabled("Ctrl + click for an exact value");
        ImGui::PushStyleColor(ImGuiCol_Button,
                              ImVec4(0.23f, 0.12f, 0.15f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                              ImVec4(0.40f, 0.17f, 0.21f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                              ImVec4(0.50f, 0.20f, 0.24f, 1.0f));
        if (ImGui::Button("Exit simulation", ImVec2(-1.0f, 0.0f))) {
            glfwSetWindowShouldClose(glfwWindow, GLFW_TRUE);
        }
        ImGui::PopStyleColor(3);
    }
    ImGui::End();
}

void Interface::buildSceneTab() {
    sectionHeading("POPULATION", "Control the number of birds in the scene.");
    ImGui::TextUnformatted("Active boids");
    const uint32_t minBoids = 0;
    const uint32_t maxBoids = 350000;
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::SliderScalar("##ActiveBoids", ImGuiDataType_U32,
                        &params.activeBoidsCount, &minBoids, &maxBoids, "%u",
                        ImGuiSliderFlags_AlwaysClamp);
    showTooltip("Number of active birds. Higher counts require more GPU work.");

    sectionHeading("WORLD", "Set the boundaries of the flock's space.");
    parameterSlider("Cube size", &params.cubeSize, 2.0f, 20.0f, "%.2f",
                    "Size of the simulation boundary.",
                    ImGuiSliderFlags_AlwaysClamp);
    parameterSlider("Boundary margin", &params.margin, 0.1f, 2.0f, "%.2f",
                    "Distance from the boundary at which birds start turning.");

    sectionHeading("FLOCKS", "Let three color groups share the same world.");
    ImGui::Checkbox("Divide into flocks", &divideFlocks);
    params.divideFlocks = divideFlocks ? 1u : 0u;
    showTooltip("Use separate flock groups with repulsion between strangers.");
    if (params.divideFlocks) {
        parameterSlider("Stranger protected range",
                        &params.strangerProtectedRange, 2.0f, 5.0f, "%.2f",
                        "Keep this distance from other flocks.");
        parameterSlider("Stranger force", &params.strangeForceFactor, 5.0f,
                        20.0f, "%.2f",
                        "Strength of repulsion from other flocks.");
    }
}

void Interface::buildBehaviorTab() {
    sectionHeading("PERCEPTION", "Define what each bird can see.");
    parameterSlider("Visual range", &params.visualRange, 0.5f, 10.0f, "%.2f",
                    "Distance within which a bird can detect its neighbors.");
    parameterSlider("Protected range", &params.protectedRange, 0.05f, 3.0f,
                    "%.2f", "Personal space used for collision avoidance.");
    parameterSlider("Field of view", &params.visionRadius, 0.0f, 360.0f,
                    "%.0f deg",
                    "Visible angle in degrees. 360 gives all-around vision.");

    sectionHeading("SPEED", "Adjust the flock's pace.");
    parameterSlider("Maximum speed", &params.maxSpeed, 1.0f, 15.0f, "%.2f",
                    "Upper limit of each bird's flight speed.");
    parameterSlider("Minimum speed", &params.minSpeed, 0.0f, 5.0f, "%.2f",
                    "Lower limit of each bird's flight speed.");

    sectionHeading("STEERING", "Balance the forces that shape the flock.");
    parameterSlider("Cohesion", &params.cohesionFactor, 0.0f, 0.05f, "%.4f",
                    "Attraction toward the center of nearby birds.");
    parameterSlider("Alignment", &params.alignmentFactor, 0.0f, 0.2f, "%.4f",
                    "How strongly birds match their neighbors' direction.");
    parameterSlider("Separation", &params.separationFactor, 0.0f, 0.2f, "%.4f",
                    "Repulsion from birds inside the protected range.");
    parameterSlider("Boundary turn force", &params.turnFactor, 0.01f, 1.0f,
                    "%.3f",
                    "How strongly birds turn away from the world boundary.");
}

void Interface::buildViewTab() {
    sectionHeading("DISPLAY", nullptr);
    const bool fullscreen = glfwGetWindowMonitor(glfwWindow) != nullptr;
    if (ImGui::Button(fullscreen ? "Windowed  F11" : "Fullscreen  F11",
                      ImVec2(-1.0f, 0.0f))) {
        requestFullscreenToggle = true;
    }
    showTooltip("Switch between fullscreen and your previous window size. "
                "Shortcut: F11.");

    sectionHeading("CAMERA", "Choose how you explore the scene.");
    if (camera != nullptr) {
        int currentMode = camera->getMode() == CameraMode::Free ? 0 : 1;
        const int previousMode = currentMode;
        ImGui::RadioButton("Free camera", &currentMode, 0);
        ImGui::SameLine();
        ImGui::RadioButton("Orbital", &currentMode, 1);
        if (currentMode != previousMode) {
            camera->setMode(currentMode == 0 ? CameraMode::Free
                                             : CameraMode::Orbital);
        }

        float speed = camera->getMovementSpeed();
        if (parameterSlider("Camera speed", &speed, 1.0f, 100.0f, "%.1f",
                            "Movement speed of the free camera.")) {
            camera->setMovementSpeed(speed);
        }
        if (camera->getMode() == CameraMode::Orbital) {
            float radius = camera->getRadius();
            if (parameterSlider(
                    "Orbital distance", &radius, 1.0f, 300.0f, "%.1f",
                    "Distance from the camera to the orbit center.")) {
                camera->setRadius(radius);
            }
        }
    } else {
        ImGui::TextDisabled("Camera controls are unavailable.");
    }

    sectionHeading("OVERLAYS", "Inspect the flock's movement.");
    ImGui::Checkbox("Velocity vectors", &showVelocity);
    showTooltip("Display the direction and speed of the birds.");
    ImGui::Checkbox("Centers of mass", &showCoM);
    showTooltip("Display the local center of mass used by each bird.");

    sectionHeading("PERFORMANCE", "Set the rendering frame limit.");
    ImGui::TextColored(accent, "%.1f FPS", ImGui::GetIO().Framerate);
    ImGui::SameLine();
    ImGui::TextDisabled("/ %.2f ms per frame",
                        1000.0f / std::max(1.0f, ImGui::GetIO().Framerate));
    ImGui::TextUnformatted("FPS limit");
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::SliderInt("##FPSLimit", &pendingTargetFPS, 0, 240,
                     pendingTargetFPS == 0 ? "Uncapped" : "%d FPS");
    if (ImGui::IsItemDeactivatedAfterEdit()) targetFPS = pendingTargetFPS;
    showTooltip(
        "Zero removes the frame limit. Applied when you finish editing.");

    sectionHeading("SHORTCUTS", nullptr);
    const char* shortcuts[][2] = {
        {"F1", "Toggle controls"},        {"F11", "Toggle fullscreen"},
        {"Right mouse", "Rotate camera"}, {"Mouse wheel", "Zoom"},
        {"W A S D", "Move free camera"},  {"Space / Shift", "Move up / down"},
        {"+ / -", "Camera speed"},        {"Ctrl + click", "Enter exact value"},
    };
    if (ImGui::BeginTable("Shortcuts", 2)) {
        for (const auto& shortcut : shortcuts) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(shortcut[0]);
            ImGui::TableNextColumn();
            ImGui::TextDisabled("%s", shortcut[1]);
        }
        ImGui::EndTable();
    }
}

void Interface::buildNavigationHint() {
    const ImVec2 display = ImGui::GetIO().DisplaySize;
    const float inset = 16.0f * currentScale;
    ImGui::SetNextWindowPos(ImVec2(display.x - inset, display.y - inset),
                            ImGuiCond_Always, ImVec2(1.0f, 1.0f));
    ImGui::SetNextWindowBgAlpha(0.7f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,
                        ImVec2(14.0f * currentScale, 8.0f * currentScale));
    if (ImGui::Begin("##Navigation", nullptr,
                     fixedWindowFlags | ImGuiWindowFlags_AlwaysAutoResize |
                         ImGuiWindowFlags_NoNav |
                         ImGuiWindowFlags_NoScrollbar |
                         ImGuiWindowFlags_NoScrollWithMouse)) {
        const char* panelLabel =
            showControlPanel ? "F1  Hide controls" : "F1  Controls";
        const char* navigation = "RMB  Rotate   /   Wheel  Zoom   /   WASD  Move";
        const ImGuiStyle& style = ImGui::GetStyle();
        const float fullWidth = ImGui::CalcTextSize(panelLabel).x +
                                style.FramePadding.x * 2.0f +
                                style.ItemSpacing.x +
                                ImGui::CalcTextSize(navigation).x +
                                style.WindowPadding.x * 2.0f;
        if (fullWidth <= display.x - inset * 2.0f) {
            ImGui::AlignTextToFramePadding();
            ImGui::TextDisabled("%s", navigation);
            ImGui::SameLine();
        }
        if (ImGui::Button(panelLabel)) {
            showControlPanel = !showControlPanel;
        }
        showTooltip("Show or hide simulation controls. Shortcut: F1.\n"
                    "Camera: right mouse to rotate, wheel to zoom, WASD to move.\n"
                    "Fullscreen: F11.");
    }
    ImGui::End();
    ImGui::PopStyleVar();
}

void Interface::draw(wgpu::RenderPassEncoder renderPass) {
    ImGui::Render();
    ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), renderPass.Get());
}

}  // namespace WGPUBoids

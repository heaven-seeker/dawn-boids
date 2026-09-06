#pragma once

#include <cstdint>

constexpr char* const ICON_PATH = "assets/app_icon.png";

constexpr const uint32_t MAX_CELLS =
    80 * 80 * 80;  // cube bounds / visual range

constexpr const uint32_t MAX_BOIDS_PER_CELL = 64;
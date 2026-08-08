/**
 * @file ST7735_Config_PanelA.hpp
 * @brief ST7735 Panel Configuration — Batch A (original batch)
 * 
 * Characteristics:
 * - BGR color order
 * - Default offsets (0,24) for landscape
 * - No inversion needed
 * - DEG_0 orientation
 * 
 * To select this panel: #define ST7735_PANEL_A before including ST7735_PanelConfig.hpp
 */

#ifndef ST7735_CONFIG_PANELA_HPP
#define ST7735_CONFIG_PANELA_HPP

#include "ST7735.hpp"

namespace ST7735Config {

// Batch-specific initialization parameters
constexpr bool   INIT_INVERT    = false;
constexpr auto   INIT_ROTATION  = e_ST7735_Rotation::DEG_0;

// Rotation presets for Batch A (original 160x80 display)
constexpr RotationCfg ROTATIONS[4] = {
    // DEG_0   (landscape, MX|MV)
    { MADCTL_MX | MADCTL_MV | MADCTL_BGR, 160, 80,  0,  24 },
    // DEG_90  (portrait,  MX|MY)
    { MADCTL_MX | MADCTL_MY | MADCTL_BGR, 80,  160, 24, 0  },
    // DEG_180 (landscape flipped, MY|MV)
    { MADCTL_MY | MADCTL_MV | MADCTL_BGR, 160, 80,  0,  24 },
    // DEG_270 (portrait flipped, BGR only)
    { MADCTL_BGR,                          80,  160, 24, 0  },
};

} // namespace ST7735Config

#endif // ST7735_CONFIG_PANELA_HPP

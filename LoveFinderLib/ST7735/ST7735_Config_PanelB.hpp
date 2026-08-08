/**
 * @file ST7735_Config_PanelB.hpp
 * @brief ST7735 Panel Configuration — Batch B (new batch)
 * 
 * Characteristics:
 * - RGB color order (MADCTL_RGB instead of BGR)
 * - Tuned offsets (1,26) for landscape
 * - Requires color inversion
 * - Mounted upside down → DEG_180 as default
 * - Y-axis mirrored (MY) for correct text direction
 * 
 * To select this panel: #define ST7735_PANEL_B before including ST7735_PanelConfig.hpp
 */

#ifndef ST7735_CONFIG_PANELB_HPP
#define ST7735_CONFIG_PANELB_HPP

#include "ST7735.hpp"

namespace ST7735Config {

// Batch-specific initialization parameters
constexpr bool   INIT_INVERT    = true;
constexpr auto   INIT_ROTATION  = e_ST7735_Rotation::DEG_180;

// Rotation presets for Batch B (new batch 160x80 display, tuned offsets)
constexpr RotationCfg ROTATIONS[4] = {
    // DEG_0   (landscape, MY|MX|MV — Y-axis mirrored)
    { MADCTL_MY | MADCTL_MX | MADCTL_MV | MADCTL_RGB, 160, 80,  1,  26 },
    // DEG_90  (portrait,  MX|MY)
    { MADCTL_MX | MADCTL_MY | MADCTL_RGB, 80,  160, 24, 0  },
    // DEG_180 (landscape 180°, MV — Y-axis mirrored)
    { MADCTL_MV | MADCTL_RGB, 160, 80,  1,  26 },
    // DEG_270 (portrait flipped)
    { MADCTL_RGB,                          80,  160, 24, 0  },
};

} // namespace ST7735Config

#endif // ST7735_CONFIG_PANELB_HPP

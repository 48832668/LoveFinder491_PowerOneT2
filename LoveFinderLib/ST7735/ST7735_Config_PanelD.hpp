/**
 * @file ST7735_Config_PanelD.hpp
 * @brief ST7735 Panel Configuration — Batch D (A variant with inverted colors)
 * 
 * Characteristics:
 * - BGR color order
 * - Default offsets (1,26) for landscape
 * - Color-inverted (INIT_INVERT = true, opposite of Panel A)
 * - DEG_0 orientation
 * 
 * This is Panel A with colors inverted — useful for displays that
 * need INVON (CMD 0x21) with the same electrical characteristics
 * as the original batch.
 * 
 * To select this panel: #define ST7735_PANEL_D before including ST7735_PanelConfig.hpp
 */

#ifndef ST7735_CONFIG_PANELD_HPP
#define ST7735_CONFIG_PANELD_HPP

#include "ST7735.hpp"

namespace ST7735Config {

// Batch-specific initialization parameters (A variant: invert toggled)
constexpr bool   INIT_INVERT    = true;
constexpr auto   INIT_ROTATION  = e_ST7735_Rotation::DEG_0;

// Rotation presets for Batch D (same as Batch A with +1 offset — original 160x80 display)
constexpr RotationCfg ROTATIONS[4] = {
    // DEG_0   (landscape, MX|MV)
    { MADCTL_MX | MADCTL_MV | MADCTL_BGR, 160, 80,  1,  26 },
    // DEG_90  (portrait,  MX|MY)
    { MADCTL_MX | MADCTL_MY | MADCTL_BGR, 80,  160, 25, 2  },
    // DEG_180 (landscape flipped, MY|MV)
    { MADCTL_MY | MADCTL_MV | MADCTL_BGR, 160, 80,  1,  26 },
    // DEG_270 (portrait flipped, BGR only)
    { MADCTL_BGR,                          80,  160, 25, 2  },
};

} // namespace ST7735Config

#endif // ST7735_CONFIG_PANELD_HPP
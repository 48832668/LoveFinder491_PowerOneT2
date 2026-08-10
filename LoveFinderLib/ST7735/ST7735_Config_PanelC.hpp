/**
 * @file ST7735_Config_PanelC.hpp
 * @brief ST7735 Panel Configuration — Batch C (new batch, horizontally mirrored)
 * 
 * Characteristics (copied from Batch B, with differences):
 *   1. Horizontally mirrored (left-right flip): MX bit toggled in all rotations
 *   2. Color order inverted relative to Batch B (B=RGB → C=BGR, and vice versa)
 *   3. Color-inverted: INIT_INVERT = true (needs INVON; verified on hardware —
 *      without inversion all colors show as their complements)
 * - Offsets same as Batch B (X: 1, Y: 26)
 * - Default rotation DEG_0 (180° rotated from Batch B's DEG_180)
 * 
 * To select this panel: #define ST7735_PANEL_C before including ST7735_PanelConfig.hpp
 */

#ifndef ST7735_CONFIG_PANELC_HPP
#define ST7735_CONFIG_PANELC_HPP

#include "ST7735.hpp"

namespace ST7735Config {

// Batch-specific initialization parameters (color-inverted, needs INVON)
constexpr bool   INIT_INVERT    = true;
constexpr auto   INIT_ROTATION  = e_ST7735_Rotation::DEG_0;

// Rotation presets for Batch C (Batch B horizontally mirrored: MX toggled,
// color order inverted: RGB → BGR, color-inverted, offsets same as B,
// default rotated 180° from Batch B: DEG_180 → DEG_0)
constexpr RotationCfg ROTATIONS[4] = {
    // DEG_0   (landscape, MV — B 的 MX 去掉 + RGB→BGR，水平镜像，默认)
    { MADCTL_MY | MADCTL_MV | MADCTL_BGR, 160, 80,  1,  26 },
    // DEG_90  (portrait,  MY — B 的 MX 去掉 + RGB→BGR，水平镜像)
    { MADCTL_MY | MADCTL_BGR, 80,  160, 24, 0  },
    // DEG_180 (landscape 180°, MX|MV — 水平镜像 + RGB→BGR)
    { MADCTL_MX | MADCTL_MV | MADCTL_BGR, 160, 80,  1,  26 },
    // DEG_270 (portrait flipped, MX — 水平镜像 + RGB→BGR)
    { MADCTL_MX | MADCTL_BGR, 80,  160, 24, 0  },
};

} // namespace ST7735Config

#endif // ST7735_CONFIG_PANELC_HPP

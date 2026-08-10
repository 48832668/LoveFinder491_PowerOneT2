/**
 * @file ST7735_PanelConfig.hpp
 * @brief ST7735 Panel Configuration Selector
 * 
 * Switch between different display batches by uncommenting the
 * appropriate #define below. Only ONE panel should be active at a time.
 * 
 * Usage:
 *   // Select the panel batch here:
 *   #define ST7735_PANEL_A    // Original batch
 *   // #define ST7735_PANEL_B  // New batch (2026)
 * 
 * Then include this file (it's already included by ST7735.hpp).
 */

#ifndef ST7735_PANELCONFIG_HPP
#define ST7735_PANELCONFIG_HPP

/*====================================================================
 * PANEL SELECTION — Uncomment ONE of the following:
 *====================================================================*/

// #define ST7735_PANEL_A          // Original batch (BGR, offset 0/24, no invert, DEG_0)
#define ST7735_PANEL_B          // New batch (RGB, offset 1/26, invert, DEG_180)

/*====================================================================
 * Do not edit below this line
 *====================================================================*/

#if defined(ST7735_PANEL_A) && defined(ST7735_PANEL_B)
#error "Only one ST7735 panel can be selected at a time"
#elif defined(ST7735_PANEL_A)
#include "ST7735_Config_PanelA.hpp"
#elif defined(ST7735_PANEL_B)
#include "ST7735_Config_PanelB.hpp"
#else
#error "Must define ST7735_PANEL_A or ST7735_PANEL_B"
#endif

#endif // ST7735_PANELCONFIG_HPP

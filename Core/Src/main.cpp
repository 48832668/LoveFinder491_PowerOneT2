/**
 * @file main.cpp
 * @brief Main Application Entry Point - C++17
 * @author LoveFinder
 * @date 2026
 * 
 * LoveFinder617-3LaneSW3526 - USB Power Delivery Analyzer
 */

/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.cpp
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dma.h"
#include "i2c.h"
#include "spi.h"
#include "tim.h"
#include "gpio.h"

/* USER CODE BEGIN Includes */
// LoveFinderLib - C++17 Headers
#include "../../LoveFinderLib/NTC_ADC/NTC_ADC.hpp"
#include "../../LoveFinderLib/SW3526/SW3526.hpp"
#include "../../LoveFinderLib/ST7735/ST7735.hpp"

#include <cstring>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

// ========== Global Device Instances ==========

// NTC Temperature Sensor
NTC_ADC ntcTemp;

// SW3526 Power Controllers (3 channels)
// C3: I2C1 direct connection -> TYPE-C #3 (physical C3 port)
// C2: I2C2 direct connection -> TYPE-C #2 output
// C1: Bit-bang I2C on PB1(SDA)/PB3(SCL) -> TYPE-C #1 output
SW3526 sw3526_C3;      // TYPE-C #3 (I2C1)
SW3526 sw3526_C1;      // TYPE-C #1 (bit-bang PB1/PB3)
SW3526 sw3526_C2;      // TYPE-C #2 (I2C2)

// SW3526 Manager for multi-device control
SW3526_Manager sw3526Manager;

// ST7735 LCD Display
ST7735 lcd;

// Running time in seconds (for internal tracking only)
volatile uint32_t runningSeconds = 0;

// ADC DMA buffer for NTC temperature sensor
volatile uint16_t adcDmaBuffer = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

void initDevices(void);
void updateDisplay(void);
void processPowerData(void);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/**
 * @brief Initialize all custom devices
 */
void initDevices(void)
{
    // Initialize NTC Temperature Sensor (103AT, B=3950)
    ntcTemp.init(&hadc1, e_NTC_Type::NTC_10K_3950_103AT, 3300.0f);
    
    // Start ADC with DMA for non-blocking temperature reading
    HAL_ADC_Start_DMA(&hadc1, (uint32_t*)&adcDmaBuffer, 1);
    ntcTemp.setDmaBuffer(&adcDmaBuffer);
    
    // Initialize SW3526 Power Controllers (3 channels)
    // C3: I2C1 direct connection -> TYPE-C #3 output
    sw3526_C3.init(&hi2c1, "SW3526_C3");
    
    // C1: Bit-bang I2C on PB1(SDA)/PB3(SCL) -> TYPE-C #1 output
    sw3526_C1.initBitbang(SYS_IIC3_SDA_GPIO_Port, SYS_IIC3_SDA_Pin,
                          SYS_IIC3_SCL_GPIO_Port, SYS_IIC3_SCL_Pin, "SW3526_C1");
    
    // C2: I2C2 direct connection -> TYPE-C #2 output
    sw3526_C2.init(&hi2c2, "SW3526_C2");
    
    // Add to manager
    sw3526Manager.addDevice(&sw3526_C3);
    sw3526Manager.addDevice(&sw3526_C1);
    sw3526Manager.addDevice(&sw3526_C2);
    
    // ========== SW3526 Full Configuration (all 3 channels) ==========
    // Config: 312kHz Buck freq, all protocols enabled, max power
    auto configSW3526 = [](SW3526& dev) {
        dev.unlockRegisters();
        
        // 0. Set current sense resistor (5mΩ on this PCB)
        dev.setSenseResistor(5);
        
        // 1. Buck switching frequency: 312kHz (REG 0xA6 Bit7=1)
        dev.setBuckFreq(312);
        
        // 2. Enable all fast charge protocols
        // CHG_CFG2 (0xA8): all protocol bits=0 means enabled
        //   [7]=LV SCP=0(enable), [5]=SFCP=0, [4]=QC2=0, [3]=QC3=0,
        //   [2]=FCP=0, [1]=AFC=0, [0]=PE=0; [6]=reserved(keep 1)
        dev.writeReg(e_SW3526_Reg::CHG_CFG2, 0x40);
        
        // CHG_CFG3 (0xA9): PD + PPS + all voltage levels enabled
        //   [7]=PPS1=0(enable), [6]=PPS0=0(enable),
        //   [5]=PD20V=0(enable), [4]=PD15V=0(enable),
        //   [3]=PD12V=0(enable), [2]=PD9V=0(enable),
        //   [0]=PD=0(enable); [1]=reserved(keep 0)
        dev.writeReg(e_SW3526_Reg::CHG_CFG3, 0x00);
        
        // CHG_CFG4 (0xAA): SCP max current=4A, DPDM enable, non-PD max V=20V
        //   [7]=1,bit3=1 -> SCP current 4A; [6]=1(reserved keep); [5]=0(DPDM enable);
        //   [1:0]=3(non-PD max 20V)
        dev.writeReg(e_SW3526_Reg::CHG_CFG4, 0xC8 | 0x03);  // 0xCB
        
        // 3. Max power configuration
        // CHG_CFG0 (0xA2): SCP max power=40W, QC wire comp enable, PDO-Vin linked
        //   [7]=1(SCP 40W), [6]=0(QC wire comp enable), [5]=1(PDO-Vin link),
        //   [4:0]=Reserved(default=0)
        dev.writeReg(e_SW3526_Reg::CHG_CFG0, 0xA0);
        
        // CHG_CFG1 (0xA4): HV SCP enable, PE2.0 12V+ enable
        //   [6]=1(HV SCP enable), [5]=1(PE2.0 12V+ enable), [7,4:0]=Reserved(default=0)
        dev.writeReg(e_SW3526_Reg::CHG_CFG1, 0x60);
        
        // POWER_CFG (0xA7): max output power = 71W (value=7)
        //   12~63 = 12W~63W, 0~7 = 64W~71W (0=64W, 7=71W)
        dev.setMaxPower(71);
        
        // CHG_CFG5 (0xAB): power source=register, port type=C
        //   [7:3]=Reserved(default=0x09), [2]=1(register mode), [0]=0(C port)
        dev.writeReg(e_SW3526_Reg::CHG_CFG5, (0x09 << 3) | 0x04);  // 0x4C
        
        // CHG_CFG6 (0xAC): fast charge enable, PDO 5V/2A rebroadcast
        //   [7:3]=Reserved(default=0x01), [2]=0(fast charge enable), [0]=1(PDO rebroadcast enable)
        dev.writeReg(e_SW3526_Reg::CHG_CFG6, (0x01 << 3) | 0x01);  // 0x09
        
        // CHG_CFG8 (0xAE): Apple mode enable, no 18W limit, fast PD response
        //   [7]=0(reserved), [6]=0(no 18W limit), [5:3]=Reserved(default=0x3),
        //   [2]=1(Apple mode enable), [0]=1(PD response 1ms)
        dev.writeReg(e_SW3526_Reg::CHG_CFG8, (0x03 << 3) | 0x05);  // 0x1D
        
        // Force buck restart to apply power settings
        dev.forceBuckOff1S();
    };
    
    // Enable LCD power FIRST - screen will show while SW3526 configures
    HAL_GPIO_WritePin(LCD_EN_GPIO_Port, LCD_EN_Pin, GPIO_PIN_SET);
    HAL_Delay(5);  // Small delay for power stabilization
    
    // Initialize ST7735 LCD Display
    lcd.init(&hspi1,
             LCD_CS_GPIO_Port, LCD_CS_Pin,
             LCD_DC_GPIO_Port, LCD_DC_Pin,
             LCD_RESET_GPIO_Port, LCD_RESET_Pin,
             "LCD_MAIN");
    lcd.setRotation(ST7735Config::INIT_ROTATION);
    lcd.initInversion(ST7735Config::INIT_INVERT);
    lcd.begin();
    
    // Clear screen and show boot splash (centered on 160x80)
    lcd.fillScreenFast(ST7735_Color::BLACK);
    lcd.print(9, 31, Font_11x18, ST7735_Color::CYAN, ST7735_Color::BLACK,
              "PowerOneT2-V4");
    
    // Configure all 3 SW3526 channels first, then wait once
    // Buck forced-off timers run independently in hardware (1s auto-clear)
    configSW3526(sw3526_C3);
    configSW3526(sw3526_C1);
    configSW3526(sw3526_C2);
    HAL_Delay(1100);  // Single wait for all 3 buck-off periods (saves 2.2s)
    
    // Clear splash text before main UI starts updating
    lcd.fillScreenFast(ST7735_Color::BLACK);
}

/**
 * @brief Update display with power data - flicker-free, centered alignment
 * 
 * Display layout (160x80):
 * Line 1 (Y=0):  VIN + Temp + Power (Font_7x10, height=10)
 * Line 2 (Y=13): [C3]/[C1]/[C2] OR Protocol (Font_11x18, height=18)
 * Line 3 (Y=33): Output voltage XX.X (Font_11x18, height=18)
 * Line 4 (Y=53): Output current X.YY (Font_11x18, height=18)
 */
void updateDisplay(void)
{
    // Column positions for Font_11x18 (centered layout)
    constexpr uint8_t COL1_X = 3;    // Column 1 X position (moved left 5px)
    constexpr uint8_t COL2_X = 58;   // Column 2 X position (center)
    constexpr uint8_t COL3_X = 113;  // Column 3 X position (moved right 5px)
    
    // ===== Unified Y Coordinates for All Lines =====
    constexpr uint8_t LINE1_Y = 0;      // VIN + Temp + Power (Font_7x10, height=10)
    constexpr uint8_t LINE2_Y = 13;     // Channel/Protocol (Font_11x18, height=18)
    constexpr uint8_t LINE3_Y = 33;     // Voltage (Font_11x18, height=18)
    constexpr uint8_t LINE4_Y = 58;     // Current (Font_11x18, height=18)
    
    // Rounded rectangle background for protocol display
    constexpr uint8_t PROTOCOL_BG_WIDTH = 48;   // 4 chars * 11px + margin
    constexpr uint8_t PROTOCOL_BG_HEIGHT = 20;  // 18px font + 2px margin
    constexpr uint8_t PROTOCOL_BG_RADIUS = 3;   // Corner radius
    constexpr uint8_t PROTOCOL_BG_Y = LINE2_Y - 2;  // Top aligned with 2px margin (extended upward)
    constexpr uint16_t PROTOCOL_BG_COLOR = ST7735_Color::SLATE;  // Blue-gray for modern tech feel
    constexpr uint16_t BAR_BG_COLOR = ST7735_Color::GUNMETAL;    // Green-gray for progress bars
    
    // Progress bar dimensions
    constexpr uint8_t BAR_WIDTH = 42;   // Width matches ~4 chars + 2px extended
    constexpr uint8_t BAR_HEIGHT = 3;    // Height in pixels
    constexpr uint8_t VOLTAGE_BAR_Y = LINE3_Y + 18 + 1;  // Below voltage + 1px gap
    constexpr uint8_t CURRENT_BAR_Y = LINE4_Y + 18 + 1;  // Below current + 1px gap
    
    // Constants for progress calculation
    constexpr uint16_t MAX_VOLTAGE_MV = 22000;  // 22V max
    constexpr uint16_t MAX_CURRENT_MA = 4200;    // 4.2A max
    
    // Previous values for change detection
    static uint16_t prevVin = 0xFFFF;
    static uint16_t prevTemp = 0xFFFF;
    static uint32_t prevPowerMw = 0xFFFFFFFF;
    static uint16_t prevC3_vout = 0xFFFF, prevC3_iout = 0xFFFF;
    static uint16_t prevC1_vout = 0xFFFF, prevC1_iout = 0xFFFF;
    static uint16_t prevC2_vout = 0xFFFF, prevC2_iout = 0xFFFF;
    static e_SW3526_FastProt prevC3_prot = static_cast<e_SW3526_FastProt>(0xFF);
    static e_SW3526_FastProt prevC1_prot = static_cast<e_SW3526_FastProt>(0xFF);
    static e_SW3526_FastProt prevC2_prot = static_cast<e_SW3526_FastProt>(0xFF);
    static uint8_t animCounter = 0;  // Animation counter for idle bars
    static uint8_t prevAnimPos = 0;  // Previous slider position for incremental update
    
    // Progress bar animation constants
    constexpr uint8_t SEG_WIDTH = 12;  // Slider width in pixels
    constexpr uint8_t MAX_POS = BAR_WIDTH - SEG_WIDTH;  // Max slider position (30)
    constexpr uint8_t ANIM_FRAMES = MAX_POS * 2;  // Total frames for round-trip (60)
    
    // Increment animation counter (ping-pong: 0->30->0->30...)
    animCounter = (animCounter + 1) % ANIM_FRAMES;
    
    // Calculate ping-pong position
    uint8_t animPos;
    if (animCounter <= MAX_POS) {
        animPos = animCounter;  // Moving right: 0 → 30
    } else {
        animPos = ANIM_FRAMES - animCounter;  // Moving left: 30 → 0
    }
    
    // Calculate movement direction for incremental update
    int8_t animDirection = (animPos > prevAnimPos) ? 1 : ((animPos < prevAnimPos) ? -1 : 0);
    
    // Update previous position (after calculating direction)
    // Special case: wrap from MAX_POS to 0 or 0 to MAX_POS - keep previous for clean transition
    static bool isFirstFrame = true;
    if (isFirstFrame) {
        isFirstFrame = false;
        // On first frame, we need to draw the full bar
    }
    prevAnimPos = animPos;
    
    // Reset bar index for this frame (used by drawAnimBar)
    // static uint8_t barIndex = 0;  // REMOVED - using explicit indices instead
    
    // ========== Protocol Color Helper ==========
    auto getProtocolColor = [](e_SW3526_FastProt prot) -> uint16_t {
        // All protocols display in green
        (void)prot;  // Unused
        return ST7735_Color::GREEN;
    };
    
    // Get VIN from C3 (input voltage shared across all channels)
    uint16_t vin = sw3526_C3.getVin();
    
    // Read temperature from NTC sensor (DMA mode, non-blocking)
    float tempFloat = ntcTemp.readTemperature();
    uint16_t temp = (tempFloat < -50.0f) ? 0 : static_cast<uint16_t>(tempFloat);
    
    // Get measurements from each channel
    SW3526_Measurement measC3 = sw3526_C3.getAllMeasurements();
    SW3526_Measurement measC1 = sw3526_C1.getAllMeasurements();
    SW3526_Measurement measC2 = sw3526_C2.getAllMeasurements();
    
    // Calculate total power (in milliwatts)
    uint32_t totalPowerMw = measC3.power_mw + measC1.power_mw + measC2.power_mw;
    uint16_t totalPowerW = totalPowerMw / 1000;
    uint16_t totalPowerDeciW = (totalPowerMw % 1000) / 100;
    
    // Get protocols
    e_SW3526_FastProt protC3 = sw3526_C3.getFastStatus();
    e_SW3526_FastProt protC1 = sw3526_C1.getFastStatus();
    e_SW3526_FastProt protC2 = sw3526_C2.getFastStatus();
    
    // ========== Line 1: VIN (left) + Temperature (center) + Power (right) ==========
    // Independent refresh for each value (optimized)
    if (vin != prevVin) {
        uint16_t vin_v = vin / 1000;
        uint16_t vin_mv = (vin % 1000) / 10;
        lcd.print(2, LINE1_Y, Font_7x10, ST7735_Color::BRIGHT_RED, ST7735_Color::BLACK,
                  "%02d.%02dV", vin_v, vin_mv);
        prevVin = vin;
    }
    
    if (temp != prevTemp) {
        // Temperature color coding: 0-10=blue, 10-30=green, 30-50=yellow, 50+=red
        uint16_t temp_color;
        if (temp < 10) {
            temp_color = ST7735_Color::BRIGHT_BLUE;  // Cold: blue
        } else if (temp < 30) {
            temp_color = ST7735_Color::GREEN;        // Normal: green
        } else if (temp < 50) {
            temp_color = ST7735_Color::YELLOW;       // Warm: yellow
        } else {
            temp_color = ST7735_Color::BRIGHT_RED;   // Hot: red
        }
        lcd.print(65, LINE1_Y, Font_7x10, temp_color, ST7735_Color::BLACK,
                  "%02d'C", temp % 100);
        prevTemp = temp;
    }
    
    if (totalPowerMw != prevPowerMw) {
        lcd.print(114, LINE1_Y, Font_7x10, ST7735_Color::WHITE, ST7735_Color::BLACK,
                  "%03d.%dW", totalPowerW % 1000, totalPowerDeciW);
        prevPowerMw = totalPowerMw;
    }
    
    // ========== Line 2: Channel names or Protocol (Font_11x18) ==========
    // Show [C3]/[C1]/[C2] in WHITE when NONE, otherwise show protocol with unique colors
    if (protC3 != prevC3_prot || protC1 != prevC1_prot || protC2 != prevC2_prot) {
        // Draw rounded rectangle backgrounds (gray with rounded corners)
        // Simple approach: draw main rectangle, then fix corners with black
        auto drawRoundRectBg = [&](uint8_t rx, uint8_t ry) {
            // Draw main rectangle
            lcd.fillRectangleFast(rx, ry, PROTOCOL_BG_WIDTH, PROTOCOL_BG_HEIGHT, PROTOCOL_BG_COLOR);
            
            // Clear corners to create rounded appearance
            // Top-left corner (clear 2x2 area, keep only pixels at radius)
            lcd.fillRectangleFast(rx, ry, PROTOCOL_BG_RADIUS, 1, ST7735_Color::BLACK);
            lcd.fillRectangleFast(rx, ry + 1, 1, PROTOCOL_BG_RADIUS - 1, ST7735_Color::BLACK);
            
            // Top-right corner
            lcd.fillRectangleFast(rx + PROTOCOL_BG_WIDTH - PROTOCOL_BG_RADIUS, ry, PROTOCOL_BG_RADIUS, 1, ST7735_Color::BLACK);
            lcd.fillRectangleFast(rx + PROTOCOL_BG_WIDTH - 1, ry + 1, 1, PROTOCOL_BG_RADIUS - 1, ST7735_Color::BLACK);
            
            // Bottom-left corner
            lcd.fillRectangleFast(rx, ry + PROTOCOL_BG_HEIGHT - 1, PROTOCOL_BG_RADIUS, 1, ST7735_Color::BLACK);
            lcd.fillRectangleFast(rx, ry + PROTOCOL_BG_HEIGHT - PROTOCOL_BG_RADIUS + 1, 1, PROTOCOL_BG_RADIUS - 1, ST7735_Color::BLACK);
            
            // Bottom-right corner
            lcd.fillRectangleFast(rx + PROTOCOL_BG_WIDTH - PROTOCOL_BG_RADIUS, ry + PROTOCOL_BG_HEIGHT - 1, PROTOCOL_BG_RADIUS, 1, ST7735_Color::BLACK);
            lcd.fillRectangleFast(rx + PROTOCOL_BG_WIDTH - 1, ry + PROTOCOL_BG_HEIGHT - PROTOCOL_BG_RADIUS + 1, 1, PROTOCOL_BG_RADIUS - 1, ST7735_Color::BLACK);
        };
        
        // Draw backgrounds for all three protocol areas
        drawRoundRectBg(COL1_X - 2, PROTOCOL_BG_Y);
        drawRoundRectBg(COL2_X - 2, PROTOCOL_BG_Y);
        drawRoundRectBg(COL3_X - 2, PROTOCOL_BG_Y);
        
        // C3 display
        const char* c3_display;
        uint16_t c3_color;
        if (protC3 == e_SW3526_FastProt::NONE) {
            c3_display = " C3 ";  // Centered with spaces: " C3 "
            c3_color = ST7735_Color::WHITE;
        } else {
            c3_display = SW3526::getProtocolName(protC3);
            c3_color = getProtocolColor(protC3);
        }
        
        // C1 display
        const char* c1_display;
        uint16_t c1_color;
        if (protC1 == e_SW3526_FastProt::NONE) {
            c1_display = " C1 ";  // Centered with spaces: " C1 "
            c1_color = ST7735_Color::WHITE;
        } else {
            c1_display = SW3526::getProtocolName(protC1);
            c1_color = getProtocolColor(protC1);
        }
        
        // C2 display
        const char* c2_display;
        uint16_t c2_color;
        if (protC2 == e_SW3526_FastProt::NONE) {
            c2_display = " C2 ";  // Centered with spaces: " C2 "
            c2_color = ST7735_Color::WHITE;
        } else {
            c2_display = SW3526::getProtocolName(protC2);
            c2_color = getProtocolColor(protC2);
        }
        
        lcd.print(COL1_X, LINE2_Y, Font_11x18, c1_color, PROTOCOL_BG_COLOR, "%-4s", c1_display);
        lcd.print(COL2_X, LINE2_Y, Font_11x18, c2_color, PROTOCOL_BG_COLOR, "%-4s", c2_display);
        lcd.print(COL3_X, LINE2_Y, Font_11x18, c3_color, PROTOCOL_BG_COLOR, "%-4s", c3_display);
        
        prevC1_prot = protC1;
        prevC2_prot = protC2;
        prevC3_prot = protC3;
    }
    
    // ========== Line 3: Output voltage (XX.X format with Font_11x18) ==========
    if (measC1.vout_mv != prevC1_vout || measC2.vout_mv != prevC2_vout || measC3.vout_mv != prevC3_vout) {
        // C1 voltage: XX.X (zero-padded)
        uint16_t c1_v = measC1.vout_mv / 1000;
        uint16_t c1_dv = (measC1.vout_mv % 1000) / 100;
        lcd.print(COL1_X, LINE3_Y, Font_11x18, ST7735_Color::CYAN, ST7735_Color::BLACK,
                  "%02d.%d", c1_v, c1_dv);
        
        // C2 voltage: XX.X (zero-padded)
        uint16_t c2_v = measC2.vout_mv / 1000;
        uint16_t c2_dv = (measC2.vout_mv % 1000) / 100;
        lcd.print(COL2_X, LINE3_Y, Font_11x18, ST7735_Color::CYAN, ST7735_Color::BLACK,
                  "%02d.%d", c2_v, c2_dv);
        
        // C3 voltage: XX.X (zero-padded)
        uint16_t c3_v = measC3.vout_mv / 1000;
        uint16_t c3_dv = (measC3.vout_mv % 1000) / 100;
        lcd.print(COL3_X, LINE3_Y, Font_11x18, ST7735_Color::CYAN, ST7735_Color::BLACK,
                  "%02d.%d", c3_v, c3_dv);
        
        prevC1_vout = measC1.vout_mv;
        prevC2_vout = measC2.vout_mv;
        prevC3_vout = measC3.vout_mv;
    }
    
    // Progress bar animation helper - incremental update (only redraws changed pixels)
    // Uses static variables to track previous state per bar position
    static uint8_t prevBarPos[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};  // Invalid initial = force full redraw
    
    auto drawAnimBar = [&](uint8_t barIdx, uint8_t x, uint8_t y, uint16_t color) {
        // barIdx: 0=C3电压, 1=C1电压, 2=C2电压, 3=C3电流, 4=C1电流, 5=C2电流
        // Column mapping: COL1=C1(idx1/4), COL2=C2(idx2/5), COL3=C3(idx0/3)
        if (barIdx >= 6) barIdx = 0;  // Safety check
        
        uint8_t prevPos = prevBarPos[barIdx];
        
        // Full redraw on first frame, position change, or every frame for simplicity
        // Clear the entire bar area first
        lcd.fillRectangleFast(x, y, BAR_WIDTH, BAR_HEIGHT, BAR_BG_COLOR);
        
        // Draw slider at current position (always 12 pixels wide)
        lcd.fillRectangleFast(x + animPos, y, SEG_WIDTH, BAR_HEIGHT, color);
        
        prevBarPos[barIdx] = animPos;
    };
    
    // ========== Voltage Progress Bars (always update for animation) ==========
    // C1 voltage bar: scroll when voltage=0 OR current=0
    uint8_t c1_vbar_width = (measC1.vout_mv * BAR_WIDTH) / MAX_VOLTAGE_MV;
    if (c1_vbar_width > BAR_WIDTH) c1_vbar_width = BAR_WIDTH;
    lcd.fillRectangleFast(COL1_X, VOLTAGE_BAR_Y, BAR_WIDTH, BAR_HEIGHT, BAR_BG_COLOR);
    if (measC1.vout_mv > 0 && measC1.iout_ma > 0) {
        lcd.fillRectangleFast(COL1_X, VOLTAGE_BAR_Y, c1_vbar_width, BAR_HEIGHT, ST7735_Color::CYAN);
    } else {
        drawAnimBar(1, COL1_X, VOLTAGE_BAR_Y, ST7735_Color::CYAN);
    }
    
    // C2 voltage bar: scroll when voltage=0 OR voltage<1V
    uint8_t c2_vbar_width = (measC2.vout_mv * BAR_WIDTH) / MAX_VOLTAGE_MV;
    if (c2_vbar_width > BAR_WIDTH) c2_vbar_width = BAR_WIDTH;
    lcd.fillRectangleFast(COL2_X, VOLTAGE_BAR_Y, BAR_WIDTH, BAR_HEIGHT, BAR_BG_COLOR);
    if (measC2.vout_mv >= 1000) {
        lcd.fillRectangleFast(COL2_X, VOLTAGE_BAR_Y, c2_vbar_width, BAR_HEIGHT, ST7735_Color::CYAN);
    } else {
        drawAnimBar(2, COL2_X, VOLTAGE_BAR_Y, ST7735_Color::CYAN);
    }
    
    // C3 voltage bar: scroll when voltage=0 OR voltage<1V
    uint8_t c3_vbar_width = (measC3.vout_mv * BAR_WIDTH) / MAX_VOLTAGE_MV;
    if (c3_vbar_width > BAR_WIDTH) c3_vbar_width = BAR_WIDTH;
    lcd.fillRectangleFast(COL3_X, VOLTAGE_BAR_Y, BAR_WIDTH, BAR_HEIGHT, BAR_BG_COLOR);
    if (measC3.vout_mv >= 1000) {
        lcd.fillRectangleFast(COL3_X, VOLTAGE_BAR_Y, c3_vbar_width, BAR_HEIGHT, ST7735_Color::CYAN);
    } else {
        drawAnimBar(0, COL3_X, VOLTAGE_BAR_Y, ST7735_Color::CYAN);
    }
    
    // ========== Line 3: Output current (X.YY format with Font_11x18) ==========
    if (measC1.iout_ma != prevC1_iout || measC2.iout_ma != prevC2_iout || measC3.iout_ma != prevC3_iout) {
        // C1 current: X.YY
        uint16_t c1_a = measC1.iout_ma / 1000;
        uint16_t c1_hma = (measC1.iout_ma % 1000) / 10;
        lcd.print(COL1_X, LINE4_Y, Font_11x18, ST7735_Color::YELLOW, ST7735_Color::BLACK,
                  "%d.%02d", c1_a, c1_hma);
        
        // C2 current: X.YY
        uint16_t c2_a = measC2.iout_ma / 1000;
        uint16_t c2_hma = (measC2.iout_ma % 1000) / 10;
        lcd.print(COL2_X, LINE4_Y, Font_11x18, ST7735_Color::YELLOW, ST7735_Color::BLACK,
                  "%d.%02d", c2_a, c2_hma);
        
        // C3 current: X.YY
        uint16_t c3_a = measC3.iout_ma / 1000;
        uint16_t c3_hma = (measC3.iout_ma % 1000) / 10;
        lcd.print(COL3_X, LINE4_Y, Font_11x18, ST7735_Color::YELLOW, ST7735_Color::BLACK,
                  "%d.%02d", c3_a, c3_hma);
        
        prevC1_iout = measC1.iout_ma;
        prevC2_iout = measC2.iout_ma;
        prevC3_iout = measC3.iout_ma;
    }
    
    // ========== Current Progress Bars (always update for animation) ==========
    // C1 current bar
    uint8_t c1_ibar_width = (measC1.iout_ma * BAR_WIDTH) / MAX_CURRENT_MA;
    if (c1_ibar_width > BAR_WIDTH) c1_ibar_width = BAR_WIDTH;
    lcd.fillRectangleFast(COL1_X, CURRENT_BAR_Y, BAR_WIDTH, BAR_HEIGHT, BAR_BG_COLOR);
    if (measC1.iout_ma > 0) {
        lcd.fillRectangleFast(COL1_X, CURRENT_BAR_Y, c1_ibar_width, BAR_HEIGHT, ST7735_Color::YELLOW);
    } else {
        drawAnimBar(4, COL1_X, CURRENT_BAR_Y, ST7735_Color::YELLOW);
    }
    
    // C2 current bar: show actual value when voltage >= 1V, regardless of current
    uint8_t c2_ibar_width = (measC2.iout_ma * BAR_WIDTH) / MAX_CURRENT_MA;
    if (c2_ibar_width > BAR_WIDTH) c2_ibar_width = BAR_WIDTH;
    lcd.fillRectangleFast(COL2_X, CURRENT_BAR_Y, BAR_WIDTH, BAR_HEIGHT, BAR_BG_COLOR);
    if (measC2.vout_mv >= 1000) {
        // Voltage >= 1V: always show actual current (even if 0)
        lcd.fillRectangleFast(COL2_X, CURRENT_BAR_Y, c2_ibar_width, BAR_HEIGHT, ST7735_Color::YELLOW);
    } else {
        // Voltage < 1V: idle animation
        drawAnimBar(5, COL2_X, CURRENT_BAR_Y, ST7735_Color::YELLOW);
    }
    
    // C3 current bar: show actual value when voltage >= 1V, regardless of current
    uint8_t c3_ibar_width = (measC3.iout_ma * BAR_WIDTH) / MAX_CURRENT_MA;
    if (c3_ibar_width > BAR_WIDTH) c3_ibar_width = BAR_WIDTH;
    lcd.fillRectangleFast(COL3_X, CURRENT_BAR_Y, BAR_WIDTH, BAR_HEIGHT, BAR_BG_COLOR);
    if (measC3.vout_mv >= 1000) {
        // Voltage >= 1V: always show actual current (even if 0)
        lcd.fillRectangleFast(COL3_X, CURRENT_BAR_Y, c3_ibar_width, BAR_HEIGHT, ST7735_Color::YELLOW);
    } else {
        // Voltage < 1V: idle animation
        drawAnimBar(3, COL3_X, CURRENT_BAR_Y, ST7735_Color::YELLOW);
    }
}

/**
 * @brief Process power data and log
 */
void processPowerData(void)
{
    // Temperature monitoring and power data processing
    // UART logging moved to main loop for unified output format
    (void)0;  // Placeholder - no operation needed
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_I2C1_Init();
  MX_I2C2_Init();
  MX_SPI1_Init();
  MX_TIM17_Init();
  /* USER CODE BEGIN 2 */
  
  // Initialize custom devices
  initDevices();
  
  // Start timer for polling mode
  HAL_TIM_Base_Start(&htim17);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    
    // Main application loop
    updateDisplay();
    processPowerData();
    
    // Polling mode: check timer for 1-second interval
    static uint32_t lastTimerValue = 0;
    static uint32_t timeAccumulated = 0;
    
    uint32_t currentTimerValue = __HAL_TIM_GET_COUNTER(&htim17);
    uint32_t timerDelta;
    
    // Calculate delta since last check (handle timer wrap-around)
    if (currentTimerValue >= lastTimerValue) {
        timerDelta = currentTimerValue - lastTimerValue;
    } else {
        // Timer wrapped from ARR to 0
        timerDelta = (htim17.Instance->ARR + 1) - lastTimerValue + currentTimerValue;
    }
    
    timeAccumulated += timerDelta;
    lastTimerValue = currentTimerValue;
    
    // Check if 1 second has passed (1000 counts at 1kHz timer)
    if (timeAccumulated >= 1000) {
        timeAccumulated -= 1000;
        runningSeconds++;
    }
    
    // Small delay to prevent overwhelming the system
    HAL_Delay(100);
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV1;
  RCC_OscInitStruct.PLL.PLLN = 8;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

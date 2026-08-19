/**
 * @file ST7735.hpp
 * @brief ST7735 LCD Display Driver - C++17 with DMA Optimization
 * @author LoveFinder
 * @date 2026
 * 
 * Features:
 * - DMA-optimized transfers (double buffering)
 * - Non-blocking operations where possible
 * - Multi-device support
 */

#ifndef ST7735_HPP
#define ST7735_HPP

#include "main.h"
#include "font.h"
#include <cstdint>
#include <cstdarg>
#include <array>

/*============================================================================
 * Display Configuration Constants
 *============================================================================*/

namespace ST7735Config {
    // DMA Buffer Configuration
    // Max line width * 2 bytes = 160 * 2 = 320
    constexpr uint16_t DMA_BUFFER_SIZE = 320;
    constexpr uint32_t DMA_TIMEOUT_MS = 100;   // DMA transfer timeout
    
    // MADCTL bit definitions
    constexpr uint8_t MADCTL_MY  = 0x80;
    constexpr uint8_t MADCTL_MX  = 0x40;
    constexpr uint8_t MADCTL_MV  = 0x20;
    constexpr uint8_t MADCTL_ML  = 0x10;
    constexpr uint8_t MADCTL_RGB = 0x00;
    constexpr uint8_t MADCTL_BGR = 0x08;
    constexpr uint8_t MADCTL_MH  = 0x04;
    
    // Per-orientation config
    struct RotationCfg {
        uint8_t  madctl;
        uint16_t width;
        uint16_t height;
        uint16_t xStart;
        uint16_t yStart;
    };
}

// Include panel-specific configuration (ROTATIONS, INIT_INVERT, INIT_ROTATION)
// Switch between batches by editing ST7735_PanelConfig.hpp

// Forward-declare rotation enum (needed by panel config before full definition)
enum class e_ST7735_Rotation : uint8_t {
    DEG_0   = 0,
    DEG_90  = 1,
    DEG_180 = 2,
    DEG_270 = 3
};

#include "ST7735_PanelConfig.hpp"

/*============================================================================
 * Rotation Enum (already declared above for panel config)
 *============================================================================*/

/*============================================================================
 * ST7735 Command Definitions
 *============================================================================*/

enum class e_ST7735_Cmd : uint8_t {
    NOP     = 0x00,
    SWRESET = 0x01,
    RDDID   = 0x04,
    RDDST   = 0x09,
    SLPIN   = 0x10,
    SLPOUT  = 0x11,
    PTLON   = 0x12,
    NORON   = 0x13,
    INVOFF  = 0x20,
    INVON   = 0x21,
    GAMSET  = 0x26,
    DISPOFF = 0x28,
    DISPON  = 0x29,
    CASET   = 0x2A,
    RASET   = 0x2B,
    RAMWR   = 0x2C,
    RAMRD   = 0x2E,
    PTLAR   = 0x30,
    COLMOD  = 0x3A,
    MADCTL  = 0x36,
    FRMCTR1 = 0xB1,
    FRMCTR2 = 0xB2,
    FRMCTR3 = 0xB3,
    INVCTR  = 0xB4,
    DISSET5 = 0xB6,
    PWCTR1  = 0xC0,
    PWCTR2  = 0xC1,
    PWCTR3  = 0xC2,
    PWCTR4  = 0xC3,
    PWCTR5  = 0xC4,
    VMCTR1  = 0xC5,
    RDID1   = 0xDA,
    RDID2   = 0xDB,
    RDID3   = 0xDC,
    RDID4   = 0xDD,
    PWCTR6  = 0xFC,
    GMCTRP1 = 0xE0,
    GMCTRN1 = 0xE1
};

/*============================================================================
 * Color Definitions (RGB565)
 *============================================================================*/

namespace ST7735_Color {
    constexpr uint16_t BLACK       = 0x0000;
    constexpr uint16_t BLUE        = 0x001F;
    constexpr uint16_t RED         = 0xF800;
    constexpr uint16_t GREEN       = 0x07E0;
    constexpr uint16_t CYAN        = 0x07FF;
    constexpr uint16_t MAGENTA     = 0xF81F;
    constexpr uint16_t YELLOW      = 0xFFE0;
    constexpr uint16_t WHITE       = 0xFFFF;
    constexpr uint16_t ORANGE      = 0xFC00;
    constexpr uint16_t GRAY        = 0x8410;
    
    // Enhanced gray tones for modern UI
    constexpr uint16_t DARK_GRAY   = 0x3186;  // Deep charcoal (49, 49, 49)
    constexpr uint16_t SLATE       = 0x3CD5;  // Blue-gray (60, 70, 80) - modern tech feel
    constexpr uint16_t CHARCOAL    = 0x2124;  // Dark warm gray (33, 36, 38)
    constexpr uint16_t GUNMETAL    = 0x2A69;  // Green-gray (42, 52, 50) - industrial
    constexpr uint16_t OBSIDIAN    = 0x18E3;  // Very dark blue-gray (25, 28, 35)
    constexpr uint16_t GRAPHITE    = 0x4A69;  // Medium cool gray (72, 78, 80)
    
    // Vibrant colors
    constexpr uint16_t BRIGHT_RED  = 0xF980;  // Bright red with slight orange tint (255, 48, 0)
    constexpr uint16_t BRIGHT_BLUE = 0x065F;  // Bright blue (0, 100, 255)
    
    constexpr uint16_t RGB565(uint8_t r, uint8_t g, uint8_t b) {
        return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3);
    }
}

/*============================================================================
 * Gamma Settings
 *============================================================================*/

enum class e_ST7735_Gamma : uint8_t {
    GAMMA_10 = 0x01,
    GAMMA_25 = 0x02,
    GAMMA_22 = 0x04,
    GAMMA_18 = 0x08
};

/*============================================================================
 * ST7735 Display Class - DMA Optimized
 *============================================================================*/

class ST7735 {
public:
    ST7735() = default;
    
    ST7735(SPI_HandleTypeDef* spi,
           GPIO_TypeDef* cs_port, uint16_t cs_pin,
           GPIO_TypeDef* dc_port, uint16_t dc_pin,
           GPIO_TypeDef* reset_port, uint16_t reset_pin,
           const char* name = "ST7735");
    
    void init(SPI_HandleTypeDef* spi,
              GPIO_TypeDef* cs_port, uint16_t cs_pin,
              GPIO_TypeDef* dc_port, uint16_t dc_pin,
              GPIO_TypeDef* reset_port, uint16_t reset_pin,
              const char* name = "ST7735");
    
    void begin();
    void initInversion(bool invert);
    
    // ========== Low-level Operations ==========
    
    void select();
    void deselect();
    void reset();
    
    // Command/Data (blocking, small data)
    void writeCommand(e_ST7735_Cmd cmd);
    void writeCommand(uint8_t cmd);
    void writeData(const uint8_t* data, size_t len);
    void writeData(uint8_t data);
    
    // DMA Operations
    void writeDataDMA(const uint8_t* data, size_t len);
    bool waitForDMA(uint32_t timeout_ms = ST7735Config::DMA_TIMEOUT_MS);
    bool isDMAReady() const;
    
    // ========== Drawing Primitives (DMA Optimized) ==========
    
    void drawPixel(uint16_t x, uint16_t y, uint16_t color);
    
    // Fill rectangle - ALL use DMA internally
    void fillRectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
    void fillRectangleFast(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
    void fillRectangleDMA(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
    
    // Fill screen - uses DMA
    void fillScreen(uint16_t color);
    void fillScreenFast(uint16_t color);
    void fillScreenDMA(uint16_t color);
    
    // Draw image - uses DMA
    void drawImage(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t* data);
    void drawImageDMA(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t* data);
    
    // ========== Text Drawing (DMA Optimized) ==========
    
    void writeChar(uint16_t x, uint16_t y, char ch, const FontDef& font, 
                   uint16_t color, uint16_t bgcolor);
    void writeCharDMA(uint16_t x, uint16_t y, char ch, const FontDef& font,
                      uint16_t color, uint16_t bgcolor);
    
    void writeString(uint16_t x, uint16_t y, const char* str, const FontDef& font,
                     uint16_t color, uint16_t bgcolor);
    void writeStringDMA(uint16_t x, uint16_t y, const char* str, const FontDef& font,
                        uint16_t color, uint16_t bgcolor);
    
    bool print(uint16_t x, uint16_t y, const FontDef& font, 
               uint16_t color, uint16_t bgcolor, const char* format, ...);
    bool printDMA(uint16_t x, uint16_t y, const FontDef& font,
                  uint16_t color, uint16_t bgcolor, const char* format, ...);
    
    bool vprint(uint16_t x, uint16_t y, const FontDef& font,
                uint16_t color, uint16_t bgcolor, const char* format, va_list args);
    
    // ========== Display Control ==========
    
    void invertColors(bool invert);
    void setGamma(e_ST7735_Gamma gamma);
    void setAddressWindow(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);
    void setRotation(e_ST7735_Rotation rotation);
    
    // ========== Utility ==========
    
    e_ST7735_Rotation getRotation() const { return m_rotation; }
    uint16_t getWidth() const { return m_width; }
    uint16_t getHeight() const { return m_height; }
    const char* getName() const { return m_name; }
    bool isInitialized() const { return m_spi != nullptr; }

private:
    SPI_HandleTypeDef* m_spi = nullptr;
    GPIO_TypeDef* m_csPort = nullptr;
    uint16_t m_csPin = 0;
    GPIO_TypeDef* m_dcPort = nullptr;
    uint16_t m_dcPin = 0;
    GPIO_TypeDef* m_resetPort = nullptr;
    uint16_t m_resetPin = 0;
    const char* m_name = "ST7735";
    bool m_begun = false;
    bool m_invertOnInit = false;
    
    // Rotation state
    e_ST7735_Rotation m_rotation = e_ST7735_Rotation::DEG_0;
    uint16_t m_width = 160;
    uint16_t m_height = 80;
    uint16_t m_xStart = 0;
    uint16_t m_yStart = 24;
    
    // Format buffer for print operations
    static constexpr size_t PRINT_BUFFER_SIZE = 256;
    char m_printBuffer[PRINT_BUFFER_SIZE];
    
    // DMA Double Buffer (ping-pong buffer for maximum throughput)
    // Aligned for DMA efficiency
    alignas(4) static uint8_t s_dmaBuffer[2][ST7735Config::DMA_BUFFER_SIZE];
    static uint8_t s_activeBuffer;  // Which buffer is currently being used
    
    void executeCommandList(const uint8_t* addr);
    
    // Helper: Fill line buffer with color
    static void fillLineBuffer(uint8_t* buffer, uint16_t color, uint16_t width);
    
    // Helper: DMA transfer with wait
    bool dmaTransfer(const uint8_t* data, size_t len);
};

/*============================================================================
 * ST7735 Manager - Multi-display Support
 *============================================================================*/

class ST7735_Manager {
public:
    static constexpr size_t MAX_DISPLAYS = 4;
    
    int8_t addDisplay(ST7735* display);
    ST7735* getDisplay(size_t index);
    size_t getDisplayCount() const { return m_count; }
    void fillAllScreens(uint16_t color);
    
private:
    std::array<ST7735*, MAX_DISPLAYS> m_displays = {};
    size_t m_count = 0;
};

/*============================================================================
 * C API Compatibility Layer
 *============================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

void v_ST7735_Init(ST7735* dev, SPI_HandleTypeDef* spi,
                   GPIO_TypeDef* cs_port, uint16_t cs_pin,
                   GPIO_TypeDef* dc_port, uint16_t dc_pin,
                   GPIO_TypeDef* reset_port, uint16_t reset_pin);
void v_ST7735_Begin(ST7735* dev);
void v_ST7735_FillScreen(ST7735* dev, uint16_t color);
void v_ST7735_FillScreenDMA(ST7735* dev, uint16_t color);
void v_ST7735_WriteString(ST7735* dev, uint16_t x, uint16_t y, const char* str,
                          const FontDef* font, uint16_t color, uint16_t bgcolor);

#ifdef __cplusplus
}
#endif

#endif // ST7735_HPP

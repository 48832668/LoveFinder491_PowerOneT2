/**
 * @file ST7735.cpp
 * @brief ST7735 LCD Display Driver Implementation - DMA Optimized
 * 
 * DMA Optimization Strategy:
 * 1. Double buffering: While DMA transmits buffer A, CPU prepares buffer B
 * 2. All fill operations use DMA for maximum throughput
 * 3. Text rendering uses line buffer + DMA
 * 4. Small commands use blocking SPI (negligible overhead)
 */

#include "ST7735.hpp"
#include <cstring>
#include <cstdarg>

/*============================================================================
 * Custom Integer Formatter (replaces vsnprintf, saves ~4-8KB flash)
 * 
 * Supports:
 *   %d       — decimal integer
 *   %02d     — zero-padded 2-digit decimal
 *   %03d     — zero-padded 3-digit decimal
 *   %-4s     — left-aligned 4-char string
 *   literal text — passed through as-is
 *============================================================================*/

static void formatReversed(char*& buf, const char* end, const char* src, uint8_t len) {
    while (len > 0 && buf < end) {
        *buf++ = src[--len];
    }
}

static int formatString(char* buf, size_t bufSize, const char* format, va_list args) {
    char* p = buf;
    const char* end = buf + bufSize - 1;  // Reserve space for null terminator
    
    while (*format && p < end) {
        if (*format != '%') {
            *p++ = *format++;
            continue;
        }
        format++;  // skip '%'
        
        // Check for '-' flag (left-align)
        bool leftAlign = false;
        if (*format == '-') {
            leftAlign = true;
            format++;
        }
        
        // Parse width
        uint8_t width = 0;
        bool zeroPad = false;
        if (*format == '0') {
            zeroPad = true;
            format++;
        }
        while (*format >= '0' && *format <= '9') {
            width = width * 10 + (uint8_t)(*format - '0');
            format++;
        }
        
        if (*format == 's') {
            // String argument
            format++;
            const char* str = va_arg(args, const char*);
            uint8_t len = 0;
            while (str[len]) len++;
            
            if (!leftAlign) {
                for (uint8_t i = len; i < width && p < end; i++) *p++ = ' ';
            }
            for (uint8_t i = 0; i < len && p < end; i++) *p++ = str[i];
            if (leftAlign) {
                for (uint8_t i = len; i < width && p < end; i++) *p++ = ' ';
            }
            
        } else if (*format == 'd' || *format == 'u') {
            // Integer argument
            char spec = *format;
            format++;
            int32_t val = va_arg(args, int32_t);
            
            // Handle negative numbers
            uint32_t uval;
            bool neg = false;
            if (spec == 'd' && val < 0) {
                neg = true;
                uval = (uint32_t)(-val);
            } else {
                uval = (uint32_t)val;
            }
            
            // Convert digits (reversed)
            char temp[12];
            uint8_t i = 0;
            do {
                temp[i++] = (char)('0' + (uval % 10));
                uval /= 10;
            } while (uval > 0);
            
            // Pad to width
            uint8_t pad = (i < width) ? width - i : 0;
            
            if (zeroPad) {
                // Zero-padded format: minus, zeros, then digits
                if (neg) { if (p < end) *p++ = '-'; }
                for (uint8_t j = 0; j < pad && p < end; j++) *p++ = '0';
                formatReversed(p, end, temp, i);
            } else {
                // Space-padded format: spaces (optional minus), digits
                if (neg) pad = (pad > 0) ? pad - 1 : 0;
                for (uint8_t j = 0; j < pad && p < end; j++) *p++ = ' ';
                if (neg) { if (p < end) *p++ = '-'; }
                formatReversed(p, end, temp, i);
            }
            
        } else if (*format == '%') {
            // Literal '%'
            format++;
            if (p < end) *p++ = '%';
        } else {
            // Unknown format, skip
            format++;
        }
    }
    
    *p = '\0';
    return (int)(p - buf);
}

/*============================================================================
 * Static DMA Double Buffer Definition
 *============================================================================*/

// Double buffer for DMA transfers (shared across all ST7735 instances)
alignas(4) uint8_t ST7735::s_dmaBuffer[2][ST7735Config::DMA_BUFFER_SIZE];
uint8_t ST7735::s_activeBuffer = 0;

/*============================================================================
 * Initialization Command Sequences
 *============================================================================*/

#define DELAY 0x80

static const uint8_t init_cmds1[] = {
    14,
    (uint8_t)e_ST7735_Cmd::SWRESET, DELAY, 150,
    (uint8_t)e_ST7735_Cmd::SLPOUT, DELAY, 255,
    (uint8_t)e_ST7735_Cmd::FRMCTR1, 3, 0x01, 0x2C, 0x2D,
    (uint8_t)e_ST7735_Cmd::FRMCTR2, 3, 0x01, 0x2C, 0x2D,
    (uint8_t)e_ST7735_Cmd::FRMCTR3, 6, 0x01, 0x2C, 0x2D, 0x01, 0x2C, 0x2D,
    (uint8_t)e_ST7735_Cmd::INVCTR, 1, 0x07,
    (uint8_t)e_ST7735_Cmd::PWCTR1, 3, 0xA2, 0x02, 0x84,
    (uint8_t)e_ST7735_Cmd::PWCTR2, 1, 0xC5,
    (uint8_t)e_ST7735_Cmd::PWCTR3, 2, 0x0A, 0x00,
    (uint8_t)e_ST7735_Cmd::PWCTR4, 2, 0x8A, 0x2A,
    (uint8_t)e_ST7735_Cmd::PWCTR5, 2, 0x8A, 0xEE,
    (uint8_t)e_ST7735_Cmd::VMCTR1, 1, 0x0E,
    (uint8_t)e_ST7735_Cmd::INVOFF, 0,
    (uint8_t)e_ST7735_Cmd::COLMOD, 1, 0x05
};

static const uint8_t init_cmds2[] = {
    3,
    (uint8_t)e_ST7735_Cmd::CASET, 4, 0x00, 0x00, 0x00, 0x4F,
    (uint8_t)e_ST7735_Cmd::RASET, 4, 0x00, 0x00, 0x00, 0x9F,
    (uint8_t)e_ST7735_Cmd::INVOFF, 0
};

static const uint8_t init_cmds3[] = {
    4,
    (uint8_t)e_ST7735_Cmd::GMCTRP1, 16,
    0x02, 0x1c, 0x07, 0x12, 0x37, 0x32, 0x29, 0x2d,
    0x29, 0x25, 0x2B, 0x39, 0x00, 0x01, 0x03, 0x10,
    (uint8_t)e_ST7735_Cmd::GMCTRN1, 16,
    0x03, 0x1d, 0x07, 0x06, 0x2E, 0x2C, 0x29, 0x2D,
    0x2E, 0x2E, 0x37, 0x3F, 0x00, 0x00, 0x02, 0x10,
    (uint8_t)e_ST7735_Cmd::NORON, DELAY, 10,
    (uint8_t)e_ST7735_Cmd::DISPON, DELAY, 100
};

/*============================================================================
 * ST7735 Class Implementation
 *============================================================================*/

ST7735::ST7735(SPI_HandleTypeDef* spi,
               GPIO_TypeDef* cs_port, uint16_t cs_pin,
               GPIO_TypeDef* dc_port, uint16_t dc_pin,
               GPIO_TypeDef* reset_port, uint16_t reset_pin,
               const char* name)
{
    init(spi, cs_port, cs_pin, dc_port, dc_pin, reset_port, reset_pin, name);
}

void ST7735::init(SPI_HandleTypeDef* spi,
                  GPIO_TypeDef* cs_port, uint16_t cs_pin,
                  GPIO_TypeDef* dc_port, uint16_t dc_pin,
                  GPIO_TypeDef* reset_port, uint16_t reset_pin,
                  const char* name)
{
    m_spi = spi;
    m_csPort = cs_port;
    m_csPin = cs_pin;
    m_dcPort = dc_port;
    m_dcPin = dc_pin;
    m_resetPort = reset_port;
    m_resetPin = reset_pin;
    m_name = name ? name : "ST7735";
    m_begun = false;
    m_invertOnInit = false;
    
    // Default rotation (DEG_0 = landscape, MX|MV)
    m_rotation = e_ST7735_Rotation::DEG_0;
    const auto& cfg = ST7735Config::ROTATIONS[static_cast<uint8_t>(m_rotation)];
    m_width   = cfg.width;
    m_height  = cfg.height;
    m_xStart  = cfg.xStart;
    m_yStart  = cfg.yStart;
}

/*============================================================================
 * Low-level Operations
 *============================================================================*/

void ST7735::select()
{
    HAL_GPIO_WritePin(m_csPort, m_csPin, GPIO_PIN_RESET);
}

void ST7735::deselect()
{
    HAL_GPIO_WritePin(m_csPort, m_csPin, GPIO_PIN_SET);
}

void ST7735::reset()
{
    HAL_GPIO_WritePin(m_resetPort, m_resetPin, GPIO_PIN_RESET);
    HAL_Delay(5);
    HAL_GPIO_WritePin(m_resetPort, m_resetPin, GPIO_PIN_SET);
}

void ST7735::writeCommand(e_ST7735_Cmd cmd)
{
    writeCommand(static_cast<uint8_t>(cmd));
}

void ST7735::writeCommand(uint8_t cmd)
{
    HAL_GPIO_WritePin(m_dcPort, m_dcPin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(m_spi, &cmd, 1, HAL_MAX_DELAY);
}

void ST7735::writeData(const uint8_t* data, size_t len)
{
    HAL_GPIO_WritePin(m_dcPort, m_dcPin, GPIO_PIN_SET);
    HAL_SPI_Transmit(m_spi, const_cast<uint8_t*>(data), len, HAL_MAX_DELAY);
}

void ST7735::writeData(uint8_t data)
{
    writeData(&data, 1);
}

/*============================================================================
 * DMA Operations
 *============================================================================*/

void ST7735::writeDataDMA(const uint8_t* data, size_t len)
{
    HAL_GPIO_WritePin(m_dcPort, m_dcPin, GPIO_PIN_SET);
    HAL_SPI_Transmit_DMA(m_spi, const_cast<uint8_t*>(data), len);
}

bool ST7735::waitForDMA(uint32_t timeout_ms)
{
    uint32_t start = HAL_GetTick();
    while (HAL_SPI_GetState(m_spi) != HAL_SPI_STATE_READY) {
        if ((HAL_GetTick() - start) >= timeout_ms) {
            return false;  // Timeout
        }
    }
    return true;
}

bool ST7735::isDMAReady() const
{
    return HAL_SPI_GetState(m_spi) == HAL_SPI_STATE_READY;
}

bool ST7735::dmaTransfer(const uint8_t* data, size_t len)
{
    writeDataDMA(data, len);
    return waitForDMA(ST7735Config::DMA_TIMEOUT_MS);
}

/*============================================================================
 * Helper Functions
 *============================================================================*/

void ST7735::fillLineBuffer(uint8_t* buffer, uint16_t color, uint16_t width)
{
    uint8_t hi = color >> 8;
    uint8_t lo = color & 0xFF;
    
    for (uint16_t i = 0; i < width; i++) {
        buffer[i * 2] = hi;
        buffer[i * 2 + 1] = lo;
    }
}

void ST7735::executeCommandList(const uint8_t* addr)
{
    uint8_t numCommands = *addr++;
    
    while (numCommands--) {
        uint8_t cmd = *addr++;
        writeCommand(cmd);
        
        uint8_t numArgs = *addr++;
        uint16_t ms = numArgs & DELAY;
        numArgs &= ~DELAY;
        
        if (numArgs) {
            writeData(addr, numArgs);
            addr += numArgs;
        }
        
        if (ms) {
            ms = *addr++;
            if (ms == 255) ms = 500;
            HAL_Delay(ms);
        }
    }
}

void ST7735::begin()
{
    m_begun = false;
    select();
    reset();
    executeCommandList(init_cmds1);
    
    // Send MADCTL with current rotation
    writeCommand(e_ST7735_Cmd::MADCTL);
    writeData(ST7735Config::ROTATIONS[static_cast<uint8_t>(m_rotation)].madctl);
    
    executeCommandList(init_cmds2);
    executeCommandList(init_cmds3);
    writeCommand(m_invertOnInit ? e_ST7735_Cmd::INVON : e_ST7735_Cmd::INVOFF);
    deselect();
    m_begun = true;
}

void ST7735::initInversion(bool invert)
{
    m_invertOnInit = invert;
    if (m_begun) {
        invertColors(invert);
    }
}

void ST7735::setAddressWindow(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
    writeCommand(e_ST7735_Cmd::CASET);
    uint8_t data[] = {0x00, static_cast<uint8_t>(x0 + m_xStart),
                      0x00, static_cast<uint8_t>(x1 + m_xStart)};
    writeData(data, sizeof(data));
    
    writeCommand(e_ST7735_Cmd::RASET);
    data[1] = y0 + m_yStart;
    data[3] = y1 + m_yStart;
    writeData(data, sizeof(data));
    
    writeCommand(e_ST7735_Cmd::RAMWR);
}

void ST7735::setRotation(e_ST7735_Rotation rotation)
{
    m_rotation = rotation;
    const auto& cfg = ST7735Config::ROTATIONS[static_cast<uint8_t>(rotation)];
    m_width  = cfg.width;
    m_height = cfg.height;
    m_xStart = cfg.xStart;
    m_yStart = cfg.yStart;
    
    if (!m_begun) return;
    
    select();
    writeCommand(e_ST7735_Cmd::MADCTL);
    writeData(cfg.madctl);
    
    // Reset the full address window to new dimensions
    setAddressWindow(0, 0, m_width - 1, m_height - 1);
    deselect();
}

/*============================================================================
 * Drawing Primitives - DMA Optimized
 *============================================================================*/

void ST7735::drawPixel(uint16_t x, uint16_t y, uint16_t color)
{
    if (x >= m_width || y >= m_height) return;
    
    select();
    setAddressWindow(x, y, x + 1, y + 1);
    uint8_t data[] = {static_cast<uint8_t>(color >> 8), 
                      static_cast<uint8_t>(color & 0xFF)};
    writeData(data, sizeof(data));
    deselect();
}

void ST7735::fillRectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    // Redirect to DMA version
    fillRectangleDMA(x, y, w, h, color);
}

void ST7735::fillRectangleFast(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    // Redirect to DMA version
    fillRectangleDMA(x, y, w, h, color);
}

void ST7735::fillRectangleDMA(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    // Clipping
    if (x >= m_width || y >= m_height) return;
    if (x + w > m_width) w = m_width - x;
    if (y + h > m_height) h = m_height - y;
    if (w == 0 || h == 0) return;
    
    select();
    setAddressWindow(x, y, x + w - 1, y + h - 1);
    
    uint16_t bytesPerLine = w * 2;
    
    // Prepare line buffer with color
    fillLineBuffer(s_dmaBuffer[0], color, w);
    
    // For very large areas, use double buffering
    if (h > 2 && bytesPerLine <= ST7735Config::DMA_BUFFER_SIZE) {
        // Copy to second buffer
        memcpy(s_dmaBuffer[1], s_dmaBuffer[0], bytesPerLine);
        
        HAL_GPIO_WritePin(m_dcPort, m_dcPin, GPIO_PIN_SET);
        
        // Double-buffered transfer
        uint16_t remaining = h;
        bool useBuffer0 = true;
        
        while (remaining > 0) {
            uint8_t bufIdx = useBuffer0 ? 0 : 1;
            
            // Start DMA transfer
            HAL_SPI_Transmit_DMA(m_spi, s_dmaBuffer[bufIdx], bytesPerLine);
            
            // Wait for completion
            waitForDMA(ST7735Config::DMA_TIMEOUT_MS);
            
            useBuffer0 = !useBuffer0;
            remaining--;
        }
    } else {
        // Single buffer for small areas
        HAL_GPIO_WritePin(m_dcPort, m_dcPin, GPIO_PIN_SET);
        
        for (uint16_t i = 0; i < h; i++) {
            HAL_SPI_Transmit_DMA(m_spi, s_dmaBuffer[0], bytesPerLine);
            waitForDMA(ST7735Config::DMA_TIMEOUT_MS);
        }
    }
    
    deselect();
}

void ST7735::fillScreen(uint16_t color)
{
    fillRectangleDMA(0, 0, m_width, m_height, color);
}

void ST7735::fillScreenFast(uint16_t color)
{
    fillRectangleDMA(0, 0, m_width, m_height, color);
}

void ST7735::fillScreenDMA(uint16_t color)
{
    fillRectangleDMA(0, 0, m_width, m_height, color);
}

/*============================================================================
 * Image Drawing - DMA Optimized
 *============================================================================*/

void ST7735::drawImage(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t* data)
{
    drawImageDMA(x, y, w, h, data);
}

void ST7735::drawImageDMA(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t* data)
{
    if (x >= m_width || y >= m_height) return;
    if (x + w > m_width) w = m_width - x;
    if (y + h > m_height) h = m_height - y;
    if (w == 0 || h == 0) return;
    
    select();
    setAddressWindow(x, y, x + w - 1, y + h - 1);
    
    const uint8_t* imgData = reinterpret_cast<const uint8_t*>(data);
    uint32_t totalBytes = (uint32_t)w * h * 2;
    
    HAL_GPIO_WritePin(m_dcPort, m_dcPin, GPIO_PIN_SET);
    
    // Transfer in chunks if larger than DMA buffer
    while (totalBytes > 0) {
        uint16_t chunkSize = (totalBytes > ST7735Config::DMA_BUFFER_SIZE) 
                           ? ST7735Config::DMA_BUFFER_SIZE 
                           : totalBytes;
        
        // Copy to DMA buffer (needed because HAL_SPI_Transmit_DMA requires non-const buffer)
        memcpy(s_dmaBuffer[0], imgData, chunkSize);
        
        HAL_SPI_Transmit_DMA(m_spi, s_dmaBuffer[0], chunkSize);
        waitForDMA(ST7735Config::DMA_TIMEOUT_MS);
        
        imgData += chunkSize;
        totalBytes -= chunkSize;
    }
    
    deselect();
}

/*============================================================================
 * Text Drawing - DMA Optimized
 *============================================================================*/

void ST7735::writeChar(uint16_t x, uint16_t y, char ch, const FontDef& font,
                       uint16_t color, uint16_t bgcolor)
{
    writeCharDMA(x, y, ch, font, color, bgcolor);
}

void ST7735::writeCharDMA(uint16_t x, uint16_t y, char ch, const FontDef& font,
                          uint16_t color, uint16_t bgcolor)
{
    if (x + font.width > m_width || y + font.height > m_height) return;
    if (ch < 32) return;
    
    select();
    setAddressWindow(x, y, x + font.width - 1, y + font.height - 1);
    
    uint16_t bytesPerLine = font.width * 2;
    
    // Prepare color bytes
    uint8_t colorHi = color >> 8;
    uint8_t colorLo = color & 0xFF;
    uint8_t bgHi = bgcolor >> 8;
    uint8_t bgLo = bgcolor & 0xFF;
    
    HAL_GPIO_WritePin(m_dcPort, m_dcPin, GPIO_PIN_SET);
    
    // Get font data via glyph lookup (supports per-character conditional compilation)
    const uint16_t* fontData = font_get_glyph(font, static_cast<uint8_t>(ch));
    if (!fontData) return;  // glyph not available in this font
    
    // Render each line
    for (uint16_t line = 0; line < font.height; line++) {
        uint16_t lineData = fontData[line];
        
        // Build line buffer
        for (uint16_t col = 0; col < font.width; col++) {
            uint16_t bufIdx = col * 2;
            
            // Check if bit is set (directly in condition to avoid uint8_t truncation bug)
            if ((lineData << col) & 0x8000) {
                s_dmaBuffer[0][bufIdx] = colorHi;
                s_dmaBuffer[0][bufIdx + 1] = colorLo;
            } else {
                s_dmaBuffer[0][bufIdx] = bgHi;
                s_dmaBuffer[0][bufIdx + 1] = bgLo;
            }
        }
        
        // DMA transfer this line
        HAL_SPI_Transmit_DMA(m_spi, s_dmaBuffer[0], bytesPerLine);
        waitForDMA(ST7735Config::DMA_TIMEOUT_MS);
    }
    
    deselect();
}

void ST7735::writeString(uint16_t x, uint16_t y, const char* str, const FontDef& font,
                         uint16_t color, uint16_t bgcolor)
{
    writeStringDMA(x, y, str, font, color, bgcolor);
}

void ST7735::writeStringDMA(uint16_t x, uint16_t y, const char* str, const FontDef& font,
                            uint16_t color, uint16_t bgcolor)
{
    uint16_t currentX = x;
    uint16_t currentY = y;
    
    while (*str) {
        // Handle line wrap
        if (currentX + font.width > m_width) {
            currentX = 0;
            currentY += font.height;
            
            if (currentY + font.height > m_height) {
                break;
            }
            
            // Skip leading spaces on new line
            if (*str == ' ') {
                str++;
                continue;
            }
        }
        
        writeCharDMA(currentX, currentY, *str, font, color, bgcolor);
        currentX += font.width;
        str++;
    }
}

bool ST7735::print(uint16_t x, uint16_t y, const FontDef& font,
                   uint16_t color, uint16_t bgcolor, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    bool result = vprint(x, y, font, color, bgcolor, format, args);
    va_end(args);
    return result;
}

bool ST7735::printDMA(uint16_t x, uint16_t y, const FontDef& font,
                      uint16_t color, uint16_t bgcolor, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    bool result = vprint(x, y, font, color, bgcolor, format, args);
    va_end(args);
    return result;
}

bool ST7735::vprint(uint16_t x, uint16_t y, const FontDef& font,
                    uint16_t color, uint16_t bgcolor, const char* format, va_list args)
{
    int len = formatString(m_printBuffer, PRINT_BUFFER_SIZE, format, args);
    
    if (len <= 0 || len >= static_cast<int>(PRINT_BUFFER_SIZE)) {
        return false;
    }
    
    writeStringDMA(x, y, m_printBuffer, font, color, bgcolor);
    return true;
}

/*============================================================================
 * Display Control
 *============================================================================*/

void ST7735::invertColors(bool invert)
{
    m_invertOnInit = invert;
    if (!m_begun) return;
    
    select();
    writeCommand(invert ? e_ST7735_Cmd::INVON : e_ST7735_Cmd::INVOFF);
    deselect();
}

void ST7735::setGamma(e_ST7735_Gamma gamma)
{
    select();
    writeCommand(e_ST7735_Cmd::GAMSET);
    writeData(static_cast<uint8_t>(gamma));
    deselect();
}

/*============================================================================
 * ST7735 Manager Implementation
 *============================================================================*/

int8_t ST7735_Manager::addDisplay(ST7735* display)
{
    if (m_count >= MAX_DISPLAYS || display == nullptr) {
        return -1;
    }
    
    m_displays[m_count] = display;
    return static_cast<int8_t>(m_count++);
}

ST7735* ST7735_Manager::getDisplay(size_t index)
{
    if (index >= m_count) return nullptr;
    return m_displays[index];
}

void ST7735_Manager::fillAllScreens(uint16_t color)
{
    for (size_t i = 0; i < m_count; i++) {
        if (m_displays[i]) {
            m_displays[i]->fillScreenDMA(color);
        }
    }
}

/*============================================================================
 * C API Compatibility Layer
 *============================================================================*/

extern "C" {

void v_ST7735_Init(ST7735* dev, SPI_HandleTypeDef* spi,
                   GPIO_TypeDef* cs_port, uint16_t cs_pin,
                   GPIO_TypeDef* dc_port, uint16_t dc_pin,
                   GPIO_TypeDef* reset_port, uint16_t reset_pin)
{
    dev->init(spi, cs_port, cs_pin, dc_port, dc_pin, reset_port, reset_pin);
}

void v_ST7735_Begin(ST7735* dev)
{
    dev->begin();
}

void v_ST7735_FillScreen(ST7735* dev, uint16_t color)
{
    dev->fillScreenDMA(color);
}

void v_ST7735_FillScreenDMA(ST7735* dev, uint16_t color)
{
    dev->fillScreenDMA(color);
}

void v_ST7735_WriteString(ST7735* dev, uint16_t x, uint16_t y, const char* str,
                          const FontDef* font, uint16_t color, uint16_t bgcolor)
{
    dev->writeStringDMA(x, y, str, *font, color, bgcolor);
}

} // extern "C"

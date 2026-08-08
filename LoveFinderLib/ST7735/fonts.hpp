/**
 * @file fonts.hpp
 * @brief Font Definitions for LCD Display - C++17
 * @author LoveFinder
 * @date 2026
 * 
 * Provides bitmap fonts for ST7735 LCD display.
 * Font selection is controlled by USE_FONT_* / USE_* defines
 * from fonts_config.hpp — only enabled fonts and characters are compiled.
 * 
 * Each character is a separate static constexpr array, allowing
 * per-character selective compilation via PickSoul (FontHub Editor).
 */

#ifndef FONTS_HPP
#define FONTS_HPP

#include <cstdint>
#include "fonts_config.hpp"

/*============================================================================
 * Font Structure
 *============================================================================*/

struct FontDef {
    const uint8_t width;    // Character width in pixels
    const uint8_t height;   // Character height in pixels
    const uint16_t* data;   // Bitmap data array (nullptr for per-glyph fonts)
};

/*============================================================================
 * Available Fonts (extern declarations — conditional)
 *============================================================================*/

#if USE_FONT_7X10
// Small font: 7x10 pixels
extern const FontDef Font_7x10;
#endif

#if USE_FONT_11X18
// Medium font: 11x18 pixels
extern const FontDef Font_11x18;
#endif

#if USE_FONT_16X26
// Large font: 16x26 pixels
extern const FontDef Font_16x26;
#endif

/*============================================================================
 * Glyph Lookup — Per-Character Conditional Access
 *============================================================================*/

/**
 * @brief Get glyph bitmap data for a character in the given font.
 * 
 * This replaces direct array indexing of font.data[].
 * Each font stores only its used characters; missing chars return nullptr.
 * 
 * @param font Font definition
 * @param ch   ASCII code (32-126)
 * @return Pointer to height uint16_t values, or nullptr if glyph absent
 */
const uint16_t* font_get_glyph(const FontDef& font, uint8_t ch);

/**
 * @brief Get glyph bitmap data for a Unicode character.
 * @param font Font definition
 * @param uni  Unicode code point
 * @return Pointer to height uint16_t values, or nullptr if glyph absent
 */
const uint16_t* font_get_glyph_unicode(const FontDef& font, uint16_t uni);

/*============================================================================
 * Font Manager (Optional — for runtime font selection)
 *============================================================================*/

enum class e_Font_Size : uint8_t {
#if USE_FONT_7X10
    Small  = 0,   // 7x10
#endif
#if USE_FONT_11X18
    Medium = 1,   // 11x18
#endif
#if USE_FONT_16X26
    Large  = 2,   // 16x26
#endif
};

namespace FontManager {
    /**
     * @brief Get font by size enum
     * @param size Font size
     * @return Reference to font definition
     */
    inline const FontDef& getFont(e_Font_Size size) {
        switch (size) {
#if USE_FONT_7X10
            case e_Font_Size::Small:  return Font_7x10;
#endif
#if USE_FONT_11X18
            case e_Font_Size::Medium: return Font_11x18;
#endif
#if USE_FONT_16X26
            case e_Font_Size::Large:  return Font_16x26;
#endif
            default:
#if USE_FONT_7X10
                return Font_7x10;
#else
                return *static_cast<const FontDef*>(nullptr);
#endif
        }
    }
    
    /**
     * @brief Calculate text width
     * @param text Text string
     * @param font Font to use
     * @return Width in pixels
     */
    inline uint16_t getTextWidth(const char* text, const FontDef& font) {
        uint16_t width = 0;
        while (*text) {
            width += font.width;
            text++;
        }
        return width;
    }
    
    /**
     * @brief Calculate text height
     * @param font Font to use
     * @return Height in pixels
     */
    inline uint8_t getTextHeight(const FontDef& font) {
        return font.height;
    }
}

#endif // FONTS_HPP

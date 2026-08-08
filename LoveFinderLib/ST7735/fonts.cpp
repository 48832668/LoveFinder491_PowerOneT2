/**
 * @file fonts.cpp
 * @brief Font Bitmap Data — Per-Character Selective Compilation
 *
 * Based on: https://github.com/afiskon/stm32-st7735
 * Adapted for C++17 with constexpr support and per-glyph lookup.
 * Each character is guarded by its own USE_<SIZE>_<NAME> macro.
 * Only characters with their macro set to 1 are compiled into firmware.
 */

#include "fonts.hpp"
#include "fonts_config.hpp"

// ============================================================
// Font_7x10 (7x10) — per-character conditional compilation
// Used for status line: VIN/Temp/Power
// ============================================================

#if USE_FONT_7X10

#if USE_7X10_SPACE
// ASCII 32 ( )
static constexpr uint16_t s1_space[10] = {
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
};
#endif

#if USE_7X10_SQUOTE
// ASCII 39 (')
static constexpr uint16_t s1_squote[10] = {
    0x1000, 0x1000, 0x1000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
};
#endif

#if USE_7X10_DOT
// ASCII 46 (.)
static constexpr uint16_t s1_dot[10] = {
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x1000, 0x0000, 0x0000,
};
#endif

#if USE_7X10_0
// ASCII 48 (0)
static constexpr uint16_t s1_0[10] = {
    0x3800, 0x4400, 0x4400, 0x5400, 0x4400, 0x4400, 0x4400, 0x3800, 0x0000, 0x0000,
};
#endif

#if USE_7X10_1
// ASCII 49 (1)
static constexpr uint16_t s1_1[10] = {
    0x1000, 0x3000, 0x5000, 0x1000, 0x1000, 0x1000, 0x1000, 0x1000, 0x0000, 0x0000,
};
#endif

#if USE_7X10_2
// ASCII 50 (2)
static constexpr uint16_t s1_2[10] = {
    0x3800, 0x4400, 0x4400, 0x0400, 0x0800, 0x1000, 0x2000, 0x7C00, 0x0000, 0x0000,
};
#endif

#if USE_7X10_3
// ASCII 51 (3)
static constexpr uint16_t s1_3[10] = {
    0x3800, 0x4400, 0x0400, 0x1800, 0x0400, 0x0400, 0x4400, 0x3800, 0x0000, 0x0000,
};
#endif

#if USE_7X10_4
// ASCII 52 (4)
static constexpr uint16_t s1_4[10] = {
    0x0800, 0x1800, 0x2800, 0x2800, 0x4800, 0x7C00, 0x0800, 0x0800, 0x0000, 0x0000,
};
#endif

#if USE_7X10_5
// ASCII 53 (5)
static constexpr uint16_t s1_5[10] = {
    0x7C00, 0x4000, 0x4000, 0x7800, 0x0400, 0x0400, 0x4400, 0x3800, 0x0000, 0x0000,
};
#endif

#if USE_7X10_6
// ASCII 54 (6)
static constexpr uint16_t s1_6[10] = {
    0x3800, 0x4400, 0x4000, 0x7800, 0x4400, 0x4400, 0x4400, 0x3800, 0x0000, 0x0000,
};
#endif

#if USE_7X10_7
// ASCII 55 (7)
static constexpr uint16_t s1_7[10] = {
    0x7C00, 0x0400, 0x0800, 0x1000, 0x1000, 0x2000, 0x2000, 0x2000, 0x0000, 0x0000,
};
#endif

#if USE_7X10_8
// ASCII 56 (8)
static constexpr uint16_t s1_8[10] = {
    0x3800, 0x4400, 0x4400, 0x3800, 0x4400, 0x4400, 0x4400, 0x3800, 0x0000, 0x0000,
};
#endif

#if USE_7X10_9
// ASCII 57 (9)
static constexpr uint16_t s1_9[10] = {
    0x3800, 0x4400, 0x4400, 0x4400, 0x3C00, 0x0400, 0x4400, 0x3800, 0x0000, 0x0000,
};
#endif

#if USE_7X10_C
// ASCII 67 (C)
static constexpr uint16_t s1_C[10] = {
    0x3800, 0x4400, 0x4000, 0x4000, 0x4000, 0x4000, 0x4400, 0x3800, 0x0000, 0x0000,
};
#endif

#if USE_7X10_V
// ASCII 86 (V)
static constexpr uint16_t s1_V[10] = {
    0x4400, 0x4400, 0x4400, 0x2800, 0x2800, 0x2800, 0x1000, 0x1000, 0x0000, 0x0000,
};
#endif

#if USE_7X10_W
// ASCII 87 (W)
static constexpr uint16_t s1_W[10] = {
    0x4400, 0x4400, 0x5400, 0x5400, 0x5400, 0x6C00, 0x2800, 0x2800, 0x0000, 0x0000,
};
#endif

static const uint16_t* glyph_table_s1(uint8_t ch) {
    switch (ch) {
#if USE_7X10_SPACE
        case 32:  return s1_space;
#endif
#if USE_7X10_SQUOTE
        case 39:  return s1_squote;
#endif
#if USE_7X10_DOT
        case 46:  return s1_dot;
#endif
#if USE_7X10_0
        case 48:  return s1_0;
#endif
#if USE_7X10_1
        case 49:  return s1_1;
#endif
#if USE_7X10_2
        case 50:  return s1_2;
#endif
#if USE_7X10_3
        case 51:  return s1_3;
#endif
#if USE_7X10_4
        case 52:  return s1_4;
#endif
#if USE_7X10_5
        case 53:  return s1_5;
#endif
#if USE_7X10_6
        case 54:  return s1_6;
#endif
#if USE_7X10_7
        case 55:  return s1_7;
#endif
#if USE_7X10_8
        case 56:  return s1_8;
#endif
#if USE_7X10_9
        case 57:  return s1_9;
#endif
#if USE_7X10_C
        case 67:  return s1_C;
#endif
#if USE_7X10_V
        case 86:  return s1_V;
#endif
#if USE_7X10_W
        case 87:  return s1_W;
#endif
        default:  return nullptr;
    }
}

const FontDef Font_7x10 = {7, 10, nullptr};
#endif /* USE_FONT_7X10 */

// ============================================================
// Font_11x18 (11x18) — per-character conditional compilation
// Used for protocol/voltage/current display
// ============================================================

#if USE_FONT_11X18

#if USE_11X18_SPACE
// ASCII 32 ( )
static constexpr uint16_t s2_space[18] = {
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
};
#endif

#if USE_11X18_DASH
// ASCII 45 (-)
static constexpr uint16_t s2_dash[18] = {
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x1E00, 0x1E00, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
};
#endif

#if USE_11X18_DOT
// ASCII 46 (.)
static constexpr uint16_t s2_dot[18] = {
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0C00, 0x0C00, 0x0000, 0x0000, 0x0000,
};
#endif

#if USE_11X18_0
// ASCII 48 (0)
static constexpr uint16_t s2_0[18] = {
    0x0000, 0x1E00, 0x3F00, 0x3300, 0x6180, 0x6180, 0x6180, 0x6D80, 0x6D80,
    0x6180, 0x6180, 0x6180, 0x3300, 0x3F00, 0x1E00, 0x0000, 0x0000, 0x0000,
};
#endif

#if USE_11X18_1
// ASCII 49 (1)
static constexpr uint16_t s2_1[18] = {
    0x0000, 0x0600, 0x0E00, 0x1E00, 0x3600, 0x2600, 0x0600, 0x0600, 0x0600,
    0x0600, 0x0600, 0x0600, 0x0600, 0x0600, 0x0600, 0x0000, 0x0000, 0x0000,
};
#endif

#if USE_11X18_2
// ASCII 50 (2)
static constexpr uint16_t s2_2[18] = {
    0x0000, 0x1E00, 0x3F00, 0x7380, 0x6180, 0x6180, 0x0180, 0x0300, 0x0600,
    0x0C00, 0x1800, 0x3000, 0x6000, 0x7F80, 0x7F80, 0x0000, 0x0000, 0x0000,
};
#endif

#if USE_11X18_3
// ASCII 51 (3)
static constexpr uint16_t s2_3[18] = {
    0x0000, 0x1C00, 0x3E00, 0x6300, 0x6300, 0x0300, 0x0E00, 0x0E00, 0x0300,
    0x0180, 0x0180, 0x6180, 0x7380, 0x3F00, 0x1E00, 0x0000, 0x0000, 0x0000,
};
#endif

#if USE_11X18_4
// ASCII 52 (4)
static constexpr uint16_t s2_4[18] = {
    0x0000, 0x0600, 0x0E00, 0x0E00, 0x1E00, 0x1E00, 0x1600, 0x3600, 0x3600,
    0x6600, 0x7F80, 0x7F80, 0x0600, 0x0600, 0x0600, 0x0000, 0x0000, 0x0000,
};
#endif

#if USE_11X18_5
// ASCII 53 (5)
static constexpr uint16_t s2_5[18] = {
    0x0000, 0x7F00, 0x7F00, 0x6000, 0x6000, 0x6000, 0x6E00, 0x7F00, 0x6380,
    0x0180, 0x0180, 0x6180, 0x7380, 0x3F00, 0x1E00, 0x0000, 0x0000, 0x0000,
};
#endif

#if USE_11X18_6
// ASCII 54 (6)
static constexpr uint16_t s2_6[18] = {
    0x0000, 0x1E00, 0x3F00, 0x3380, 0x6180, 0x6000, 0x6E00, 0x7F00, 0x7380,
    0x6180, 0x6180, 0x6180, 0x3380, 0x3F00, 0x1E00, 0x0000, 0x0000, 0x0000,
};
#endif

#if USE_11X18_7
// ASCII 55 (7)
static constexpr uint16_t s2_7[18] = {
    0x0000, 0x7F80, 0x7F80, 0x0180, 0x0300, 0x0300, 0x0600, 0x0600, 0x0C00,
    0x0C00, 0x0C00, 0x0800, 0x1800, 0x1800, 0x1800, 0x0000, 0x0000, 0x0000,
};
#endif

#if USE_11X18_8
// ASCII 56 (8)
static constexpr uint16_t s2_8[18] = {
    0x0000, 0x1E00, 0x3F00, 0x6380, 0x6180, 0x6180, 0x2100, 0x1E00, 0x3F00,
    0x6180, 0x6180, 0x6180, 0x6180, 0x3F00, 0x1E00, 0x0000, 0x0000, 0x0000,
};
#endif

#if USE_11X18_9
// ASCII 57 (9)
static constexpr uint16_t s2_9[18] = {
    0x0000, 0x1E00, 0x3F00, 0x7300, 0x6180, 0x6180, 0x6180, 0x7380, 0x3F80,
    0x1D80, 0x0180, 0x6180, 0x7300, 0x3F00, 0x1E00, 0x0000, 0x0000, 0x0000,
};
#endif

#if USE_11X18_A
// ASCII 65 (A)
static constexpr uint16_t s2_A[18] = {
    0x0000, 0x0E00, 0x0E00, 0x1B00, 0x1B00, 0x1B00, 0x1B00, 0x3180, 0x3180,
    0x3F80, 0x3F80, 0x3180, 0x60C0, 0x60C0, 0x60C0, 0x0000, 0x0000, 0x0000,
};
#endif

#if USE_11X18_C
// ASCII 67 (C)
static constexpr uint16_t s2_C[18] = {
    0x0000, 0x1E00, 0x3F00, 0x3180, 0x6180, 0x6000, 0x6000, 0x6000, 0x6000,
    0x6000, 0x6000, 0x6180, 0x3180, 0x3F00, 0x1E00, 0x0000, 0x0000, 0x0000,
};
#endif

#if USE_11X18_D
// ASCII 68 (D)
static constexpr uint16_t s2_D[18] = {
    0x0000, 0x7C00, 0x7F00, 0x6300, 0x6380, 0x6180, 0x6180, 0x6180, 0x6180,
    0x6180, 0x6180, 0x6300, 0x6300, 0x7E00, 0x7C00, 0x0000, 0x0000, 0x0000,
};
#endif

#if USE_11X18_E
// ASCII 69 (E)
static constexpr uint16_t s2_E[18] = {
    0x0000, 0x7F80, 0x7F80, 0x6000, 0x6000, 0x6000, 0x6000, 0x7F00, 0x7F00,
    0x6000, 0x6000, 0x6000, 0x6000, 0x7F80, 0x7F80, 0x0000, 0x0000, 0x0000,
};
#endif

#if USE_11X18_F
// ASCII 70 (F)
static constexpr uint16_t s2_F[18] = {
    0x0000, 0x7F80, 0x7F80, 0x6000, 0x6000, 0x6000, 0x6000, 0x7F00, 0x7F00,
    0x6000, 0x6000, 0x6000, 0x6000, 0x6000, 0x6000, 0x0000, 0x0000, 0x0000,
};
#endif

#if USE_11X18_K
// ASCII 75 (K)
static constexpr uint16_t s2_K[18] = {
    0x0000, 0x60C0, 0x6180, 0x6300, 0x6600, 0x6600, 0x6C00, 0x7800, 0x7C00,
    0x6600, 0x6600, 0x6300, 0x6180, 0x6180, 0x60C0, 0x0000, 0x0000, 0x0000,
};
#endif

#if USE_11X18_N
// ASCII 78 (N)
static constexpr uint16_t s2_N[18] = {
    0x0000, 0x7180, 0x7180, 0x7980, 0x7980, 0x7980, 0x6D80, 0x6D80, 0x6D80,
    0x6580, 0x6780, 0x6780, 0x6780, 0x6380, 0x6380, 0x0000, 0x0000, 0x0000,
};
#endif

#if USE_11X18_O
// ASCII 79 (O)
static constexpr uint16_t s2_O[18] = {
    0x0000, 0x1E00, 0x3F00, 0x3300, 0x6180, 0x6180, 0x6180, 0x6180, 0x6180,
    0x6180, 0x6180, 0x6180, 0x3300, 0x3F00, 0x1E00, 0x0000, 0x0000, 0x0000,
};
#endif

#if USE_11X18_P
// ASCII 80 (P)
static constexpr uint16_t s2_P[18] = {
    0x0000, 0x7E00, 0x7F00, 0x6380, 0x6180, 0x6180, 0x6180, 0x6380, 0x7F00,
    0x7E00, 0x6000, 0x6000, 0x6000, 0x6000, 0x6000, 0x0000, 0x0000, 0x0000,
};
#endif

#if USE_11X18_Q
// ASCII 81 (Q)
static constexpr uint16_t s2_Q[18] = {
    0x0000, 0x1E00, 0x3F00, 0x3300, 0x6180, 0x6180, 0x6180, 0x6180, 0x6180,
    0x6180, 0x6580, 0x6780, 0x3300, 0x3F80, 0x1E40, 0x0000, 0x0000, 0x0000,
};
#endif

#if USE_11X18_S
// ASCII 83 (S)
static constexpr uint16_t s2_S[18] = {
    0x0000, 0x0E00, 0x1F00, 0x3180, 0x3180, 0x3000, 0x3800, 0x1E00, 0x0700,
    0x0380, 0x6180, 0x6180, 0x3180, 0x3F00, 0x1E00, 0x0000, 0x0000, 0x0000,
};
#endif

#if USE_11X18_T
// ASCII 84 (T)
static constexpr uint16_t s2_T[18] = {
    0x0000, 0xFFC0, 0xFFC0, 0x0C00, 0x0C00, 0x0C00, 0x0C00, 0x0C00, 0x0C00,
    0x0C00, 0x0C00, 0x0C00, 0x0C00, 0x0C00, 0x0C00, 0x0000, 0x0000, 0x0000,
};
#endif

#if USE_11X18_U
// ASCII 85 (U)
static constexpr uint16_t s2_U[18] = {
    0x0000, 0x6180, 0x6180, 0x6180, 0x6180, 0x6180, 0x6180, 0x6180, 0x6180,
    0x6180, 0x6180, 0x6180, 0x7380, 0x3F00, 0x1E00, 0x0000, 0x0000, 0x0000,
};
#endif

#if USE_11X18_V
// ASCII 86 (V)
static constexpr uint16_t s2_V[18] = {
    0x0000, 0x60C0, 0x60C0, 0x60C0, 0x3180, 0x3180, 0x3180, 0x1B00, 0x1B00,
    0x1B00, 0x1B00, 0x0E00, 0x0E00, 0x0E00, 0x0400, 0x0000, 0x0000, 0x0000,
};
#endif

#if USE_11X18_W
// ASCII 87 (W)
static constexpr uint16_t s2_W[18] = {
    0x0000, 0xC0C0, 0xC0C0, 0xC0C0, 0xC0C0, 0xC0C0, 0xCCC0, 0x4C80, 0x4C80,
    0x5E80, 0x5280, 0x5280, 0x7380, 0x6180, 0x6180, 0x0000, 0x0000, 0x0000,
};
#endif

#if USE_11X18_X
// ASCII 88 (X)
static constexpr uint16_t s2_X[18] = {
    0x0000, 0xC0C0, 0x6080, 0x6180, 0x3300, 0x3B00, 0x1E00, 0x0C00, 0x0C00,
    0x1E00, 0x1F00, 0x3B00, 0x7180, 0x6180, 0xC0C0, 0x0000, 0x0000, 0x0000,
};
#endif

#if USE_11X18_o
// ASCII 111 (o)
static constexpr uint16_t s2_o[18] = {
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x1E00, 0x3F00, 0x7380, 0x6180,
    0x6180, 0x6180, 0x6180, 0x7380, 0x3F00, 0x1E00, 0x0000, 0x0000, 0x0000,
};
#endif

#if USE_11X18_w
// ASCII 119 (w)
static constexpr uint16_t s2_w[18] = {
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xDD80, 0xDD80, 0xDD80, 0x5500,
    0x5500, 0x5500, 0x7700, 0x7700, 0x2200, 0x2200, 0x0000, 0x0000, 0x0000,
};
#endif

#if USE_11X18_e
// ASCII 101 (e)
static constexpr uint16_t s2_e[18] = {
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x1E00, 0x3F00, 0x7300, 0x6180,
    0x7F80, 0x7F80, 0x6000, 0x7180, 0x3F00, 0x1E00, 0x0000, 0x0000, 0x0000,
};
#endif

#if USE_11X18_r
// ASCII 114 (r)
static constexpr uint16_t s2_r[18] = {
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x6700, 0x3F80, 0x3900, 0x3000,
    0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x0000, 0x0000, 0x0000,
};
#endif

#if USE_11X18_n
// ASCII 110 (n)
static constexpr uint16_t s2_n[18] = {
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x6F00, 0x7F80, 0x7180, 0x6180,
    0x6180, 0x6180, 0x6180, 0x6180, 0x6180, 0x6180, 0x0000, 0x0000, 0x0000,
};
#endif

static const uint16_t* glyph_table_s2(uint8_t ch) {
    switch (ch) {
#if USE_11X18_SPACE
        case 32:  return s2_space;
#endif
#if USE_11X18_DASH
        case 45:  return s2_dash;
#endif
#if USE_11X18_DOT
        case 46:  return s2_dot;
#endif
#if USE_11X18_0
        case 48:  return s2_0;
#endif
#if USE_11X18_1
        case 49:  return s2_1;
#endif
#if USE_11X18_2
        case 50:  return s2_2;
#endif
#if USE_11X18_3
        case 51:  return s2_3;
#endif
#if USE_11X18_4
        case 52:  return s2_4;
#endif
#if USE_11X18_5
        case 53:  return s2_5;
#endif
#if USE_11X18_6
        case 54:  return s2_6;
#endif
#if USE_11X18_7
        case 55:  return s2_7;
#endif
#if USE_11X18_8
        case 56:  return s2_8;
#endif
#if USE_11X18_9
        case 57:  return s2_9;
#endif
#if USE_11X18_A
        case 65:  return s2_A;
#endif
#if USE_11X18_C
        case 67:  return s2_C;
#endif
#if USE_11X18_D
        case 68:  return s2_D;
#endif
#if USE_11X18_E
        case 69:  return s2_E;
#endif
#if USE_11X18_F
        case 70:  return s2_F;
#endif
#if USE_11X18_K
        case 75:  return s2_K;
#endif
#if USE_11X18_N
        case 78:  return s2_N;
#endif
#if USE_11X18_O
        case 79:  return s2_O;
#endif
#if USE_11X18_P
        case 80:  return s2_P;
#endif
#if USE_11X18_Q
        case 81:  return s2_Q;
#endif
#if USE_11X18_S
        case 83:  return s2_S;
#endif
#if USE_11X18_T
        case 84:  return s2_T;
#endif
#if USE_11X18_U
        case 85:  return s2_U;
#endif
#if USE_11X18_V
        case 86:  return s2_V;
#endif
#if USE_11X18_W
        case 87:  return s2_W;
#endif
#if USE_11X18_X
        case 88:  return s2_X;
#endif
#if USE_11X18_e
        case 101: return s2_e;
#endif
#if USE_11X18_n
        case 110: return s2_n;
#endif
#if USE_11X18_o
        case 111: return s2_o;
#endif
#if USE_11X18_r
        case 114: return s2_r;
#endif
#if USE_11X18_w
        case 119: return s2_w;
#endif
        default:  return nullptr;
    }
}

const FontDef Font_11x18 = {11, 18, nullptr};
#endif /* USE_FONT_11X18 */

// ============================================================
// Font_16x26 (16x26) — DISABLED
// ============================================================

#if USE_FONT_16X26
const FontDef Font_16x26 = {16, 26, nullptr};
#endif /* USE_FONT_16X26 */

/*============================================================================
 * Global Glyph Lookup
 *============================================================================*/

const uint16_t* font_get_glyph(const FontDef& font, uint8_t ch) {
#if USE_FONT_7X10
    if (&font == &Font_7x10) return glyph_table_s1(ch);
#endif
#if USE_FONT_11X18
    if (&font == &Font_11x18) return glyph_table_s2(ch);
#endif
    return nullptr;
}

const uint16_t* font_get_glyph_unicode(const FontDef& font, uint16_t uni) {
    if (uni <= 127) {
        return font_get_glyph(font, static_cast<uint8_t>(uni));
    }
    return nullptr;
}

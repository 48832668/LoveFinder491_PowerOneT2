/**
 * @file fonts_config.hpp
 * @brief Font Compilation Configuration — Per-Character Selective Compilation
 * @author LoveFinder
 * @date 2026
 * 
 * Each font AND each individual character can be independently enabled/disabled.
 * Override any define via compiler -D flag before modifying this file.
 * 
 * Naming convention:
 *   USE_FONT_<SIZE>   — enable the font struct itself (must be 1 for any char to work)
 *   USE_<SIZE>_<NAME> — enable a specific character glyph
 * 
 * Special character names:
 *   space exclamation dquote hash dollar percent ampersand squote
 *   lparen rparen asterisk plus comma dash dot slash
 *   colon semicolon lt eq gt question at
 *   lbracket backslash rbracket caret underscore backtick
 *   lbrace pipe rbrace tilde
 */

#ifndef FONTS_CONFIG_HPP
#define FONTS_CONFIG_HPP

// ==================================================================
// Font-level switches (must be 1 for any character in that font)
// ==================================================================

#ifndef USE_FONT_7X10
#define USE_FONT_7X10    1
#endif

#ifndef USE_FONT_11X18
#define USE_FONT_11X18   1
#endif

#ifndef USE_FONT_16X26
#define USE_FONT_16X26   0
#endif

// ==================================================================
// Font_7x10 — Per-character switches
// Status line: VIN/Temp/Power — uses digits, V, C, W, ., '
// ==================================================================

#ifndef USE_7X10_SPACE
#define USE_7X10_SPACE       0   // Not used — all formats use zero-padding
#endif

#ifndef USE_7X10_DOT
#define USE_7X10_DOT         1
#endif

#ifndef USE_7X10_SQUOTE
#define USE_7X10_SQUOTE      1
#endif

#ifndef USE_7X10_0
#define USE_7X10_0           1
#endif

#ifndef USE_7X10_1
#define USE_7X10_1           1
#endif

#ifndef USE_7X10_2
#define USE_7X10_2           1
#endif

#ifndef USE_7X10_3
#define USE_7X10_3           1
#endif

#ifndef USE_7X10_4
#define USE_7X10_4           1
#endif

#ifndef USE_7X10_5
#define USE_7X10_5           1
#endif

#ifndef USE_7X10_6
#define USE_7X10_6           1
#endif

#ifndef USE_7X10_7
#define USE_7X10_7           1
#endif

#ifndef USE_7X10_8
#define USE_7X10_8           1
#endif

#ifndef USE_7X10_9
#define USE_7X10_9           1
#endif

#ifndef USE_7X10_C
#define USE_7X10_C           1
#endif

#ifndef USE_7X10_V
#define USE_7X10_V           1
#endif

#ifndef USE_7X10_W
#define USE_7X10_W           1
#endif

// ==================================================================
// Font_11x18 — Per-character switches
// Protocol/voltage/current display — extensive character set
// ==================================================================

#ifndef USE_11X18_SPACE
#define USE_11X18_SPACE      1
#endif

#ifndef USE_11X18_DASH
#define USE_11X18_DASH       1
#endif

#ifndef USE_11X18_DOT
#define USE_11X18_DOT        1
#endif

#ifndef USE_11X18_0
#define USE_11X18_0          1
#endif

#ifndef USE_11X18_1
#define USE_11X18_1          1
#endif

#ifndef USE_11X18_2
#define USE_11X18_2          1
#endif

#ifndef USE_11X18_3
#define USE_11X18_3          1
#endif

#ifndef USE_11X18_4
#define USE_11X18_4          1
#endif

#ifndef USE_11X18_5
#define USE_11X18_5          1
#endif

#ifndef USE_11X18_6
#define USE_11X18_6          1
#endif

#ifndef USE_11X18_7
#define USE_11X18_7          1
#endif

#ifndef USE_11X18_8
#define USE_11X18_8          1
#endif

#ifndef USE_11X18_9
#define USE_11X18_9          1
#endif

#ifndef USE_11X18_A
#define USE_11X18_A          1
#endif

#ifndef USE_11X18_C
#define USE_11X18_C          1
#endif

#ifndef USE_11X18_D
#define USE_11X18_D          1
#endif

#ifndef USE_11X18_E
#define USE_11X18_E          1
#endif

#ifndef USE_11X18_F
#define USE_11X18_F          1
#endif

#ifndef USE_11X18_K
#define USE_11X18_K          1
#endif

#ifndef USE_11X18_N
#define USE_11X18_N          1
#endif

#ifndef USE_11X18_O
#define USE_11X18_O          1
#endif

#ifndef USE_11X18_P
#define USE_11X18_P          1
#endif

#ifndef USE_11X18_Q
#define USE_11X18_Q          1
#endif

#ifndef USE_11X18_S
#define USE_11X18_S          1
#endif

#ifndef USE_11X18_T
#define USE_11X18_T          1
#endif

#ifndef USE_11X18_U
#define USE_11X18_U          1
#endif

#ifndef USE_11X18_V
#define USE_11X18_V          1
#endif

#ifndef USE_11X18_W
#define USE_11X18_W          0   // Not used — no uppercase W in any output
#endif

#ifndef USE_11X18_X
#define USE_11X18_X          1
#endif

#ifndef USE_11X18_o
#define USE_11X18_o          1
#endif

#ifndef USE_11X18_w
#define USE_11X18_w          1
#endif

#ifndef USE_11X18_e
#define USE_11X18_e          1
#endif

#ifndef USE_11X18_r
#define USE_11X18_r          1
#endif

#ifndef USE_11X18_n
#define USE_11X18_n          1
#endif

// ==================================================================
// Font_16x26 — Per-character switches (font disabled, placeholder)
// ==================================================================

// Font_16x26 is disabled (USE_FONT_16X26=0). No per-character macros needed.
// Add USE_16X26_<NAME> macros here when enabling this font.

#endif // FONTS_CONFIG_HPP

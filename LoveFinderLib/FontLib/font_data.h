/**
 * @file font_data.h
 * @brief FontLib 字符级编译字体 — 每字符独立字模 + 查表接口 (生成物)
 * @author LoveFinder
 *
 * 本文件由 PickSoul (FontHub Editor) 生成, 请勿手工编辑。
 * 字形集合 (FONT_SUBSET_*_CHARS) 由 PickSoul 管理, 修改请通过 PickSoul。
 */

#ifndef FONT_DATA_H
#define FONT_DATA_H

#include <stdint.h>
#include "font.h"

// 字符级编译子集字体 (data == nullptr, 走查表)
#if USE_FONT_SUBSET_7X10
extern FontDef Font_Subset_7x10;
#endif

#if USE_FONT_SUBSET_11X18
extern FontDef Font_Subset_11x18;
#endif

/**
 * @brief 按 ASCII 码查字模 (子集字体专用)
 * @param ch ASCII 码 (32-126)
 * @return 指向 height 个 uint16_t 的指针; 字符未编译返回 nullptr
 */
#if USE_FONT_SUBSET_7X10
const uint16_t* subset_glyph_7x10(uint8_t ch);
#endif

#if USE_FONT_SUBSET_11X18
const uint16_t* subset_glyph_11x18(uint8_t ch);
#endif

#endif /* FONT_DATA_H */
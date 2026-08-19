/**
 * @file font.h
 * @brief FontLib 驱动无关公共接口 — 屏幕驱动唯一需要 include 的头文件
 * @author LoveFinder
 *
 * ============================================================================
 *  FontLib 是什么?
 * ============================================================================
 * FontLib 是"驱动无关"的字库模块: 不依赖任何 LCD/屏幕驱动,
 * 只提供 FontDef 类型 + 字符级编译的字形查表接口。
 * ST7735 / 其他屏幕驱动 / 其他显示技术, 都通过 include 本文件使用字库。
 *
 * 字库的"唯一管理入口"是 PickSoul (FontHub Editor):
 *   - font_manifest.json 是本目录下的唯一真相源 (PickSoul 读写)
 *   - 本文件 / font_config.hpp / font_data.cpp / fonts.cpp 是生成物
 *   - 不要手工编辑生成物 — 修改请通过 PickSoul
 *
 * ============================================================================
 *  双模式字库
 * ============================================================================
 *   - 模式 A (传统完整字库): font.data != nullptr -> 整套字库固定索引
 *   - 模式 B (字符级子集):   font.data == nullptr -> font_get_glyph 查表
 *     仅编译 FONT_SUBSET_*_CHARS 中声明的字符, 未编译字符渲染时自动跳过
 *
 * 使用 (三步):
 * 1. 在编译期通过 USE_FONT_* 开关启用需要的字体 (见 font_config.hpp)
 * 2. 渲染端调用 ST7735 等驱动时, 传入对应 FontDef
 * 3. 字符级编译的字体, 字形集合由 PickSoul 管理, 无需手工维护
 */
#ifndef FONT_H
#define FONT_H

#include <stdint.h>
#include "font_config.hpp"

/*============================================================================
 * FontDef — 字库统一描述结构
 *   data != nullptr : 模式 A, 固定索引 (ch-32)*height
 *   data == nullptr : 模式 B, 走 font_get_glyph / font_get_glyph_unicode 查表
 *==========================================================================*/
typedef struct FontDef {
    const uint8_t width;
    uint8_t height;
    const uint16_t *data;
} FontDef;

/*============================================================================
 * 语言链接: FontLib 全部为 C++ (无 C 文件) — 所有声明统一默认 C++ 链接,
 *   font_data.cpp / fonts.cpp 中的定义与之匹配
 *==========================================================================*/
/*============================================================================
 * 模式 A — 传统完整字库 (生成物: fonts.cpp)
 *==========================================================================*/
/*============================================================================
 * 模式 B — 字符级编译子集字体 (生成物: font_data.cpp)
 *   data == nullptr, 走查表; 字形集合由 PickSoul 管理
 *   查表函数声明见 font_data.h
 *==========================================================================*/
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


/**
 * @brief 通用字形分发: 按 ASCII 查任意子集字体的字模 (模式 B 专用)
 * @param font 字体定义 (data==nullptr 时有效)
 * @param ch    ASCII 码 (32-126)
 * @return 指向 height 个 uint16_t 的指针; 字符未编译返回 nullptr
 */
const uint16_t* font_get_glyph(const FontDef& font, uint8_t ch);

/**
 * @brief 通用字形分发: 按 Unicode 查任意子集字体的字模 (模式 B 专用)
 * @param font 字体定义 (data==nullptr 时有效)
 * @param uni   Unicode 码点 (如 0x4F60=你, 0x597D=好)
 * @return 指向 height 个 uint16_t 的指针; 未编译返回 nullptr
 */
const uint16_t* font_get_glyph_unicode(const FontDef& font, uint16_t uni);

#endif /* FONT_H */
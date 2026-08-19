/**
 * @file font_config.hpp
 * @brief 字体编译配置 — 字符级编译 (Per-Character Compilation) (生成物)
 * @author LoveFinder
 *
 * 本文件由 PickSoul (FontHub Editor) 生成, 请勿手工编辑。
 * 字形集合 / 编译开关的修改请通过 PickSoul (font_manifest.json 是唯一真相源)。
 */

#ifndef FONT_CONFIG_HPP
#define FONT_CONFIG_HPP

// ------------------------------------------------------------------
// FONT_SUBSET — 字符级编译子集字体 (推荐)
// ------------------------------------------------------------------
#ifndef USE_FONT_SUBSET_11X18
#define USE_FONT_SUBSET_11X18  1
#endif

#ifndef USE_FONT_SUBSET_7X10
#define USE_FONT_SUBSET_7X10   1
#endif

// ------------------------------------------------------------------
// 子集字符集定义 — 仅这些字符会被编译进固件! 其余字符渲染时自动跳过。
// 字符集请保持 ASCII 升序, 便于生成器维护。
// ------------------------------------------------------------------
#ifndef FONT_SUBSET_7X10_CHARS
#define FONT_SUBSET_7X10_CHARS   "'.0123456789CVW"
#endif

#ifndef FONT_SUBSET_11X18_CHARS
#define FONT_SUBSET_11X18_CHARS  " -.0123456789ACDEFKNOPQSTUVXenorw"
#endif

#endif // FONT_CONFIG_HPP

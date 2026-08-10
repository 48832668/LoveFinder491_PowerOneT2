#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
ST7735 面板类型管理器 & SW3526 调试工具
=======================================

功能：
  1. 查看 / 切换 ST7735 显示屏的面板类型（A / B）
     - 面板类型由 LoveFinderLib/ST7735/ST7735_PanelConfig.hpp 控制
     - 同一时间只能激活一种面板类型
  2. 调试选项 A：配置所有 SW3526S 的检流电阻（采样电阻）
     - 选项 1：10 毫欧（10mΩ，芯片 ADC 出厂标定值）
     - 选项 2：5 毫欧（5mΩ，实际电流 = ADC 读数 × 2）
     - 修改 Core/Src/main.cpp 中 configSW3526 对全部 3 路 SW3526 的配置

用法：
  python st7735_panel_tool.py           进入交互菜单
  python st7735_panel_tool.py get       查看当前面板类型
  python st7735_panel_tool.py set A|B   切换面板类型
  python st7735_panel_tool.py sw3526    查看当前 SW3526 检流电阻
  python st7735_panel_tool.py sw3526 1|2|10|5   配置检流电阻（1=10mΩ，2=5mΩ）
"""

import re
import sys
from pathlib import Path

# 强制 UTF-8 输出，保证中文界面在 Windows 终端正常显示
for stream in (sys.stdout, sys.stderr):
    try:
        stream.reconfigure(encoding="utf-8")
    except (AttributeError, ValueError):
        pass

PROJECT_ROOT = Path(__file__).resolve().parent

PANEL_CONFIG_FILE = PROJECT_ROOT / "LoveFinderLib" / "ST7735" / "ST7735_PanelConfig.hpp"
MAIN_CPP_FILE = PROJECT_ROOT / "Core" / "Src" / "main.cpp"

PANELS = ("A", "B")

SENSE_RESISTOR_OPTIONS = {
    "1": 10,   # 10 毫欧
    "2": 5,    # 5 毫欧
}

# 5 毫欧检流电阻的 PPS 电流限制警告
SENSE_5MOHM_WARNING = (
    "警告：PPS 的电流限制将会是协商值的两倍！\n"
    "例如：PPS 中要求仅需 1A 电流，但 SW3526S 实际只会把电流控制在 2A 以内。\n"
    "请注意：这可能会损坏基于 PPS 直冲的电池！"
)

# 匹配已注释与未注释的 #define
_PANEL_DEFINE_RE = re.compile(
    r"^\s*(?://\s*)?#define\s+(ST7735_PANEL_([AB]))\b", re.MULTILINE
)
# 匹配 main.cpp 中 configSW3526 里的 setSenseResistor 调用
_SENSE_RESISTOR_RE = re.compile(r"(dev\.setSenseResistor\()(\d+)(\);)")


class ToolError(Exception):
    pass


def read_text(path: Path) -> str:
    if not path.exists():
        raise ToolError(f"文件不存在：{path}")
    return path.read_text(encoding="utf-8")


def write_text(path: Path, text: str) -> None:
    path.write_text(text, encoding="utf-8")


# =====================================================================
# ST7735 面板类型
# =====================================================================

def get_current_panel(config: str | None = None) -> str:
    """返回当前激活的面板类型，如 'A'。"""
    config = config or read_text(PANEL_CONFIG_FILE)
    active = []
    for m in _PANEL_DEFINE_RE.finditer(config):
        line = m.group(0)
        if re.match(r"^\s*#define", line) and not re.match(r"^\s*//", line):
            active.append(m.group(2))
    if not active:
        raise ToolError("未找到激活的面板类型，请在配置文件中定义 ST7735_PANEL_A 或 ST7735_PANEL_B。")
    if len(active) > 1:
        raise ToolError(f"检测到多个激活面板类型：{', '.join(active)}，请只保留一个。")
    return active[0]


def set_panel(target: str) -> str:
    """切换面板类型，返回新的激活类型。"""
    target = target.upper()
    if target not in PANELS:
        raise ToolError(f"未知面板类型 '{target}'，可用：{', '.join(PANELS)}")

    config = read_text(PANEL_CONFIG_FILE)
    found = {m.group(2): m for m in _PANEL_DEFINE_RE.finditer(config)}
    missing = [p for p in PANELS if p not in found]
    if missing:
        raise ToolError(f"配置文件中缺少面板类型定义：{', '.join(missing)}")

    lines = config.splitlines()
    for i, line in enumerate(lines):
        m = re.match(r"^\s*(//\s*)?#define\s+(ST7735_PANEL_([AB]))\b", line)
        if not m:
            continue
        is_active = m.group(1) is None
        panel = m.group(3)
        if panel == target:
            lines[i] = re.sub(r"^\s*//\s*", "", line).strip()
        elif is_active:
            lines[i] = "// " + line.strip()

    write_text(PANEL_CONFIG_FILE, "\n".join(lines) + "\n")
    return target


# =====================================================================
# SW3526 检流电阻（调试选项 A）
# =====================================================================

def get_sense_resistor(config: str | None = None) -> int:
    """读取 main.cpp 中当前配置的检流电阻（毫欧）。"""
    config = config or read_text(MAIN_CPP_FILE)
    m = _SENSE_RESISTOR_RE.search(config)
    if not m:
        raise ToolError("未在 main.cpp 中找到 dev.setSenseResistor(...) 调用。")
    return int(m.group(2))


def set_sense_resistor(value: int) -> int:
    """配置所有 SW3526S 的检流电阻（毫欧），返回新值。"""
    if value not in (5, 10):
        raise ToolError("检流电阻仅支持 5 或 10 毫欧。")
    config = read_text(MAIN_CPP_FILE)
    if not _SENSE_RESISTOR_RE.search(config):
        raise ToolError("未在 main.cpp 中找到 dev.setSenseResistor(...) 调用，无法修改。")
    new_config, count = _SENSE_RESISTOR_RE.subn(f"\\g<1>{value}\\g<3>", config)
    if count < 1:
        raise ToolError("修改失败：未匹配到 dev.setSenseResistor(...) 调用。")
    write_text(MAIN_CPP_FILE, new_config)
    return value


# =====================================================================
# 交互菜单（中文界面）
# =====================================================================

def cmd_get_panel() -> int:
    try:
        print(f"当前 ST7735 面板类型：ST7735_PANEL_{get_current_panel()}")
        return 0
    except ToolError as e:
        print(f"错误：{e}", file=sys.stderr)
        return 1


def cmd_set_panel(target: str) -> int:
    try:
        new = set_panel(target)
        print(f"已切换面板类型为 ST7735_PANEL_{new}")
        print(f"配置文件：{PANEL_CONFIG_FILE}")
        return 0
    except ToolError as e:
        print(f"错误：{e}", file=sys.stderr)
        return 1


def prompt(text: str) -> str:
    """安全输入：stdin 关闭时返回空字符串，不抛异常。"""
    try:
        return input(text).strip().lower()
    except EOFError:
        return ""


def cmd_sw3526(value: str | None) -> int:
    try:
        if value is None:
            print(f"当前 SW3526S 检流电阻：{get_sense_resistor()} 毫欧（所有 3 路通道）")
            return 0
        key = value.strip().lower()
        if key in SENSE_RESISTOR_OPTIONS:
            new = SENSE_RESISTOR_OPTIONS[key]
        elif key in ("10", "5"):
            new = int(key)
        else:
            raise ToolError("检流电阻选项无效：1=10毫欧，2=5毫欧（或直接输入 10 / 5）。")

        # 选择 5 毫欧时给出 PPS 电流限制警告，并请求确认
        if new == 5:
            print("\n" + "=" * 60)
            print(SENSE_5MOHM_WARNING)
            print("=" * 60)
            confirm = prompt("\n确定要配置为 5 毫欧吗？继续操作将应用到所有 3 路 SW3526S（Y/N）：")
            if confirm not in ("y", "yes", "是", "确认", "确定"):
                print("已取消，未做任何修改。")
                return 0

        set_sense_resistor(new)
        print(f"已配置所有 SW3526S 检流电阻为：{new} 毫欧")
        print(f"源文件：{MAIN_CPP_FILE}")
        return 0
    except ToolError as e:
        print(f"错误：{e}", file=sys.stderr)
        return 1


def cmd_debug_menu() -> int:
    """调试选项菜单：A 配置所有 SW3526S 检流电阻"""
    print("\n========== 调试选项 ==========")
    print("  A. 配置所有 SW3526S 检流电阻")
    print("     （1 = 10 毫欧，2 = 5 毫欧）")
    print("  B. 返回主菜单")
    print("================================")

    choice = prompt("请选择（A/B）：")

    if choice == "a":
        print("\n当前检流电阻：" + _describe_sense())
        print("请选择新的检流电阻：")
        print("  1 = 10 毫欧（芯片 ADC 标定值，默认）")
        print("  2 = 5 毫欧（实际电流 = ADC 读数 × 2）")
        sub = prompt("请输入（1/2）：")
        return cmd_sw3526(sub)
    elif choice == "b":
        return 0
    else:
        print("无效输入，请重试。")
        return 1


def _describe_sense() -> str:
    try:
        return f"{get_sense_resistor()} 毫欧（所有 3 路通道）"
    except ToolError as e:
        return f"读取失败（{e}）"


def interactive_menu() -> None:
    while True:
        print("\n=========================================")
        print("   ST7735 面板类型管理器 & SW3526 调试工具")
        print("=========================================")
        print("  1. 查看当前面板类型")
        print("  2. 切换面板类型（A / B）")
        print("  3. 调试选项（配置 SW3526S 检流电阻）")
        print("  0. 退出")
        print("=========================================")

        choice = prompt("请选择操作（0-3）：")

        if choice == "1":
            cmd_get_panel()
        elif choice == "2":
            target = prompt("请输入目标面板类型（A 或 B）：")
            cmd_set_panel(target)
        elif choice == "3":
            cmd_debug_menu()
        elif choice in ("0", "q", "quit", "exit"):
            print("再见！")
            break
        else:
            print("无效输入，请重试。")


# =====================================================================
# 入口
# =====================================================================

def main() -> int:
    args = [a for a in sys.argv[1:] if a]

    if not args:
        interactive_menu()
        return 0

    cmd = args[0].lower()

    if cmd in ("help", "list", "-h", "--help"):
        print(__doc__)
        return 0
    if cmd in ("get", "status"):
        return cmd_get_panel()
    if cmd in ("set", "switch"):
        if len(args) < 2:
            print("用法：python st7735_panel_tool.py set A|B", file=sys.stderr)
            return 1
        return cmd_set_panel(args[1])
    if cmd in ("sw3526", "sense", "resistor"):
        return cmd_sw3526(args[1] if len(args) > 1 else None)
    if cmd in ("debug", "menu"):
        return cmd_debug_menu()

    print(f"未知命令 '{cmd}'，输入 'python {Path(__file__).name} help' 查看帮助。", file=sys.stderr)
    return 1


if __name__ == "__main__":
    sys.exit(main())

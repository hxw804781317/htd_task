#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
读取 .h 文件中指定宏的值

用法：
    python read_macro.py <header_file> <macro_name>
    python read_macro.py version.h SDK_VERSION
    python read_macro.py version.h SDK_VERSION --raw
"""

import argparse
import re
import sys
from pathlib import Path


def strip_comments(text: str) -> str:
    """去掉 // 行注释和 /* */ 块注释（粗粒度，够用）"""
    # 块注释
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    # 行注释
    text = re.sub(r"//[^\n]*", "", text)
    return text


def join_line_continuations(text: str) -> str:
    """把反斜杠续行合并成一行"""
    return re.sub(r"\\\s*\n", " ", text)


def parse_value(value: str):
    """把字符串值尝试转成 int / float，失败就原样返回字符串"""
    v = value.strip().rstrip(";").strip()

    # 去掉外层引号
    if (v.startswith('"') and v.endswith('"')) or (v.startswith("'") and v.endswith("'")):
        return v[1:-1]

    # 去掉后缀 U / UL / ULL / L / LL
    v_num = re.sub(r"(?i)(u|l)+$", "", v).strip()

    try:
        # 十六进制
        if re.fullmatch(r"0[xX][0-9a-fA-F]+", v_num):
            return int(v_num, 16)
        # 二进制
        if re.fullmatch(r"0[bB][01]+", v_num):
            return int(v_num, 2)
        # 八进制
        if re.fullmatch(r"0[0-7]+", v_num):
            return int(v_num, 8)
        # 十进制
        if re.fullmatch(r"-?\d+", v_num):
            return int(v_num, 10)
        # 浮点
        if re.fullmatch(r"-?\d+\.\d*([eE][-+]?\d+)?", v_num):
            return float(v_num)
    except ValueError:
        pass

    return v


def find_macro(file_path: Path, macro_name: str):
    """
    在文件中查找宏定义，返回 (原始值字符串, 行号) 或 (None, None)
    只匹配 #define NAME ... 形式
    """
    raw_text = file_path.read_text(encoding="utf-8", errors="ignore")
    text = join_line_continuations(strip_comments(raw_text))

    # 允许 #define 前面有空白；匹配到行尾
    pattern = re.compile(
        r"^\s*#\s*define\s+" + re.escape(macro_name) + r"(?:\s+)(.*)$",
        re.MULTILINE,
    )
    m = pattern.search(text)
    if not m:
        return None, None

    value = m.group(1).strip()

    # 找原始行号（粗略估算，用于提示）
    line_no = raw_text[: raw_text.find(macro_name)].count("\n") + 1
    return value, line_no


def main():
    parser = argparse.ArgumentParser(description="读取 .h 文件里的宏值")
    parser.add_argument("header", help="头文件路径，例如 version.h")
    parser.add_argument("macro", help="宏名，例如 SDK_VERSION")
    parser.add_argument("--raw", action="store_true", help="只输出原始字符串，不做类型转换")
    args = parser.parse_args()

    path = Path(args.header)
    if not path.is_file():
        print(f"错误：文件不存在：{path}", file=sys.stderr)
        sys.exit(1)

    value, line_no = find_macro(path, args.macro)
    if value is None:
        print(f"错误：在 {path} 中未找到宏 {args.macro}", file=sys.stderr)
        sys.exit(2)

    if args.raw:
        print(value)
    else:
        parsed = parse_value(value)
        print(parsed)


if __name__ == "__main__":
    main()
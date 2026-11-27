#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
从母工程导出头文件路径, 写到工程根目录的 bsp_inc.txt

母工程目录只从命令行 --bsp <dir> 传入(各模块 SConstruct 里给 BSP_ROOT 赋值后传下来),
相对路径按工程根(本脚本上一级)解析。

用法:
    python tools/gen_bsp_inc.py --bsp ../stm32xxx/stm32f411xx
    python tools/gen_bsp_inc.py --bsp ../stm32xxx/stm32f411xx --scons <scons.exe>

做法: 在母工程里跑一次 `scons -n -Q --verbose`(只打印命令, 不真编译),
把所有 -I 参数解析成绝对路径, 去重后落到 bsp_inc.txt。
"""

import argparse
import os
import re
import subprocess
import sys

PROJ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT  = os.path.join(os.getcwd(), 'bsp_inc.txt')   # 默认落在当前模块目录, 可用 --out 覆盖

# -I 后面可能跟 "" 也可能直接连着路径
INC_RE = re.compile(r'-I(?:"([^"]+)"|([^\s]+))')


# @brief 解码子进程输出(armcc 的报错行是 GBK)
def decode(data):
    for enc in ('utf-8', 'gbk'):
        try:
            return data.decode(enc)
        except UnicodeDecodeError:
            continue
    return data.decode('latin-1')


# @brief 找 scons 命令: 当前 python 里有 SCons 就直接 -m SCons, 否则用 PATH 里的
def scons_cmd(arg):
    if arg:
        return [arg]
    try:
        import importlib.util
        if importlib.util.find_spec('SCons'):
            return [sys.executable, '-m', 'SCons']
    except Exception:
        pass
    from shutil import which
    for name in ('scons.exe', 'scons'):
        exe = which(name)
        if exe:
            return [exe]
    return [sys.executable, '-m', 'SCons']


# @brief 母工程目录: 只认 --bsp, 由调用方(SConstruct / ua.py)把路径传进来, 相对路径按工程根解析
def bsp_root(arg):
    if not arg:
        print('no BSP dir, use: python tools/gen_bsp_inc.py --bsp <dir>')
        return None
    return os.path.normpath(arg if os.path.isabs(arg) else os.path.join(PROJ, arg))


# @brief 各模块目录下 module_config.py 里声明的额外头文件路径(相对 htd_task 根目录)
def extra_inc():
    import importlib.util
    inc = []
    for e in os.scandir(PROJ):
        cfg = os.path.join(e.path, 'module_config.py')
        if not (e.is_dir() and os.path.isfile(cfg)):
            continue
        try:
            spec = importlib.util.spec_from_file_location('module_config_' + e.name, cfg)
            mod = importlib.util.module_from_spec(spec)
            spec.loader.exec_module(mod)
            items = getattr(mod, 'MODULE_EXTRA_INC', [])
        except Exception:
            continue
        for item in items:
            if item not in inc:
                inc.append(item)
    return inc


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--bsp', help='mother project dir, relative to project root')
    ap.add_argument('--scons', help='scons executable, default: python -m SCons / PATH')
    ap.add_argument('--out', default=OUT)
    args = ap.parse_args()

    bsp = bsp_root(args.bsp)
    if not bsp or not os.path.isdir(bsp):
        print('No BSP dir: %s' % bsp)
        return 1
    repo = os.path.normpath(os.path.join(bsp, '..', '..', '..'))

    cmd = scons_cmd(args.scons) + ['-n', '-Q', '--verbose']
    print('BSP: %s' % bsp)
    print('run: %s   (cwd=%s)' % (' '.join(cmd), bsp))
    try:
        out = subprocess.check_output(cmd, cwd=bsp, stderr=subprocess.STDOUT, timeout=600)
    except Exception as e:
        out = getattr(e, 'output', b'')
        if not out:
            print('scons dry run failed: %s' % e)
            return 1
    text = decode(out)

    paths = []
    seen = set()

    def add(p):
        p = os.path.normpath(p)
        if not os.path.isdir(p):
            return
        key = p.lower()
        if key in seen:
            return
        seen.add(key)
        paths.append(p)

    for m in INC_RE.finditer(text):
        p = m.group(1) or m.group(2)
        p = p.replace('/', '\\')
        if not os.path.isabs(p):
            p = os.path.join(bsp, p)
        if not os.path.normpath(p).lower().startswith(repo.lower()):
            continue          # 工具链自带路径不导
        add(p)

    extra = extra_inc()
    for item in extra:
        add(os.path.join(repo, item))

    # 只导到了额外追加的那几条, 说明母工程的 dry run 没跑出东西, 不覆盖旧文件
    if len(paths) <= len(extra):
        print('No include path from BSP, check: %s -n -Q --verbose' % (' '.join(cmd)))
        return 1

    lines = ['# 由 tools/gen_bsp_inc.py 生成, 不要手改',
             '# 源: %s  (scons -n -Q --verbose)' % bsp]
    lines += paths
    tmp = args.out + '.tmp'
    with open(tmp, 'w', encoding='utf-8', newline='') as f:
        f.write('\r\n'.join(lines) + '\r\n')
    os.replace(tmp, args.out)

    print('write %d include dirs -> %s' % (len(paths), args.out))
    return 0


if __name__ == '__main__':
    sys.exit(main())

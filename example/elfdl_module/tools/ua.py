#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
htd_elfdl 模块构建脚本 (Python 3 / SCons 4.x)

用法:
    scons --app=helloworld        生成 helloworld/helloworld.mo
    scons --lib=helloworld        生成 helloworld/helloworld.so

模块和普通固件的区别(loader 的硬要求, 见 mod.lds):
    1. 单段 PT_LOAD, 且 p_offset == p_vaddr   (loader 按恒等映射把文件搬进程序空间)
    2. 必须有 .dynsym                         (loader 靠它建模块连接表)
    3. 重定位必须是 SHT_REL                  (ARM 32 位天然是 REL)
    4. 不能有 bss                             (p_filesz == p_memsz)
"""

import os
import sys
import subprocess
from SCons.Script import *

# 工程根(tools 的上一级): bsp_inc.txt 放这里, 各模块目录共用
PROJ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# 母工程里会影响头文件路径的文件
BSP_WATCH_FILE = ('SConscript', 'Kconfig', 'htd_config.h', '.config', 'config.py')
# 根目录下按两级扫 SConscript / Kconfig 的目录
REPO_WATCH_DIR = ('components', 'task', 'device', 'external', 'peripherals')


# @brief bsp_inc.txt 是否比母工程的相关文件旧
def BspIncIsStale(bsp_root, out):
    out_time = os.path.getmtime(out)
    repo = os.path.normpath(os.path.join(bsp_root, '..', '..', '..'))

    for name in BSP_WATCH_FILE:
        path = os.path.join(bsp_root, name)
        if os.path.isfile(path) and os.path.getmtime(path) > out_time:
            return True

    for name in REPO_WATCH_DIR:
        base = os.path.join(repo, name)
        if not os.path.isdir(base):
            continue
        for entry in os.scandir(base):
            if entry.is_file() and entry.name in BSP_WATCH_FILE:
                if entry.stat().st_mtime > out_time:
                    return True
            if entry.is_dir():
                try:
                    for sub in os.scandir(entry.path):
                        if sub.is_file() and sub.name in BSP_WATCH_FILE:
                            if sub.stat().st_mtime > out_time:
                                return True
                except OSError:
                    pass
    return False


# @brief 头文件路径导出: 文件缺失或母工程有改动时, 构建前自动跑一次
#        bsp_root 由模块的 SConstruct 赋值后传进来, 本模块不猜母工程在哪
#        bsp_inc.txt 落在当前模块目录, 各模块各一份
def EnsureBspInc(bsp_root):
    out = os.path.join(os.getcwd(), 'bsp_inc.txt')
    if '-c' in sys.argv or '--clean' in sys.argv:
        return

    if os.path.isfile(out) and not BspIncIsStale(bsp_root, out):
        return

    print('export include path from BSP: %s' % bsp_root)
    tool = os.path.join(PROJ, 'tools', 'gen_bsp_inc.py')
    if subprocess.call([sys.executable, tool, '--bsp', bsp_root, '--out', out]) != 0:
        if os.path.isfile(out):
            print('warning: export include path failed, use existing bsp_inc.txt')
            return
        Exit(-1)


# @brief 加载模块编译配置, 检查工具链和链接脚本
def ModuleEnvLoad():
    import module_config as cfg
    if not os.path.isdir(cfg.EXEC_PATH):
        print('No arm gcc found: %s' % cfg.EXEC_PATH)
        print('set ELFDL_EXEC_PATH to your arm-none-eabi bin directory')
        Exit(-1)

    # 链接脚本先找模块目录, 没有再找工程根(各模块可以共用一份)
    lds = os.path.join(os.getcwd(), cfg.MOD_LDS)
    if not os.path.isfile(lds):
        lds = os.path.join(PROJ, cfg.MOD_LDS)
    if not os.path.isfile(lds):
        print('No linker script found: %s' % lds)
        Exit(-1)
    return cfg, lds.replace('\\', '/')


# @brief 构建模块的编译环境
def BuildModuleEnv(cfg, lds):
    linkflags = cfg.M_LFLAGS + ' -Wl,-T,' + lds
    env = Environment(tools=['gcc', 'g++', 'as', 'ar', 'gnulink'],
                      AS = cfg.AS, ASFLAGS = cfg.AFLAGS,
                      CC = cfg.CC, CCFLAGS = cfg.M_CFLAGS,
                      CXX = cfg.CXX,
                      AR = cfg.AR, ARFLAGS = '-rc',
                      LINK = cfg.LINK, LINKFLAGS = linkflags)
    env.PrependENVPath('PATH', cfg.EXEC_PATH)

    if not GetOption('verbose'):
        env.Replace(
            ARCOMSTR = 'AR $TARGET',
            ASCOMSTR = 'AS $TARGET',
            CCCOMSTR = 'CC $TARGET',
            CXXCOMSTR = 'CXX $TARGET',
            LINKCOMSTR = 'LINK $TARGET')
    return env


# @brief 构建完成后检查生成的模块是否符合 loader 要求
def ModuleCheckAction(target, source, env):
    tool = os.path.join(PROJ, 'tools', 'edl_check.py')
    rc = subprocess.call([sys.executable, tool] + list(env.get('EDL_CHECK_OPTS', [])) + [str(target[0])])
    if rc != 0:
        Exit(rc)


def ModuleCheckString(target, source, env):
    return 'CHECK %s' % target[0]


# @brief 构建应用程序(模块)
def BuildApplication(TARGET, SConscriptFile, BSP_ROOT=None, lib=False):
    global Env

    AddOption('--verbose', dest='verbose', action='store_true', default=False,
              help='print verbose information during build')

    cfg, lds = ModuleEnvLoad()
    if BSP_ROOT and not os.path.isdir(BSP_ROOT):
        print('warning: BSP dir not found: %s' % BSP_ROOT)
    Env = BuildModuleEnv(cfg, lds)
    if lib:
        # 库不需要 elfdl_init / elfdl_deinit, 自检按库模式走
        Env['EDL_CHECK_OPTS'] = ['--lib']

    # SConscript 走 building.py 的 DefineGroup, 头文件路径由 SConscript 自己给
    import building
    building.SetModuleEnv(Env)
    objs = SConscript(SConscriptFile)
    target = Env.Program(TARGET, objs)
    Env.AddPostAction(target, Action(ModuleCheckAction, ModuleCheckString))
    return target


# @brief 构建库(模块形式的 .so)
def BuildLibrary(TARGET, SConscriptFile, BSP_ROOT=None):
    return BuildApplication(TARGET, SConscriptFile, BSP_ROOT, lib=True)

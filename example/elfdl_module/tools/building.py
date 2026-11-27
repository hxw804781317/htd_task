# -*- coding: utf-8 -*-
# 模块工程的 building.py: 与内核 tools/building.py 同名, 由 ua.py 保证优先命中
# 模块不参与组件裁剪(depend 只认 ''), 保留 SConscript 里用到的 GetCurrentDir / DefineGroup
import os
from SCons.Script import *

Env = None


def SetModuleEnv(env):
    global Env
    Env = env


def GetCurrentDir():
    conscript = File('SConscript')
    fn = conscript.rfile()
    return os.path.dirname(fn.abspath)


def DefineGroup(name, src, depend, **parameters):
    for key in ('CCFLAGS', 'CPPPATH', 'CPPDEFINES'):
        value = parameters.get(key)
        if not value:
            continue
        if key == 'CPPPATH':
            value = [os.path.abspath(p) for p in value]
        Env.AppendUnique(**{key: value})
    return File(src) if src else []

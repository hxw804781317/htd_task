# for module compiling
import os
from building import *
import rtconfig

cwd = GetCurrentDir()
inc = []
src = []
objs = []


src += Glob('./src/*.c');
src += Glob('./port/*.c');
inc += [cwd + '/src'];
inc += [cwd + '/port'];
objs = DefineGroup('htd_console', src, depend = ['HTD_CONSOLE_ENABLE'], CPPPATH = inc, LOCAL_CCFLAGS = '')           
Return('objs')

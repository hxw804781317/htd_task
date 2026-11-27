import os
import SCons.Tool.MSCommon.vc

# toolchains options
ARCH='sim'
CROSS_TOOL='msvc'
PROJ_NAME='vs_sdl2_simulate'
BUILD = 'debug'

#if os.getenv('ENV_CC'):
#	CROSS_TOOL = os.getenv('ENV_CC')
#if os.getenv('ENV_EXEC_PATH'):
#	EXEC_PATH = os.getenv('ENV_EXEC_PATH')

# 配置默认交叉工具路径和平台
CPU = 'win32'
PLATFORM  = 'cl'
EXEC_PATH = ''
vc_version = ''
vc_versions = SCons.Tool.MSCommon.vc.get_installed_vcs()
if not vc_versions:
    print("No vc version!")
    exit(1)
else:
    vc_version = vc_versions[0]
EXEC_PATH = SCons.Tool.MSCommon.vc.find_vc_pdir(vc_version)
if not EXEC_PATH:
    print('Installed VC %s failure!' % vc_version)
    exit(1)
else:
    print('Successfully installed VC %s, path:%s' % (vc_version, EXEC_PATH))


# 配置 toolchains
PREFIX = ''
TARGET_EXT = 'exe'
TARGET = PROJ_NAME + '.' + TARGET_EXT;
AS = PREFIX + 'cl'
CC = PREFIX + 'cl'
AR = PREFIX + 'cl'
LINK = PREFIX + 'cl'
AFLAGS = ''
CFLAGS = ''
LFLAGS = ''
if BUILD == 'debug':
    CFLAGS += ' /MTd'
    LFLAGS += ' /DEBUG'
else:
    CFLAGS += ' /MT'
    LFLAGS += ''
# /utf-8：源码按 UTF-8 解析、执行字符集也用 UTF-8
CFLAGS += ' /Zi /Od /W 3 /WL /D_Win32 /utf-8'
LFLAGS += ' /SUBSYSTEM:CONSOLE /MACHINE:X86 /INCREMENTAL:NO'
CPATH = ''
LPATH = ''
POST_ACTION = ''

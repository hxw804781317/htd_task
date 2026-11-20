# -*- coding: utf-8 -*-
import os

# toolchains options
ARCH='arm'
CPU='cortex-m0'
CROSS_TOOL='keil'
PROJ_NAME='stm32f072xx'
BUILD = 'debug'

# bsp lib config
BSP_LIBRARY_TYPE = None

# 交叉工具名字
#if os.getenv('ENV_CC'):
#    CROSS_TOOL = os.getenv('ENV_CC')
#if os.getenv('ENV_EXEC_PATH'):
#    EXEC_PATH = os.getenv('ENV_EXEC_PATH');
    
    
# 配置默认交叉工具路径和平台
if  CROSS_TOOL == 'gcc':
    PLATFORM = 'gcc'
elif CROSS_TOOL == 'keil':
    PLATFORM = 'armcc'
    EXEC_PATH = r'C:/Keil_v5'
elif CROSS_TOOL == 'iar':
    PLATFORM = 'iccarm'
    EXEC_PATH = r'C:/Program Files (x86)/IAR Systems/Embedded Workbench 8.3'



if PLATFORM == 'gcc':
    # toolchains
    PREFIX = 'arm-none-eabi-'
    CC = PREFIX + 'gcc'
    AS = PREFIX + 'gcc'
    AR = PREFIX + 'ar'
    CXX = PREFIX + 'g++'
    LINK = PREFIX + 'gcc'
    TARGET_EXT = 'elf'
    TARGET = PROJ_NAME + '.' + TARGET_EXT;
    
    SIZE = PREFIX + 'size'
    OBJDUMP = PREFIX + 'objdump'
    OBJCPY = PREFIX + 'objcopy'
    
    DEVICE = ' -mcpu=' + CPU + ' -mthumb -ffunction-sections -fdata-sections'
    CFLAGS = DEVICE + ' -Dgcc'
    AFLAGS = ' -c' + DEVICE + ' -x assembler-with-cpp -Wa,-mimplicit-it=thumb '
    LFLAGS = DEVICE + ' -Wl,--gc-sections,-Map=' + PROJ_NAME + '.map,-cref,-u,Reset_Handler -T board/linker_scripts/link.lds'

    CPATH = ''
    LPATH = ''
    if BUILD == 'debug':
        CFLAGS += ' -O2 -gdwarf-2 -g'
        AFLAGS += ' -gdwarf-2'
    else:
        CFLAGS += ' -O2'
    CXXFLAGS = CFLAGS 
    POST_ACTION = OBJCPY + ' -O binary $TARGET ' + PROJ_NAME + '.bin\n' + SIZE + ' $TARGET \n'

elif PLATFORM == 'armcc':
    # toolchains
    CC = 'armcc'
    CXX = 'armcc'
    AS = 'armasm'
    AR = 'armar'
    LINK = 'armlink'
    TARGET_EXT = 'axf'
    TARGET = PROJ_NAME + '.' + TARGET_EXT;
    
    DEVICE = ' --cpu ' + CPU + ' ';
    CFLAGS = '-c ' + DEVICE + ' --apcs=interwork --c99'
    AFLAGS = DEVICE + ' --apcs=interwork '
    LFLAGS = DEVICE + ' --scatter "board\linker_scripts\link.sct" --info sizes --info totals --info unused --info veneers --list ' + PROJ_NAME + '.map --strict'
    CFLAGS += ' -I' + EXEC_PATH + '/ARM/ARMCC/include'
    LFLAGS += ' --libpath=' + EXEC_PATH + '/ARM/ARMCC/lib'
    CFLAGS += ' -D__MICROLIB '
    AFLAGS += ' --pd "__MICROLIB SETA 1" '
    LFLAGS += ' --library_type=microlib '
    EXEC_PATH += '/ARM/ARMCC/bin/'

    if BUILD == 'debug':
        CFLAGS += ' -g -O2'
        AFLAGS += ' -g'
    else:
        CFLAGS += ' -O2'
    CXXFLAGS = CFLAGS 
    POST_ACTION = 'fromelf --bin $TARGET --output ' + PROJ_NAME + '.bin \nfromelf -z $TARGET'

elif PLATFORM == 'armclang':
    # toolchains
    CC = 'armclang'
    CXX = 'armclang'
    AS = 'armasm'
    AR = 'armar'
    LINK = 'armlink'
    TARGET_EXT = 'axf'
    TARGET = PROJ_NAME + '.' + TARGET_EXT;

    DEVICE = ' --cpu ' + CPU + ' ';
    CFLAGS = ' --target=arm-arm-none-eabi -mcpu=' + CPU + ' ';
    CFLAGS += ' -mcpu=' + CPU + ' ';
    CFLAGS += ' -c -fno-rtti -funsigned-char -fshort-enums -fshort-wchar '
    CFLAGS += ' -gdwarf-3 -ffunction-sections '
    AFLAGS = DEVICE + ' --apcs=interwork '
    LFLAGS = DEVICE + ' --info sizes --info totals --info unused --info veneers '
    LFLAGS += ' --list ' + PROJ_NAME + '.map '
    LFLAGS += r' --strict --scatter "board\linker_scripts\link.sct" '
    CFLAGS += ' -I' + EXEC_PATH + '/ARM/ARMCLANG/include'
    LFLAGS += ' --libpath=' + EXEC_PATH + '/ARM/ARMCLANG/lib'

    EXEC_PATH += '/ARM/ARMCLANG/bin/'
    if BUILD == 'debug':
        CFLAGS += ' -g -O1' # armclang recommend
        AFLAGS += ' -g'
    else:
        CFLAGS += ' -O2'
    CXXFLAGS = CFLAGS
    CFLAGS += ' -std=c99'
    POST_ACTION = 'fromelf --bin $TARGET --output ' + PROJ_NAME + '.bin \nfromelf -z $TARGET'

elif PLATFORM == 'iccarm':
    # toolchains
    CC = 'iccarm'
    CXX = 'iccarm'
    AS = 'iasmarm'
    AR = 'iarchive'
    LINK = 'ilinkarm'
    TARGET_EXT = 'out'
    TARGET = PROJ_NAME + '.' + TARGET_EXT;

    DEVICE = '-Dewarm'
    CFLAGS = DEVICE
    CFLAGS += ' --diag_suppress Pa050'
    CFLAGS += ' --no_cse'
    CFLAGS += ' --no_unroll'
    CFLAGS += ' --no_inline'
    CFLAGS += ' --no_code_motion'
    CFLAGS += ' --no_tbaa'
    CFLAGS += ' --no_clustering'
    CFLAGS += ' --no_scheduling'
    CFLAGS += ' --endian=little'
    CFLAGS += ' --cpu=' + CPU
    CFLAGS += ' -e'
    CFLAGS += ' --fpu=None'
    CFLAGS += ' --dlib_config "' + EXEC_PATH + '/arm/INC/c/DLib_Config_Normal.h"'
    CFLAGS += ' --silent'

    AFLAGS = DEVICE
    AFLAGS += ' -s+'
    AFLAGS += ' -w+'
    AFLAGS += ' -r'
    AFLAGS += ' --cpu ' + CPU 
    AFLAGS += ' --fpu None'
    AFLAGS += ' -S'

    if BUILD == 'debug':
        CFLAGS += ' --debug'
        CFLAGS += ' -Oh'
    else:
        CFLAGS += ' -Oh'
    LFLAGS = ' --config "board/linker_scripts/link.icf"'
    LFLAGS += ' --entry __iar_program_start'
    CXXFLAGS = CFLAGS
    EXEC_PATH = EXEC_PATH + '/arm/bin/'
    POST_ACTION = 'ielftool --bin $TARGET ' + PROJ_NAME + '.bin'

def dist_handle(BSP_ROOT, dist_dir):
    import sys
    cwd_path = os.getcwd()
    sys.path.append(os.path.join(os.path.dirname(BSP_ROOT), 'tools'))
    from sdk_dist import dist_do_building
    dist_do_building(BSP_ROOT, dist_dir)

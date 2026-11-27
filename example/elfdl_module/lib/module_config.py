# -*- coding: utf-8 -*-
import os

ARCH = 'arm'
CPU  = 'cortex-m4'
#PROJ_NAME = 'stm32f411xx_elfdl'

# 交叉工具链: 默认取 htd_env_windows 里的 arm_gcc, 可用环境变量 ENV_EXEC_PATH 覆盖
if os.getenv('ENV_EXEC_PATH'):
    EXEC_PATH = os.getenv('ENV_EXEC_PATH')
else:
    EXEC_PATH = os.path.normpath(os.getcwd() + '/../../../../htd_env_windows/toolchain/arm_gcc/mingw/bin')

PREFIX = 'arm-none-eabi-'
CC   = PREFIX + 'gcc'
AS   = PREFIX + 'gcc'
CXX  = PREFIX + 'g++'
AR   = PREFIX + 'ar'
LINK = PREFIX + 'gcc'

DEVICE   = ' -mcpu=' + CPU + ' -mthumb'

# 模块编译参数: -fPIC 让代码和只读数据全走 PC 相对, 不在模块里留绝对地址
M_CFLAGS = DEVICE + ' -O2 -fPIC -ffreestanding -fno-common -Wall'
AFLAGS   = '-c' + DEVICE + ' -x assembler-with-cpp'

# 模块链接参数: 单段 LOAD + p_offset == p_vaddr, 不带 build-id(会多出 .note 节)
M_LFLAGS = '-nostdlib -shared -Wl,-N -Wl,-z,max-page-size=4 -Wl,--build-id=none'

# 模块链接脚本(相对本工程根目录)
MOD_LDS = 'mod.lds'

# 模块自己要用的头文件路径
MODULE_EXTRA_INC = [
    'components/htd_elfdl',
]

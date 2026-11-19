# -*- coding: utf-8 -*-
from SCons.Script import AddOption
import platform

def AddOptions():
    ''' ===== Add generic options to SCons ===== '''
    AddOption('--dist',
                      dest = 'make-dist',
                      action = 'store_true',
                      default = False,
                      help = 'make distribution')
    AddOption('--project-path',
                      dest = 'project-path',
                      type = 'string',
                      default = None,
                      help = 'set dist-ide project output path')
    AddOption('--project-name',
                      dest = 'project-name',
                      type = 'string',
                      default = None,
                      help = 'set project name')
    AddOption('--reset-project-config',
                      dest = 'reset-project-config',
                      action = 'store_true',
                      default = False,
                      help = 'reset the project configurations to default')
    AddOption('--cscope',
                      dest = 'cscope',
                      action = 'store_true',
                      default = False,
                      help = 'Build Cscope cross reference database. Requires cscope installed.')
    AddOption('--clang-analyzer',
                      dest = 'clang-analyzer',
                      action = 'store_true',
                      default = False,
                      help = 'Perform static analyze with Clang-analyzer. ' + \
                           'Requires Clang installed.\n' + \
                           'It is recommended to use with scan-build like this:\n' + \
                           '`scan-build scons --clang-analyzer`\n' + \
                           'If things goes well, scan-build will instruct you to invoke scan-view.')
    AddOption('--buildlib',
                      dest = 'buildlib',
                      type = 'string',
                      help = 'building library of a component')
    AddOption('--cleanlib',
                      dest = 'cleanlib',
                      action = 'store_true',
                      default = False,
                      help = 'clean up the library by --buildlib')
    AddOption('--target',
                      dest = 'target',
                      type = 'string',
                      help = 'set target project: mdk/mdk4/mdk5/iar/vs/vsc/ua/cdk/ses/makefile/eclipse/codelite/cmake')
    AddOption('--stackanalysis',
                dest = 'stackanalysis',
                action = 'store_true',
                default = False,
                help = 'thread stack static analysis')
    AddOption('--genconfig',
                dest = 'genconfig',
                action = 'store_true',
                default = False,
                help = 'Generate .config from rtconfig.h')
    AddOption('--verbose',
                dest = 'verbose',
                action = 'store_true',
                default = False,
                help = 'print verbose information during build')

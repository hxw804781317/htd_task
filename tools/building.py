# -*- coding: utf-8 -*-
import os
import sys
import string
import operator
import config
import platform
import shutil

from SCons.Script import *
from options import AddOptions
import utils

BuildOptions = {}
Projects = []
os_root = ''
Env = None

# SCons PreProcessor patch
def start_handling_includes(self, t=None):
    """
    Causes the PreProcessor object to start processing #import,
    #include and #include_next lines.

    This method will be called when a #if, #ifdef, #ifndef or #elif
    evaluates True, or when we reach the #else in a #if, #ifdef,
    #ifndef or #elif block where a condition already evaluated
    False.

    """
    d = self.dispatch_table
    p = self.stack[-1] if self.stack else self.default_table

    for k in ('import', 'include', 'include_next', 'define'):
        d[k] = p[k]

def stop_handling_includes(self, t=None):
    """
    Causes the PreProcessor object to stop processing #import,
    #include and #include_next lines.

    This method will be called when a #if, #ifdef, #ifndef or #elif
    evaluates False, or when we reach the #else in a #if, #ifdef,
    #ifndef or #elif block where a condition already evaluated True.
    """
    d = self.dispatch_table
    d['import'] = self.do_nothing
    d['include'] =  self.do_nothing
    d['include_next'] =  self.do_nothing
    d['define'] =  self.do_nothing

PatchedPreProcessor = SCons.cpp.PreProcessor
PatchedPreProcessor.start_handling_includes = start_handling_includes
PatchedPreProcessor.stop_handling_includes = stop_handling_includes

class Win32Spawn:
    def spawn(self, sh, escape, cmd, args, env):
        # deal with the cmd build-in commands which cannot be used in
        # subprocess.Popen
        if cmd == 'del':
            for f in args[1:]:
                try:
                    os.remove(f)
                except Exception as e:
                    print('Error removing file: ' + e)
                    return -1
            return 0

        import subprocess
        newargs = ' '.join(args[1:])
        cmdline = cmd + " " + newargs

        # Make sure the env is constructed by strings
        _e = dict([(k, str(v)) for k, v in env.items()])

        # Windows(tm) CreateProcess does not use the env passed to it to find
        # the executables. So we have to modify our own PATH to make Popen
        # work.
        old_path = os.environ['PATH']
        os.environ['PATH'] = _e['PATH']

        try:
            proc = subprocess.Popen(cmdline, env=_e, shell=False)
        except Exception as e:
            print('Error in calling command:' + cmdline.split(' ')[0])
            print('Exception: ' + os.strerror(e.errno))
            if (os.strerror(e.errno) == "No such file or directory"):
                print ("\nPlease check Toolchains PATH setting.\n")
            return e.errno
        finally:
            os.environ['PATH'] = old_path
        return proc.wait()

# 生成 cconfig.h 文件
def GenCconfigFile(env, BuildOptions):
    if config.PLATFORM in ['gcc']:
        contents = ''
        if not os.path.isfile('cconfig.h'):
            import gcc
            gcc.GenerateGCCConfig(config)
        # try again
        if os.path.isfile('cconfig.h'):
            f = open('cconfig.h', 'r')
            if f:
                contents = f.read()
                f.close()
                prep = PatchedPreProcessor()
                prep.process_contents(contents)
                options = prep.cpp_namespace
                BuildOptions.update(options)
                # add HAVE_CCONFIG_H definition
                env.AppendUnique(CPPDEFINES = ['HAVE_CCONFIG_H'])

# 编译前环境准备
def PrepareBuilding(env, root_directory, tag='v'):
    global BuildOptions
    global Projects
    global Env
    global os_root

    AddOptions()
    Env = env;
    Env['BSP_ROOT'] = Dir('#').abspath;
    
    os_root = os.path.abspath(root_directory)
    OS_ROOT = os_root
    Export('OS_ROOT')
    Env['OS_ROOT'] = os_root
    sys.path = sys.path + [os.path.join(os_root, 'tools')]
    
    # {target_name:(CROSS_TOOL, PLATFORM)}
    tgt_dict = {'mdk4':('keil', 'armcc'),
                'mdk5':('keil', 'armcc'),
                'iar':('iar', 'iccarm'),
                'vs':('msvc', 'cl'),
                'vs2012':('msvc', 'cl'),
                'vsc' : ('gcc', 'gcc'),
                'cb':('keil', 'armcc'),
                'cdk':('gcc', 'gcc'),
                'makefile':('gcc', 'gcc'),
                'eclipse':('gcc', 'gcc'),
                'ses' : ('gcc', 'gcc'),
                'cmake':('gcc', 'gcc'),
                'cmake-armclang':('keil', 'armclang'),
                'xmake':('gcc', 'gcc'),
                'codelite' : ('gcc', 'gcc'),
                'si':('gcc', 'gcc'),
                'esp-idf': ('gcc', 'gcc')}
    tgt_name = GetOption('target')
    if tgt_name:
        # --target will change the toolchain settings which clang-analyzer is
        # depend on
        if GetOption('clang-analyzer'):
            print ('--clang-analyzer cannot be used with --target')
            sys.exit(1)
        SetOption('no_exec', 1)
        try:
            config.CROSS_TOOL, config.PLATFORM = tgt_dict[tgt_name]
            # replace the 'ENV_CC' to 'CROSS_TOOL'
            os.environ['ENV_CC'] = config.CROSS_TOOL
            utils.ReloadModule(config)
        except KeyError:
            print('Unknow target: '+ tgt_name+'. Avaible targets: ' +', '.join(tgt_dict.keys()))
            sys.exit(1)

    # auto change the 'ENV_EXEC_PATH' when 'config.EXEC_PATH' get failed
    if not os.path.exists(config.EXEC_PATH):
        if 'ENV_EXEC_PATH' in os.environ:
            # del the 'ENV_EXEC_PATH' and using the 'EXEC_PATH' setting on config.py
            del os.environ['ENV_EXEC_PATH']
            utils.ReloadModule(config)

    # add compability with Keil MDK 4.6 which changes the directory of armcc.exe
    if config.PLATFORM in ['armcc', 'armclang']:
        if config.PLATFORM == 'armcc' and not os.path.isfile(os.path.join(config.EXEC_PATH, 'armcc.exe')):
            if config.EXEC_PATH.find('bin40') > 0:
                config.EXEC_PATH = config.EXEC_PATH.replace('bin40', 'armcc/bin')
                Env['LINKFLAGS'] = Env['LINKFLAGS'].replace('RV31', 'armcc')
        # reset AR command flags
        env['ARCOM'] = '$AR --create $TARGET $SOURCES'
        env['LIBPREFIX'] = ''
        env['LIBSUFFIX'] = '.lib'
        env['LIBLINKPREFIX'] = ''
        env['LIBLINKSUFFIX'] = '.lib'
        env['LIBDIRPREFIX'] = '--userlibpath '
    elif config.PLATFORM == 'iccarm':
        env['LIBPREFIX'] = ''
        env['LIBSUFFIX'] = '.a'
        env['LIBLINKPREFIX'] = ''
        env['LIBLINKSUFFIX'] = '.a'
        env['LIBDIRPREFIX'] = '--search '

    # patch for win32 spawn
    if env['PLATFORM'] == 'win32':
        win32_spawn = Win32Spawn()
        win32_spawn.env = env
        env['SPAWN'] = win32_spawn.spawn

    if env['PLATFORM'] == 'win32':
        os.environ['PATH'] = config.EXEC_PATH + ";" + os.environ['PATH']
    else:
        os.environ['PATH'] = config.EXEC_PATH + ":" + os.environ['PATH']

    # add program path
    env.PrependENVPath('PATH', os.environ['PATH'])

    # add library build action
    act = SCons.Action.Action(BuildLibInstallAction, 'Install compiled library... $TARGET')
    bld = Builder(action = act)
    Env.Append(BUILDERS = {'BuildLib': bld})

    # parse config.h to get used component
    PreProcessor = PatchedPreProcessor()
    f = open('htd_config.h', 'r')
    contents = f.read()
    f.close()
    PreProcessor.process_contents(contents)
    BuildOptions = PreProcessor.cpp_namespace

    if GetOption('clang-analyzer'):
        # perform what scan-build does
        env.Replace(
                CC = 'ccc-analyzer',
                CXX = 'c++-analyzer',
                # skip as and link
                LINK = 'true',
                AS = 'true',)
        env["ENV"].update(x for x in os.environ.items() if x[0].startswith("CCC_"))
        # only check, don't compile. ccc-analyzer use CCC_CC as the CC.
        # fsyntax-only will give us some additional warning messages
        env['ENV']['CCC_CC']  = 'clang'
        env.Append(CFLAGS=['-fsyntax-only', '-Wall', '-Wno-invalid-source-encoding'])
        env['ENV']['CCC_CXX'] = 'clang++'
        env.Append(CXXFLAGS=['-fsyntax-only', '-Wall', '-Wno-invalid-source-encoding'])
        # remove the POST_ACTION as it will cause meaningless errors(file not
        # found or something like that).
        config.POST_ACTION = ''

    # 生成 version.h 文件
    import generate_version
    version_filepath = Env['BSP_ROOT'] + "/version.h";
    print("version_filepath=%s" % (version_filepath));
    generate_version.generate_version_file(tag, version_filepath);

    # 生成 cconfig.h 文件
    GenCconfigFile(env, BuildOptions)

    # auto append '_REENT_SMALL' when using newlib 'nano.specs' option
    if config.PLATFORM in ['gcc'] and str(env['LINKFLAGS']).find('nano.specs') != -1:
        env.AppendUnique(CPPDEFINES = ['_REENT_SMALL'])
    
    # 通过 htd_config.h 生成 .config 文件
    if GetOption('genconfig'):
        from genconfig import genconfig
        genconfig()
        exit(0)
    
    # 静态栈分析
    if GetOption('stackanalysis'):
        from WorstCaseStack import ThreadStackStaticAnalysis
        ThreadStackStaticAnalysis(Env)
        exit(0)
        
    if not GetOption('verbose'):
        # override the default verbose command string
        env.Replace(
            ARCOMSTR = 'AR $TARGET',
            ASCOMSTR = 'AS $TARGET',
            ASPPCOMSTR = 'AS $TARGET',
            CCCOMSTR = 'CC $TARGET',
            CXXCOMSTR = 'CXX $TARGET',
            LINKCOMSTR = 'LINK $TARGET'
        )
    # fix the linker for C++
    if GetDepend('RT_USING_CPLUSPLUS'):
        if env['LINK'].find('gcc') != -1:
            env['LINK'] = env['LINK'].replace('gcc', 'g++')


# 
def PrepareModuleBuilding(env, root_directory, bsp_directory):
    global BuildOptions
    global Env
    global os_root

    # patch for win32 spawn
    if env['PLATFORM'] == 'win32':
        win32_spawn = Win32Spawn()
        win32_spawn.env = env
        env['SPAWN'] = win32_spawn.spawn
    Env = env
    os_root = root_directory

    # parse bsp config.h to get used component
    PreProcessor = PatchedPreProcessor()
    f = open(bsp_directory + '/htd_config.h', 'r')
    contents = f.read()
    f.close()
    PreProcessor.process_contents(contents)
    BuildOptions = PreProcessor.cpp_namespace

    AddOption('--buildlib',
                      dest = 'buildlib',
                      type = 'string',
                      help = 'building library of a component')
    AddOption('--cleanlib',
                      dest = 'cleanlib',
                      action = 'store_true',
                      default = False,
                      help = 'clean up the library by --buildlib')
    # add program path
    env.PrependENVPath('PATH', config.EXEC_PATH)

def GetConfigValue(name):
    assert type(name) == str, 'GetConfigValue: only string parameter is valid'
    try:
        return BuildOptions[name]
    except:
        return ''

def GetDepend(depend):
    building = True
    if type(depend) == type('str'):
        if not depend in BuildOptions or BuildOptions[depend] == 0:
            building = False
        elif BuildOptions[depend] != '':
            return BuildOptions[depend]
        return building
    # for list type depend
    for item in depend:
        if item != '':
            if not item in BuildOptions or BuildOptions[item] == 0:
                building = False
    return building

def LocalOptions(config_filename):
    from SCons.Script import SCons
    # parse wiced_config.h to get used component
    PreProcessor = SCons.cpp.PreProcessor()
    f = open(config_filename, 'r')
    contents = f.read()
    f.close()
    PreProcessor.process_contents(contents)
    local_options = PreProcessor.cpp_namespace
    return local_options

def GetLocalDepend(options, depend):
    building = True
    if type(depend) == type('str'):
        if not depend in options or options[depend] == 0:
            building = False
        elif options[depend] != '':
            return options[depend]
        return building
    # for list type depend
    for item in depend:
        if item != '':
            if not item in options or options[item] == 0:
                building = False
    return building

def AddDepend(option):
    BuildOptions[option] = 1

def MergeGroup(src_group, group):
    src_group['src'] = src_group['src'] + group['src']
    src_group['src'].sort()
    if 'CFLAGS' in group:
        if 'CFLAGS' in src_group:
            src_group['CFLAGS'] = src_group['CFLAGS'] + group['CFLAGS']
        else:
            src_group['CFLAGS'] = group['CFLAGS']
    if 'CCFLAGS' in group:
        if 'CCFLAGS' in src_group:
            src_group['CCFLAGS'] = src_group['CCFLAGS'] + group['CCFLAGS']
        else:
            src_group['CCFLAGS'] = group['CCFLAGS']
    if 'CXXFLAGS' in group:
        if 'CXXFLAGS' in src_group:
            src_group['CXXFLAGS'] = src_group['CXXFLAGS'] + group['CXXFLAGS']
        else:
            src_group['CXXFLAGS'] = group['CXXFLAGS']
    if 'CPPPATH' in group:
        if 'CPPPATH' in src_group:
            src_group['CPPPATH'] = src_group['CPPPATH'] + group['CPPPATH']
        else:
            src_group['CPPPATH'] = group['CPPPATH']
    if 'CPPDEFINES' in group:
        if 'CPPDEFINES' in src_group:
            src_group['CPPDEFINES'] = src_group['CPPDEFINES'] + group['CPPDEFINES']
        else:
            src_group['CPPDEFINES'] = group['CPPDEFINES']
    if 'ASFLAGS' in group:
        if 'ASFLAGS' in src_group:
            src_group['ASFLAGS'] = src_group['ASFLAGS'] + group['ASFLAGS']
        else:
            src_group['ASFLAGS'] = group['ASFLAGS']
    # for local CCFLAGS/CPPPATH/CPPDEFINES
    if 'LOCAL_CFLAGS' in group:
        if 'LOCAL_CFLAGS' in src_group:
            src_group['LOCAL_CFLAGS'] = src_group['LOCAL_CFLAGS'] + group['LOCAL_CFLAGS']
        else:
            src_group['LOCAL_CFLAGS'] = group['LOCAL_CFLAGS']
    if 'LOCAL_CCFLAGS' in group:
        if 'LOCAL_CCFLAGS' in src_group:
            src_group['LOCAL_CCFLAGS'] = src_group['LOCAL_CCFLAGS'] + group['LOCAL_CCFLAGS']
        else:
            src_group['LOCAL_CCFLAGS'] = group['LOCAL_CCFLAGS']
    if 'LOCAL_CXXFLAGS' in group:
        if 'LOCAL_CXXFLAGS' in src_group:
            src_group['LOCAL_CXXFLAGS'] = src_group['LOCAL_CXXFLAGS'] + group['LOCAL_CXXFLAGS']
        else:
            src_group['LOCAL_CXXFLAGS'] = group['LOCAL_CXXFLAGS']
    if 'LOCAL_CPPPATH' in group:
        if 'LOCAL_CPPPATH' in src_group:
            src_group['LOCAL_CPPPATH'] = src_group['LOCAL_CPPPATH'] + group['LOCAL_CPPPATH']
        else:
            src_group['LOCAL_CPPPATH'] = group['LOCAL_CPPPATH']
    if 'LOCAL_CPPDEFINES' in group:
        if 'LOCAL_CPPDEFINES' in src_group:
            src_group['LOCAL_CPPDEFINES'] = src_group['LOCAL_CPPDEFINES'] + group['LOCAL_CPPDEFINES']
        else:
            src_group['LOCAL_CPPDEFINES'] = group['LOCAL_CPPDEFINES']

    if 'LINKFLAGS' in group:
        if 'LINKFLAGS' in src_group:
            src_group['LINKFLAGS'] = src_group['LINKFLAGS'] + group['LINKFLAGS']
        else:
            src_group['LINKFLAGS'] = group['LINKFLAGS']
    if 'LIBS' in group:
        if 'LIBS' in src_group:
            src_group['LIBS'] = src_group['LIBS'] + group['LIBS']
        else:
            src_group['LIBS'] = group['LIBS']
    if 'LIBPATH' in group:
        if 'LIBPATH' in src_group:
            src_group['LIBPATH'] = src_group['LIBPATH'] + group['LIBPATH']
        else:
            src_group['LIBPATH'] = group['LIBPATH']
    if 'LOCAL_ASFLAGS' in group:
        if 'LOCAL_ASFLAGS' in src_group:
            src_group['LOCAL_ASFLAGS'] = src_group['LOCAL_ASFLAGS'] + group['LOCAL_ASFLAGS']
        else:
            src_group['LOCAL_ASFLAGS'] = group['LOCAL_ASFLAGS']

def _PretreatListParameters(target_list):
    while '' in target_list: # remove null strings
        target_list.remove('')
    while ' ' in target_list: # remove ' '
        target_list.remove(' ')
    if(len(target_list) == 0):
        return False # ignore this list, don't add this list to the parameter
    return True # permit to add this list to the parameter

def DefineGroup(name, src, depend, **parameters):
    global Env
    if not GetDepend(depend):
        return []

    # find exist group and get path of group
    group_path = ''
    for g in Projects:
        if g['name'] == name:
            group_path = g['path']
    if group_path == '':
        group_path = GetCurrentDir()

    group = parameters
    group['name'] = name
    group['path'] = group_path
    if type(src) == type([]):
        # remove duplicate elements from list
        src = list(set(src))
        group['src'] = File(src)
    else:
        group['src'] = src

    if 'CFLAGS' in group:
        target = group['CFLAGS']
        if len(target) > 0:
            Env.AppendUnique(CFLAGS = target)
    if 'CCFLAGS' in group:
        target = group['CCFLAGS']
        if len(target) > 0:
            Env.AppendUnique(CCFLAGS = target)
    if 'CXXFLAGS' in group:
        target = group['CXXFLAGS']
        if len(target) > 0:
            Env.AppendUnique(CXXFLAGS = target)
    if 'CPPPATH' in group:
        target = group['CPPPATH']
        if _PretreatListParameters(target) == True:
            paths = []
            for item in target:
                paths.append(os.path.abspath(item))
            target = paths
            Env.AppendUnique(CPPPATH = target)
    if 'CPPDEFINES' in group:
        target = group['CPPDEFINES']
        if _PretreatListParameters(target) == True:
            Env.AppendUnique(CPPDEFINES = target)
    if 'LINKFLAGS' in group:
        target = group['LINKFLAGS']
        if len(target) > 0:
            Env.AppendUnique(LINKFLAGS = target)
    if 'ASFLAGS' in group:
        target = group['ASFLAGS']
        if len(target) > 0:
            Env.AppendUnique(ASFLAGS = target)
    if 'LOCAL_CPPPATH' in group:
        paths = []
        for item in group['LOCAL_CPPPATH']:
            paths.append(os.path.abspath(item))
        group['LOCAL_CPPPATH'] = paths
    if config.PLATFORM in ['gcc']:
        if 'CFLAGS' in group:
            group['CFLAGS'] = utils.GCCC99Patch(group['CFLAGS'])
        if 'CCFLAGS' in group:
            group['CCFLAGS'] = utils.GCCC99Patch(group['CCFLAGS'])
        if 'CXXFLAGS' in group:
            group['CXXFLAGS'] = utils.GCCC99Patch(group['CXXFLAGS'])
        if 'LOCAL_CCFLAGS' in group:
            group['LOCAL_CCFLAGS'] = utils.GCCC99Patch(group['LOCAL_CCFLAGS'])
        if 'LOCAL_CXXFLAGS' in group:
            group['LOCAL_CXXFLAGS'] = utils.GCCC99Patch(group['LOCAL_CXXFLAGS'])
        if 'LOCAL_CFLAGS' in group:
            group['LOCAL_CFLAGS'] = utils.GCCC99Patch(group['LOCAL_CFLAGS'])
    # check whether to clean up library
    if GetOption('cleanlib') and os.path.exists(os.path.join(group['path'], GroupLibFullName(name, Env))):
        if group['src'] != []:
            print('Remove library:'+ GroupLibFullName(name, Env))
            fn = os.path.join(group['path'], GroupLibFullName(name, Env))
            if os.path.exists(fn):
                os.unlink(fn)

    if 'LIBS' in group:
        target = group['LIBS']
        if _PretreatListParameters(target) == True:
            Env.AppendUnique(LIBS = target)
    if 'LIBPATH' in group:
        target = group['LIBPATH']
        if _PretreatListParameters(target) == True:
            Env.AppendUnique(LIBPATH = target)

    # check whether to build group library
    if 'LIBRARY' in group:
        objs = Env.Library(name, group['src'])
    else:
        # only add source
        objs = group['src']

    # merge group
    for g in Projects:
        if g['name'] == name:
            # merge to this group
            MergeGroup(g, group)
            return objs
    def PriorityInsertGroup(groups, group):
        length = len(groups)
        for i in range(0, length):
            if operator.gt(groups[i]['name'].lower(), group['name'].lower()):
                groups.insert(i, group)
                return
        groups.append(group)
    # add a new group
    PriorityInsertGroup(Projects, group)
    return objs

def GetCurrentDir():
    conscript = File('SConscript')
    fn = conscript.rfile()
    name = fn.name
    path = os.path.dirname(fn.abspath)
    return path

PREBUILDING = []
def RegisterPreBuildingAction(act):
    global PREBUILDING
    assert callable(act), 'Could only register callable objects. %s received' % repr(act)
    PREBUILDING.append(act)

def PreBuilding():
    global PREBUILDING
    for a in PREBUILDING:
        a()

def GroupLibName(name, env):
    if config.PLATFORM in ['armcc']:
        return name + '_rvds'
    elif config.PLATFORM in ['gcc']:
        return name + '_gcc'
    return name

def GroupLibFullName(name, env):
    return env['LIBPREFIX'] + GroupLibName(name, env) + env['LIBSUFFIX']

def copy_file(src, dst):
    # check source file
    if not os.path.exists(src):
        return
    path = os.path.dirname(dst)
    # mkdir if path not exist
    if not os.path.exists(path):
        os.makedirs(path)
    shutil.copy2(src, dst)

def BuildLibInstallAction(target, source, env):
    lib_name = GetOption('buildlib')
    for Group in Projects:
        if Group['name'] == lib_name:
            lib_name = GroupLibFullName(Group['name'], env)
            dst_name = os.path.join(Group['path'], lib_name)
            print('Copy '+lib_name+' => ' + dst_name)
            copy_file(lib_name, dst_name)
            break

# 构建
def DoBuilding(target, objects):
    # merge all objects into one list
    def one_list(l):
        lst = []
        for item in l:
            if type(item) == type([]):
                lst += one_list(item)
            else:
                lst.append(item)
        return lst

    # handle local group
    def local_group(group, objects):
        if 'LOCAL_CFLAGS' in group or 'LOCAL_CXXFLAGS' in group or 'LOCAL_CCFLAGS' in group or 'LOCAL_CPPPATH' in group or 'LOCAL_CPPDEFINES' in group or 'LOCAL_ASFLAGS' in group:
            CFLAGS = Env.get('CFLAGS', '') + group.get('LOCAL_CFLAGS', '')
            CCFLAGS = Env.get('CCFLAGS', '') + group.get('LOCAL_CCFLAGS', '')
            CXXFLAGS = Env.get('CXXFLAGS', '') + group.get('LOCAL_CXXFLAGS', '')
            CPPPATH = list(Env.get('CPPPATH', [''])) + group.get('LOCAL_CPPPATH', [''])
            CPPDEFINES = list(Env.get('CPPDEFINES', [''])) + group.get('LOCAL_CPPDEFINES', [''])
            ASFLAGS = Env.get('ASFLAGS', '') + group.get('LOCAL_ASFLAGS', '')
            for source in group['src']:
                objects.append(Env.Object(source, CFLAGS = CFLAGS, CCFLAGS = CCFLAGS, CXXFLAGS = CXXFLAGS, ASFLAGS = ASFLAGS,
                    CPPPATH = CPPPATH, CPPDEFINES = CPPDEFINES))
            return True
        return False

    PreBuilding();
    objects = one_list(objects);
    program = None
    # check whether special buildlib option
    lib_name = GetOption('buildlib')
    if lib_name:
        objects = [] # remove all of objects
        # build library with special component
        for Group in Projects:
            if Group['name'] == lib_name:
                lib_name = GroupLibName(Group['name'], Env)
                if not local_group(Group, objects):
                    objects = Env.Object(Group['src'])
                program = Env.Library(lib_name, objects)
                # add library copy action
                Env.BuildLib(lib_name, program)
                break
    else:
        # remove source files with local flags setting
        for group in Projects:
            if 'LOCAL_CFLAGS' in group or 'LOCAL_CXXFLAGS' in group or 'LOCAL_CCFLAGS' in group or 'LOCAL_CPPPATH' in group or 'LOCAL_CPPDEFINES' in group:
                for source in group['src']:
                    for obj in objects:
                        if source.abspath == obj.abspath or (len(obj.sources) > 0 and source.abspath == obj.sources[0].abspath):
                            objects.remove(obj)

        # re-add the source files to the objects
        objects_in_group = []
        for group in Projects:
            local_group(group, objects_in_group)

        # sort seperately, because the data type of
        # the members of the two lists are different
        objects_in_group = sorted(objects_in_group)
        objects = sorted(objects)
        objects.append(objects_in_group)
        program = Env.Program(target, objects)
    EndBuilding(target, program);

# 生成目标项目文件
def GenTargetProject(program = None):
    Export('utils');
    target_name = GetOption('target');
    if target_name == 'mdk4':
        from ide.keil import MDK4Project
        MDK4Project('project.uvproj', Projects)
    elif target_name == 'mdk5':
        from ide.keil import MDK5Project
        MDK5Project('project.uvprojx', Projects)
    elif target_name == 'iar':
        from ide.iar import IARProject
        IARProject('project.ewp', Projects)
    elif target_name == 'vs':
        from ide.vs import VSProject
        VSProject('project.vcproj', Projects, program)
    elif target_name == 'vs2012':
        from ide.vs2012 import VS2012Project
        VS2012Project('project.vcxproj', Projects, program)
    elif target_name == 'cb':
        from ide.codeblocks import CBProject
        CBProject('project.cbp', Projects, program)
    elif target_name == 'vsc':
        from ide.vsc import GenerateVSCode
        GenerateVSCode(Env)
    elif target_name == 'cdk':
        from ide.cdk import CDKProject
        CDKProject('project.cdkproj', Projects)
    elif target_name == 'ses':
        from ide.ses import SESProject
        SESProject(Env)
    elif target_name == 'makefile':
        from ide.makefile import TargetMakefile
        TargetMakefile(Env)
    elif target_name == 'eclipse':
        from ide.eclipse import TargetEclipse
        TargetEclipse(Env, GetOption('reset-project-config'), GetOption('project-name'))
    elif target_name == 'si':
        from ide.sourceinsight import TargetSI
        TargetSI(Env)
    elif target_name == 'codelite':
        from ide.codelite import TargetCodelite
        TargetCodelite(Projects, program)
    elif target_name == 'cmake' or target_name == 'cmake-armclang':
        from ide.cmake import CMakeProject
        CMakeProject(Env,Projects)
    elif target_name == 'xmake':
        from ide.xmake import XMakeProject
        XMakeProject(Env, Projects)

def EndBuilding(target, program = None):
    need_exit = False
    Env['target'] = program
    Env['project'] = Projects
    if hasattr(config, 'BSP_LIBRARY_TYPE'):
        Env['bsp_lib_type'] = config.BSP_LIBRARY_TYPE
    if hasattr(config, 'dist_handle'):
        Env['dist_handle'] = config.dist_handle

    Env.AddPostAction(target, config.POST_ACTION)
    # Add addition clean files
    Clean(target, 'cconfig.h')
    Clean(target, 'rtua.py')
    Clean(target, 'rtua.pyc')
    Clean(target, '.sconsign.dblite')

    if GetOption('target'):
        GenTargetProject(program);

    BSP_ROOT = Dir('#').abspath
    if GetOption('make-dist') and program != None:
        from mkdist import MkDist
        MkDist(program, BSP_ROOT, os_root, Env)
    if GetOption('cscope'):
        from cscope import CscopeDatabase
        CscopeDatabase(Projects)
    if not GetOption('help') and not GetOption('target'):
        if not os.path.exists(config.EXEC_PATH):
            print ("Error: the toolchain path (" + config.EXEC_PATH + ") is not exist, please check 'EXEC_PATH' in path or config.py.")
            need_exit = True
    if need_exit:
        exit(0)

# 移除
def SrcRemove(src, remove):
    if not src:
        return
    src_bak = src[:]
    if type(remove) == type('str'):
        if os.path.isabs(remove):
            remove = os.path.relpath(remove, GetCurrentDir())
        remove = os.path.normpath(remove)
        for item in src_bak:
            if type(item) == type('str'):
                item_str = item
            else:
                item_str = item.rstr()
            if os.path.isabs(item_str):
                item_str = os.path.relpath(item_str, GetCurrentDir())
            item_str = os.path.normpath(item_str)
            if item_str == remove:
                src.remove(item)
    else:
        for remove_item in remove:
            remove_str = str(remove_item)
            if os.path.isabs(remove_str):
                remove_str = os.path.relpath(remove_str, GetCurrentDir())
            remove_str = os.path.normpath(remove_str)
            for item in src_bak:
                if type(item) == type('str'):
                    item_str = item
                else:
                    item_str = item.rstr()
                if os.path.isabs(item_str):
                    item_str = os.path.relpath(item_str, GetCurrentDir())
                item_str = os.path.normpath(item_str)
                if item_str == remove_str:
                    src.remove(item)


def GlobSubDir(sub_dir, ext_name):
    import os
    import glob
    def glob_source(sub_dir, ext_name):
        list = os.listdir(sub_dir)
        src = glob.glob(os.path.join(sub_dir, ext_name))
        for item in list:
            full_subdir = os.path.join(sub_dir, item)
            if os.path.isdir(full_subdir):
                src += glob_source(full_subdir, ext_name)
        return src
    dst = []
    src = glob_source(sub_dir, ext_name)
    for item in src:
        dst.append(os.path.relpath(item, sub_dir))
    return dst

def PackageSConscript(package):
    from package import BuildPackage
    return BuildPackage(package)


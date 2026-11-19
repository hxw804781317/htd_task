# -*- coding: utf-8 -*-
import os
import json
import utils
import config
from utils import make_path_relative


def GenerateCFiles(env):
    if not os.path.exists('.vscode'):
        os.mkdir('.vscode')

    vsc_file = open('.vscode/c_cpp_properties.json', 'w')
    if vsc_file:
        info = utils.ProjectInfo(env)
        cc = os.path.join(config.EXEC_PATH, config.CC)
        cc = os.path.abspath(cc).replace('\\', '/')
        config_obj = {}
        config_obj['name'] = 'Win32'
        config_obj['defines'] = info['CPPDEFINES']
        config_obj['intelliSenseMode'] = 'clang-x64'
        config_obj['compilerPath'] = cc
        config_obj['cStandard'] = "c99"
        config_obj['cppStandard'] = "c++11"

        # format "a/b," to a/b. remove first quotation mark("),and remove end (",)
        includePath = []
        for i in info['CPPPATH']:
            if i[0] == '\"' and i[len(i) - 2:len(i)] == '\",':
                includePath.append(make_path_relative(os.getcwd(), i[1:len(i) - 2]))
            else:
                includePath.append(make_path_relative(os.getcwd(), i))
        config_obj['includePath'] = includePath

        json_obj = {}
        json_obj['configurations'] = [config_obj]
        vsc_file.write(json.dumps(json_obj, ensure_ascii=False, indent=4))
        vsc_file.close()
    return

def GenerateVSCode(env):
    print('Update setting files for VSCode...')
    GenerateCFiles(env)
    print('Done!')
    return

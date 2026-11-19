# -*- coding: utf-8 -*-
import os
import sys
import glob
import config
from utils import *


def TargetSI(env):
    project = ProjectInfo(env)
    filelist = open("si_filelist.txt","w");
    BSP_ROOT = os.path.abspath(env['BSP_ROOT']);
    OS_ROOT = os.path.abspath(env['OS_ROOT']);

    match_bsp = False;
    if BSP_ROOT.startswith(OS_ROOT): 
        match_bsp = True;

    Files = project['FILES'];
    Headers = project['HEADERS'];
    CPPDEFINES = project['CPPDEFINES'];

    paths = [os.path.normpath(i) for i in project['CPPPATH']];
    CPPPATH = [];
    for path in paths:
        fn = os.path.normpath(path);
        if fn[-1]==',':
            fn = fn[1:-2];
        #print(fn)
        hlist=glob.glob(fn+"\\*.h")
        #print(hlist);
        for hfile in hlist:
            filelist.write(hfile+"\n");
        if match_bsp:
            if fn.startswith(BSP_ROOT):
                fn = '$(BSP_ROOT)' + fn.replace(BSP_ROOT, '')
            elif fn.startswith(OS_ROOT):
                fn = '$(OS_ROOT)' + fn.replace(OS_ROOT, '')
        else:
            if fn.startswith(OS_ROOT):
                fn = '$(OS_ROOT)' + fn.replace(OS_ROOT, '')
            elif fn.startswith(BSP_ROOT):
                fn = '$(BSP_ROOT)' + fn.replace(BSP_ROOT, '')
        CPPPATH.append(fn)
    path = ''
    paths = CPPPATH
    for item in paths:
        path += '\t-I%s \\\n' % item

    """
    defines = ''
    for item in project['CPPDEFINES']:
        defines += ' -D%s' % item
    make.write('DEFINES :=')
    make.write(defines)
    make.write('\n')
    """
    files = Files;
    Files = [];
    for file in files:
        fn = os.path.normpath(file);
        filelist.write(fn + "\n");
        if match_bsp:
            if fn.startswith(BSP_ROOT):
                fn = '$(BSP_ROOT)' + fn.replace(BSP_ROOT, '');
            elif fn.startswith(OS_ROOT):
                fn = '$(OS_ROOT)' + fn.replace(OS_ROOT, '');
        else:
            if fn.startswith(OS_ROOT):
                fn = '$(OS_ROOT)' + fn.replace(OS_ROOT, '');
            elif fn.startswith(BSP_ROOT):
                fn = '$(BSP_ROOT)' + fn.replace(BSP_ROOT, '');
        Files.append(fn);
    filelist.close();
    return

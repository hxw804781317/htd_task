// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#include "htd_vfs_port.h"
#include "htd_vfs_process.h"

#if defined(HTD_VFS_USING_POSIX) && defined(HTD_ELFDL_ENABLE)
#include "htd_elfdl_process.h"

ELFDL_EXPORT(open);
ELFDL_EXPORT(creat);
ELFDL_EXPORT(close);
ELFDL_EXPORT(read);
ELFDL_EXPORT(write);
ELFDL_EXPORT(lseek);
ELFDL_EXPORT(unlink);
ELFDL_EXPORT(stat);
ELFDL_EXPORT(fstat);
ELFDL_EXPORT(fsync);
ELFDL_EXPORT(fcntl);
ELFDL_EXPORT(ioctl);
ELFDL_EXPORT(ftruncate);
ELFDL_EXPORT(statfs);
ELFDL_EXPORT(mkdir);
ELFDL_EXPORT(mkdir_pp);
ELFDL_EXPORT(rmdir);
ELFDL_EXPORT(rmdir_rf);
ELFDL_EXPORT(opendir);
ELFDL_EXPORT(readdir);
ELFDL_EXPORT(telldir);
ELFDL_EXPORT(seekdir);
ELFDL_EXPORT(rewinddir);
ELFDL_EXPORT(closedir);
ELFDL_EXPORT(chdir);
ELFDL_EXPORT(getcwd);
ELFDL_EXPORT(access);

#ifndef _WIN32
    ELFDL_EXPORT(rename);
#endif
#endif /*defined(HTD_VFS_USING_POSIX) && defined(HTD_ELFDL_ENABLE)*/



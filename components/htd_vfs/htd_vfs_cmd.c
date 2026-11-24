// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#include "htd_vfs_process.h"

#if defined(HTD_CONSOLE_ENABLE)
#include "htd_console_process.h"

// @brief 控制台命令 ls
static int vfs_cmd_ls(int argc, char **argv)
{
    if (argc == 1) {
        htd_vfs_ls(htd_vfs_work_dir_get());
    } else {
        htd_vfs_ls(argv[1]);
    }
    return 0;
}
CONSOLE_CMD_EXPORT(vfs_cmd_ls, ls);

// @brief 控制台命令 touch
void vfs_cmd_touch(int argc, char** argv)
{
    if (argc < 2) {
        htd_print("ex. %s file\n", argv[0]);
        return;
    }

    // 去除尾部文件名字
    char* p_path = htd_vfs_strdup(argv[1]);
    char* p_end = htd_vfs_path_up_one(p_path, htd_strlen(p_path));
    *p_end = '\0';
    htd_vfs_mkdir_pp(p_path);
    htd_free(p_path);

    // 创建文件
    int fid = htd_vfs_file_open(argv[1], O_CREAT|O_RDWR|O_BINARY);   /*O_TRUNC*/
    if (fid < 0) {
        htd_print("create file failed. %s\n", argv[1]);
        return ;
    }
    htd_vfs_file_close(fid);
}
CONSOLE_CMD_EXPORT(vfs_cmd_touch, touch);

// @brief 控制台命令 cp
static int vfs_cmd_cp(int argc, char **argv)
{
    if (argc != 3) {
        htd_print("Usage: cp SOURCE DEST\n");
        htd_print("Copy SOURCE to DEST.\n");
    } else {
        htd_vfs_copy(argv[1], argv[2]);
    }
    return 0;
}
CONSOLE_CMD_EXPORT(vfs_cmd_cp, cp);

// @brief 控制台命令 mv
static int vfs_cmd_mv(int argc, char **argv)
{
    if (argc != 3) {
        htd_print("Usage: mv SOURCE DEST\n");
        htd_print("Rename SOURCE to DEST, or move SOURCE(s) to DIRECTORY.\n");
    } else {
        char *dest = NULL;
        htd_print("%s => %s\n", argv[1], argv[2]);
        int fd = htd_vfs_file_open(argv[2], O_DIRECTORY);
        if (fd > 0) {
            char *src;
            htd_vfs_file_close(fd);

            dest = (char *)htd_malloc(VFS_PATH_MAX);
            if (dest == NULL) {
                htd_print("out of memory\n");
                return -(ERROR_CODE_NO_MEM);
            }
            src = argv[1] + htd_strlen(argv[1]);
            while (src != argv[1]) {
                if (*src == '/') {
                    break;
                }
                src--;
            }
            htd_snprintf(dest, VFS_PATH_MAX - 1, "%s/%s", argv[2], src);
        } else {
            fd = htd_vfs_file_open(argv[2], O_RDONLY);
            if (fd > 0) {
                htd_vfs_file_close(fd);
                htd_vfs_file_unlink(argv[2]);
            }
            dest = argv[2];
        }

        htd_vfs_rename(argv[1], dest);
        if (dest != NULL && dest != argv[2]) {
            htd_free(dest);
        }
    }
    return 0;
}
CONSOLE_CMD_EXPORT(vfs_cmd_mv, mv);

// @brief 控制台命令 cat
static int vfs_cmd_cat(int argc, char **argv)
{
    if (argc != 2) {
        htd_print("Usage: cat [FILE]...\n");
        return 0;
    } 
    htd_vfs_file_cat(argv[1]);
    return 0;
}
CONSOLE_CMD_EXPORT(vfs_cmd_cat, cat);

// @brief 控制台命令 rm
static int vfs_cmd_rm(int argc, char **argv)
{
    if (argc == 1) {
        htd_print("Usage: rm option(s) FILE...\n");
        return 0;
    } else if (argc == 2) {
        htd_vfs_file_unlink(argv[1]);
        return 0;
    } else {
        if (htd_memcmp(argv[1], "-rf", 4) == 0) {
            htd_vfs_rmdir_rf(argv[2]);
        }
    }
    return 0;
}
CONSOLE_CMD_EXPORT(vfs_cmd_rm, rm);

// @brief 控制台命令 cd
static int vfs_cmd_cd(int argc, char **argv)
{
    if (argc == 1) {
        htd_print("%s\n", htd_vfs_work_dir_get());
    } else if (argc == 2) {
        if (chdir(argv[1]) != 0) {
            htd_print("No such directory: %s\n", argv[1]);
        }
    }
    return 0;
}
CONSOLE_CMD_EXPORT(vfs_cmd_cd, cd);

// @brief 控制台命令 pwd
static int vfs_cmd_pwd(int argc, char **argv)
{
    htd_print("%s\n", htd_vfs_work_dir_get());
    return 0;
}
CONSOLE_CMD_EXPORT(vfs_cmd_pwd, pwd);

// @brief 控制台命令 mkdir 
static int vfs_cmd_mkdir(int argc, char **argv)
{
    if (argc == 1) {
        htd_print("Usage: mkdir [OPTION] DIRECTORY\n");
        htd_print("Create the DIRECTORY, if they do not already exist.\n");
    } else if (argc == 2) {
        htd_vfs_mkdir(argv[1]);
    } else {
        if (0 == htd_memcmp(argv[1], "-pp", 4)) {
            htd_vfs_mkdir_pp(argv[2]);
        }
    }
    return 0;
}
CONSOLE_CMD_EXPORT(vfs_cmd_mkdir, mkdir);

// @brief 控制台命令 mkfs
static int vfs_cmd_mkfs(int argc, char **argv)
{
    int result = 0;
    char *type = "elm"; 

    if (argc == 2) {
        result = htd_vfs_mkfs(type, NULL);
    } else if (argc == 4) {
        if (htd_strcmp(argv[1], "-t") == 0) {
            type = argv[2];
            result = htd_vfs_mkfs(type, NULL);
        }
    } else {
        htd_print("Usage: mkfs [-t type]\n");
        return 0;
    }
    if (result != ERROR_CODE_SUCCESS) {
        htd_print("mkfs failed, result=%d\n", result);
    }
    return 0;
}
CONSOLE_CMD_EXPORT(vfs_cmd_mkfs, mkfs);

// @brief 控制台命令 mount
static int vfs_cmd_mount(int argc, char **argv)
{
    if (argc == 1) {
        htd_print("filesystem  device  mountpoint\n");
        htd_print("----------  ------  ----------\n");
        htd_vfs_t *p_iter = NULL;
        while (1) {
            p_iter = htd_vfs_query(p_iter);
            if (p_iter == NULL) {
                break;
            }
            htd_print("%d  %-s\n", p_iter->type, p_iter->p_path);
        }
        return 0;
    } else if (argc == 3) {
        htd_vfs_type_t fs_type = atoi(argv[1]);
        char *path = argv[2];

        // FIXME: 根据路径 path 从配置表里面获取配置信息，创建设备，然后再挂载
        
        htd_print("mount %s onto %s ... ", fs_type, path);
        if (htd_vfs_mount(fs_type, path, NULL, false) == 0) {
            htd_print("succeed!\n");
            return 0;
        } else {
            htd_print("failed!\n");
            return -1;
        }
    } else {
        htd_print("Usage: mount <device> <mountpoint> <fstype>.\n");
        return -1;
    }
}
CONSOLE_CMD_EXPORT(vfs_cmd_mount, mount);

// @brief 控制台命令 umount
static int vfs_cmd_umount(int argc, char **argv)
{
    char *path = argv[1];
    if (argc != 2) {
        htd_print("Usage: unmount <mountpoint>.\n");
        return -1;
    }

    htd_print("unmount %s ... ", path);
    if (htd_vfs_unmount(path) < 0) {
        htd_print("failed!\n");
        return -1;
    } else {
        htd_print("succeed!\n");
        return 0;
    }
}
CONSOLE_CMD_EXPORT(vfs_cmd_umount, umount);

// @brief 控制台命令 df
static int vfs_cmd_df(int argc, char **argv)
{
    if (argc != 2) {
        htd_vfs_df("/");
    } else {
        if ((htd_strcmp(argv[1], "--help") == 0) || (htd_strcmp(argv[1], "-h") == 0)) {
            htd_print("df [path]\n");
        } else {
            htd_vfs_df(argv[1]);
        }
    }
    return 0;
}
CONSOLE_CMD_EXPORT(vfs_cmd_df, df);

// @brief 控制台命令 echo 
static int vfs_cmd_echo(int argc, char **argv)
{
    if (argc == 2) {
        htd_print("%s\n", argv[1]);
    } else if (argc == 3) {
        int fd = htd_vfs_file_open(argv[2], O_RDWR | O_APPEND | O_CREAT);
        if (fd > 0) {
            htd_vfs_file_write(fd, argv[1], htd_strlen(argv[1]));
            htd_vfs_file_close(fd);
        } else {
            htd_print("open file:%s failed!\n", argv[2]);
        }
    } else {
        htd_print("Usage: echo \"string\" [filename]\n");
    }
    return 0;
}
CONSOLE_CMD_EXPORT(vfs_cmd_echo, echo);

// @brief 控制台命令 tail
static int vfs_cmd_tail(int argc, char **argv)
{
    char *file_name = NULL;
    uint32_t total_lines = 0;
    uint32_t target_line = 0;
    uint32_t current_line = 0;
    uint32_t required_lines = 0;
    uint32_t start_line = 0;

    if (argc < 2) {
        htd_print("Usage: tail [-n numbers] <filename>\n");
        return -1;
    }
    
    if (argc == 2) {
        required_lines = 10; /* default: 10 lines from tail */
        file_name = argv[1];
    } else if (htd_strcmp(argv[1], "-n") == 0) {
        if (argv[2][0] != '+') {
            required_lines = atoi(argv[2]);
        } else {
            start_line = atoi(&argv[2][1]); /* eg: +100, to get the 100 */
        }
        file_name = argv[3];
    } else {
        htd_print("Usage: tail [-n numbers] <filename>\n");
        return -1;
    }

    htd_vfs_fd_t* p_fd = htd_vfs_fopen(file_name, "r");
    if (NULL != p_fd) {
        htd_print("File doesn't exist\n");
        return -1;
    }

    char c;
    while (htd_vfs_fread(p_fd, &c, sizeof(char)) > 0) {
        if(total_lines == 0) {
            total_lines++;
        }
        if (c == '\n') {
            total_lines++;
        }
    }
    htd_print("\nTotal Number of lines:%d\n", total_lines);

    if (start_line != 0) {
        if (total_lines >= start_line) {
            required_lines = total_lines - start_line + 1;
        } else {
            htd_print("\nError:Required lines are more than total number of lines\n");
            htd_vfs_fclose(p_fd);
            return -ERROR_CODE_FAILED;
        }
    }

    if (required_lines > total_lines) {
        htd_print("\nError:Required lines are more than total number of lines\n");
        htd_vfs_fclose(p_fd);
        return -ERROR_CODE_FAILED;
    }
    htd_print("Required Number of lines:%d\n", required_lines);

    target_line = total_lines - required_lines;
    htd_vfs_fseek(p_fd, 0, SEEK_SET);
    while (htd_vfs_fread(p_fd, &c, sizeof(char)) > 0) {
        if (current_line >= target_line) {
            htd_print("%c", c);
        }
        if (c == '\n') {
            current_line++;
        }
    }
    htd_print("\n");

    htd_vfs_fclose(p_fd);
    return 0;
}
CONSOLE_CMD_EXPORT(vfs_cmd_tail, tail);
#endif /*defined(HTD_CONSOLE_ENABLE)*/


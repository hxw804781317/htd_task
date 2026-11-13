// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#include "htd_console_process.h"
#include "htd_console_history.h"

#if defined(HTD_CONSOLE_SUPPORT_CMD)
//////////////////////////////////////////////////////////////////////
HTD_SECTION_DEF(CONSOLE_SECTION_NAME, csl_cmd_cb_t);
static htd_console_t m_htd_console;


// @brief 显示帮助命令
static int console_help_cmd(int argc, char **argv)
{
    const uint32_t* p_item;
    csl_cmd_cb_t *p_shell;
    
    htd_console_print("shell commands:\r\n");
    p_item = htd_section_first_item_get(m_htd_console.cmd_table_begin, m_htd_console.cmd_table_end);
    while (p_item) {
        p_shell = (csl_cmd_cb_t*)p_item;
        if (MEMCMP(p_shell->p_name, "_cmd_", 6) == 0) {
            htd_console_print("%s \r\n", &p_shell->p_name[5]);
        }
        p_item = htd_section_next_item_get(p_item, m_htd_console.cmd_table_end, sizeof(csl_cmd_cb_t));
    }
    htd_console_print("\r\n");
    return 0;
}
CONSOLE_CMD_EXPORT(console_help_cmd, help);

// @brief 
static int console_clear_cmd(int argc, char **argv)
{
    htd_console_print("\x1b[2J\x1b[H");
    return 0;
}
CONSOLE_CMD_EXPORT(console_clear_cmd, clear);


// @brief 控制行命令参数分割
static int console_split(char *cmd, uint32_t cmd_len, char *argv[HTD_CONSOLE_ARG_MAX])
{
    char *ptr = cmd;
    uint32_t position = 0;
    uint32_t argc = 0, i;
    while (position < cmd_len) {
        // 跳过空格和tab键
        while ((*ptr == ' ' || *ptr == '\t') && position < cmd_len) {
            *ptr = '\0';
            ptr++;
            position++;
        }

        if (argc >= HTD_CONSOLE_ARG_MAX) {
            htd_console_print("Too many args! only Use:");
            for (i=0; i<argc; i++) {
                htd_console_print("%s ", argv[i]);
            }
            htd_console_print("\r\n");
            break;
        }

        if (position >= cmd_len) {
            break;
        }

        /* handle string */
        if (*ptr == '"') {
            ptr++;
            position++;
            argv[argc] = ptr;
            argc++;

            // 跳过当前字符串
            while (*ptr != '"' && position < cmd_len) {
                if (*ptr == '\\') {
                    if (*(ptr + 1) == '"') {
                        ptr++;
                        position++;
                    }
                }
                ptr++;
                position++;
            }
            if (position >= cmd_len) {
                break;
            }

            /* skip '"' */
            *ptr = '\0';
            ptr++;
            position++;
        } else {
            argv[argc] = ptr;
            argc++;
            while ((*ptr != ' ' && *ptr != '\t') && position < cmd_len) {
                ptr++;
                position++;
            }
            if (position >= cmd_len) {
                break;
            }
        }
    }
    return argc;
}

// @brief 获取命令
static cmd_function_t console_shell_cmd_get(const char *cmd, int size)
{
    const uint32_t* p_item;
    csl_cmd_cb_t *p_shell;
    cmd_function_t cmd_func = NULL;
    p_item = htd_section_first_item_get(m_htd_console.cmd_table_begin, m_htd_console.cmd_table_end);
    while (p_item) {
        p_shell = (csl_cmd_cb_t*)p_item;
        //htd_console_print("p_name=%s\n", p_shell->p_name);
        if (MEMCMP(p_shell->p_name, "_cmd_", 6) == 0) {
            if (MEMCMP(&p_shell->p_name[5], cmd, size) == 0 && p_shell->p_name[5 + size] == '\0') {
                //htd_console_print("p_name=%s\n", p_shell->p_name);
                cmd_func = p_shell->func;
                break;
            }
        }
        p_item = htd_section_next_item_get(p_item, m_htd_console.cmd_table_end, sizeof(csl_cmd_cb_t));
    }
    return cmd_func;
}

// @brief 获取第一个单词
static uint32_t console_first_word_offset_get(char* cmd, uint32_t cmd_len)
{
    uint32_t offset = 0;
    while ((cmd[offset] != ' ' && cmd[offset] != '\t') && offset < cmd_len) {
        offset++;
    }
    return offset;
}

// @brief 执行对应的命令
static int console_exec_cmd(char *cmd, uint32_t cmd_len, int *retp)
{
    ASSERT(cmd);
    ASSERT(retp);
    
    uint32_t first_word_offset = console_first_word_offset_get(cmd, cmd_len);
    if (first_word_offset == 0) {
        return -1;
    }
    
    cmd_function_t cmd_func = console_shell_cmd_get(cmd, first_word_offset);
    if (cmd_func == NULL) {
        return -1;
    }

    // 分割命令和参数
    char *argv[HTD_CONSOLE_ARG_MAX];
    MEMSET(argv, 0x00, sizeof(argv));
    int argc = console_split(cmd, cmd_len, argv);
    if (argc == 0) {
        htd_console_print("split failed.\n");
        return -1;
    }
    
    // 执行命令
    *retp = cmd_func(argc, argv);
    return 0;
}

// @brief 相同部分长度
static int str_common_len(const char *p_str1, const char *p_str2)
{
    const char *p_start = p_str1;
    while (*p_str1 && *p_str2 && (*p_str1 == *p_str2)) {
        p_str1++;
        p_str2++;
    }
    return (p_str1 - p_start);
}

#if defined(HTD_VFS_ENABLE)
#include "htd_vfs_process.h"
// @brief 自动填充路径
void htd_auto_complete_path(char *path)
{
    if (NULL == path) {
        return;
    }

    char* p_fullpath = (char *)MALLOC(256);
    if (p_fullpath == NULL) {
        return; 
    }
    uint32_t fullpath_len = 0;
    if (*path != '/') {
        getcwd(p_fullpath, 256);
        fullpath_len = strlen(p_fullpath);
        if (p_fullpath[fullpath_len - 1] != '/') {
            p_fullpath[fullpath_len++] = '/';
            p_fullpath[fullpath_len] = 0;
        }
    } else {
        *p_fullpath = '\0';
    }

    // p_index 最后目录名字
    char* p_index = path;
    char* p_ptr = path;
    while (*p_ptr) {
        if (*p_ptr == '/') {
            p_index = p_ptr + 1;
        }
        p_ptr++;
    }
    uint32_t length = p_index - path;
    MEMCPY(&p_fullpath[fullpath_len], path, length);
    fullpath_len += length;
    p_fullpath[fullpath_len] = 0;
    
    struct dirent *dirent = NULL;
    DIR* dir = opendir(p_fullpath);
    if (dir == NULL)  {
        FREE(p_fullpath);
        return;
    }
    
    if (*p_index == '\0')  {
        for (;;) {
            dirent = readdir(dir);
            if (dirent == NULL){
                break;
            }
            htd_console_print("%s\n", dirent->d_name);
        }
    } else {
        uint32_t index_len = strlen(p_index);
        fullpath_len = 0;
        for (;;) {
            dirent = readdir(dir);
            if (dirent == NULL) {
                break;
            }
            if (MEMCMP(p_index, dirent->d_name, index_len) == 0) {
                if (dirent->d_type == DT_DIR) {       // 文件夹
                    htd_console_print("\t\033[0;35m%s\n\033[0m", dirent->d_name);
                } else {
                    htd_console_print("\t%s\n", dirent->d_name);
                }
                if (fullpath_len == 0) {
                    fullpath_len = strlen(dirent->d_name);
                    MEMCPY(p_fullpath, dirent->d_name, fullpath_len+1);
                } else {
                    length = str_common_len(dirent->d_name, p_fullpath);
                    if (length < fullpath_len) {
                        fullpath_len = length;
                    }
                }
            }
        }
        if (fullpath_len) {
            MEMCPY(p_index, p_fullpath, fullpath_len);
            p_index[fullpath_len] = 0;
        }
    }
    closedir(dir);
    FREE(p_fullpath);
}
#endif

// @brief 自动对齐命令
void htd_console_auto_complete(char *prefix)
{
    if (*prefix == '\0') {
        console_help_cmd(0, NULL);
        return;
    }
    
    int length, min_length = 0;
    const char *p_cmd_name;
    const char *p_name = NULL;
    csl_cmd_cb_t *p_shell;
    uint32_t prefix_len = strlen(prefix);
    const uint32_t* p_item = htd_section_first_item_get(m_htd_console.cmd_table_begin, m_htd_console.cmd_table_end);
    while (p_item) {
        p_shell = (csl_cmd_cb_t*)p_item;
        if (MEMCMP(p_shell->p_name, "_cmd_", 5) == 0) {
            p_cmd_name = (const char *)&p_shell->p_name[5];
            if (MEMCMP(prefix, p_cmd_name, prefix_len) == 0) {
                // 查找最短的命令
                if (min_length == 0) {
                    p_name = p_cmd_name;
                    min_length = strlen(p_name);
                }
                length = str_common_len(p_name, p_cmd_name);
                if (length < min_length) {
                    min_length = length;
                    p_name = p_cmd_name;
                }
                htd_console_print("%s\r\n", p_cmd_name);
            }
        } 
        p_item = htd_section_next_item_get(p_item, m_htd_console.cmd_table_end, sizeof(csl_cmd_cb_t));
    }
    if (p_name) {
        MEMCPY(prefix, p_name, min_length + 1);
        return ;
    }
    
#if defined(HTD_VFS_ENABLE) && 1
    p_name = prefix + prefix_len;
    while (p_name > prefix) {
        if (*p_name == ' ') {
            htd_auto_complete_path((char*)p_name + 1);
            break;
        }
        p_name--;
    }
#endif
    return ;
}

// @brief 执行对应的命令
int htd_console_exec(char *cmd, uint32_t cmd_len)
{
    // 跳过前面空格
    while (*cmd  == ' ' || *cmd == '\t') {
        cmd++;
        cmd_len--;
    }
    if (cmd_len == 0) {
        return 0;
    }
    
    // 执行命令
    int cmd_ret = 0;
    if (console_exec_cmd(cmd, cmd_len, &cmd_ret) == 0) {
        return cmd_ret;
    }
    
    // 打印提示
    uint32_t first_word_offset = console_first_word_offset_get(cmd, cmd_len);
    cmd[first_word_offset] = 0;
    htd_console_print("%s: command not found.\r\n", cmd);
    return -1;
}

// @brief 自动补全
static void console_auto_complete(char *prefix)
{
    htd_console_print("\r\n");
    htd_console_auto_complete(prefix);
    htd_console_print("%s%s", CONSOLE_PROMPT, prefix);
}

// @brief 控制台输入字符函数
void htd_console_process_loop(uint8_t ch)
{
    int i;
    htd_console_t *p_console = &m_htd_console;
    if (ch == 0) {
        return;
    }
    
    /*
     * handle control key
     * 上键  : 0x1b 0x5b 0x41
     * 下键:   0x1b 0x5b 0x42
     * 右键: 0x1b 0x5b 0x43
     * 左键: 0x1b 0x5b 0x44
     */
    if (ch == 0x1b) {
        p_console->stat = HTD_CSL_INPUT_STAT_WAIT_SPEC_KEY;
        return;
        
    } else if (p_console->stat == HTD_CSL_INPUT_STAT_WAIT_SPEC_KEY) {
        if (ch == 0x5b) {
            p_console->stat = HTD_CSL_INPUT_STAT_WAIT_FUNC_KEY;
            return;
        }
        p_console->stat = HTD_CSL_INPUT_STAT_NONE;
        
    } else if (p_console->stat == HTD_CSL_INPUT_STAT_WAIT_FUNC_KEY) {
        p_console->stat = HTD_CSL_INPUT_STAT_NONE;
        if (ch == 0x41) {                   // 上键, 查看上一次的命令
            console_key_up(p_console);
            return;
        } 
        if (ch == 0x42) {                   // 下键: 查看下一个命令
            console_key_down(p_console);
            return;
        } 
        if (ch == 0x44) {
            // 左键, 移动光标
            if (p_console->line_curpos) {
                htd_console_print("\b");
                p_console->line_curpos--;
            }
            return;
        } 
        if (ch == 0x43) {
            // 右键, 移动光标
            if (p_console->line_curpos < p_console->line_position) {
                htd_console_print("%c", p_console->line[p_console->line_curpos]);
                p_console->line_curpos++;
            }
            return;
        }
    }

    /* received null or error */
    if (ch == '\0' || ch == 0xFF){
        return;
        
    } else if (ch == '\t') {
        /* move the cursor to the beginning of line */
        for (i=0; i<p_console->line_curpos; i++) {
            htd_console_print("\b");
        }
        // 自动补齐
        console_auto_complete(&p_console->line[0]);
        p_console->line_curpos = p_console->line_position = strlen(p_console->line);
        return;
        
    } else if (ch == 0x7f || ch == 0x08) {
        /* note that p_console->line_curpos >= 0 */
        if (p_console->line_curpos == 0) {
            return;
        }
        p_console->line_position--;
        p_console->line_curpos--;

        if (p_console->line_position > p_console->line_curpos) {
            MEMMOVE(&p_console->line[p_console->line_curpos],
                    &p_console->line[p_console->line_curpos + 1],
                    p_console->line_position - p_console->line_curpos);
            p_console->line[p_console->line_position] = 0;
            htd_console_print("\b%s  \b", &p_console->line[p_console->line_curpos]);

            /* move the cursor to the origin position */
            for (i=p_console->line_curpos; i<=p_console->line_position; i++) {
                htd_console_print("\b");
            }
        } else {
            htd_console_print("\b \b");
            p_console->line[p_console->line_position] = 0;
        }
        return;
    }

    // 行尾
    if (ch == '\r' || ch == '\n') {
        htd_console_print("\r\n");
        if (0 == htd_console_exec(p_console->line, p_console->line_position)) {
            console_history_cmd_save(p_console);
        } else {
            console_history_cmd_reset(p_console);
        }
        htd_console_print(CONSOLE_PROMPT);
        MEMSET(p_console->line, 0, sizeof(p_console->line));
        p_console->line_curpos = p_console->line_position = 0;
        return;
    }
    
    // 长度超过，丢掉
    if (p_console->line_position >= HTD_CONSOLE_CMD_SIZE) {
        p_console->line_position = 0;
    }
    
    // 正常字符
    if (p_console->line_curpos < p_console->line_position) {
        MEMMOVE(&p_console->line[p_console->line_curpos + 1], 
                &p_console->line[p_console->line_curpos], 
                p_console->line_position - p_console->line_curpos);
        p_console->line[p_console->line_curpos] = ch;
        htd_console_print("%s", &p_console->line[p_console->line_curpos]);
        
        // 移动光标到新的位置
        for (i=p_console->line_curpos; i<p_console->line_position; i++) {
            htd_console_print("\b");
        }
    } else {
        p_console->line[p_console->line_position] = ch;
        htd_console_print("%c", ch);
    }

    ch = 0;
    p_console->line_position++;
    p_console->line_curpos++;
    if (p_console->line_position >= HTD_CONSOLE_CMD_SIZE) {
        p_console->line_position = 0;
        p_console->line_curpos = 0;
    }
}

// @brief 初始化控制台
int32_t htd_console_process_init(htd_csl_write_handle_t write_handle)
{
    ASSERT(write_handle);
    MEMSET(&m_htd_console, 0, sizeof(htd_console_t));
    m_htd_console.cmd_table_begin = (uint32_t*)HTD_SECTION_START_ADDR(CONSOLE_SECTION_NAME);
    m_htd_console.cmd_table_end = (uint32_t*)HTD_SECTION_END_ADDR(CONSOLE_SECTION_NAME);
    m_htd_console.write_handle = write_handle;
    return 0;
}

// @brief 输出
static void console_fprintf_fwrite(void const * p_user_ctx, char const * p_str, size_t length)
{
    htd_console_t* p_console = (htd_console_t*)p_user_ctx;
    if (p_console && p_console->write_handle) {
        p_console->write_handle((uint8_t*)p_str, length);
    }
}

// @brief 控制台打印, 通过初始化输入的write_handle输出数据
int32_t htd_console_print(const char *fmt, ...)
{
    uint32_t buff_len;
    char buff[64];

    htd_printf_ctx_t ctx;
    ctx.p_user_ctx = &m_htd_console;
    ctx.fwrite = console_fprintf_fwrite;
    ctx.p_io_buffer = buff;
    ctx.io_buffer_size = sizeof(buff);
    ctx.io_buffer_cnt = 0;

    va_list args;
    va_start(args, fmt);
    buff_len = htd_vfprintf(&ctx, fmt, &args);
    va_end(args);
    return buff_len;
}
 
// @brief 控制台打印输出，返回打印的字符长度
uint32_t htd_console_fprintf(char const *p_fmt, va_list *p_args)
{
    char buff[64];
    htd_printf_ctx_t ctx;
    ctx.p_user_ctx = &m_htd_console;
    ctx.fwrite = console_fprintf_fwrite;
    ctx.p_io_buffer = buff;
    ctx.io_buffer_size = sizeof(buff);
    ctx.io_buffer_cnt = 0;
    return htd_vfprintf(&ctx, p_fmt, p_args);
}

#else
//////////////////////////////////////////////////////////////////////
static htd_csl_write_handle_t m_log_write_handle = NULL;


// @brief 初始化控制台
int32_t htd_console_process_init(htd_csl_write_handle_t write_handle)
{
    m_log_write_handle = write_handle;
    return 0;
}

// @brief 输出
static void console_fprintf_fwrite(void const * p_user_ctx, char const * p_str, size_t length)
{
    if (m_log_write_handle) {
        m_log_write_handle((uint8_t*)p_str, length);
    }
}

// @brief 控制台打印, 通过初始化输入的write_handle输出数据
int32_t htd_console_print(const char *fmt, ...)
{
    uint32_t buff_len;
    char buff[64];

    htd_printf_ctx_t ctx;
    ctx.fwrite = console_fprintf_fwrite;
    ctx.p_io_buffer = buff;
    ctx.io_buffer_size = sizeof(buff);
    ctx.io_buffer_cnt = 0;

    va_list args;
    va_start(args, fmt);
    buff_len = htd_vfprintf(&ctx, fmt, &args);
    va_end(args);
    return buff_len;
}
 
// @brief 控制台打印输出，返回打印的字符长度
uint32_t htd_console_fprintf(char const *p_fmt, va_list *p_args)
{
    char buff[64];
    htd_printf_ctx_t ctx;
    ctx.fwrite = console_fprintf_fwrite;
    ctx.p_io_buffer = buff;
    ctx.io_buffer_size = sizeof(buff);
    ctx.io_buffer_cnt = 0;
    return htd_vfprintf(&ctx, p_fmt, p_args);
}
#endif /*HTD_CONSOLE_SUPPORT_CMD*/


// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#include "htd_console_process.h"
#include "htd_console_history.h"


#if defined(HTD_CONSOLE_SUPPORT_CMD_HISTORY)
// @brief 打印历史数据
static bool console_print_history(htd_console_t *p_console)
{
    htd_console_print("\033[2K\r");
    htd_console_print("%s%s", CONSOLE_PROMPT, p_console->line);
    return false;
}
#endif


// @brief 保存历史指令
void console_history_cmd_save(htd_console_t *p_console)
{
#if defined(HTD_CONSOLE_SUPPORT_CMD_HISTORY)
    if (p_console->line_position == 0) {
        return;
    }

    htd_slist_t *p_node;
    history_cmd_t* p_new = MALLOC(sizeof(history_cmd_t));
    ASSERT(p_new);
    MEMSET(p_new->history_cmd, 0, sizeof(p_new->history_cmd));
    MEMCPY(p_new->history_cmd, p_console->line, p_console->line_position);
    htd_slist_append(&p_console->history_head, &p_new->node);
    p_console->history_count++;
    if (p_console->history_count > HTD_CONSOLE_HISTORY_LINES) {
        p_console->history_count--;
        p_node = htd_slist_first(&p_console->history_head);
        htd_slist_remove(&p_console->history_head, p_node);
        p_new = htd_slist_entry(p_node, history_cmd_t, node);
        FREE(p_new);
    }
    p_console->history_current = NULL;
#endif
}

// @brief 重置历史指令
void console_history_cmd_reset(htd_console_t *p_console)
{
#if defined(HTD_CONSOLE_SUPPORT_CMD_HISTORY)
    p_console->history_current = NULL;
#endif
}

// @brief 上键
void console_key_up(htd_console_t *p_console)
{
#if defined(HTD_CONSOLE_SUPPORT_CMD_HISTORY)
    // 往前挪一个节点
    htd_slist_node_t *p_node;
    history_cmd_t* p_find = NULL;
    if (p_console->history_current == NULL) {
        p_node = htd_slist_tail(&p_console->history_head);
        p_find = htd_slist_entry(p_node, history_cmd_t, node);
    } else {
        history_cmd_t* p_cmd;
        p_node = &p_console->history_head;
        while (p_node->next) {
            p_cmd = htd_slist_entry(p_node->next, history_cmd_t, node);
            if (p_cmd == p_console->history_current) {
                if (p_node != &p_console->history_head) {
                    p_find = htd_slist_entry(p_node, history_cmd_t, node);
                    break;
                }
            }
            p_node = p_node->next;
        }
    }
    if (p_find) {
        p_console->history_current = p_find;
        MEMCPY(p_console->line, p_find->history_cmd, HTD_CONSOLE_CMD_SIZE);
        p_console->line_curpos = p_console->line_position = strlen(p_console->line);
        console_print_history(p_console);
        return;
    }
#endif
}

// @brief 下键
void console_key_down(htd_console_t *p_console)
{
#if defined(HTD_CONSOLE_SUPPORT_CMD_HISTORY)
    if (p_console->history_current == NULL) {
        return;
    }

    // 往后挪一个节点
    htd_slist_node_t *p_node = p_console->history_current->node.next;
    if (p_node == NULL) {
        p_console->history_current = NULL;
        MEMSET(p_console->line, 0, sizeof(p_console->line));
        p_console->line_curpos = p_console->line_position = 0;
    } else {
        p_console->history_current = htd_slist_entry(p_node, history_cmd_t, node);
        MEMCPY(p_console->line, p_console->history_current->history_cmd, HTD_CONSOLE_CMD_SIZE);
        p_console->line_curpos = p_console->line_position = strlen(p_console->line);
    }
    console_print_history(p_console);
#endif
}



// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_SLIST_H_
#define _HTD_SLIST_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

// 单链表
struct htd_slist_node {
    struct htd_slist_node *next; 
};
typedef struct htd_slist_node htd_slist_t; 
typedef struct htd_slist_node htd_slist_node_t;


// @brief 初始化单链表
static inline void htd_slist_init(htd_slist_t *p_slist)
{
    p_slist->next = NULL;
}

// @brief 在链表尾部添加一个新的节点
static inline void htd_slist_append(htd_slist_t *p_slist, htd_slist_node_t *p_new_node)
{
    struct htd_slist_node *p_node  = p_slist;
    while (p_node->next) {
        p_node = p_node->next;
    }
    p_node->next = p_new_node;
    p_new_node->next = NULL;
}

// @brief 在p_node节点后插入一个新的节点
static inline void htd_slist_insert(htd_slist_node_t *p_node, htd_slist_node_t *p_new_node)
{
    p_new_node->next = p_node->next;
    p_node->next = p_new_node;
}

// @brief 统计单链表节点个数
static inline unsigned int htd_slist_len(const htd_slist_t *p_slist)
{
    unsigned int len = 0;
    const htd_slist_t *list = p_slist->next;
    while (list != NULL) {
        list = list->next;
        len++;
    }
    return len;
}

// @brief 从单链表中移除一个节点
static inline void htd_slist_remove(htd_slist_t *p_slist, htd_slist_t *p_remove_node)
{
    // 查找节点前一个节点
    htd_slist_node_t *p_node = p_slist;
    while (p_node->next && p_node->next != p_remove_node) {
        p_node = p_node->next;
    }
    // 移除节点
    if (p_node->next != NULL) {
        p_node->next = p_node->next->next;
    }
}

// @brief 节点替换，返回替换掉的节点指针
static inline htd_slist_t* htd_slist_replace(htd_slist_t *p_slist, htd_slist_t *p_replace_node, 
                                                      bool (*compare_func)(htd_slist_t *p_node1, htd_slist_t *p_node2))
{
    htd_slist_node_t *p_replace;
    htd_slist_node_t *p_node = p_slist;
    while (p_node->next) {
        if (compare_func(p_node->next, p_replace_node)) {
            p_replace = p_node->next;
            p_replace_node = p_node->next->next;
            p_node->next = p_replace_node;
            return p_replace;
        }
        p_node = p_node->next;
    }
    return NULL;
}

// @brief 第一个节点
static inline htd_slist_t *htd_slist_first(htd_slist_t *p_slist)
{
    return p_slist->next;
}

// @brief 最后一个节点
static inline htd_slist_t *htd_slist_tail(htd_slist_t *p_slist)
{
    while (p_slist->next) {
        p_slist = p_slist->next;
    }
    return p_slist;
}

// @brief 某个节点下一个节点
static inline htd_slist_t *htd_slist_next(htd_slist_t *p_node)
{
    return p_node->next;
}

// @brief 判断链表是否为空
static inline int htd_slist_is_empty(htd_slist_t *p_slist)
{
    return p_slist->next == NULL;
}
    
#define htd_container_of(ptr, type, member)         ((type *)((char *)(ptr) - (unsigned long)(&((type *)0)->member)))
// 
#define htd_slist_entry(node, type, member)         htd_container_of(node, type, member)
// 第一个节点(链表不能为空)
#define htd_slist_first_entry(ptr, type, member)    htd_slist_entry((ptr)->next, type, member)
// 最后一个节点(链表不能为空)
#define htd_slist_tail_entry(ptr, type, member)     htd_slist_entry(htd_slist_tail(ptr), type, member)
// 遍历链表节点
#define htd_slist_for_each(pos, head)               for (pos = (head)->next; pos != NULL; pos = pos->next)
// 遍历链表数据
#define htd_slist_for_each_entry(pos, head, member) \
        for (pos = htd_slist_entry((head)->next, typeof(*pos), member); \
             &pos->member != (NULL); \
             pos = htd_slist_entry(pos->member.next, typeof(*pos), member))
    
    
#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_SLIST_H_*/


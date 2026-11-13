// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_LIST_H_
#define _HTD_LIST_H_

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

#ifndef CONTAINER_OF
    #define CONTAINER_OF(ptr, type, member)         ((type *)((char *)(ptr) - (unsigned long)(&((type *)0)->member)))
#endif
struct htd_list_node {
    struct htd_list_node *next;
    struct htd_list_node *prev; 
};
typedef struct htd_list_node htd_list_node_t;
typedef struct htd_list_node htd_list_head_t;


#define htd_list_entry(node, type, member)      CONTAINER_OF(node, type, member)
// 遍历链表
#define htd_list_for_each(pos, head)            for (pos = (head)->next; pos != (head); pos = pos->next)
// 安全遍历整个链表
#define htd_list_for_each_safe(pos, n, head)    for (pos = (head)->next, n = pos->next; pos != (head); pos = n, n = pos->next)


// @brief 初始化链表
__STATIC_INLINE void htd_list_head_init(htd_list_node_t* p_node)
{
    p_node->next = p_node->prev = p_node;
}
// @brief 将新节点添加在某个节点 node 前面
__STATIC_INLINE void htd_list_insert_before(htd_list_node_t* p_new, htd_list_node_t* p_node)
{
    p_node->prev->next = p_new;
    p_new->prev = p_node->prev;
    p_node->prev = p_new;
    p_new->next = p_node;
}
// @brief 将新节点添加在某个节点 node 后面
__STATIC_INLINE void htd_list_insert_after(htd_list_node_t* p_new, htd_list_node_t* p_node)
{
    p_node->next->prev = p_new;
    p_new->next = p_node->next;
    p_node->next = p_new;
    p_new->prev = p_node;
}
// @brief 删除链表节点
__STATIC_INLINE void htd_list_del(htd_list_node_t *p_node)
{
    p_node->next->prev = p_node->prev;
    p_node->prev->next = p_node->next;
    p_node->next = p_node->prev = p_node;
}
// @brief 替换
__STATIC_INLINE void htd_list_replace(htd_list_node_t *p_old, htd_list_node_t *p_new)
{
    p_new->next = p_old->next;
    p_new->next->prev = p_new;
    p_new->prev = p_old->prev;
    p_new->prev->next = p_new;
}
// @brief 判断链表是否为空
__STATIC_INLINE int htd_list_is_empty(const htd_list_node_t *p_head)
{
    return (p_head->next == p_head);
}
// @brief 判断节点是否为最后一个节点
__STATIC_INLINE int htd_list_is_last(const htd_list_node_t *p_list, const htd_list_node_t *p_head)
{
    return (p_list->next == p_head);
}

#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_LIST_H_*/


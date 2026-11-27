// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#include "htd_task.h"

#if defined(HTD_ELFDL_ENABLE)
#include "htd_elfdl_process.h"

ELFDL_EXPORT(htd_print);
ELFDL_EXPORT(htd_task_load_timer_function);
ELFDL_EXPORT(htd_task_unload_timer_function);
ELFDL_EXPORT(htd_task_load_idle_function);


ELFDL_EXPORT(htd_task_load_function_fisr);
ELFDL_EXPORT(htd_task_load_function);
ELFDL_EXPORT(htd_task_load_function_data);
ELFDL_EXPORT(htd_task_load_function_type_data);


ELFDL_EXPORT(htd_timer_create);
ELFDL_EXPORT(htd_timer_start);
ELFDL_EXPORT(htd_timer_restart);
ELFDL_EXPORT(htd_timer_stop);
ELFDL_EXPORT(htd_timer_destroy);
ELFDL_EXPORT(htd_timer_delete);
ELFDL_EXPORT(htd_timer_is_active);
ELFDL_EXPORT(htd_timer_param_get);


ELFDL_EXPORT(htd_sem_create);
ELFDL_EXPORT(htd_sem_destroy);
ELFDL_EXPORT(htd_sem_take);
ELFDL_EXPORT(htd_sem_give);
ELFDL_EXPORT(htd_sem_give_fisr);
ELFDL_EXPORT(htd_sem_reset);


ELFDL_EXPORT(htd_sem_lock_create);
ELFDL_EXPORT(htd_sem_lock_destroy);
ELFDL_EXPORT(htd_sem_lock);
ELFDL_EXPORT(htd_sem_unlock);


ELFDL_EXPORT(htd_mutex_lock_create);
ELFDL_EXPORT(htd_mutex_lock_destroy);
ELFDL_EXPORT(htd_mutex_lock);
ELFDL_EXPORT(htd_mutex_unlock);


ELFDL_EXPORT(__htd_memset);
ELFDL_EXPORT(__htd_memcpy);
ELFDL_EXPORT(__htd_memmove);
ELFDL_EXPORT(__htd_memcmp);
ELFDL_EXPORT(__htd_strlen);
ELFDL_EXPORT(__htd_strnlen);
ELFDL_EXPORT(__htd_strcpy);
ELFDL_EXPORT(__htd_strncpy);
ELFDL_EXPORT(__htd_strcmp);
ELFDL_EXPORT(__htd_strcasecmp);
ELFDL_EXPORT(__htd_strstr);




ELFDL_EXPORT(htd_bitmap_pos_set);
ELFDL_EXPORT(htd_bitmap_pos_get);
ELFDL_EXPORT(htd_bitmap_pos_is_set);
ELFDL_EXPORT(htd_bitmap_unused_pos_get);
ELFDL_EXPORT(htd_bitmap_popcount);

ELFDL_EXPORT(htd_base16_encode);
ELFDL_EXPORT(htd_base16_decode);




#endif /*defined(HTD_ELFDL_ENABLE)*/


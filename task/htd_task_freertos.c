// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#define DEBUG_LVL 4
#define DEBUG_TAG              "task.freertos"
#include "htd_task.h"

#if defined(HTD_TASK_FREERTOS)
//////////////////////////////////////////////////////////////////////
#include "htd_task_freertos.h"

// @brief 修正系统 ticks
void htd_task_step_tick(uint32_t sleep_ms)
{
#if defined(configUSE_TICKLESS_IDLE) && configUSE_TICKLESS_IDLE != 0
    uint32_t ticks = sleep_ms * configTICK_RATE_HZ / 1000;
    vTaskStepTick(ticks);
#endif /*defined(configUSE_TICKLESS_IDLE) && configUSE_TICKLESS_IDLE != 0*/
}

// @brief 线程睡眠
void htd_sleep_ms(uint32_t ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms));
}

// @brief 获取系统时间，单位毫秒
uint32_t htd_time_ms_get(void)
{
    uint32_t ticks = xTaskGetTickCount();
    uint32_t time_ms = pdTICKS_TO_MS(ticks);
    if (time_ms == 0) {
        time_ms = 1;
    }
    return time_ms;
}

// @brief 获取系统时间，单位毫秒(中断中调用)
uint32_t htd_time_ms_get_fisr(void)
{
    uint32_t ticks = xTaskGetTickCountFromISR();
    uint32_t time_ms = pdTICKS_TO_MS(ticks);
    if (time_ms == 0) {
        time_ms = 1;
    }
    return time_ms;
}

// @brief FreeRTOS 定时器回调句柄
static void htd_freertos_timer_handle(htd_timer_ctrl_t* p_timer)
{
    timer_param_t *p_param = (timer_param_t*)pvTimerGetTimerID(p_timer);
    p_param->timer_handler_cb(p_param->arg);
}

// @brief 创建定时器
void* htd_timer_create(const char* name, unsigned int ms, bool is_repeat, void (*timer_handler_cb)(void*), void* param)
{
    timer_param_t* p_param = (timer_param_t*)htd_malloc(sizeof(timer_param_t));
    ASSERT(p_param);
    p_param->timer_handler_cb = timer_handler_cb;
    p_param->arg = param;
    p_param->timer_id = xTimerCreate(name, pdMS_TO_TICKS(ms), is_repeat, param, htd_freertos_timer_handle);
    if (p_param->timer_id) {
    #if defined(USE_TIMER_LIST_CMD) && (USE_TIMER_LIST_CMD == 1)
        freertos_timer_register(p_param->timer_id);
    #endif
        return p_param->timer_id;
    }
    return NULL;
}

// @brief 销毁定时器
void htd_timer_destroy(void* handle)
{
    timer_param_t *p_param = (timer_param_t*)pvTimerGetTimerID((htd_timer_ctrl_t*)handle);
    xTimerDelete((htd_timer_ctrl_t*)handle, 0);
#if defined(USE_TIMER_LIST_CMD) && (USE_TIMER_LIST_CMD == 1)
    freertos_timer_unregister(handle);
#endif
    if (p_param) {
        htd_free(p_param);
    }
}



////////////////////////////////////////////////////////////
#if (defined HTD_CONSOLE_ENABLE && configUSE_STATS_FORMATTING_FUNCTIONS && configUSE_TRACE_FACILITY)
#include "htd_console_process.h"
/**
 * @brief 打印所有FreeRTOS任务的信息，类似Linux的ps命令格式。
 *
 * 需要配置以下FreeRTOS宏：
 * - configUSE_TRACE_FACILITY 必须为1
 * - configUSE_STATS_FORMATTING_FUNCTIONS 通常为1（可选，但推荐）
 * - configGENERATE_RUN_TIME_STATS 可设置为1以显示CPU使用率
 *
 * 若内存分配失败，则自动降级使用vTaskList()输出原始表格。
 */
static int list_thread(int argc, char **argv)
{
    uint32_t uxArraySize = uxTaskGetNumberOfTasks();
    TaskStatus_t *pxTaskStatusArray = htd_malloc(uxArraySize * sizeof(TaskStatus_t));
    if (pxTaskStatusArray == NULL) {
        htd_print("Failed to allocate memory for task status array. Using vTaskList():\n");
        char pcWriteBuffer[512];
        vTaskList(pcWriteBuffer);
        htd_print("%s", pcWriteBuffer);
        return 0;
    }

    /* 获取任务状态数组，实际填充的任务数可能小于分配的大小 */
    uxArraySize = uxTaskGetSystemState(pxTaskStatusArray, uxArraySize, NULL);
    if (uxArraySize == 0) {
        htd_print("No tasks found.\n");
        htd_free(pxTaskStatusArray);
        return -1;
    }

    /* 如果启用了运行时间统计，计算总运行时间用于CPU百分比 */
#if (configGENERATE_RUN_TIME_STATS == 1)
    uint32_t ulTotalRunTime = 0;
    for (uint32_t i = 0; i < uxArraySize; i++) {
        ulTotalRunTime += pxTaskStatusArray[i].ulRunTimeCounter;
    }
    if (ulTotalRunTime == 0) {
        ulTotalRunTime = 1;   /* 防止除零 */
    }
#endif

    /* 打印表头 */
    htd_print("\nPID        STATE PRIO  STACK_LEFT  ");
#if (configGENERATE_RUN_TIME_STATS == 1)
    htd_print("CPU%%   ");
#endif
    htd_print("NAME\n");
    htd_print("---------- ----- ---- ----------- ");
#if (configGENERATE_RUN_TIME_STATS == 1)
    htd_print("------ ");
#endif
    htd_print("----------------\n");

    /* 遍历所有任务并输出 */
    for (uint32_t i = 0; i < uxArraySize; i++) {
        TaskStatus_t *task = &pxTaskStatusArray[i];

        /* PID：使用任务句柄的地址作为唯一标识 */
        htd_print("0x%08lx ", (unsigned long)task->xHandle);

        /* 状态映射：eRunning -> R, eReady -> R, eBlocked -> S, eSuspended -> T, eDeleted -> Z */
        char state_char;
        switch (task->eCurrentState) {
            case eRunning:   state_char = 'R'; break;
            case eReady:     state_char = 'R'; break;
            case eBlocked:   state_char = 'S'; break;
            case eSuspended: state_char = 'T'; break;
            case eDeleted:   state_char = 'Z'; break;
            default:         state_char = '?'; break;
        }
        htd_print("%c     ", state_char);
        /* 任务优先级 */
        htd_print("%-4u ", task->uxCurrentPriority);
        /* 栈剩余空间（高水位线，以字为单位） */
        htd_print("%-10u ", task->usStackHighWaterMark);
        
        /* CPU使用率（如果启用了运行时间统计） */
#if (configGENERATE_RUN_TIME_STATS == 1)
        uint32_t cpu_percent = (task->ulRunTimeCounter * 100) / ulTotalRunTime;
        if (cpu_percent > 100) {
            cpu_percent = 100;
        }
        htd_print("%-3u%%  ", cpu_percent);
#endif
        /* 任务名 */
        htd_print("%s\n", task->pcTaskName);
    }

    htd_free(pxTaskStatusArray);
    return 0;
}
CONSOLE_CMD_EXPORT(list_thread, list_thread);


// @brief 显示运行线程信息
int cmd_ps(int argc, char **argv)
{
    list_thread(argc, argv);
    return 0;
}
CONSOLE_CMD_EXPORT(cmd_ps, ps);


////////////////////////////////////////////////////////////
#if defined(USE_MSG_QUEUE_LIST_CMD) && USE_MSG_QUEUE_LIST_CMD == 1
typedef struct MsgQueueInfo {
    QueueHandle_t   handle;          /* 队列句柄 */
    const char     *name;            /* 队列名称 */
    uint16_t        itemSize;        /* 单个消息的大小（字节） */
    uint16_t        queueLength;     /* 队列长度（最大消息数） */
    struct MsgQueueInfo *next;       /* 链表指针 */
} MsgQueueInfo_t;

static MsgQueueInfo_t *msgqueue_list = NULL;   /* 全局链表头 */

/* ------------------ 注册/注销函数 ------------------ */
/**
 * @brief 注册一个消息队列到全局列表
 * @param handle     队列句柄
 * @param name       队列名称（需为静态字符串或持久化内存）
 * @param itemSize   消息大小（字节）
 * @param queueLength队列长度（容量）
 */
void freertos_msgqueue_register(QueueHandle_t handle, const char *name, uint16_t itemSize, uint16_t queueLength)
{
    if (handle == NULL || name == NULL) {
        return;
    }

    /* 分配节点内存 */
    MsgQueueInfo_t *node = htd_malloc(sizeof(MsgQueueInfo_t));
    if (node == NULL) {
        htd_print("Failed to allocate MsgQueueInfo node.\n");
        return;
    }

    node->handle = handle;
    node->name   = name;
    node->itemSize = itemSize;
    node->queueLength = queueLength;
    node->next = msgqueue_list;
    msgqueue_list = node;
}

/**
 * @brief 从全局列表中注销一个消息队列
 * @param handle 队列句柄
 */
void freertos_msgqueue_unregister(QueueHandle_t handle)
{
    MsgQueueInfo_t **curr = &msgqueue_list;
    while (*curr != NULL) {
        if ((*curr)->handle == handle) {
            MsgQueueInfo_t *tmp = *curr;
            *curr = tmp->next;
            htd_free(tmp);
            return;
        }
        curr = &((*curr)->next);
    }
}

/**
 * @brief 显示所有已注册消息队列的信息，格式类似 RT-Thread 的 list_msgqueue
 */
int list_msgqueue(int argc, char **argv)
{
    MsgQueueInfo_t *node = msgqueue_list;
    if (node == NULL) {
        htd_print("No registered message queues.\n");
        return 0;
    }

    /* 打印表头 */
    htd_print("\n%-20s %-10s %-8s %-8s %-8s %-8s\n",
           "Queue Name", "Handle", "MsgSize", "Length", "CurrMsgs", "Free");
    htd_print("-------------------- ---------- -------- -------- -------- --------\n");

    while (node != NULL) {
        /* 获取当前消息数和空闲空间 */
        uint32_t msgs_waiting = uxQueueMessagesWaiting(node->handle);
        uint32_t spaces_avail = uxQueueSpacesAvailable(node->handle);
        /* 队列长度 = 当前消息数 + 剩余空间 */
        uint32_t length = node->queueLength;  // 或 msgs_waiting + spaces_avail

        htd_print("%-20s 0x%08lx %-8lu %-8lu %-8lu %-8lu\n",
               node->name,
               (unsigned long)node->handle,
               node->itemSize,
               length,
               msgs_waiting,
               spaces_avail);

        node = node->next;
    }
    return 0;
}
CONSOLE_CMD_EXPORT(list_msgqueue, list_msgqueue);
#endif /*defined(USE_MSG_QUEUE_LIST_CMD) && USE_MSG_QUEUE_LIST_CMD == 1*/


////////////////////////////////////////////////////////////
#if defined(USE_TIMER_LIST_CMD) && (USE_TIMER_LIST_CMD == 1)
typedef struct TimerInfo {
    TimerHandle_t       handle;         /* 定时器句柄 */
    struct TimerInfo   *next;           /* 链表指针 */
} TimerInfo_t;
static TimerInfo_t *timer_list = NULL;   /* 全局链表头 */

/**
 * @brief 注册一个软件定时器到全局列表
 * @param handle      定时器句柄
 * @param name        定时器名称（需为静态字符串或持久化内存）
 * @param period      定时器周期（ticks）
 * @param autoReload  是否自动重载（pdTRUE/pdFALSE）
 */
void freertos_timer_register(TimerHandle_t handle)
{
    if (handle == NULL) {
        return;
    }

    /* 进入临界区，保护链表操作 */
    taskENTER_CRITICAL();
    TimerInfo_t *node = pvPortMalloc(sizeof(TimerInfo_t));
    if (node == NULL) {
        taskEXIT_CRITICAL();
        htd_print("Failed to allocate TimerInfo node.\n");
        return;
    }
    node->handle = handle;
    node->next = timer_list;
    timer_list = node;
    taskEXIT_CRITICAL();
}

/**
 * @brief 从全局列表中注销一个软件定时器
 * @param handle 定时器句柄
 */
void freertos_timer_unregister(TimerHandle_t handle)
{
    taskENTER_CRITICAL();

    TimerInfo_t **curr = &timer_list;
    while (*curr != NULL) {
        if ((*curr)->handle == handle) {
            TimerInfo_t *tmp = *curr;
            *curr = tmp->next;
            vPortFree(tmp);
            break;
        }
        curr = &((*curr)->next);
    }

    taskEXIT_CRITICAL();
}

/**
 * @brief 显示所有已注册软件定时器的信息
 *        格式类似 RT-Thread 的 list_timer
 */
int32_t list_timer(int argc, char **argv)
{
    TimerInfo_t *node = NULL, *p_info;
    const char *state_str;
    const char *mode_str, *p_name;
    uint32_t period_ms;

    if (timer_list == NULL) {
        htd_print("No registered software timers.\n");
        return 0;
    }

    /* 打印表头 */
    htd_print("\n%-20s %-10s %-10s %-10s %-10s %-10s\n",
           "Timer Name", "Handle", "Period(ms)", "State", "Mode", "TimerID");
    htd_print("-------------------- ---------- ---------- ---------- ---------- ----------\n");

    node = timer_list;
    while (node != NULL) {
        /* 获取定时器状态（需要进入临界区保护） */
        taskENTER_CRITICAL();
        BaseType_t is_active = xTimerIsTimerActive(node->handle);
        p_info = node;
        node = node->next;
        taskEXIT_CRITICAL();

        /* 状态字符串 */
        if (is_active != pdFALSE) {
            state_str = "active";
        } else {
            state_str = "inactive";
        }

        /* 模式字符串 */
        mode_str = (xTimerGetReloadMode(p_info->handle) == pdTRUE) ? "periodic" : "oneshot";
        p_name = pcTimerGetName(p_info->handle);

        /* 周期转换为毫秒（需要 configTICK_RATE_HZ） */
        period_ms = pdTICKS_TO_MS(xTimerGetPeriod(p_info->handle)); 

        /* 获取 Timer ID（可用于存储额外信息） */
        void *timer_id = pvTimerGetTimerID(p_info->handle);
        htd_print("%-20s 0x%08lx %-10lu %-10s %-10s 0x%08lx\n",
               p_name,
               (unsigned long)p_info->handle,
               (unsigned long)period_ms,
               state_str,
               mode_str,
               (unsigned long)timer_id);
    }
    return 0;
}
CONSOLE_CMD_EXPORT(list_timer, list_timer);
#endif /*defined(USE_TIMER_LIST_CMD) && (USE_TIMER_LIST_CMD == 1)*/
#endif /*(defined HTD_CONSOLE_ENABLE*/
#endif /*HTD_OS_FREERTOS*/


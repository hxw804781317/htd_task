# 1、介绍

htd_task 是一款开源的轻量级嵌入式任务管理器，方便开发者基于裸机系统和RTOS系统的开发和移植。资源占用地，支持各种 MCU，主要有以下几个功能。

## 1.1 下载方式

git clone https://github.com/hxw804781317/htd_task

## 1.2 fifo任务调度

提供完全裸机的fifo任务调度器，提供基于RTOS的任务调度器，和裸机的接口完全一致，从裸机开发之后切换到RTOS只需要更改配置即可，当然，从RTOS开发的项目切换到裸机任务调度也只需要更改配置即可。

## 1.3 定时任务调度

不管是RTOS还是裸机，都支持定时任务调度，实际就是软件定时器，执行的主体都在主线程中。

## 1.4 支持 htd_malloc/htd_free

内置 First Fit 堆管理器，代码量量小，实现简单，分配快，支持多块不连续内存拼在一起，支持多个内存块挂载。

## 1.5 内置 SEGGER_RTT

SEGGER_RTT用于日志输出调试

## 1.6 其他功能

支持简单的单链表，双链表，fifo，section，sprintf等

# 2、使用方式

htd_task 是面向函数的任务调度，将函数指针作为参数发送给任务调度器，任务调度器根据调用的先后关系进行执行。

## 2.1 编译选项

使用 Kconfig 配置选项，支持menuconfig配置，可查看目录下的 Kconfig，了解对应的配置选项。

## 2.1 系统对接

打开 port 文件，查看 htd_task_port.h 文件和 htd_task_port.c 文件，里面有详细说明。

## 2.3 开机调用

必须先初始化memheap，接口为 htd_memheap_ff_init 和 htd_memheap_sram_dev_add；接着调用htd_task_sch_init(5)，最后调用 htd_task_sch_loop(NULL);如下所示：

![](C:\Users\Administrator\AppData\Roaming\marktext\images\1b285428b717639c5f7be85289757105a870542f.png)

## 2.4 接口说明

详细阅读 htd_task_nos.h 和 htd_task_sch.h 两个文件。

## 2.5 样例工程

提供 example\stm32f103xx，开发板如下图所示：

![](C:\Users\Administrator\AppData\Roaming\marktext\images\11d96bdb94e2a3bda1549256253c7e2144f3c2c9.jpg)

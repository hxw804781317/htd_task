#ifndef _SYS_ARCH_H_
#define _SYS_ARCH_H_

#include "arch/cc.h"
#include "htd_os.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/



typedef uint32_t sys_prot_t;
#ifndef BSP_USING_NONOS
    typedef htd_sem_t* sys_sem_t;
    typedef htd_mutex_lock_t* sys_mutex_t;
    typedef htd_mq_t  sys_mbox_t;
    typedef htd_thread_t sys_thread_t;
#endif /**/

// @brief 系统初始化，外部调用
int32_t htd_lwip_init(void);


#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_SYS_ARCH_H_*/


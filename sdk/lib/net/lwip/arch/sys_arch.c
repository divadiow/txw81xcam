/*
 * Copyright (c) 2017 Simon Goldschmidt
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Simon Goldschmidt
 *
 */


#include <lwip/opt.h>
#include <lwip/arch.h>
#if !NO_SYS
#include <lwip/sys_arch.h>
#endif
#include <lwip/stats.h>
#include <lwip/debug.h>
#include <lwip/sys.h>

#include <string.h>

/* lwIP includes. */
#include "lwip/opt.h"
#include "lwip/debug.h"
#include "lwip/def.h"
#include "lwip/sys.h"
#include "lwip/mem.h"

/* unify os includes. */
#include "osal/msgqueue.h"
#include "osal/mutex.h"
#include "osal/semaphore.h"
#include "osal/task.h"
#include "osal/timer.h"
#include "osal/sleep.h"
#include "osal/irq.h"


u32_t lwip_sys_now = 0;
static u32_t thread_pool_index = 0;
static struct os_task thread_pool[LWIP_SYS_THREAD_POOL_N];
static const u32_t null_mesg;
static struct os_mutex lwip_sys_mutex;

u32_t
sys_jiffies(void)
{
    lwip_sys_now += 1 + (sys_now() / 10000);
    return lwip_sys_now;
}

u32_t
sys_now(void)
{
    return os_jiffies_to_msecs(os_jiffies());
}

void
sys_init(void)
{
    s32_t ret = 0;
    lwip_sys_now = 0;
    ret = os_mutex_init(&lwip_sys_mutex);
    ASSERT(!ret);
}

#if !NO_SYS

void sys_msleep(u32_t ms)
{
    os_sleep_ms(ms);
}

err_t
sys_sem_new(sys_sem_t *sem, u8_t count)
{
    ASSERT(sem);
    memset(sem, 0, sizeof(sys_sem_t));
    if (RET_OK != os_sema_init(&sem->sem, count)) {
        lwip_printf("sys_sem_new create error\n");
        return RET_ERR;
    }
    sem->sem_vaild = TRUE;
    return ERR_OK;
}

void
sys_sem_free(sys_sem_t *sem)
{
    LWIP_ASSERT("sem != NULL", sem != NULL);
    if (RET_OK != os_sema_del(&sem->sem)) {
        lwip_printf("sys_sem_free error!");
        ASSERT(0);
    }
    sem->sem_vaild = FALSE;
}

void
sys_sem_set_invalid(sys_sem_t *sem)
{
    LWIP_ASSERT("sem != NULL", sem != NULL);
    sem->sem_vaild = FALSE;
}

/* semaphores are 1-based because RAM is initialized as 0, which would be valid */
u32_t
sys_arch_sem_wait(sys_sem_t *sem, u32_t timeout)
{
    LWIP_ASSERT("sem != NULL", sem != NULL);
    LWIP_ASSERT("sem->sem_vaild != NULL", sem->sem_vaild != FALSE);
    u32_t start = sys_now();

    if (os_sema_down(&sem->sem, (timeout != 0) ? (timeout) : (osWaitForever)) < 1) {
        return SYS_ARCH_TIMEOUT;
    }
    return (DIFF_JIFFIES(start, sys_now())) / 1000;
}

void
sys_sem_signal(sys_sem_t *sem)
{
    LWIP_ASSERT("sem != NULL", sem != NULL);
    LWIP_ASSERT("sem->sem_vaild != NULL", sem->sem_vaild != FALSE);
    os_sema_up(&sem->sem);
}

err_t
sys_mutex_new(sys_mutex_t *mutex)
{
    LWIP_ASSERT("mutex != NULL", mutex != NULL);
    memset(mutex,0,sizeof(sys_mutex_t));
    if (RET_OK != os_mutex_init(&mutex->mutex)) {
        lwip_printf("sys_mutex_new error\n");
        return ERR_MEM;
    }
    mutex->mutex_vaild = TRUE;
    return ERR_OK;
}

void
sys_mutex_free(sys_mutex_t *mutex)
{
    /* parameter check */
    LWIP_ASSERT("mutex != NULL", mutex != NULL);
    if (RET_OK != os_mutex_del(&mutex->mutex)) {
        lwip_printf("sys_mutex_free error\n");
    }
    mutex->mutex_vaild = FALSE;
}

void
sys_mutex_set_invalid(sys_mutex_t *mutex)
{
    LWIP_ASSERT("mutex != NULL", mutex != NULL);
    mutex->mutex_vaild = FALSE;
}

void
sys_mutex_lock(sys_mutex_t *mutex)
{
    /* nothing to do, no multithreading supported */
    LWIP_ASSERT("mutex != NULL", mutex != NULL);
    LWIP_ASSERT("mutex->mutex_vaild != FALSE", mutex->mutex_vaild != FALSE);
    /* check that the mutext is valid and unlocked (no nested locking) */
    os_mutex_lock(&mutex->mutex, osWaitForever);
}

void
sys_mutex_unlock(sys_mutex_t *mutex)
{
    /* nothing to do, no multithreading supported */
    LWIP_ASSERT("mutex != NULL", mutex != NULL);
    LWIP_ASSERT("mutex->mutex_vaild != FALSE", mutex->mutex_vaild != FALSE);
    /* we count down just to check the correct pairing of lock/unlock */
    os_mutex_unlock(&mutex->mutex);
}


sys_thread_t
sys_thread_new(const char *name, lwip_thread_fn function, void *arg, int stacksize, int prio)
{
	lwip_printf("Check para:name:%s,handle:%p,arg:%p,size:%d,proi:%x\n",
            name,function,arg,stacksize,prio);
    /* threads not supported */
    ASSERT(stacksize > 0);

    sys_thread_t task_hdl = NULL;

    if (thread_pool_index >= LWIP_SYS_THREAD_POOL_N) {
        //error handle?
        lwip_printf("calling sys_thread_new too much\n");
        return NULL;
    }
    task_hdl = (sys_thread_t)&thread_pool[thread_pool_index];
    thread_pool_index++;

    LWIP_DEBUGF(SYS_DEBUG, ("New Thread: %s\n", name));

    OS_TASK_INIT(name, task_hdl, function, *((uint32 *)arg), prio, stacksize);

    return task_hdl;
}

err_t
sys_mbox_new(sys_mbox_t *mbox, int size)
{
    LWIP_ASSERT("mbox != NULL", mbox != NULL);
    LWIP_ASSERT("size > 0", size > 0);
    memset(mbox,0,sizeof(sys_mbox_t));
    if (RET_OK == os_msgq_init(&mbox->msgq, size)) {
        mbox->mbox_vaild = TRUE;
        return ERR_OK;
    } else {
        lwip_printf("os_msgq_init error!\n");
        return ERR_MEM;
    }
}

void
sys_mbox_free(sys_mbox_t *mbox)
{
    /* parameter check */
    LWIP_ASSERT("mbox != NULL", mbox != NULL);

    os_msgq_del(&mbox->msgq);
    mbox->mbox_vaild = FALSE;
}

void
sys_mbox_set_invalid(sys_mbox_t *mbox)
{
    LWIP_ASSERT("mbox != NULL", mbox != NULL);
    mbox->mbox_vaild =  FALSE;
}

void
sys_mbox_post(sys_mbox_t *q, void *msg)
{
    LWIP_ASSERT("q != NULL", q != NULL);
    LWIP_ASSERT("q->mbox_vaild != FALSE", q->mbox_vaild != FALSE);

    if (msg == NULL) {
        msg = (void *)&null_mesg;
    }
    os_msgq_put(&q->msgq, (u32_t)msg, osWaitForever);
}

err_t
sys_mbox_trypost(sys_mbox_t *q, void *msg)
{
    LWIP_ASSERT("q != NULL", q != NULL);
    LWIP_ASSERT("q->mbox_vaild != FALSE", q->mbox_vaild != FALSE);
	
    if (msg == NULL) {
        msg = (void *)&null_mesg;
    }
    return (ERR_OK == os_msgq_put(&q->msgq, (u32_t)msg, 0)) ? (ERR_OK) : (ERR_MEM);
}

err_t
sys_mbox_trypost_fromisr(sys_mbox_t *q, void *msg)
{
    return sys_mbox_trypost(q, msg);
}

u32_t
sys_arch_mbox_fetch(sys_mbox_t *q, void **msg, u32_t timeout)
{
    u32_t start     = sys_now();
    u32_t handle    = 0;
    LWIP_ASSERT("q != NULL", q != NULL);
    LWIP_ASSERT("q->mbox_vaild != FALSE", q->mbox_vaild != FALSE);

    handle = os_msgq_get(&q->msgq, (timeout != 0) ? (timeout) : (osWaitForever));

    if (handle != 0) {
        if (handle == (u32_t)&null_mesg) {
            *msg = NULL;
        } else {
            *msg = (void *)handle;
        }
    } else {
        return SYS_ARCH_TIMEOUT;
    }
    return (DIFF_JIFFIES(sys_now(), start)) >> 10;
}


u32_t
sys_arch_mbox_tryfetch(sys_mbox_t *q, void **msg)
{
    LWIP_ASSERT("q != NULL", q != NULL);
    LWIP_ASSERT("q->mbox_vaild != FALSE", q->mbox_vaild != FALSE);

    uint32 handle = 0;
    handle = os_msgq_get(&q->msgq, 0);

    if (handle != 0) {
        if (handle == (u32_t)&null_mesg) {
            *msg = NULL;
        } else {
            *msg = (void *)handle;
        }
    } else {
        return SYS_ARCH_TIMEOUT;
    }
    return ERR_OK;
}

/*---------------------------------------------------------------------------*
 * Routine:  sys_arch_protect
 *---------------------------------------------------------------------------*
 * Description:
 *      This optional function does a "fast" critical region protection and
 *      returns the previous protection level. This function is only called
 *      during very short critical regions. An embedded system which supports
 *      ISR-based drivers might want to implement this function by disabling
 *      interrupts. Task-based systems might want to implement this by using
 *      a mutex or disabling tasking. This function should support recursive
 *      calls from the same task or interrupt. In other words,
 *      sys_arch_protect() could be called while already protected. In
 *      that case the return value indicates that it is already protected.
 *
 *      sys_arch_protect() is only required if your port is supporting an
 *      operating system.
 * Outputs:
 *      sys_prot_t              -- Previous protection level (not used here)
 *---------------------------------------------------------------------------*/
sys_prot_t sys_arch_protect(void)
{   
    //return disable_irq();
    return os_mutex_lock(&lwip_sys_mutex, osWaitForever);
}

/*---------------------------------------------------------------------------*
 * Routine:  sys_arch_unprotect
 *---------------------------------------------------------------------------*
 * Description:
 *      This optional function does a "fast" set of critical region
 *      protection to the value specified by pval. See the documentation for
 *      sys_arch_protect() for more information. This function is only
 *      required if your port is supporting an operating system.
 * Inputs:
 *      sys_prot_t              -- Previous protection level (not used here)
 *---------------------------------------------------------------------------*/
void sys_arch_unprotect(sys_prot_t p)
{
    //enable_irq(p);
    os_mutex_unlock(&lwip_sys_mutex);
}


#if LWIP_NETCONN_SEM_PER_THREAD
#error LWIP_NETCONN_SEM_PER_THREAD==1 not supported
#endif /* LWIP_NETCONN_SEM_PER_THREAD */

#endif /* !NO_SYS */

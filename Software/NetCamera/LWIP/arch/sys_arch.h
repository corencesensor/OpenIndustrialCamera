/*
 * Copyright (c) 2001-2003 Swedish Institute of Computer Science.
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
 * Author: Adam Dunkels <adam@sics.se>
 *
 */
#ifndef __ARCH_SYS_ARCH_H__
#define __ARCH_SYS_ARCH_H__ 
#include <includes.h>
#include "arch/cc.h"
#include "includes.h"
 
#ifdef SYS_ARCH_GLOBALS
#define SYS_ARCH_EXT
#else
#define SYS_ARCH_EXT extern
#endif
 
#define MAX_QUEUES        		10	// ��Ϣ���������
#define MAX_QUEUE_ENTRIES 		20	// ÿ����Ϣ����Ĵ�С
//LWIP��Ϣ����ṹ��
typedef struct {
    OS_EVENT*   pQ;    //UCOS��ָ���¼����ƿ��ָ��
    void*       pvQEntries[MAX_QUEUE_ENTRIES];//��Ϣ���� MAX_QUEUE_ENTRIES��Ϣ�����������Ϣ��
} TQ_DESCR, *PQ_DESCR;


typedef OS_EVENT *sys_sem_t;    //LWIPʹ�õ��ź���
typedef OS_EVENT *sys_mutex_t; 	//LWIPʹ�õĻ����ź���
typedef PQ_DESCR sys_mbox_t;    //LWIPʹ�õ���Ϣ����,��ʵ����UCOS�е���Ϣ����
typedef INT8U sys_thread_t;     //�߳�ID,Ҳ�����������ȼ�
#endif 

/* __SYS_RTXC_H__ */


































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
/*  Porting by Michael Vysotsky <michaelvy@hotmail.com> August 2011   */

#define SYS_ARCH_GLOBALS

/* lwIP includes. */
#include "lwip/def.h"
#include "lwip/sys.h"
#include "lwip/mem.h"
#include "sys_arch.h"


const u32_t NullValue; // ����һ������ֵ�����ڿ�ָ��
 
/********************************************************
 * �������ܣ�����һ����Ϣ����
 * ��    �Σ�mbox����Ϣ����
             size�������С
 * �� �� ֵ��ERR_OK=�����ɹ�������=����ʧ��
 ********************************************************/
err_t sys_mbox_new( sys_mbox_t *mbox, int size)
{
	if(size > MAX_QUEUE_ENTRIES)
	{
		size = MAX_QUEUE_ENTRIES; // ��Ϣ�ж��е���Ϣ��Ŀ���
	}
 
// ������Ϣ�жӣ�����Ϣ�жӴ��ָ��(4�ֽ�)
	mbox->xQueue = xQueueCreate(size, sizeof(void *));
	if(mbox->xQueue != NULL)
	{
		return ERR_OK; // ��Ϣ�жӴ����ɹ�
	}
	return ERR_MEM; // ��Ϣ�жӴ���ʧ��
}
 
/********************************************************
 * �������ܣ��ͷŲ�ɾ��һ����Ϣ����
 * ��    �Σ�mbox����Ϣ����
 * �� �� ֵ����
 ********************************************************/
void sys_mbox_free(sys_mbox_t *mbox)
{
	vQueueDelete(mbox->xQueue);
	mbox->xQueue = NULL;
}
 
/********************************************************
 * �������ܣ�����Ϣ�����з���һ����Ϣ(�ȴ����ͳɹ��Ż᷵��)
 * ��    �Σ�mbox����Ϣ����
             msg��Ҫ���͵���Ϣ
 * �� �� ֵ����
 ********************************************************/
void sys_mbox_post(sys_mbox_t *mbox, void *msg)
{
   BaseType_t xHigherPriorityTaskWoken = pdFALSE;
 
   if(msg == NULL)
   {
      msg = (void *)&NullValue; // ��ָ��Ĵ���ʽ���ó����ĵ�ַ�滻��
   }
 
   if((SCB_ICSR_REG & 0xFF) == 0) // �߳�ִ��
   {
      while(xQueueSendToBack(mbox->xQueue, &msg, portMAX_DELAY) != pdPASS); // �ȴ����ͳɹ�
   }
   else // �ж�ִ��
   {
      while(xQueueSendToBackFromISR(mbox->xQueue, &msg, &xHigherPriorityTaskWoken) != pdPASS);
      portYIELD_FROM_ISR(xHigherPriorityTaskWoken); // �ж��Ƿ�Ҫ���������л�
   }
}
 
/********************************************************
 * �������ܣ�����Ϣ�����з���һ����Ϣ(���������������)
 * ��    �Σ�mbox����Ϣ����
             msg��Ҫ���͵���Ϣ
 * �� �� ֵ��ERR_OK=����OK, ERR_MEM=����ʧ��
 ********************************************************/
err_t sys_mbox_trypost(sys_mbox_t *mbox, void *msg)
{
   BaseType_t xHigherPriorityTaskWoken = pdFALSE;
 
   if(msg == NULL)
   {
      msg = (void *)&NullValue; // ��ָ��Ĵ���ʽ���ó����ĵ�ַ�滻��
   }
 
   if((SCB_ICSR_REG & 0xFF) == 0) // �߳�ִ��
   {
      if(xQueueSendToBack(mbox->xQueue, &msg, 0) != pdPASS)
      {
         return ERR_MEM;
      }
   }
   else
   {
      if(xQueueSendToBackFromISR(mbox->xQueue, &msg, &xHigherPriorityTaskWoken) != pdPASS)
      {
         return ERR_MEM;
      }
   }
 
   return ERR_OK;
}
 
/********************************************************
 * �������ܣ��ȴ������е���Ϣ
 * ��    �Σ�mbox����Ϣ����
             msg��Ҫ�ȴ�����Ϣ
             timeout:��ʱʱ�䣨��λ��ms����0��ʾһֱ�ȴ�
 * �� �� ֵ��ERR_OK=����OK, ERR_MEM=����ʧ��
 ********************************************************/
u32_t sys_arch_mbox_fetch(sys_mbox_t *mbox, void **msg, u32_t timeout)
{
   u32_t start_time = sys_now(); // ��ȡϵͳʱ�䣬���ڼ���ȴ�ʱ��
 
   // �ȴ������е���Ϣ
   if(xQueueReceive(mbox->xQueue, msg, (timeout == 0)? portMAX_DELAY : timeout) == errQUEUE_EMPTY)
   {
      timeout = SYS_ARCH_TIMEOUT; // ����ʱ
      *msg = NULL;
   }
   else
   {
      if(*msg != NULL)
      {
         if(*msg == (void *)&NullValue)
         {
            *msg = NULL;
         }
      }
  
      timeout = sys_now();
      if(timeout >= start_time)
      {
         // ����������Ϣ��ʹ�õ�ʱ��(ʱ��δ���)
         timeout = timeout - start_time;
      }
      else
      {
         // ����������Ϣ��ʹ�õ�ʱ��(ʱ�������)
         timeout += 0xFFFFFFFFUL - start_time;
      }
   }
   return timeout;
}
 
/********************************************************
 * �������ܣ����Դ���Ϣ�����н���һ������Ϣ(������ʽ)
 * ��    �Σ�mbox����Ϣ����
             msg��Ҫ�ȴ�����Ϣ
 * �� �� ֵ���ȴ���Ϣ���õ�ʱ��/SYS_ARCH_TIMEOUT
 ********************************************************/
u32_t sys_arch_mbox_tryfetch(sys_mbox_t *mbox, void **msg)
{
   return sys_arch_mbox_fetch(mbox, msg, 1); // ���Ի�ȡһ����Ϣ
}
 
/********************************************************
 * �������ܣ����һ����Ϣ�����Ƿ���Ч
 * ��    �Σ�mbox����Ϣ����
 * �� �� ֵ��1=��Ч. 0=��Ч
 ********************************************************/
int sys_mbox_valid(sys_mbox_t *mbox)
{
   if(mbox->xQueue != NULL)
   {
      return 1;
   }
   return 0;
}
 
/********************************************************
 * �������ܣ�����һ����Ϣ����Ϊ��Ч
 * ��    �Σ�mbox����Ϣ����
 * �� �� ֵ����
 ********************************************************/
void sys_mbox_set_invalid(sys_mbox_t *mbox)
{
   mbox->xQueue = NULL;
}
 
/********************************************************
 * �������ܣ�����һ���ź���
 * ��    �Σ�sem���ź���ָ��
             count���ź�����ֵ
 * �� �� ֵ��ERR_OK=�����ɹ�������=����ʧ��
 ********************************************************/
err_t sys_sem_new(sys_sem_t *sem, u8_t count)
{
   *sem = xSemaphoreCreateCounting(0xFF, count);
   if(*sem == NULL)
   {
      return ERR_MEM;
   }
 
   return ERR_OK;
}
 
/********************************************************
 * �������ܣ��ȴ�һ���ź���
 * ��    �Σ�sem���ź���ָ��
             timeout����ʱʱ�䣬0��ʾ���޵ȴ�
 * �� �� ֵ���ɹ��ͷ��صȴ���ʱ�䣬ʧ�ܾͷ��س�ʱSYS_ARCH_TIMEOUT
 ********************************************************/
u32_t sys_arch_sem_wait(sys_sem_t *sem, u32_t timeout)
{
   u32_t start_time = sys_now(); // ��ȡϵͳʱ�䣬���ڼ���ȴ�ʱ��
   if(xSemaphoreTake(*sem, (timeout == 0)? portMAX_DELAY : timeout) != pdPASS)
   {
      timeout = SYS_ARCH_TIMEOUT; // ����ʱ
   }
   else
   {
      timeout = sys_now();
      if(timeout >= start_time)
      {
         timeout = timeout - start_time; // ����������Ϣ��ʹ�õ�ʱ��
      }
      else
      {
         timeout += 0xFFFFFFFFUL - start_time;
      }
   }
 
   return timeout;
}
 
/********************************************************
 * �������ܣ�����һ���ź���
 * ��    �Σ�sem���ź���ָ��
 * �� �� ֵ����
 ********************************************************/
void sys_sem_signal(sys_sem_t *sem)
{
   BaseType_t pxHigherPriorityTaskWoken;
 
   if((SCB_ICSR_REG & 0xFF) == 0) // �߳�ִ��
   {
      xSemaphoreGive(*sem);
   }
   else
   {
      xSemaphoreGiveFromISR(*sem, &pxHigherPriorityTaskWoken);
      portYIELD_FROM_ISR(pxHigherPriorityTaskWoken); // ����Ƿ���Ҫ���������л�
   }
}
 
/********************************************************
 * �������ܣ��ͷŲ�ɾ��һ���ź���
 * ��    �Σ�sem���ź���ָ��
 * �� �� ֵ����
 ********************************************************/
void sys_sem_free(sys_sem_t *sem)
{
   vSemaphoreDelete(*sem);
   *sem = NULL;
}
 
/********************************************************
 * �������ܣ���ѯ�ź�����״̬
 * ��    �Σ�sem���ź���ָ��
 * �� �� ֵ��1=��Ч��0=��Ч
 ********************************************************/
int sys_sem_valid(sys_sem_t *sem)
{
   if(*sem == NULL)
   {
      return 0; // ��Ч
   }
   return 1; // ��Ч
}
 
/********************************************************
 * �������ܣ��ź�����Ч����
 * ��    �Σ�sem���ź���ָ��
 * �� �� ֵ����
 ********************************************************/
void sys_sem_set_invalid(sys_sem_t *sem)
{
   *sem = NULL;
}
 
/********************************************************
 * �������ܣ������߳�
 * ��    �Σ�name���߳���
             thred���̺߳���
             arg���߳��������Ĳ���
             stacksize���߳�ջ��С
             prio���߳����ȼ�
 * �� �� ֵ��0=�ɹ���1=ʧ��
 ********************************************************/
sys_thread_t sys_thread_new(const char *name, lwip_thread_fn thread, void *arg, int stacksize, int prio)
{
   BaseType_t flag;
   taskENTER_CRITICAL(); // �����ٽ���
   flag = xTaskCreate(thread, name, stacksize, arg, prio, NULL); // ���� TCP/IP �ں��߳�
   taskEXIT_CRITICAL(); // �˳��ٽ���
   return (flag == pdPASS)? 0 : 1;
}
 
/********************************************************
 * �������ܣ�ARCH��ʼ��
 * ��    �Σ���
 * �� �� ֵ����
 ********************************************************/
void sys_init(void)
{
   // ���ﲻ���κ�����
}
 
/********************************************************
 * �������ܣ�LwIP�ں���ʱ
 * ��    �Σ�ms����ʱʱ�䣨��λ��ms��
 * �� �� ֵ����
 ********************************************************/
void sys_msleep(u32_t ms)
{
   vTaskDelay(ms);
}
 
/********************************************************
 * �������ܣ���ȡϵͳʱ�䣨LWIP1.4.1�����ӵĺ�����
 * ��    �Σ���
 * �� �� ֵ����ǰϵͳʱ��(��λ:����)
 ********************************************************/
u32_t sys_now(void)
{
   if((SCB_ICSR_REG & 0xFF) == 0) // �߳�ִ��
   {
      return xTaskGetTickCount(); // ��ȡϵͳʱ��
   }
   else
   {
      return xTaskGetTickCountFromISR(); // ��ȡϵͳʱ��
   }
}
 
/********************************************************
 * �������ܣ������ٽ���
 * ��    �Σ���
 * �� �� ֵ���жϼĴ���basepri�ı���ֵ
 ********************************************************/
u32_t Enter_Critical(void)
{
   if(SCB_ICSR_REG & 0xFF)
   {
      return taskENTER_CRITICAL_FROM_ISR(); // ���ж���
   }
   else
   {
      taskENTER_CRITICAL(); // ���߳���
      return 0;
   }
}
 
/********************************************************
 * �������ܣ��˳��ٽ���
 * ��    �Σ��жϼĴ���basepri�ı���ֵ
 * �� �� ֵ����
 ********************************************************/
void Exit_Critical(u32_t lev)
{
   if(SCB_ICSR_REG & 0xFF)
   {
      taskEXIT_CRITICAL_FROM_ISR(lev); // ���ж���
   }
   else
   {
      taskEXIT_CRITICAL(); // ���߳���
   }
}

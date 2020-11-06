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


const u32_t NullValue; // 定义一个常量值，用于空指针
 
/********************************************************
 * 函数功能：创建一个消息邮箱
 * 形    参：mbox：消息邮箱
             size：邮箱大小
 * 返 回 值：ERR_OK=创建成功，其他=创建失败
 ********************************************************/
err_t sys_mbox_new( sys_mbox_t *mbox, int size)
{
	if(size > MAX_QUEUE_ENTRIES)
	{
		size = MAX_QUEUE_ENTRIES; // 消息列队中的消息数目检查
	}
 
// 创建消息列队，该消息列队存放指针(4字节)
	mbox->xQueue = xQueueCreate(size, sizeof(void *));
	if(mbox->xQueue != NULL)
	{
		return ERR_OK; // 消息列队创建成功
	}
	return ERR_MEM; // 消息列队创建失败
}
 
/********************************************************
 * 函数功能：释放并删除一个消息邮箱
 * 形    参：mbox：消息邮箱
 * 返 回 值：无
 ********************************************************/
void sys_mbox_free(sys_mbox_t *mbox)
{
	vQueueDelete(mbox->xQueue);
	mbox->xQueue = NULL;
}
 
/********************************************************
 * 函数功能：向消息邮箱中发送一条消息(等待发送成功才会返回)
 * 形    参：mbox：消息邮箱
             msg：要发送的消息
 * 返 回 值：无
 ********************************************************/
void sys_mbox_post(sys_mbox_t *mbox, void *msg)
{
   BaseType_t xHigherPriorityTaskWoken = pdFALSE;
 
   if(msg == NULL)
   {
      msg = (void *)&NullValue; // 空指针的处理方式（用常量的地址替换）
   }
 
   if((SCB_ICSR_REG & 0xFF) == 0) // 线程执行
   {
      while(xQueueSendToBack(mbox->xQueue, &msg, portMAX_DELAY) != pdPASS); // 等待发送成功
   }
   else // 中断执行
   {
      while(xQueueSendToBackFromISR(mbox->xQueue, &msg, &xHigherPriorityTaskWoken) != pdPASS);
      portYIELD_FROM_ISR(xHigherPriorityTaskWoken); // 判断是否要进行任务切换
   }
}
 
/********************************************************
 * 函数功能：向消息邮箱中发送一条消息(发送完就立即返回)
 * 形    参：mbox：消息邮箱
             msg：要发送的消息
 * 返 回 值：ERR_OK=发送OK, ERR_MEM=发送失败
 ********************************************************/
err_t sys_mbox_trypost(sys_mbox_t *mbox, void *msg)
{
   BaseType_t xHigherPriorityTaskWoken = pdFALSE;
 
   if(msg == NULL)
   {
      msg = (void *)&NullValue; // 空指针的处理方式（用常量的地址替换）
   }
 
   if((SCB_ICSR_REG & 0xFF) == 0) // 线程执行
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
 * 函数功能：等待邮箱中的消息
 * 形    参：mbox：消息邮箱
             msg：要等待的消息
             timeout:超时时间（单位：ms），0表示一直等待
 * 返 回 值：ERR_OK=发送OK, ERR_MEM=发送失败
 ********************************************************/
u32_t sys_arch_mbox_fetch(sys_mbox_t *mbox, void **msg, u32_t timeout)
{
   u32_t start_time = sys_now(); // 获取系统时间，用于计算等待时间
 
   // 等待邮箱中的消息
   if(xQueueReceive(mbox->xQueue, msg, (timeout == 0)? portMAX_DELAY : timeout) == errQUEUE_EMPTY)
   {
      timeout = SYS_ARCH_TIMEOUT; // 请求超时
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
         // 计算请求消息所使用的时间(时间未溢出)
         timeout = timeout - start_time;
      }
      else
      {
         // 计算请求消息所使用的时间(时间溢出了)
         timeout += 0xFFFFFFFFUL - start_time;
      }
   }
   return timeout;
}
 
/********************************************************
 * 函数功能：尝试从消息邮箱中接收一个新消息(非阻塞式)
 * 形    参：mbox：消息邮箱
             msg：要等待的消息
 * 返 回 值：等待消息所用的时间/SYS_ARCH_TIMEOUT
 ********************************************************/
u32_t sys_arch_mbox_tryfetch(sys_mbox_t *mbox, void **msg)
{
   return sys_arch_mbox_fetch(mbox, msg, 1); // 尝试获取一个消息
}
 
/********************************************************
 * 函数功能：检查一个消息邮箱是否有效
 * 形    参：mbox：消息邮箱
 * 返 回 值：1=有效. 0=无效
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
 * 函数功能：设置一个消息邮箱为无效
 * 形    参：mbox：消息邮箱
 * 返 回 值：无
 ********************************************************/
void sys_mbox_set_invalid(sys_mbox_t *mbox)
{
   mbox->xQueue = NULL;
}
 
/********************************************************
 * 函数功能：创建一个信号量
 * 形    参：sem：信号量指针
             count：信号量初值
 * 返 回 值：ERR_OK=创建成功，其它=创建失败
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
 * 函数功能：等待一个信号量
 * 形    参：sem：信号量指针
             timeout：超时时间，0表示无限等待
 * 返 回 值：成功就返回等待的时间，失败就返回超时SYS_ARCH_TIMEOUT
 ********************************************************/
u32_t sys_arch_sem_wait(sys_sem_t *sem, u32_t timeout)
{
   u32_t start_time = sys_now(); // 获取系统时间，用于计算等待时间
   if(xSemaphoreTake(*sem, (timeout == 0)? portMAX_DELAY : timeout) != pdPASS)
   {
      timeout = SYS_ARCH_TIMEOUT; // 请求超时
   }
   else
   {
      timeout = sys_now();
      if(timeout >= start_time)
      {
         timeout = timeout - start_time; // 计算请求消息所使用的时间
      }
      else
      {
         timeout += 0xFFFFFFFFUL - start_time;
      }
   }
 
   return timeout;
}
 
/********************************************************
 * 函数功能：发送一个信号量
 * 形    参：sem：信号量指针
 * 返 回 值：无
 ********************************************************/
void sys_sem_signal(sys_sem_t *sem)
{
   BaseType_t pxHigherPriorityTaskWoken;
 
   if((SCB_ICSR_REG & 0xFF) == 0) // 线程执行
   {
      xSemaphoreGive(*sem);
   }
   else
   {
      xSemaphoreGiveFromISR(*sem, &pxHigherPriorityTaskWoken);
      portYIELD_FROM_ISR(pxHigherPriorityTaskWoken); // 检查是否需要进行任务切换
   }
}
 
/********************************************************
 * 函数功能：释放并删除一个信号量
 * 形    参：sem：信号量指针
 * 返 回 值：无
 ********************************************************/
void sys_sem_free(sys_sem_t *sem)
{
   vSemaphoreDelete(*sem);
   *sem = NULL;
}
 
/********************************************************
 * 函数功能：查询信号量的状态
 * 形    参：sem：信号量指针
 * 返 回 值：1=有效，0=无效
 ********************************************************/
int sys_sem_valid(sys_sem_t *sem)
{
   if(*sem == NULL)
   {
      return 0; // 无效
   }
   return 1; // 有效
}
 
/********************************************************
 * 函数功能：信号量无效设置
 * 形    参：sem：信号量指针
 * 返 回 值：无
 ********************************************************/
void sys_sem_set_invalid(sys_sem_t *sem)
{
   *sem = NULL;
}
 
/********************************************************
 * 函数功能：创建线程
 * 形    参：name：线程名
             thred：线程函数
             arg：线程任务函数的参数
             stacksize：线程栈大小
             prio：线程优先级
 * 返 回 值：0=成功，1=失败
 ********************************************************/
sys_thread_t sys_thread_new(const char *name, lwip_thread_fn thread, void *arg, int stacksize, int prio)
{
   BaseType_t flag;
   taskENTER_CRITICAL(); // 进入临界区
   flag = xTaskCreate(thread, name, stacksize, arg, prio, NULL); // 创建 TCP/IP 内核线程
   taskEXIT_CRITICAL(); // 退出临界区
   return (flag == pdPASS)? 0 : 1;
}
 
/********************************************************
 * 函数功能：ARCH初始化
 * 形    参：无
 * 返 回 值：无
 ********************************************************/
void sys_init(void)
{
   // 这里不做任何事情
}
 
/********************************************************
 * 函数功能：LwIP内核延时
 * 形    参：ms：延时时间（单位：ms）
 * 返 回 值：无
 ********************************************************/
void sys_msleep(u32_t ms)
{
   vTaskDelay(ms);
}
 
/********************************************************
 * 函数功能：获取系统时间（LWIP1.4.1新增加的函数）
 * 形    参：无
 * 返 回 值：当前系统时间(单位:毫秒)
 ********************************************************/
u32_t sys_now(void)
{
   if((SCB_ICSR_REG & 0xFF) == 0) // 线程执行
   {
      return xTaskGetTickCount(); // 获取系统时间
   }
   else
   {
      return xTaskGetTickCountFromISR(); // 获取系统时间
   }
}
 
/********************************************************
 * 函数功能：进入临界区
 * 形    参：无
 * 返 回 值：中断寄存器basepri的备份值
 ********************************************************/
u32_t Enter_Critical(void)
{
   if(SCB_ICSR_REG & 0xFF)
   {
      return taskENTER_CRITICAL_FROM_ISR(); // 在中断里
   }
   else
   {
      taskENTER_CRITICAL(); // 在线程中
      return 0;
   }
}
 
/********************************************************
 * 函数功能：退出临界区
 * 形    参：中断寄存器basepri的备份值
 * 返 回 值：无
 ********************************************************/
void Exit_Critical(u32_t lev)
{
   if(SCB_ICSR_REG & 0xFF)
   {
      taskEXIT_CRITICAL_FROM_ISR(lev); // 在中断里
   }
   else
   {
      taskEXIT_CRITICAL(); // 在线程中
   }
}

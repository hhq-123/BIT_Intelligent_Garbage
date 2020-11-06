#ifndef __MR_TASK_INIT_H
#define __MR_TASK_INIT_H

#include "sys.h"
#include "delay.h"
#include "usart.h"

#include "FreeRTOS.h"
#include "task.h"

//�������ȼ�
#define START_TASK_PRIO		1
//�����ջ��С	
#define START_STK_SIZE 		128  
//������
TaskHandle_t StartTask_Handler;
//������
void start_task(void *pvParameters);

/*
//�������ȼ�
#define TASK1_TASK_PRIO		2
//�����ջ��С	
#define TASK1_STK_SIZE 		128  
//������
TaskHandle_t Task1Task_Handler;
//������
void task1_task(void *pvParameters);


//�������ȼ�
#define TASK2_TASK_PRIO		3
//�����ջ��С	
#define TASK2_STK_SIZE 		128  
//������
TaskHandle_t Task2Task_Handler;
//������
void task2_task(void *pvParameters);

*/

//�������ȼ�
#define RGB565_TEST_TASK_PRIO		2
//�����ջ��С	
#define RGB565_TEST_STK_SIZE 		128  
//������
TaskHandle_t Rgb565TestTask_Handler;
//������
void rgb565_test_task(void *pvParameters);

//�������ȼ�
#define TCP_CLIENT_TASK_PRIO		2
//�����ջ��С	
#define TCP_CLIENT_STK_SIZE 		128  
//������
TaskHandle_t TcpClientTask_Handler;
//������
void tcp_client_task(void *pvParameters);

#endif

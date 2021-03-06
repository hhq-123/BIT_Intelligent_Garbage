#ifndef __BIG_TASK_INIT_H
#define __BIG_TASK_INIT_H

#include "sys.h"
#include "delay.h"
#include "usart.h"

#include "FreeRTOS.h"
#include "task.h"

//任务优先级
#define START_TASK_PRIO		1
//任务堆栈大小	
#define START_STK_SIZE 		128  
//任务句柄
TaskHandle_t StartTask_Handler;
//任务函数
void start_task(void *pvParameters);

/*
//任务优先级
#define TASK1_TASK_PRIO		2
//任务堆栈大小	
#define TASK1_STK_SIZE 		128  
//任务句柄
TaskHandle_t Task1Task_Handler;
//任务函数
void task1_task(void *pvParameters);


//任务优先级
#define TASK2_TASK_PRIO		3
//任务堆栈大小	
#define TASK2_STK_SIZE 		128  
//任务句柄
TaskHandle_t Task2Task_Handler;
//任务函数
void task2_task(void *pvParameters);

*/
//任务优先级
#define JPEG_TEST_TASK_PRIO		2
//任务堆栈大小	
#define JPEG_TEST_STK_SIZE 		128  
//任务句柄
TaskHandle_t JpegTestTask_Handler;
//任务函数
void jpeg_test_task(void *pvParameters);

//任务优先级
#define RGB565_TEST_TASK_PRIO		2
//任务堆栈大小	
#define RGB565_TEST_STK_SIZE 		128  
//任务句柄
TaskHandle_t Rgb565TestTask_Handler;
//任务函数
void rgb565_test_task(void *pvParameters);

//任务优先级
#define TCP_CLIENT_TASK_PRIO		3
//任务堆栈大小	
#define TCP_CLIENT_STK_SIZE 		128  
//任务句柄
TaskHandle_t TcpClientTask_Handler;
//任务函数
void tcp_client_task(void *pvParameters);

//任务优先级
#define SINGLE_IMAGE_TRANSFER_TASK_PRIO		3
//任务堆栈大小	
#define SINGLE_IMAGE_TRANSFER_STK_SIZE 		128  
//任务句柄
TaskHandle_t SingleImageTransfer_Handler;
//任务函数
void single_image_transfer_task(void *pvParameters);

//任务优先级
#define IMAGE_TRANSMISSION_TASK_PRIO		3
//任务堆栈大小	
#define IMAGE_TRANSMISSION_STK_SIZE 		128  
//任务句柄
TaskHandle_t ImageTransmission_Handler;
//任务函数
void image_transmisson_task(void *pvParameters);

#endif

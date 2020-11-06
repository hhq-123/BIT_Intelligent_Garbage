#ifndef __MR_TASK_H
#define __MR_TASK_H

#include "sys.h"
#include "delay.h"
#include "usart.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "key.h"
#include "led.h"
#include "lcd.h"
#include "usart2.h"  

#include "ov2640.h" 
#include "dcmi.h" 

#include "sys_eth.h"
#include "tcp_client.h"

#include "MR_CV.h"


void MR_start(void);


#endif

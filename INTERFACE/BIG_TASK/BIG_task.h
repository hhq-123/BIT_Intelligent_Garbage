#ifndef __BIG_TASK_H
#define __BIG_TASK_H


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
#include "sram.h"


#include "ov2640.h" 
#include "dcmi.h" 

#include "lwip_comm.h"
#include "LAN8720.h"
#include "lwipopts.h"


//#include "BIG_CV.h"
#include "BIG_ImageTransmission.h"

void BIG_start(void);


#endif

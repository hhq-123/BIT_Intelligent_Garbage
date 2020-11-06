#include "MR_task.h"
#include "MR_task_init.h"

extern const u8*EFFECTS_TBL[7];
extern const u8*JPEG_SIZE_TBL[9]; 

// TCP客户端接收数据结构
#pragma pack(push, 1)
typedef struct _sTcpClientRxMsg
{
   unsigned char *pbuf;
   unsigned int length;
}sTcpClientRxMsg_t;
#pragma pack(pop)

void MR_start(void)
{
	POINT_COLOR=RED;//设置字体为红色 
	LCD_ShowString(30,50,200,16,16,"Explorer STM32F4");	
	LCD_ShowString(30,70,200,16,16,"OV2640 TEST");	
	LCD_ShowString(30,90,200,16,16,"ATOM@ALIENTEK");
	LCD_ShowString(30,110,200,16,16,"2014/5/14");  	 
	while(OV2640_Init())//初始化OV2640
	{
		LCD_ShowString(30,130,240,16,16,"OV2640 ERR");
		delay_ms(200);
	    LCD_Fill(30,130,239,170,WHITE);
		delay_ms(200);
	}
	LCD_ShowString(30,130,200,16,16,"OV2640 OK");  	
	
	//创建开始任务
    xTaskCreate((TaskFunction_t )start_task,            //任务函数
                (const char*    )"start_task",          //任务名称
                (uint16_t       )START_STK_SIZE,        //任务堆栈大小
                (void*          )NULL,                  //传递给任务函数的参数
                (UBaseType_t    )START_TASK_PRIO,       //任务优先级
                (TaskHandle_t*  )&StartTask_Handler);   //任务句柄              
    vTaskStartScheduler();          //开启任务调度
}

int lcd_discolor[14]={	WHITE, BLACK, BLUE,  BRED,      
						GRED,  GBLUE, RED,   MAGENTA,       	 
						GREEN, CYAN,  YELLOW,BROWN, 			
						BRRED, GRAY };


//开始任务任务函数
void start_task(void *pvParameters)
{
    taskENTER_CRITICAL();           //进入临界区
    //创建TASK1任务
    xTaskCreate((TaskFunction_t )tcp_client_task,             
                (const char*    )"tcp_client_task",           
                (uint16_t       )TCP_CLIENT_STK_SIZE,        
                (void*          )NULL,                  
                (UBaseType_t    )TCP_CLIENT_TASK_PRIO,        
                (TaskHandle_t*  )&TcpClientTask_Handler);   

    vTaskDelete(StartTask_Handler); //删除开始任务
    taskEXIT_CRITICAL();            //退出临界区
}


/*
//task1任务函数
void task1_task(void *pvParameters)
{
	u8 task1_num=0;
	
	POINT_COLOR = BLACK;

	LCD_DrawRectangle(5,110,115,314); 	//画一个矩形	
	LCD_DrawLine(5,130,115,130);		//画线
	POINT_COLOR = BLUE;
	LCD_ShowString(6,111,110,16,16,"Task1 Run:000");
	while(1)
	{
		task1_num++;	//任务执1行次数加1 注意task1_num1加到255的时候会清零！！
		LED0=!LED0;
		printf("任务1已经执行：%d次\r\n",task1_num);
		if(task1_num==5) 
		{
            vTaskDelete(Task2Task_Handler);//任务1执行5次删除任务2
			printf("任务1删除了任务2!\r\n");
		}
		LCD_Fill(6,131,114,313,lcd_discolor[task1_num%14]); //填充区域
		LCD_ShowxNum(86,111,task1_num,3,16,0x80);	//显示任务执行次数
        vTaskDelay(1000);                           //延时1s，也就是1000个时钟节拍	
	}
}

//task2任务函数
void task2_task(void *pvParameters)
{
	u8 task2_num=0;
	
	POINT_COLOR = BLACK;

	LCD_DrawRectangle(125,110,234,314); //画一个矩形	
	LCD_DrawLine(125,130,234,130);		//画线
	POINT_COLOR = BLUE;
	LCD_ShowString(126,111,110,16,16,"Task2 Run:000");
	while(1)
	{
		task2_num++;	//任务2执行次数加1 注意task1_num2加到255的时候会清零！！
        LED1=!LED1;
		printf("任务2已经执行：%d次\r\n",task2_num);
		LCD_ShowxNum(206,111,task2_num,3,16,0x80);  //显示任务执行次数
		LCD_Fill(126,131,233,313,lcd_discolor[13-task2_num%14]); //填充区域
        vTaskDelay(1000);                           //延时1s，也就是1000个时钟节拍	
	}
}
*/

void rgb565_test_task(void *pvParameters)
{
	u8 key;
	u8 effect=0,saturation=2,contrast=2;
	u8 scale=1;		//默认是全尺寸缩放
	u8 msgbuf[15];	//消息缓存区 
	LCD_Clear(WHITE);
    POINT_COLOR=RED; 
	LCD_ShowString(30,50,200,16,16,"ALIENTEK STM32F4");
	LCD_ShowString(30,70,200,16,16,"OV2640 RGB565 Mode");
	
	LCD_ShowString(30,100,200,16,16,"KEY0:Contrast");			//对比度
	LCD_ShowString(30,130,200,16,16,"KEY1:Saturation"); 		//色彩饱和度
	LCD_ShowString(30,150,200,16,16,"KEY2:Effects"); 			//特效 
	LCD_ShowString(30,170,200,16,16,"KEY_UP:FullSize/Scale");	//1:1尺寸(显示真实尺寸)/全尺寸缩放
	
	OV2640_RGB565_Mode();	//RGB565模式
	My_DCMI_Init();			//DCMI配置
	DCMI_DMA_Init((u32)&LCD->LCD_RAM,1,DMA_MemoryDataSize_HalfWord,DMA_MemoryInc_Disable);//DCMI DMA配置  
 	OV2640_OutSize_Set(lcddev.width,lcddev.height); 
	DCMI_Start(); 		//启动传输
	while(1)
	{ 
		key=KEY_Scan(0); 
		if(key)
		{ 
			DCMI_Stop(); //停止显示
			switch(key)
			{				    
				case KEY0_PRES:	//对比度设置
					contrast++;
					if(contrast>4)contrast=0;
					OV2640_Contrast(contrast);
					sprintf((char*)msgbuf,"Contrast:%d",(signed char)contrast-2);
					break;
				case KEY1_PRES:	//饱和度Saturation
					saturation++;
					if(saturation>4)saturation=0;
					OV2640_Color_Saturation(saturation);
					sprintf((char*)msgbuf,"Saturation:%d",(signed char)saturation-2);
					break;
				case KEY2_PRES:	//特效设置				 
					effect++;
					if(effect>6)effect=0;
					OV2640_Special_Effects(effect);//设置特效
					sprintf((char*)msgbuf,"%s",EFFECTS_TBL[effect]);
					break;
				case WKUP_PRES:	//1:1尺寸(显示真实尺寸)/缩放	    
					scale=!scale;  
					if(scale==0)
					{
						OV2640_ImageWin_Set((1600-lcddev.width)/2,(1200-lcddev.height)/2,lcddev.width,lcddev.height);//1:1真实尺寸
						OV2640_OutSize_Set(lcddev.width,lcddev.height); 
						sprintf((char*)msgbuf,"Full Size 1:1");
					}else 
					{
						OV2640_ImageWin_Set(0,0,1600,1200);				//全尺寸缩放
						OV2640_OutSize_Set(lcddev.width,lcddev.height); 
						sprintf((char*)msgbuf,"Scale");
					}
					break;
			}
			LCD_ShowString(30,50,210,16,16,msgbuf);//显示提示内容
			delay_ms(800); 
			DCMI_Start();//重新开始传输
		} 
		delay_ms(10);		
	}    
}



void tcp_client_task(void *pvParameters)
{
	u8 buf[30]; 
   sLwipDev_t sLwipDev = {0};
   sTcpClientRxMsg_t msg = {0};
 
   // 以太网、LwIP协议栈初始化
   LCD_ShowString(30,30,200,16,16,"lwIP Initing...");
	 eth_default_ip_get(&sLwipDev);
   
	while(eth_init(&sLwipDev) != 0)
  {
    LCD_ShowString(30,50,200,16,16,"lwIP Init failed!");
		vTaskDelay(500);
		LCD_Fill(30,50,230,110+16,WHITE);//清除显示
		LCD_ShowString(30,50,200,16,16,"Retrying...");  		 
  }
	LCD_ShowString(30,50,200,16,16,"lwIP Init Successed");
  tcp_client_init(&sLwipDev);
	
	sprintf((char*)buf,"mask:%d.%d.%d.%d",sLwipDev.mask[0],sLwipDev.mask[1],sLwipDev.mask[2],sLwipDev.mask[3]);
	LCD_ShowString(30,70,210,16,16,buf);
	sprintf((char*)buf,"gateway:%d.%d.%d.%d",sLwipDev.gateway[0],sLwipDev.gateway[1],sLwipDev.gateway[2],sLwipDev.gateway[3]);
	LCD_ShowString(30,90,210,16,16,buf); 
	sprintf((char*)buf,"local IP:%d.%d.%d.%d",sLwipDev.localip[0],sLwipDev.localip[1],sLwipDev.localip[2],sLwipDev.localip[3]);
	LCD_ShowString(30,110,210,16,16,buf); 
	sprintf((char*)buf,"remote IP:%d.%d.%d.%d",sLwipDev.remoteip[0],sLwipDev.remoteip[1],sLwipDev.remoteip[2],sLwipDev.remoteip[3]);
	LCD_ShowString(30,130,210,16,16,buf); 	
	sprintf((char*)buf,"localport:%d",sLwipDev.localport);
	LCD_ShowString(30,150,210,16,16,buf); 		LCD_ShowString(30,130,210,16,16,buf); 	
	sprintf((char*)buf,"remoteport:%d",sLwipDev.remoteport);
	LCD_ShowString(30,170,210,16,16,buf); 	
  
	while(1)
	{
		if(xQueueReceive(tcp_client_queue(), &msg, portMAX_DELAY) == pdPASS)
    {
			tcp_client_write(msg.pbuf, msg.length);
			vPortFree(msg.pbuf);
    }
  }
}
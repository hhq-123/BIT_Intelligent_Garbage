#include "BIG_task.h"
#include "BIG_task_init.h"

/***************************  图像传输相关 *******************************/
/*DMA的双缓冲 buf暂时使用静态内存*/
extern __align(4) u32 jpeg_buf0[jpeg_dma_bufsize];
extern __align(4) u32 jpeg_buf1[jpeg_dma_bufsize];

extern u32 *jpeg_data_buf;   /*将图像的缓存定义到外部SRAM 探索者最大可用 1MB*/

//u16 Image_WidthHeight =200; /*图像的长度和宽度尺寸*/

extern volatile u32 jpeg_data_len;/*图像数据大小  单位u32*/
extern volatile u8  jpeg_data_ok;   /*0 图像还没有采集完成，1 采集完成  2 处理完成*/


extern void (*dcmi_rx_callback)(void);

/*************************************************************************/
//JPEG尺寸支持列表

extern const u16 jpeg_img_size_tbl[][2];

extern const u8*EFFECTS_TBL[7];
extern const u8*JPEG_SIZE_TBL[9]; 


void BIG_start(void)
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
//    xTaskCreate((TaskFunction_t )single_image_transfer_task,             
//                (const char*    )"single_image_transfer_task",           
//                (uint16_t       )SINGLE_IMAGE_TRANSFER_STK_SIZE,        
//                (void*          )NULL,                  
//                (UBaseType_t    )SINGLE_IMAGE_TRANSFER_TASK_PRIO,        
//                (TaskHandle_t*  )&SingleImageTransfer_Handler);  
	
	

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


void single_image_transfer_task(void *pvParameters)
{
	//网络传输相关
	u8 buf[30]; 
  sLwipDev_t sLwipDev = {0};
  sTcpClientRxMsg_t msg = {0};

	//LCD显示屏相关
	u32 i; 
	u8 *p;
	u8 key;
	u8 effect=0,saturation=2,contrast=2;
	u8 size=3;		//默认是QVGA 320*240尺寸
	u8 msgbuf[15];	//消息缓存区 
	
	
	//DCMI相关
	u8 res=0;
	u8 *jpeg_pbuf;

  // 以太网、LwIP协议栈初始化
	LCD_Clear(WHITE);
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
	sprintf((char*)buf,"lwIP Init Successed");
	tcp_client_write(buf, 20);
	
	sprintf((char*)buf,"mask:%d.%d.%d.%d",sLwipDev.mask[0],sLwipDev.mask[1],sLwipDev.mask[2],sLwipDev.mask[3]);
	LCD_ShowString(30,70,210,16,16,buf);
	sprintf((char*)buf,"gateway:%d.%d.%d.%d",sLwipDev.gateway[0],sLwipDev.gateway[1],sLwipDev.gateway[2],sLwipDev.gateway[3]);
	LCD_ShowString(30,90,210,16,16,buf); 
	sprintf((char*)buf,"local IP:%d.%d.%d.%d",sLwipDev.localip[0],sLwipDev.localip[1],sLwipDev.localip[2],sLwipDev.localip[3]);
	LCD_ShowString(30,110,210,16,16,buf); 
	sprintf((char*)buf,"remote IP:%d.%d.%d.%d",sLwipDev.remoteip[0],sLwipDev.remoteip[1],sLwipDev.remoteip[2],sLwipDev.remoteip[3]);
	LCD_ShowString(30,130,210,16,16,buf); 	
	sprintf((char*)buf,"localport:%d",sLwipDev.localport);
	LCD_ShowString(30,150,210,16,16,buf);
	sprintf((char*)buf,"remoteport:%d",sLwipDev.remoteport);
	LCD_ShowString(30,170,210,16,16,buf); 
	
	//DCMI初始化
	dcmi_rx_callback=jpeg_dcmi_rx_callback;//回调函数
	DCMI_DMA_Init((u32)jpeg_buf0,(u32)jpeg_buf1,jpeg_dma_bufsize,DMA_MemoryDataSize_Word,DMA_MemoryInc_Enable);//DCMI DMA配置(双缓冲模式)
	OV2640_JPEG_Mode();		//切换为JPEG模式 
	OV2640_ImageWin_Set(0,0,1600,1200);			 
	OV2640_OutSize_Set(1600,1200);//拍照尺寸为1600*1200
	while(1)
	{

		DCMI_Start(); 			//启动传输 

		while(jpeg_data_ok!=1);	//等待第一帧图片采集完

		DCMI_Stop(); 			//停止DMA搬运
		 
		jpeg_pbuf=(u8*)jpeg_data_buf;
		for(i=0;i<jpeg_data_len*4;i++)//查找0XFF,0XD8
		{
			if((jpeg_pbuf[i]==0XFF)&&(jpeg_pbuf[i+1]==0XD8))break;
		}
		if(i==jpeg_data_len*4)res=0XFD;//没找到0XFF,0XD8
		else//找到了
		{
			jpeg_pbuf+=i;//偏移到0XFF,0XD8处
			tcp_client_write(jpeg_pbuf,jpeg_data_len*4-i);
		}
		jpeg_data_len=0;
		
		vTaskDelay(1000);                           //延时1s，也就是1000个时钟节拍	
		jpeg_data_ok=2;			//启动下一帧采集
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
	
	tcp_client_write(buf, 20);
	
	sprintf((char*)buf,"mask:%d.%d.%d.%d",sLwipDev.mask[0],sLwipDev.mask[1],sLwipDev.mask[2],sLwipDev.mask[3]);
	LCD_ShowString(30,70,210,16,16,buf);
	sprintf((char*)buf,"gateway:%d.%d.%d.%d",sLwipDev.gateway[0],sLwipDev.gateway[1],sLwipDev.gateway[2],sLwipDev.gateway[3]);
	LCD_ShowString(30,90,210,16,16,buf); 
	sprintf((char*)buf,"local IP:%d.%d.%d.%d",sLwipDev.localip[0],sLwipDev.localip[1],sLwipDev.localip[2],sLwipDev.localip[3]);
	LCD_ShowString(30,110,210,16,16,buf); 
	sprintf((char*)buf,"remote IP:%d.%d.%d.%d",sLwipDev.remoteip[0],sLwipDev.remoteip[1],sLwipDev.remoteip[2],sLwipDev.remoteip[3]);
	LCD_ShowString(30,130,210,16,16,buf); 	
	sprintf((char*)buf,"localport:%d",sLwipDev.localport);
	LCD_ShowString(30,150,210,16,16,buf); 
	sprintf((char*)buf,"remoteport:%d",sLwipDev.remoteport);
	LCD_ShowString(30,170,210,16,16,buf); 	
  
	while(1)
	{
		if(xQueueReceive(tcp_client_queue(), &msg, portMAX_DELAY) == pdPASS)
    {
			tcp_client_write(buf, 20);
			tcp_client_write(msg.pbuf, msg.length);
			vPortFree(msg.pbuf);
    }
  }
}

*/

void show_address(u8 mode)
{
	u8 buf[30];
	if(mode==2)
	{
		sprintf((char*)buf,"DHCP IP :%d.%d.%d.%d",lwipdev.ip[0],lwipdev.ip[1],lwipdev.ip[2],lwipdev.ip[3]);						//打印动态IP地址
		LCD_ShowString(30,130,210,16,16,buf); 
		sprintf((char*)buf,"DHCP GW :%d.%d.%d.%d",lwipdev.gateway[0],lwipdev.gateway[1],lwipdev.gateway[2],lwipdev.gateway[3]);	//打印网关地址
		LCD_ShowString(30,150,210,16,16,buf); 
		sprintf((char*)buf,"NET MASK:%d.%d.%d.%d",lwipdev.netmask[0],lwipdev.netmask[1],lwipdev.netmask[2],lwipdev.netmask[3]);	//打印子网掩码地址
		LCD_ShowString(30,170,210,16,16,buf); 
	}
	else 
	{
		sprintf((char*)buf,"Static IP:%d.%d.%d.%d",lwipdev.ip[0],lwipdev.ip[1],lwipdev.ip[2],lwipdev.ip[3]);						//打印动态IP地址
		LCD_ShowString(30,130,210,16,16,buf); 
		sprintf((char*)buf,"Static GW:%d.%d.%d.%d",lwipdev.gateway[0],lwipdev.gateway[1],lwipdev.gateway[2],lwipdev.gateway[3]);	//打印网关地址
		LCD_ShowString(30,150,210,16,16,buf); 
		sprintf((char*)buf,"NET MASK :%d.%d.%d.%d",lwipdev.netmask[0],lwipdev.netmask[1],lwipdev.netmask[2],lwipdev.netmask[3]);	//打印子网掩码地址
		LCD_ShowString(30,170,210,16,16,buf); 
	}	
}

void tcp_client_task(void *pvParameters)
{
	#if LWIP_DHCP
	lwip_comm_dhcp_creat(); //创建DHCP任务
	#endif
		while(1)
		{ 
		#if LWIP_DHCP									//当开启DHCP的时候
		if(lwipdev.dhcpstatus != 0) 			//开启DHCP
		{
			show_address(lwipdev.dhcpstatus );	//显示地址信息

		}
		#else
		show_address(0); 						//显示静态地址
		OSTaskSuspend(OS_PRIO_SELF); 			//显示完地址信息后挂起自身任务
		#endif //LWIP_DHCP
		vTaskDelay(500);  
	}
	
}


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


extern u8 tcp_client_flag;	 

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
	

	LCD_ShowString(30,110,200,20,16,"Lwip Init Success!"); 		//lwip初始化成功
	printf("Lwip Init Success");
	
//	while(OV2640_Init())//初始化OV2640
//	{
//		LCD_ShowString(30,130,240,16,16,"OV2640 ERR");
//		delay_ms(200);
//	    LCD_Fill(30,130,239,170,WHITE);
//		delay_ms(200);
//	}
	LCD_ShowString(30,130,200,16,16,"OV2640 OK");  	
	printf("OV2640 OK/r/n");
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
	
//tcp_client_task
//image_transmisson_task
//    xTaskCreate((TaskFunction_t )image_transmisson_task,             
//                (const char*    )"image_transmisson_task",           
//                (uint16_t       )IMAGE_TRANSMISSION_STK_SIZE,        
//                (void*          )NULL,                  
//                (UBaseType_t    )IMAGE_TRANSMISSION_TASK_PRIO,        
//                (TaskHandle_t*  )&ImageTransmission_Handler);  		
		
		xTaskCreate((TaskFunction_t )tcp_client_task,             
                (const char*    )"tcp_client_task",           
                (uint16_t       )TCP_CLIENT_STK_SIZE,        
                (void*          )NULL,                  
                (UBaseType_t    )TCP_CLIENT_TASK_PRIO,        
                (TaskHandle_t*  )&TcpClientTask_Handler);  		
				
//		printf("tcp_client_task创建成功/r/n");
    vTaskDelete(StartTask_Handler); //删除开始任务
    taskEXIT_CRITICAL();            //退出临界区
}



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
//tcp_client_flag|=1<<7;

void tcp_client_task(void *pvParameters)
{
//	u8 t;
//	
//	LCD_ShowString(30,110,200,16,16,"lwIP Initing...");

//	LCD_ShowString(30,110,200,16,16,"lwIP Init Successed");
//	//等待DHCP获取 
// 	LCD_ShowString(30,130,200,16,16,"DHCP IP configing...");
//	while((lwipdev.dhcpstatus!=2)&&(lwipdev.dhcpstatus!=0XFF))//等待DHCP获取成功/超时溢出
//	{
//		lwip_periodic_create();
//		//printf("retry/r/n");
//	}
//	
//		while(1)
//	{
//				tcp_client_test();
//				//lwip_test_ui(3);//重新加载UI
//		
//		
//		lwip_periodic_create();
//		delay_ms(2);
//		t++;
//		if(t==100)LCD_ShowString(30,230,200,16,16,"Please choose a mode!");
//		if(t==200)
//		{ 
//			t=0;
//			LCD_Fill(30,230,230,230+16,WHITE);//清除显示
//			LED0=!LED0;
//		} 
//	}
	while(lwip_comm_init()) 	//lwip初始化
	{
		LCD_ShowString(30,110,200,20,16,"Lwip Init failed!"); 	//lwip初始化失败
		vTaskDelay(500);
		LCD_Fill(30,110,230,150,WHITE);
		vTaskDelay(500);
	}
	LCD_ShowString(30,110,200,16,16,"lwIP Init Successed");
	//等待DHCP获取 
	LCD_ShowString(30,130,200,16,16,"DHCP IP configing...");
	//lwip_periodic_create();
	
	while((lwipdev.dhcpstatus!=2)&&(lwipdev.dhcpstatus!=0XFF))
	{
		printf("等待DHCP获取\r\n");
		lwip_periodic_handle();
		vTaskDelay(10);  
	}
	
	while(1)
	{
		tcp_client_test();
		lwip_periodic_handle();
		vTaskDelay(2); 
	}
	
//	#if LWIP_DHCP
//	lwip_comm_dhcp_create(); //创建DHCP任务
//	#endif
//		while(1)
//		{ 
//		#if LWIP_DHCP									//当开启DHCP的时候
//		if(lwipdev.dhcpstatus != 0) 			//开启DHCP
//		{
//			show_address(lwipdev.dhcpstatus );	//显示地址信息
//		}
//		#else
//		show_address(0); 						//显示静态地址
//		OSTaskSuspend(OS_PRIO_SELF); 			//显示完地址信息后挂起自身任务
//		#endif //LWIP_DHCP
		//vTaskDelay(500);  
	
}

//void image_transmisson_task(void *pvParameters)
//{
//	struct tcp_pcb *tcppcb;  	//定义一个TCP服务器控制块
//	struct ip_addr rmtipaddr;  	//远端ip地址
//	
//	
//	u32 i; 
//	u8 res=0;
//	u8* pbuf;
//	u8 key;
//	u8 effect=0,saturation=2,contrast=2;
//	u8 size=3;		//默认是QVGA 320*240尺寸
//	u8 msgbuf[15];	//消息缓存区 

//	sprintf((char*)msgbuf,"JPEG Size:%s",JPEG_SIZE_TBL[size]);
//	LCD_ShowString(30,180,200,16,16,msgbuf);					//显示当前JPEG分辨率
//	
// 	OV2640_JPEG_Mode();		//JPEG模式
//	My_DCMI_Init();			//DCMI配置
//	dcmi_rx_callback=jpeg_dcmi_rx_callback;//回调函数
//	DCMI_DMA_Init((u32)jpeg_buf0,(u32)jpeg_buf1,jpeg_dma_bufsize,DMA_MemoryDataSize_Word,DMA_MemoryInc_Enable);//DCMI DMA配置(双缓冲模式)
//	
//	/**********客户端设置开始**********/
//	tcp_client_set_remoteip();//先选择IP
//	
//	tbuf=mymalloc(SRAMIN,200);	//申请内存
//	if(tbuf==NULL)return ;		//内存申请失败了,直接退出
//	
//	tcppcb=tcp_new();	//创建一个新的pcb
//	if(tcppcb)			//创建成功
//	{
//		IP4_ADDR(&rmtipaddr,lwipdev.remoteip[0],lwipdev.remoteip[1],lwipdev.remoteip[2],lwipdev.remoteip[3]); 
//		tcp_connect(tcppcb,&rmtipaddr,TCP_CLIENT_PORT,tcp_client_connected);  //连接到目的地址的指定端口上,当连接成功后回调tcp_client_connected()函数
// 	}else res=1;
//	
//	/**********客户端设置结束**********/
//	
// 	OV2640_ImageWin_Set(0,0,1600,1200);			 
//	OV2640_OutSize_Set(1600,1200);//拍照尺寸为1600*1200
//	DCMI_Start(); 			//启动传输 
//	
//	while(1)
//	{
//	while(jpeg_data_ok!=1);	//等待第一帧图片采集完
//	
//	pbuf=(u8*)jpeg_data_buf;
//		for(i=0;i<jpeg_data_len*4;i++)//查找0XFF,0XD8
//		{
//			if((pbuf[i]==0XFF)&&(pbuf[i+1]==0XD8))break;
//		}
//		if(i==jpeg_data_len*4)res=0XFD;//没找到0XFF,0XD8
//		else//找到了
//		{
//			pbuf+=i;//偏移到0XFF,0XD8处
//			LCD_ShowString(30,210,210,16,16,"Sending JPEG data..."); //提示正在传输数据
//			for(i=0;i<jpeg_data_len*4;i++)		//dma传输1次等于4字节,所以乘以4.
//			{
////				while(USART_GetFlagStatus(USART2,USART_FLAG_TC)==RESET);	//循环发送,直到发送完毕  		
////				USART_SendData(USART2,pbuf[i]); 
//				tcp_server_sendbuf = pbuf[i];
//				tcp_client_flag|=1<<7;//标记要发送数据
//				
//			} 
//			LCD_ShowString(30,210,210,16,16,"Send data complete!!");//提示传输结束设置 
//		}
//		jpeg_data_ok=2;			//启动下一帧采集
//	}

//}

#include "BIG_task.h"
#include "BIG_task_init.h"

/***************************  ͼ������� *******************************/
/*DMA��˫���� buf��ʱʹ�þ�̬�ڴ�*/
extern __align(4) u32 jpeg_buf0[jpeg_dma_bufsize];
extern __align(4) u32 jpeg_buf1[jpeg_dma_bufsize];

extern u32 *jpeg_data_buf;   /*��ͼ��Ļ��涨�嵽�ⲿSRAM ̽���������� 1MB*/

//u16 Image_WidthHeight =200; /*ͼ��ĳ��ȺͿ�ȳߴ�*/

extern volatile u32 jpeg_data_len;/*ͼ�����ݴ�С  ��λu32*/
extern volatile u8  jpeg_data_ok;   /*0 ͼ��û�вɼ���ɣ�1 �ɼ����  2 �������*/


extern void (*dcmi_rx_callback)(void);

/*************************************************************************/
//JPEG�ߴ�֧���б�

extern const u16 jpeg_img_size_tbl[][2];

extern const u8*EFFECTS_TBL[7];
extern const u8*JPEG_SIZE_TBL[9]; 


void BIG_start(void)
{
	
	POINT_COLOR=RED;//��������Ϊ��ɫ 
	LCD_ShowString(30,50,200,16,16,"Explorer STM32F4");	
	LCD_ShowString(30,70,200,16,16,"OV2640 TEST");	
	LCD_ShowString(30,90,200,16,16,"ATOM@ALIENTEK");
	LCD_ShowString(30,110,200,16,16,"2014/5/14");  	 
	while(OV2640_Init())//��ʼ��OV2640
	{
		LCD_ShowString(30,130,240,16,16,"OV2640 ERR");
		delay_ms(200);
	    LCD_Fill(30,130,239,170,WHITE);
		delay_ms(200);
	}
	LCD_ShowString(30,130,200,16,16,"OV2640 OK");  	
	
	//������ʼ����
    xTaskCreate((TaskFunction_t )start_task,            //������
                (const char*    )"start_task",          //��������
                (uint16_t       )START_STK_SIZE,        //�����ջ��С
                (void*          )NULL,                  //���ݸ��������Ĳ���
                (UBaseType_t    )START_TASK_PRIO,       //�������ȼ�
                (TaskHandle_t*  )&StartTask_Handler);   //������              
    vTaskStartScheduler();          //�����������
								
}

int lcd_discolor[14]={	WHITE, BLACK, BLUE,  BRED,      
						GRED,  GBLUE, RED,   MAGENTA,       	 
						GREEN, CYAN,  YELLOW,BROWN, 			
						BRRED, GRAY };


//��ʼ����������
void start_task(void *pvParameters)
{
    taskENTER_CRITICAL();           //�����ٽ���
    //����TASK1����
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

    vTaskDelete(StartTask_Handler); //ɾ����ʼ����
    taskEXIT_CRITICAL();            //�˳��ٽ���
}


/*
//task1������
void task1_task(void *pvParameters)
{
	u8 task1_num=0;
	
	POINT_COLOR = BLACK;

	LCD_DrawRectangle(5,110,115,314); 	//��һ������	
	LCD_DrawLine(5,130,115,130);		//����
	POINT_COLOR = BLUE;
	LCD_ShowString(6,111,110,16,16,"Task1 Run:000");
	while(1)
	{
		task1_num++;	//����ִ1�д�����1 ע��task1_num1�ӵ�255��ʱ������㣡��
		LED0=!LED0;
		printf("����1�Ѿ�ִ�У�%d��\r\n",task1_num);
		if(task1_num==5) 
		{
            vTaskDelete(Task2Task_Handler);//����1ִ��5��ɾ������2
			printf("����1ɾ��������2!\r\n");
		}
		LCD_Fill(6,131,114,313,lcd_discolor[task1_num%14]); //�������
		LCD_ShowxNum(86,111,task1_num,3,16,0x80);	//��ʾ����ִ�д���
        vTaskDelay(1000);                           //��ʱ1s��Ҳ����1000��ʱ�ӽ���	
	}
}

//task2������
void task2_task(void *pvParameters)
{
	u8 task2_num=0;
	
	POINT_COLOR = BLACK;

	LCD_DrawRectangle(125,110,234,314); //��һ������	
	LCD_DrawLine(125,130,234,130);		//����
	POINT_COLOR = BLUE;
	LCD_ShowString(126,111,110,16,16,"Task2 Run:000");
	while(1)
	{
		task2_num++;	//����2ִ�д�����1 ע��task1_num2�ӵ�255��ʱ������㣡��
        LED1=!LED1;
		printf("����2�Ѿ�ִ�У�%d��\r\n",task2_num);
		LCD_ShowxNum(206,111,task2_num,3,16,0x80);  //��ʾ����ִ�д���
		LCD_Fill(126,131,233,313,lcd_discolor[13-task2_num%14]); //�������
        vTaskDelay(1000);                           //��ʱ1s��Ҳ����1000��ʱ�ӽ���	
	}
}


void single_image_transfer_task(void *pvParameters)
{
	//���紫�����
	u8 buf[30]; 
  sLwipDev_t sLwipDev = {0};
  sTcpClientRxMsg_t msg = {0};

	//LCD��ʾ�����
	u32 i; 
	u8 *p;
	u8 key;
	u8 effect=0,saturation=2,contrast=2;
	u8 size=3;		//Ĭ����QVGA 320*240�ߴ�
	u8 msgbuf[15];	//��Ϣ������ 
	
	
	//DCMI���
	u8 res=0;
	u8 *jpeg_pbuf;

  // ��̫����LwIPЭ��ջ��ʼ��
	LCD_Clear(WHITE);
	LCD_ShowString(30,30,200,16,16,"lwIP Initing...");
	eth_default_ip_get(&sLwipDev);
   
	while(eth_init(&sLwipDev) != 0)
  {
    LCD_ShowString(30,50,200,16,16,"lwIP Init failed!");
		vTaskDelay(500);
		LCD_Fill(30,50,230,110+16,WHITE);//�����ʾ
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
	
	//DCMI��ʼ��
	dcmi_rx_callback=jpeg_dcmi_rx_callback;//�ص�����
	DCMI_DMA_Init((u32)jpeg_buf0,(u32)jpeg_buf1,jpeg_dma_bufsize,DMA_MemoryDataSize_Word,DMA_MemoryInc_Enable);//DCMI DMA����(˫����ģʽ)
	OV2640_JPEG_Mode();		//�л�ΪJPEGģʽ 
	OV2640_ImageWin_Set(0,0,1600,1200);			 
	OV2640_OutSize_Set(1600,1200);//���ճߴ�Ϊ1600*1200
	while(1)
	{

		DCMI_Start(); 			//�������� 

		while(jpeg_data_ok!=1);	//�ȴ���һ֡ͼƬ�ɼ���

		DCMI_Stop(); 			//ֹͣDMA����
		 
		jpeg_pbuf=(u8*)jpeg_data_buf;
		for(i=0;i<jpeg_data_len*4;i++)//����0XFF,0XD8
		{
			if((jpeg_pbuf[i]==0XFF)&&(jpeg_pbuf[i+1]==0XD8))break;
		}
		if(i==jpeg_data_len*4)res=0XFD;//û�ҵ�0XFF,0XD8
		else//�ҵ���
		{
			jpeg_pbuf+=i;//ƫ�Ƶ�0XFF,0XD8��
			tcp_client_write(jpeg_pbuf,jpeg_data_len*4-i);
		}
		jpeg_data_len=0;
		
		vTaskDelay(1000);                           //��ʱ1s��Ҳ����1000��ʱ�ӽ���	
		jpeg_data_ok=2;			//������һ֡�ɼ�
	}
} 



void tcp_client_task(void *pvParameters)
{
	u8 buf[30]; 
   sLwipDev_t sLwipDev = {0};
   sTcpClientRxMsg_t msg = {0};
 
   // ��̫����LwIPЭ��ջ��ʼ��
   LCD_ShowString(30,30,200,16,16,"lwIP Initing...");
	 eth_default_ip_get(&sLwipDev);
   
	while(eth_init(&sLwipDev) != 0)
  {
    LCD_ShowString(30,50,200,16,16,"lwIP Init failed!");
		vTaskDelay(500);
		LCD_Fill(30,50,230,110+16,WHITE);//�����ʾ
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
		sprintf((char*)buf,"DHCP IP :%d.%d.%d.%d",lwipdev.ip[0],lwipdev.ip[1],lwipdev.ip[2],lwipdev.ip[3]);						//��ӡ��̬IP��ַ
		LCD_ShowString(30,130,210,16,16,buf); 
		sprintf((char*)buf,"DHCP GW :%d.%d.%d.%d",lwipdev.gateway[0],lwipdev.gateway[1],lwipdev.gateway[2],lwipdev.gateway[3]);	//��ӡ���ص�ַ
		LCD_ShowString(30,150,210,16,16,buf); 
		sprintf((char*)buf,"NET MASK:%d.%d.%d.%d",lwipdev.netmask[0],lwipdev.netmask[1],lwipdev.netmask[2],lwipdev.netmask[3]);	//��ӡ���������ַ
		LCD_ShowString(30,170,210,16,16,buf); 
	}
	else 
	{
		sprintf((char*)buf,"Static IP:%d.%d.%d.%d",lwipdev.ip[0],lwipdev.ip[1],lwipdev.ip[2],lwipdev.ip[3]);						//��ӡ��̬IP��ַ
		LCD_ShowString(30,130,210,16,16,buf); 
		sprintf((char*)buf,"Static GW:%d.%d.%d.%d",lwipdev.gateway[0],lwipdev.gateway[1],lwipdev.gateway[2],lwipdev.gateway[3]);	//��ӡ���ص�ַ
		LCD_ShowString(30,150,210,16,16,buf); 
		sprintf((char*)buf,"NET MASK :%d.%d.%d.%d",lwipdev.netmask[0],lwipdev.netmask[1],lwipdev.netmask[2],lwipdev.netmask[3]);	//��ӡ���������ַ
		LCD_ShowString(30,170,210,16,16,buf); 
	}	
}

void tcp_client_task(void *pvParameters)
{
	#if LWIP_DHCP
	lwip_comm_dhcp_creat(); //����DHCP����
	#endif
		while(1)
		{ 
		#if LWIP_DHCP									//������DHCP��ʱ��
		if(lwipdev.dhcpstatus != 0) 			//����DHCP
		{
			show_address(lwipdev.dhcpstatus );	//��ʾ��ַ��Ϣ

		}
		#else
		show_address(0); 						//��ʾ��̬��ַ
		OSTaskSuspend(OS_PRIO_SELF); 			//��ʾ���ַ��Ϣ�������������
		#endif //LWIP_DHCP
		vTaskDelay(500);  
	}
	
}


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


extern u8 tcp_client_flag;	 

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
	

	LCD_ShowString(30,110,200,20,16,"Lwip Init Success!"); 		//lwip��ʼ���ɹ�
	printf("Lwip Init Success");
	
//	while(OV2640_Init())//��ʼ��OV2640
//	{
//		LCD_ShowString(30,130,240,16,16,"OV2640 ERR");
//		delay_ms(200);
//	    LCD_Fill(30,130,239,170,WHITE);
//		delay_ms(200);
//	}
	LCD_ShowString(30,130,200,16,16,"OV2640 OK");  	
	printf("OV2640 OK/r/n");
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
				
//		printf("tcp_client_task�����ɹ�/r/n");
    vTaskDelete(StartTask_Handler); //ɾ����ʼ����
    taskEXIT_CRITICAL();            //�˳��ٽ���
}



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
//tcp_client_flag|=1<<7;

void tcp_client_task(void *pvParameters)
{
//	u8 t;
//	
//	LCD_ShowString(30,110,200,16,16,"lwIP Initing...");

//	LCD_ShowString(30,110,200,16,16,"lwIP Init Successed");
//	//�ȴ�DHCP��ȡ 
// 	LCD_ShowString(30,130,200,16,16,"DHCP IP configing...");
//	while((lwipdev.dhcpstatus!=2)&&(lwipdev.dhcpstatus!=0XFF))//�ȴ�DHCP��ȡ�ɹ�/��ʱ���
//	{
//		lwip_periodic_create();
//		//printf("retry/r/n");
//	}
//	
//		while(1)
//	{
//				tcp_client_test();
//				//lwip_test_ui(3);//���¼���UI
//		
//		
//		lwip_periodic_create();
//		delay_ms(2);
//		t++;
//		if(t==100)LCD_ShowString(30,230,200,16,16,"Please choose a mode!");
//		if(t==200)
//		{ 
//			t=0;
//			LCD_Fill(30,230,230,230+16,WHITE);//�����ʾ
//			LED0=!LED0;
//		} 
//	}
	while(lwip_comm_init()) 	//lwip��ʼ��
	{
		LCD_ShowString(30,110,200,20,16,"Lwip Init failed!"); 	//lwip��ʼ��ʧ��
		vTaskDelay(500);
		LCD_Fill(30,110,230,150,WHITE);
		vTaskDelay(500);
	}
	LCD_ShowString(30,110,200,16,16,"lwIP Init Successed");
	//�ȴ�DHCP��ȡ 
	LCD_ShowString(30,130,200,16,16,"DHCP IP configing...");
	//lwip_periodic_create();
	
	while((lwipdev.dhcpstatus!=2)&&(lwipdev.dhcpstatus!=0XFF))
	{
		printf("�ȴ�DHCP��ȡ\r\n");
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
//	lwip_comm_dhcp_create(); //����DHCP����
//	#endif
//		while(1)
//		{ 
//		#if LWIP_DHCP									//������DHCP��ʱ��
//		if(lwipdev.dhcpstatus != 0) 			//����DHCP
//		{
//			show_address(lwipdev.dhcpstatus );	//��ʾ��ַ��Ϣ
//		}
//		#else
//		show_address(0); 						//��ʾ��̬��ַ
//		OSTaskSuspend(OS_PRIO_SELF); 			//��ʾ���ַ��Ϣ�������������
//		#endif //LWIP_DHCP
		//vTaskDelay(500);  
	
}

//void image_transmisson_task(void *pvParameters)
//{
//	struct tcp_pcb *tcppcb;  	//����һ��TCP���������ƿ�
//	struct ip_addr rmtipaddr;  	//Զ��ip��ַ
//	
//	
//	u32 i; 
//	u8 res=0;
//	u8* pbuf;
//	u8 key;
//	u8 effect=0,saturation=2,contrast=2;
//	u8 size=3;		//Ĭ����QVGA 320*240�ߴ�
//	u8 msgbuf[15];	//��Ϣ������ 

//	sprintf((char*)msgbuf,"JPEG Size:%s",JPEG_SIZE_TBL[size]);
//	LCD_ShowString(30,180,200,16,16,msgbuf);					//��ʾ��ǰJPEG�ֱ���
//	
// 	OV2640_JPEG_Mode();		//JPEGģʽ
//	My_DCMI_Init();			//DCMI����
//	dcmi_rx_callback=jpeg_dcmi_rx_callback;//�ص�����
//	DCMI_DMA_Init((u32)jpeg_buf0,(u32)jpeg_buf1,jpeg_dma_bufsize,DMA_MemoryDataSize_Word,DMA_MemoryInc_Enable);//DCMI DMA����(˫����ģʽ)
//	
//	/**********�ͻ������ÿ�ʼ**********/
//	tcp_client_set_remoteip();//��ѡ��IP
//	
//	tbuf=mymalloc(SRAMIN,200);	//�����ڴ�
//	if(tbuf==NULL)return ;		//�ڴ�����ʧ����,ֱ���˳�
//	
//	tcppcb=tcp_new();	//����һ���µ�pcb
//	if(tcppcb)			//�����ɹ�
//	{
//		IP4_ADDR(&rmtipaddr,lwipdev.remoteip[0],lwipdev.remoteip[1],lwipdev.remoteip[2],lwipdev.remoteip[3]); 
//		tcp_connect(tcppcb,&rmtipaddr,TCP_CLIENT_PORT,tcp_client_connected);  //���ӵ�Ŀ�ĵ�ַ��ָ���˿���,�����ӳɹ���ص�tcp_client_connected()����
// 	}else res=1;
//	
//	/**********�ͻ������ý���**********/
//	
// 	OV2640_ImageWin_Set(0,0,1600,1200);			 
//	OV2640_OutSize_Set(1600,1200);//���ճߴ�Ϊ1600*1200
//	DCMI_Start(); 			//�������� 
//	
//	while(1)
//	{
//	while(jpeg_data_ok!=1);	//�ȴ���һ֡ͼƬ�ɼ���
//	
//	pbuf=(u8*)jpeg_data_buf;
//		for(i=0;i<jpeg_data_len*4;i++)//����0XFF,0XD8
//		{
//			if((pbuf[i]==0XFF)&&(pbuf[i+1]==0XD8))break;
//		}
//		if(i==jpeg_data_len*4)res=0XFD;//û�ҵ�0XFF,0XD8
//		else//�ҵ���
//		{
//			pbuf+=i;//ƫ�Ƶ�0XFF,0XD8��
//			LCD_ShowString(30,210,210,16,16,"Sending JPEG data..."); //��ʾ���ڴ�������
//			for(i=0;i<jpeg_data_len*4;i++)		//dma����1�ε���4�ֽ�,���Գ���4.
//			{
////				while(USART_GetFlagStatus(USART2,USART_FLAG_TC)==RESET);	//ѭ������,ֱ���������  		
////				USART_SendData(USART2,pbuf[i]); 
//				tcp_server_sendbuf = pbuf[i];
//				tcp_client_flag|=1<<7;//���Ҫ��������
//				
//			} 
//			LCD_ShowString(30,210,210,16,16,"Send data complete!!");//��ʾ����������� 
//		}
//		jpeg_data_ok=2;			//������һ֡�ɼ�
//	}

//}

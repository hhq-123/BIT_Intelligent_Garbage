#include "sys.h"
#include "delay.h"
#include "usart.h"

#include "timer.h"

#include "BIG_task.h"
#include "malloc.h"

/************************************************
 ALIENTEK ̽����STM32F407������ FreeRTOSʵ��6-1
 FreeRTOS���񴴽���ɾ��(��̬����)-�⺯���汾
 ����֧�֣�www.openedv.com
 �Ա����̣�http://eboard.taobao.com 
 ��ע΢�Ź���ƽ̨΢�źţ�"����ԭ��"����ѻ�ȡSTM32���ϡ�
 ������������ӿƼ����޹�˾  
 ���ߣ�����ԭ�� @ALIENTEK
************************************************/


int main(void)
{ 
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);//����ϵͳ�ж����ȼ�����4
	delay_init(168);					//��ʼ����ʱ����
	uart_init(115200);     				//��ʼ������
	usart2_init(42,115200);		//��ʼ������2������Ϊ115200
	LED_Init();		        			//��ʼ��LED�˿�
	LCD_Init();							//��ʼ��LCD
	FSMC_SRAM_Init();			//��ʼ���ⲿSRAM.
 	
	my_mem_init(SRAMIN);    //��ʼ���ڲ��ڴ��
	my_mem_init(SRAMEX);  	//��ʼ���ⲿ�ڴ��
//	mymem_init(SRAMCCM); 	//��ʼ��CCM�ڴ��
	
	KEY_Init();					//������ʼ�� 
	/*
	POINT_COLOR = RED;
	LCD_ShowString(30,10,200,16,16,"ATK STM32F103/F407");	
	LCD_ShowString(30,30,200,16,16,"FreeRTOS Examp 6-1");
	LCD_ShowString(30,50,200,16,16,"Task Creat and Del");
	LCD_ShowString(30,70,200,16,16,"ATOM@ALIENTEK");
	LCD_ShowString(30,90,200,16,16,"2016/11/25");
	*/
	while(lwip_comm_init()) 	//lwip��ʼ��
	{
		LCD_ShowString(30,110,200,20,16,"Lwip Init failed!"); 	//lwip��ʼ��ʧ��
		delay_ms(500);
		LCD_Fill(30,110,230,150,WHITE);
		delay_ms(500);
	}
	LCD_ShowString(30,110,200,20,16,"Lwip Init Success!"); 		//lwip��ʼ���ɹ�
	
	BIG_start();
}





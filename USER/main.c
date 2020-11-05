#include "sys.h"
#include "delay.h"
#include "usart.h"

#include "timer.h"

#include "MR_task.h"
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
 	KEY_Init();					//������ʼ�� 
	
	POINT_COLOR = RED;
	LCD_ShowString(30,10,200,16,16,"ATK STM32F103/F407");	
	LCD_ShowString(30,30,200,16,16,"FreeRTOS Examp 6-1");
	LCD_ShowString(30,50,200,16,16,"Task Creat and Del");
	LCD_ShowString(30,70,200,16,16,"ATOM@ALIENTEK");
	LCD_ShowString(30,90,200,16,16,"2016/11/25");
	
	MR_start();
}





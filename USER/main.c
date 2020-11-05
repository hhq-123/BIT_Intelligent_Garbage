#include "sys.h"
#include "delay.h"
#include "usart.h"

#include "timer.h"

#include "MR_task.h"
/************************************************
 ALIENTEK 探索者STM32F407开发板 FreeRTOS实验6-1
 FreeRTOS任务创建和删除(动态方法)-库函数版本
 技术支持：www.openedv.com
 淘宝店铺：http://eboard.taobao.com 
 关注微信公众平台微信号："正点原子"，免费获取STM32资料。
 广州市星翼电子科技有限公司  
 作者：正点原子 @ALIENTEK
************************************************/


int main(void)
{ 
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);//设置系统中断优先级分组4
	delay_init(168);					//初始化延时函数
	uart_init(115200);     				//初始化串口
	usart2_init(42,115200);		//初始化串口2波特率为115200
	LED_Init();		        			//初始化LED端口
	LCD_Init();							//初始化LCD
 	KEY_Init();					//按键初始化 
	
	POINT_COLOR = RED;
	LCD_ShowString(30,10,200,16,16,"ATK STM32F103/F407");	
	LCD_ShowString(30,30,200,16,16,"FreeRTOS Examp 6-1");
	LCD_ShowString(30,50,200,16,16,"Task Creat and Del");
	LCD_ShowString(30,70,200,16,16,"ATOM@ALIENTEK");
	LCD_ShowString(30,90,200,16,16,"2016/11/25");
	
	MR_start();
}





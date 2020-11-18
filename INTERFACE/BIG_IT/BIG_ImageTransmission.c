//Image transmission of big trash can
#include "BIG_ImageTransmission.h"

volatile u32 jpeg_data_len=0; 			//buf中的JPEG有效数据长度(*4字节)
volatile u8 jpeg_data_ok=0;				//JPEG数据采集完成标志 
										//0,数据没有采集完;
										//1,数据采集完了,但是还没处理;
										//2,数据已经处理完成了,可以开始下一帧接收
										
//u32 *jpeg_buf0;							//JPEG数据缓存buf,通过malloc申请内存
//u32 *jpeg_buf1;							//JPEG数据缓存buf,通过malloc申请内存
//u32 *jpeg_data_buf;						//JPEG数据缓存buf,通过malloc申请内存
__align(4) u32 jpeg_buf0[jpeg_dma_bufsize];							//JPEG数据缓存buf0
__align(4) u32 jpeg_buf1[jpeg_dma_bufsize];							//JPEG数据缓存buf1

u32 *jpeg_data_buf=(u32 *)0x68000000;/*将图像的缓存定义到外部SRAM 探索者最大可用 1MB*/

void jpeg_data_process(void)
{
	u16 i;
	u16 rlen;//剩余数据长度
	u32 *pbuf;

		if(jpeg_data_ok==0)	//jpeg数据还未采集完?
		{
			DMA_Cmd(DMA2_Stream1,DISABLE);		//停止当前传输
			while(DMA_GetCmdStatus(DMA2_Stream1) != DISABLE);	//等待DMA2_Stream1可配置 
			rlen=jpeg_dma_bufsize-DMA_GetCurrDataCounter(DMA2_Stream1);//得到剩余数据长度	
			pbuf=jpeg_data_buf+jpeg_data_len;//偏移到有效数据末尾,继续添加
			if(DMA2_Stream1->CR&(1<<19))
				for(i=0;i<rlen;i++)
					pbuf[i]=jpeg_buf1[i];//读取buf1里面的剩余数据
			else 
				for(i=0;i<rlen;i++)
					pbuf[i]=jpeg_buf0[i];//读取buf0里面的剩余数据 
			jpeg_data_len+=rlen;			//加上剩余长度
			jpeg_data_ok=1; 				//标记JPEG数据采集完按成,等待其他函数处理
		}
		if(jpeg_data_ok==2)	//上一次的jpeg数据已经被处理了
		{ DMA_SetCurrDataCounter(DMA2_Stream1,jpeg_dma_bufsize);//传输长度为jpeg_buf_size*4字节
			DMA_Cmd(DMA2_Stream1,ENABLE); //重新传输
			jpeg_data_ok=0;					//标记数据未采集
			jpeg_data_len=0;				//数据重新开始
		}
}

//jpeg数据接收回调函数
void jpeg_dcmi_rx_callback(void)
{ 
	u16 i;
	u32 *pbuf;
	pbuf=jpeg_data_buf+jpeg_data_len;//偏移到有效数据末尾
	if(DMA2_Stream1->CR&(1<<19))//buf0已满,正常处理buf1
	{ 
		for(i=0;i<jpeg_dma_bufsize;i++)pbuf[i]=jpeg_buf0[i];//读取buf0里面的数据
		jpeg_data_len+=jpeg_dma_bufsize;//偏移
	}else //buf1已满,正常处理buf0
	{
		for(i=0;i<jpeg_dma_bufsize;i++)pbuf[i]=jpeg_buf1[i];//读取buf1里面的数据
		jpeg_data_len+=jpeg_dma_bufsize;//偏移 
	} 	
}


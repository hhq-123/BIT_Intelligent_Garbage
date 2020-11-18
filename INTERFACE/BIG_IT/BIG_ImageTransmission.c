//Image transmission of big trash can
#include "BIG_ImageTransmission.h"

volatile u32 jpeg_data_len=0; 			//buf�е�JPEG��Ч���ݳ���(*4�ֽ�)
volatile u8 jpeg_data_ok=0;				//JPEG���ݲɼ���ɱ�־ 
										//0,����û�вɼ���;
										//1,���ݲɼ�����,���ǻ�û����;
										//2,�����Ѿ����������,���Կ�ʼ��һ֡����
										
//u32 *jpeg_buf0;							//JPEG���ݻ���buf,ͨ��malloc�����ڴ�
//u32 *jpeg_buf1;							//JPEG���ݻ���buf,ͨ��malloc�����ڴ�
//u32 *jpeg_data_buf;						//JPEG���ݻ���buf,ͨ��malloc�����ڴ�
__align(4) u32 jpeg_buf0[jpeg_dma_bufsize];							//JPEG���ݻ���buf0
__align(4) u32 jpeg_buf1[jpeg_dma_bufsize];							//JPEG���ݻ���buf1

u32 *jpeg_data_buf=(u32 *)0x68000000;/*��ͼ��Ļ��涨�嵽�ⲿSRAM ̽���������� 1MB*/

void jpeg_data_process(void)
{
	u16 i;
	u16 rlen;//ʣ�����ݳ���
	u32 *pbuf;

		if(jpeg_data_ok==0)	//jpeg���ݻ�δ�ɼ���?
		{
			DMA_Cmd(DMA2_Stream1,DISABLE);		//ֹͣ��ǰ����
			while(DMA_GetCmdStatus(DMA2_Stream1) != DISABLE);	//�ȴ�DMA2_Stream1������ 
			rlen=jpeg_dma_bufsize-DMA_GetCurrDataCounter(DMA2_Stream1);//�õ�ʣ�����ݳ���	
			pbuf=jpeg_data_buf+jpeg_data_len;//ƫ�Ƶ���Ч����ĩβ,�������
			if(DMA2_Stream1->CR&(1<<19))
				for(i=0;i<rlen;i++)
					pbuf[i]=jpeg_buf1[i];//��ȡbuf1�����ʣ������
			else 
				for(i=0;i<rlen;i++)
					pbuf[i]=jpeg_buf0[i];//��ȡbuf0�����ʣ������ 
			jpeg_data_len+=rlen;			//����ʣ�೤��
			jpeg_data_ok=1; 				//���JPEG���ݲɼ��갴��,�ȴ�������������
		}
		if(jpeg_data_ok==2)	//��һ�ε�jpeg�����Ѿ���������
		{ DMA_SetCurrDataCounter(DMA2_Stream1,jpeg_dma_bufsize);//���䳤��Ϊjpeg_buf_size*4�ֽ�
			DMA_Cmd(DMA2_Stream1,ENABLE); //���´���
			jpeg_data_ok=0;					//�������δ�ɼ�
			jpeg_data_len=0;				//�������¿�ʼ
		}
}

//jpeg���ݽ��ջص�����
void jpeg_dcmi_rx_callback(void)
{ 
	u16 i;
	u32 *pbuf;
	pbuf=jpeg_data_buf+jpeg_data_len;//ƫ�Ƶ���Ч����ĩβ
	if(DMA2_Stream1->CR&(1<<19))//buf0����,��������buf1
	{ 
		for(i=0;i<jpeg_dma_bufsize;i++)pbuf[i]=jpeg_buf0[i];//��ȡbuf0���������
		jpeg_data_len+=jpeg_dma_bufsize;//ƫ��
	}else //buf1����,��������buf0
	{
		for(i=0;i<jpeg_dma_bufsize;i++)pbuf[i]=jpeg_buf1[i];//��ȡbuf1���������
		jpeg_data_len+=jpeg_dma_bufsize;//ƫ�� 
	} 	
}


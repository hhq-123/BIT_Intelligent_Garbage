//Image transmission of big trash can
#ifndef __BIG_IT_H
#define __BIG_IT_H
#include "sys.h"
#include "dcmi.h"
#include "ov2640.h"

#define jpeg_dma_bufsize	1024		//����JPEG DMA����ʱ���ݻ���jpeg_buf0/1�Ĵ�С(*4�ֽ�)

void jpeg_data_process(void);
void jpeg_dcmi_rx_callback(void);


#endif

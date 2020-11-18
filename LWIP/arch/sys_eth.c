#include "netif/etharp.h"
#include "lwip/tcpip.h"
#include "lwip/netif.h"
#include "lwip/dhcp.h"
#include "lwip/err.h"
#include "sys_eth.h"
#include "stm32f4x7_eth.h"
#include "FreeRTOS.h"
#include "task.h"
 
// LAN8720 PHY芯片地址，PHYAD0上拉为0x01，下拉为0x00
#define LAN8720_PHY_ADDRESS   (unsigned char)(0x01U)
struct netif eth0; // 有线网络接口
static sLwipDev_t sLwipDev; // LwIP控制结构体
static unsigned char *pRxData; // 以太网底层驱动接收数据指针
static unsigned char *pTxData; // 以太网底层驱动发送数据指针
static ETH_DMADESCTypeDef *DMARxDscrTab; // 以太网DMA接收描述符数据结构体指针
static ETH_DMADESCTypeDef *DMATxDscrTab; // 以太网DMA发送描述符数据结构体指针
extern ETH_DMADESCTypeDef *DMATxDescToSet; // DMA发送描述符追踪指针
extern ETH_DMADESCTypeDef *DMARxDescToGet; // DMA接收描述符追踪指针
 
/********************************************************
 * 函数功能：初始化ETH MAC层及DMA配置
 * 形    参：无
 * 返 回 值：0=成功，1=失败
 ********************************************************/
static unsigned char eth_mac_dma_config(void)
{
  ETH_InitTypeDef ETH_InitStructure;
   // 使能以太网MAC以及MAC接收和发送时钟
   RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_ETH_MAC | RCC_AHB1Periph_ETH_MAC_Tx | RCC_AHB1Periph_ETH_MAC_Rx, ENABLE);
   ETH_DeInit(); // AHB总线重启以太网
   ETH_SoftwareReset(); // 软件重启网络
   while (ETH_GetSoftwareResetStatus() == SET); // 等待软件重启网络完成
   ETH_StructInit(&ETH_InitStructure); // 初始化网络为默认值
   // 网络MAC参数设置
   ETH_InitStructure.ETH_AutoNegotiation = ETH_AutoNegotiation_Enable;      // 开启网络自适应功能
   ETH_InitStructure.ETH_LoopbackMode = ETH_LoopbackMode_Disable;     // 关闭反馈
   ETH_InitStructure.ETH_RetryTransmission = ETH_RetryTransmission_Disable;   // 关闭重传功能
   ETH_InitStructure.ETH_AutomaticPadCRCStrip = ETH_AutomaticPadCRCStrip_Disable;  // 关闭自动去除PDA/CRC功能
   ETH_InitStructure.ETH_ReceiveAll = ETH_ReceiveAll_Disable;      // 关闭接收所有的帧
   ETH_InitStructure.ETH_BroadcastFramesReception = ETH_BroadcastFramesReception_Enable; // 允许接收所有广播帧
   ETH_InitStructure.ETH_PromiscuousMode = ETH_PromiscuousMode_Disable;   // 关闭混合模式的地址过滤
   ETH_InitStructure.ETH_MulticastFramesFilter = ETH_MulticastFramesFilter_Perfect;// 对于组播地址使用完美地址过滤
   ETH_InitStructure.ETH_UnicastFramesFilter = ETH_UnicastFramesFilter_Perfect; // 对单播地址使用完美地址过滤
#ifdef CHECKSUM_BY_HARDWARE
   ETH_InitStructure.ETH_ChecksumOffload = ETH_ChecksumOffload_Enable;    // 开启ipv4和TCP/UDP/ICMP的帧校验和卸载
#endif
   // 当我们使用帧校验和卸载功能的时候，一定要使能存储转发模式,存储转发模式中要保证整个帧存储在FIFO中,
   // 这样MAC能插入/识别出帧校验值,当真校验正确的时候DMA就可以处理帧,否则就丢弃掉该帧
   ETH_InitStructure.ETH_DropTCPIPChecksumErrorFrame = ETH_DropTCPIPChecksumErrorFrame_Enable; // 开启丢弃TCP/IP错误帧
   ETH_InitStructure.ETH_ReceiveStoreForward = ETH_ReceiveStoreForward_Enable;     // 开启接收数据的存储转发模式
   ETH_InitStructure.ETH_TransmitStoreForward = ETH_TransmitStoreForward_Enable;   // 开启发送数据的存储转发模式
   ETH_InitStructure.ETH_ForwardErrorFrames = ETH_ForwardErrorFrames_Disable;      // 禁止转发错误帧
   ETH_InitStructure.ETH_ForwardUndersizedGoodFrames = ETH_ForwardUndersizedGoodFrames_Disable; // 不转发过小的好帧
   ETH_InitStructure.ETH_SecondFrameOperate = ETH_SecondFrameOperate_Enable;    // 打开处理第二帧功能
   ETH_InitStructure.ETH_AddressAlignedBeats = ETH_AddressAlignedBeats_Enable;   // 开启DMA传输的地址对齐功能
   ETH_InitStructure.ETH_FixedBurst = ETH_FixedBurst_Enable;               // 开启固定突发功能
   ETH_InitStructure.ETH_RxDMABurstLength = ETH_RxDMABurstLength_32Beat;       // DMA发送的最大突发长度为32个节拍
   ETH_InitStructure.ETH_TxDMABurstLength = ETH_TxDMABurstLength_32Beat;   // DMA接收的最大突发长度为32个节拍
   ETH_InitStructure.ETH_DMAArbitration = ETH_DMAArbitration_RoundRobin_RxTx_2_1;
   if(ETH_Init(&ETH_InitStructure, LAN8720_PHY_ADDRESS) == ETH_SUCCESS) // 配置ETH成功
   {
      ETH_DMAITConfig(ETH_DMA_IT_NIS | ETH_DMA_IT_R, ENABLE); // 使能以太网接收中断
      return 0;
   }
   return 1;
}
 
/********************************************************
 * 函数功能：释放ETH底层驱动申请的内存
 * 形    参：无
 * 返 回 值：无
 ********************************************************/
static void eth_mem_free(void)
{
   vPortFree(DMARxDscrTab);
   vPortFree(DMATxDscrTab);
   vPortFree(pRxData);
   vPortFree(pTxData);
}
 
/********************************************************
 * 函数功能：为ETH底层驱动申请内存
 * 形    参：无
 * 返 回 值：0=成功，其它=失败
 ********************************************************/
static unsigned char eth_mem_malloc(void)
{
   eth_mem_free(); // 此处释放是避免重复初始化时将内存消耗完
   DMARxDscrTab = pvPortMalloc(ETH_RXBUFNB * sizeof(ETH_DMADESCTypeDef)); // 申请内存
   DMATxDscrTab = pvPortMalloc(ETH_TXBUFNB * sizeof(ETH_DMADESCTypeDef)); // 申请内存
   pRxData = pvPortMalloc(ETH_RX_BUF_SIZE * ETH_RXBUFNB); // 申请内存
   pTxData = pvPortMalloc(ETH_TX_BUF_SIZE * ETH_TXBUFNB); // 申请内存
   if(!DMARxDscrTab)
   {
      vPortFree(DMARxDscrTab);
      return 1; // 内存申请失败
   }
   if(!DMATxDscrTab)
   {
      vPortFree(DMATxDscrTab);
      return 2; // 内存申请失败
   }
   if(!pRxData)
   {
      vPortFree(pRxData);
      return 3; // 内存申请失败
   }
   if(!pTxData)
   {
      vPortFree(pTxData);
      return 4; // 内存申请失败
   }
   return 0; // 内存申请成功
}
 
/********************************************************
 * 函数功能：以太网初始化
 * 形    参：无
 * 返 回 值：0=成功，其它=失败
 ********************************************************/
static unsigned int eht_init(void)
{
   GPIO_InitTypeDef GPIO_InitStructure;
   // 使能GPIO时钟 RMII接口
   RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOB | RCC_AHB1Periph_GPIOC, ENABLE);
   RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE); // 使能SYSCFG时钟
   // MAC和PHY之间使用RMII接口
   SYSCFG_ETH_MediaInterfaceConfig(SYSCFG_ETH_MediaInterface_RMII);
// STM32F407VGTx 网络引脚设置 RMII接口
// ETH_MDIO -------------------------> PA2
// ETH_MDC --------------------------> PC1
// ETH_RMII_REF_CLK------------------> PA1
// ETH_RMII_CRS_DV ------------------> PA7
// ETH_RMII_RXD0 --------------------> PC4
// ETH_RMII_RXD1 --------------------> PC5
// ETH_RMII_TX_EN -------------------> PG11
// ETH_RMII_TXD0 --------------------> PG13
// ETH_RMII_TXD1 --------------------> PG14
// ETH_RESET-------------------------> PD3
// STM32F407VETx 网络引脚设置 RMII接口
// ETH_RMII_REF_CLK------------------> PA1
// ETH_MDIO -------------------------> PA2
// ETH_RMII_CRS_DV ------------------> PA7
// ETH_RMII_TX_EN -------------------> PB11
// ETH_RMII_TXD0 --------------------> PB12
// ETH_RMII_TXD1 --------------------> PB13
// ETH_RESET-------------------------> PC0
// ETH_MDC --------------------------> PC1
// ETH_RMII_RXD0 --------------------> PC4
// ETH_RMII_RXD1 --------------------> PC5
   // 引脚复用到网络接口上
   GPIO_PinAFConfig(GPIOA, GPIO_PinSource1, GPIO_AF_ETH);
   GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_ETH);
   GPIO_PinAFConfig(GPIOA, GPIO_PinSource7, GPIO_AF_ETH);
   GPIO_PinAFConfig(GPIOB, GPIO_PinSource11, GPIO_AF_ETH);
   GPIO_PinAFConfig(GPIOB, GPIO_PinSource12, GPIO_AF_ETH);
   GPIO_PinAFConfig(GPIOB, GPIO_PinSource13, GPIO_AF_ETH);
   GPIO_PinAFConfig(GPIOC, GPIO_PinSource1, GPIO_AF_ETH);
   GPIO_PinAFConfig(GPIOC, GPIO_PinSource4, GPIO_AF_ETH);
   GPIO_PinAFConfig(GPIOC, GPIO_PinSource5, GPIO_AF_ETH);
   // 配置 PA1 PA2 PA7
   GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_7;
   GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
   GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
   GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; // 推挽输出
   GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
   GPIO_Init(GPIOA, &GPIO_InitStructure);
   // 配置 PB11 PB12 PB13
   GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13;
   GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
   GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
   GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; // 推挽输出
   GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
   GPIO_Init(GPIOB, &GPIO_InitStructure);
   // 配置 PC1 PC4 PC5
   GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_4 | GPIO_Pin_5;
   GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
   GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
   GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; // 推挽输出
   GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
   GPIO_Init(GPIOC, &GPIO_InitStructure);
   // RESET IO
   GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
   GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
   GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
   GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
   GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
   GPIO_Init(GPIOC, &GPIO_InitStructure);
 
   // POWER IO
   GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
   GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
   GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
   GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
   GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
   GPIO_Init(GPIOA, &GPIO_InitStructure);
 
   // LAN8720 硬件复位
   GPIO_SetBits(GPIOA, GPIO_Pin_6); // 关闭电源
   vTaskDelay(100);
   GPIO_ResetBits(GPIOA, GPIO_Pin_6); // 打开电源
   vTaskDelay(100);
   GPIO_ResetBits(GPIOC, GPIO_Pin_0); // 硬件复位
   vTaskDelay(100);
   GPIO_SetBits(GPIOC, GPIO_Pin_0); // 复位结束
   vTaskDelay(100);
 
   // 设置中断优先级
   NVIC_InitTypeDef NVIC_InitStructure;
   NVIC_InitStructure.NVIC_IRQChannel = ETH_IRQn; // 以太网中断
   NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 7; // 中断寄存器组2最高优先级
   NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
   NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
   NVIC_Init(&NVIC_InitStructure);
 
   // 给ETH申请内存
   if(eth_mem_malloc())
   {
      return 1; // 内存申请失败
   }
 
   // 配置ETH的MAC及DMA
   if(eth_mac_dma_config() != 0)
   {
      return 2; // ETH配置失败
   }
 
   // 从LAN8720的31号寄存器中读取网络速度和双工模式
   unsigned char speed = (unsigned char)ETH_ReadPHYRegister(LAN8720_PHY_ADDRESS, 31);
   speed = (unsigned char)((speed & 0x1C) >> 2);
 
   // 1:10M半双工; 5:10M全双工; 2:100M半双工; 6:100M全双工
   if(speed != 0x1 && speed != 0x2 && speed != 0x5 && speed != 0x6)
   {
      return 3; // LAN8720网卡初始化失败
   }
 
   return 0;
}
 
/********************************************************
 * 函数功能：接收一个以太网数据包
 * 形    参：无
 * 返 回 值：网络数据包帧结构体
 ********************************************************/
static FrameTypeDef eth_rx_packet(void)
{
   unsigned int framelength = 0;
   FrameTypeDef frame = {0, 0};
 
   // 检查当前描述符,是否属于ETHERNET DMA(设置的时候)/CPU(复位的时候)
   if((DMARxDescToGet->Status & ETH_DMARxDesc_OWN) != (u32)RESET)
   {
      frame.length = ETH_ERROR;
      if ((ETH->DMASR & ETH_DMASR_RBUS) != (u32)RESET)
      {
         ETH->DMASR = ETH_DMASR_RBUS; // 清除ETH DMA的RBUS位
         ETH->DMARPDR = 0; // 恢复DMA接收
      }
      return frame; // 错误,OWN位被设置了
   }
 
   if((DMARxDescToGet->Status & ETH_DMARxDesc_ES) == (u32)RESET && (DMARxDescToGet->Status & ETH_DMARxDesc_LS) != (u32)RESET && (DMARxDescToGet->Status & ETH_DMARxDesc_FS) != (u32)RESET)
   {
      framelength = ((DMARxDescToGet->Status & ETH_DMARxDesc_FL) >> ETH_DMARxDesc_FrameLengthShift) - 4; // 得到接收包帧长度(不包含4字节CRC)
       frame.buffer = DMARxDescToGet->Buffer1Addr; // 得到包数据所在的位置
   }
   else
   {
      framelength = ETH_ERROR; // 错误
   }
 
   frame.length = framelength;
   frame.descriptor = DMARxDescToGet;
 
   // 更新ETH DMA全局Rx描述符为下一个Rx描述符，为下一次buffer读取设置下一个DMA Rx描述符
   DMARxDescToGet = (ETH_DMADESCTypeDef *)(DMARxDescToGet->Buffer2NextDescAddr);
   return frame;
}
 
/********************************************************
 * 函数功能：发送一个以太网数据包
 * 形    参：frame_length：数据包长度
 * 返 回 值：0=成功，1=失败
 ********************************************************/
static unsigned int eth_tx_packet(unsigned short frame_length)
{
   //检查当前描述符,是否属于ETHERNET DMA(设置的时候)/CPU(复位的时候)
   if((DMATxDescToSet->Status & ETH_DMATxDesc_OWN) != (u32)RESET)
   {
      return 1; // 错误,OWN位被设置了
   }
 
  DMATxDescToSet->ControlBufferSize = (frame_length & ETH_DMATxDesc_TBS1); // 设置帧长度,bits[12:0]
  DMATxDescToSet->Status |= ETH_DMATxDesc_LS | ETH_DMATxDesc_FS; // 设置最后一个和第一个位段置位(1个描述符传输一帧)
  DMATxDescToSet->Status |= ETH_DMATxDesc_OWN; // 设置Tx描述符的OWN位,buffer重归ETH DMA
   if((ETH->DMASR & ETH_DMASR_TBUS) != (u32)RESET) // 当Tx Buffer不可用位(TBUS)被设置的时候,重置它.恢复传输
   {
      ETH->DMASR = ETH_DMASR_TBUS; // 重置ETH DMA TBUS位
      ETH->DMATPDR = 0; // 恢复DMA发送
   }
 
   // 更新ETH DMA全局Tx描述符为下一个Tx描述符
   // 为下一次buffer发送设置下一个DMA Tx描述符
   DMATxDescToSet = (ETH_DMADESCTypeDef *)(DMATxDescToSet->Buffer2NextDescAddr);
   return 0;
}
 
/********************************************************
 * 函数功能：以太网数据包发送的底层函数(ARP发送)
 * 形    参：netif：网卡结构体指针
             p：pbuf数据结构体指针
 * 返 回 值：ERR_OK=发送正常；ERR_MEM=内存异常
 ********************************************************/
static err_t arp_output(struct netif *netif, struct pbuf *p)
{
   unsigned int length = 0;
   unsigned char *pdata = (unsigned char *)DMATxDescToSet->Buffer1Addr;
   for(struct pbuf *q = p; q != NULL; q = q->next)
   {
      unsigned char *pload = (unsigned char *)q->payload;
      for(unsigned int i = 0; i < q->len; i++)
      {
         pdata[length++] = pload[i];
      }
   }
 
   if(eth_tx_packet(length) == ETH_ERROR)
   {
      return ERR_MEM; // 返回错误状态
   }
 
   return ERR_OK;
}
 
/********************************************************
 * 函数功能：以太网接收数据(以太网中断函数调用)
 * 形    参：netif：网卡结构体指针
 * 返 回 值：ERR_OK=发送正常；ERR_MEM=内存异常
 ********************************************************/
static err_t ethernetif_input(struct netif *netif)
{
   err_t err = ERR_OK;
   unsigned int length = 0;
   FrameTypeDef frame = eth_rx_packet();
   unsigned char *pdata=(unsigned char *)frame.buffer;
   struct pbuf *p = pbuf_alloc(PBUF_RAW, frame.length, PBUF_POOL); // 从pbufs内存池中给pbuf分配内存
   if(p != NULL)
   {
      for(struct pbuf *q = p; q != NULL; q = q->next)
      {
         unsigned char *pload = (unsigned char *)q->payload;
         for(unsigned int i = 0; i < q->len; i++)
         {
            pload[i] = pdata[length++];
         }
      }
      // 将数据发送给协议栈
      if(netif->input(p, netif) != ERR_OK)
      {
         pbuf_free(p);
         p = NULL;
      }
   }
   else
   {
      err = ERR_MEM; // 内存异常
   }
 
   // 设置Rx描述符OWN位，数据进入ETH DMA
   frame.descriptor->Status = ETH_DMARxDesc_OWN;
   if((ETH->DMASR&ETH_DMASR_RBUS) != RESET)
   {
      ETH->DMASR=ETH_DMASR_RBUS; // 重置ETH DMA RBUS位
      ETH->DMARPDR = 0; // 恢复DMA接收
   }
 
   return err;
}
 
/********************************************************
 * 函数功能：以太网初始化（协议栈内核调用）
 * 形    参：netif：网卡结构体指针
 * 返 回 值：ERR_OK=发送正常；ERR_BUF=数据指针异常
 ********************************************************/
static err_t ethernetif_init(struct netif *netif)
{
   if(netif == NULL)
   {
      return ERR_BUF;
   }
 
#if LWIP_NETIF_HOSTNAME // LWIP_NETIF_HOSTNAME
   netif->hostname = "lwip"; // 初始化名称
#endif
   netif->name[0] = 'e'; // 初始化变量netif的name字段
   netif->name[1] = '0'; // 在文件外定义这里不用关心具体值
   netif->output = etharp_output; // IP层数据包发送函数
   netif->linkoutput = arp_output; // ARP模块发送数据包函数
   netif->hwaddr_len = ETHARP_HWADDR_LEN; // 设置MAC地址长度,为6个字节
          
  // 初始化MAC地址,
   // 注：MAC地址不能与网络中其他设备的MAC地址相同
   // 获取STM32的唯一ID的前32位作为MAC地址后四个字节
   unsigned int id = *(unsigned int *)(0x1FFF7A10);
   netif->hwaddr[0] = 2;
   netif->hwaddr[1] = 0;
   netif->hwaddr[2] = (id >> 24) & 0xFF;
   netif->hwaddr[3] = (id >> 16) & 0xFF;
   netif->hwaddr[4] = (id >> 8) & 0xFF;
   netif->hwaddr[5] = (id >> 0) & 0xFF;
   netif->mtu = 1500; // 最大允许传输单元
 
   // 允许该网卡广播和ARP功能，并且该网卡允许有硬件链路连接
   netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;
 
   // 向STM32F4的MAC地址寄存器中写入MAC地址
   ETH_MACAddressConfig(ETH_MAC_Address0, netif->hwaddr);
 
   // 以太网DMA传输配置
   ETH_DMATxDescChainInit(DMATxDscrTab, pTxData, ETH_TXBUFNB);
   ETH_DMARxDescChainInit(DMARxDscrTab, pRxData, ETH_RXBUFNB);
 
#ifdef CHECKSUM_BY_HARDWARE // 使用硬件帧校验
   // 使能TCP,UDP和ICMP的发送帧校验
   // TCP,UDP和ICMP的接收帧校验在DMA中配置了
   for(unsigned int i = 0; i < ETH_TXBUFNB; i++)
   {
      ETH_DMATxDescChecksumInsertionConfig(&DMATxDscrTab[i], ETH_DMATxDesc_ChecksumTCPUDPICMPFull);
   }
#endif
 
   ETH_Start(); // 开启MAC和DMA    
   return ERR_OK;
}
 
/********************************************************
 * 函数功能：以太网DMA接收中断服务函数
 * 形    参：无
 * 返 回 值：无
 ********************************************************/
void ETH_IRQHandler(void)
{
   // 首先判断是否收到数据包
   while(ETH_GetRxPktSize(DMARxDescToGet) != 0)
   {
      ethernetif_input(&eth0);
   }
   ETH_DMAClearITPendingBit(ETH_DMA_IT_R); // 清除DMA中断标志位
   ETH_DMAClearITPendingBit(ETH_DMA_IT_NIS); // 清除DMA接收中断标志位
}
 
#if LWIP_DHCP // 如果使能了DHCP
/********************************************************
 * 函数功能：DHCP处理任务
 * 形    参：psLwipDev：IP信息数据结构之指针
 * 返 回 值：无
 ********************************************************/
static void dhcp_task(sLwipDev_t * const psLwipDev)
{
   dhcp_start(&eth0); // 开启DHCP
   // 通过DHCP服务获取IP地址，如果失败且超过最大尝试次数就退出
   while(eth0.dhcp->tries < 5U && psLwipDev != NULL)
   {
      if(eth0.ip_addr.addr != 0)
      {
         // 解析出通过DHCP获取到的网关地址
         psLwipDev->gateway[3] =(uint8_t)(eth0.gw.addr >> 24);
         psLwipDev->gateway[2] =(uint8_t)(eth0.gw.addr >> 16);
         psLwipDev->gateway[1] =(uint8_t)(eth0.gw.addr >> 8);
         psLwipDev->gateway[0] =(uint8_t)(eth0.gw.addr);
   
         // 解析通过DHCP获取到的子网掩码地址
         psLwipDev->mask[3] = (uint8_t)(eth0.netmask.addr >> 24);
         psLwipDev->mask[2] = (uint8_t)(eth0.netmask.addr >> 16);
         psLwipDev->mask[1] = (uint8_t)(eth0.netmask.addr >> 8);
         psLwipDev->mask[0] = (uint8_t)(eth0.netmask.addr);
   
         // 解析出通过DHCP获取到的IP地址
         psLwipDev->localip[3] = (uint8_t)(eth0.ip_addr.addr >> 24);
         psLwipDev->localip[2] = (uint8_t)(eth0.ip_addr.addr >> 16);
         psLwipDev->localip[1] = (uint8_t)(eth0.ip_addr.addr >> 8);
         psLwipDev->localip[0] = (uint8_t)(eth0.ip_addr.addr);
         break;
      }
      vTaskDelay(50);
   }
   dhcp_stop(&eth0); // 关闭DHCP
}
#endif
 
/********************************************************
 * 函数功能：获取默认IP信息
 * 形    参：psLwipDev：IP信息数据结构指针
 * 返 回 值：0=成功，1=数据指针为NULL
 ********************************************************/
unsigned int eth_default_ip_get(sLwipDev_t * const psLwipDev)
{
   if(psLwipDev == NULL)
   {
      return 1;
   }
 
   psLwipDev->gateway[0] = 192;
   psLwipDev->gateway[1] = 168;
   psLwipDev->gateway[2] = 1;
   psLwipDev->gateway[3] = 1;
 
   psLwipDev->mask[0] = 255;
   psLwipDev->mask[1] = 255;
   psLwipDev->mask[2] = 255;
   psLwipDev->mask[3] = 0;
 
   psLwipDev->localip[0] = 192;
   psLwipDev->localip[1] = 168;
   psLwipDev->localip[2] = 0;
   psLwipDev->localip[3] = 10;
 
   psLwipDev->remoteip[0] = 169;
   psLwipDev->remoteip[1] = 254;
   psLwipDev->remoteip[2] = 174;
   psLwipDev->remoteip[3] = 92;
 
   psLwipDev->localport = 80;
   psLwipDev->remoteport = 80;
   return 0;
}
 
/********************************************************
 * 函数功能：以太网、协议栈内核等初始化
 * 形    参：psLwipDev：IP信息数据结构指针
 * 返 回 值：0=初始化成功
             1=数据指针为NULL
             2=以太网初始化失败
             3=网卡注册失败
 ********************************************************/
unsigned int eth_init(sLwipDev_t * const psLwipDev)
{
   if(psLwipDev == NULL)
   {
      return 1;
   }
 
   struct ip_addr mask; // 子网掩码
   struct ip_addr gateway; // 默认网关
   struct ip_addr localip; // 本地IP地址
   // 以太网初始化
   if(eht_init() != 0)
   {
      return 2;
   }
 
   // TCPIP内核初始化
   tcpip_init(NULL, NULL);
 
#if LWIP_DHCP // 如果使能了DHCP
   dhcp_task(psLwipDev);
#else
   // 设置默认IP地址信息
   IP4_ADDR(&mask, psLwipDev->mask[0], psLwipDev->mask[1], psLwipDev->mask[2], psLwipDev->mask[3]);
   IP4_ADDR(&gateway, psLwipDev->gateway[0], psLwipDev->gateway[1], psLwipDev->gateway[2], psLwipDev->gateway[3]);
   IP4_ADDR(&localip, psLwipDev->localip[0], psLwipDev->localip[1], psLwipDev->localip[2], psLwipDev->localip[3]);
#endif
 
   // 向网卡列表中注册一个网口
   if(netif_add(&eth0, &localip, &mask, &gateway, NULL, &ethernetif_init, &tcpip_input) == NULL)
   {
      return 3; // 网卡注册失败
   }
 
   // 网口添加成功后,设置netif为默认值,并且打开netif网口
   netif_set_default(&eth0); // 设置netif为默认网口
   netif_set_up(&eth0); // 打开netif网口
   return 0;
}

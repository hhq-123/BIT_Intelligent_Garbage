#include "netif/etharp.h"
#include "lwip/tcpip.h"
#include "lwip/netif.h"
#include "lwip/dhcp.h"
#include "lwip/err.h"
#include "sys_eth.h"
#include "stm32f4x7_eth.h"
#include "FreeRTOS.h"
#include "task.h"
 
// LAN8720 PHYоƬ��ַ��PHYAD0����Ϊ0x01������Ϊ0x00
#define LAN8720_PHY_ADDRESS   (unsigned char)(0x01U)
struct netif eth0; // ��������ӿ�
static sLwipDev_t sLwipDev; // LwIP���ƽṹ��
static unsigned char *pRxData; // ��̫���ײ�������������ָ��
static unsigned char *pTxData; // ��̫���ײ�������������ָ��
static ETH_DMADESCTypeDef *DMARxDscrTab; // ��̫��DMA�������������ݽṹ��ָ��
static ETH_DMADESCTypeDef *DMATxDscrTab; // ��̫��DMA�������������ݽṹ��ָ��
extern ETH_DMADESCTypeDef *DMATxDescToSet; // DMA����������׷��ָ��
extern ETH_DMADESCTypeDef *DMARxDescToGet; // DMA����������׷��ָ��
 
/********************************************************
 * �������ܣ���ʼ��ETH MAC�㼰DMA����
 * ��    �Σ���
 * �� �� ֵ��0=�ɹ���1=ʧ��
 ********************************************************/
static unsigned char eth_mac_dma_config(void)
{
  ETH_InitTypeDef ETH_InitStructure;
   // ʹ����̫��MAC�Լ�MAC���պͷ���ʱ��
   RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_ETH_MAC | RCC_AHB1Periph_ETH_MAC_Tx | RCC_AHB1Periph_ETH_MAC_Rx, ENABLE);
   ETH_DeInit(); // AHB����������̫��
   ETH_SoftwareReset(); // �����������
   while (ETH_GetSoftwareResetStatus() == SET); // �ȴ���������������
   ETH_StructInit(&ETH_InitStructure); // ��ʼ������ΪĬ��ֵ
   // ����MAC��������
   ETH_InitStructure.ETH_AutoNegotiation = ETH_AutoNegotiation_Enable;      // ������������Ӧ����
   ETH_InitStructure.ETH_LoopbackMode = ETH_LoopbackMode_Disable;     // �رշ���
   ETH_InitStructure.ETH_RetryTransmission = ETH_RetryTransmission_Disable;   // �ر��ش�����
   ETH_InitStructure.ETH_AutomaticPadCRCStrip = ETH_AutomaticPadCRCStrip_Disable;  // �ر��Զ�ȥ��PDA/CRC����
   ETH_InitStructure.ETH_ReceiveAll = ETH_ReceiveAll_Disable;      // �رս������е�֡
   ETH_InitStructure.ETH_BroadcastFramesReception = ETH_BroadcastFramesReception_Enable; // ����������й㲥֡
   ETH_InitStructure.ETH_PromiscuousMode = ETH_PromiscuousMode_Disable;   // �رջ��ģʽ�ĵ�ַ����
   ETH_InitStructure.ETH_MulticastFramesFilter = ETH_MulticastFramesFilter_Perfect;// �����鲥��ַʹ��������ַ����
   ETH_InitStructure.ETH_UnicastFramesFilter = ETH_UnicastFramesFilter_Perfect; // �Ե�����ַʹ��������ַ����
#ifdef CHECKSUM_BY_HARDWARE
   ETH_InitStructure.ETH_ChecksumOffload = ETH_ChecksumOffload_Enable;    // ����ipv4��TCP/UDP/ICMP��֡У���ж��
#endif
   // ������ʹ��֡У���ж�ع��ܵ�ʱ��һ��Ҫʹ�ܴ洢ת��ģʽ,�洢ת��ģʽ��Ҫ��֤����֡�洢��FIFO��,
   // ����MAC�ܲ���/ʶ���֡У��ֵ,����У����ȷ��ʱ��DMA�Ϳ��Դ���֡,����Ͷ�������֡
   ETH_InitStructure.ETH_DropTCPIPChecksumErrorFrame = ETH_DropTCPIPChecksumErrorFrame_Enable; // ��������TCP/IP����֡
   ETH_InitStructure.ETH_ReceiveStoreForward = ETH_ReceiveStoreForward_Enable;     // �����������ݵĴ洢ת��ģʽ
   ETH_InitStructure.ETH_TransmitStoreForward = ETH_TransmitStoreForward_Enable;   // �����������ݵĴ洢ת��ģʽ
   ETH_InitStructure.ETH_ForwardErrorFrames = ETH_ForwardErrorFrames_Disable;      // ��ֹת������֡
   ETH_InitStructure.ETH_ForwardUndersizedGoodFrames = ETH_ForwardUndersizedGoodFrames_Disable; // ��ת����С�ĺ�֡
   ETH_InitStructure.ETH_SecondFrameOperate = ETH_SecondFrameOperate_Enable;    // �򿪴���ڶ�֡����
   ETH_InitStructure.ETH_AddressAlignedBeats = ETH_AddressAlignedBeats_Enable;   // ����DMA����ĵ�ַ���빦��
   ETH_InitStructure.ETH_FixedBurst = ETH_FixedBurst_Enable;               // �����̶�ͻ������
   ETH_InitStructure.ETH_RxDMABurstLength = ETH_RxDMABurstLength_32Beat;       // DMA���͵����ͻ������Ϊ32������
   ETH_InitStructure.ETH_TxDMABurstLength = ETH_TxDMABurstLength_32Beat;   // DMA���յ����ͻ������Ϊ32������
   ETH_InitStructure.ETH_DMAArbitration = ETH_DMAArbitration_RoundRobin_RxTx_2_1;
   if(ETH_Init(&ETH_InitStructure, LAN8720_PHY_ADDRESS) == ETH_SUCCESS) // ����ETH�ɹ�
   {
      ETH_DMAITConfig(ETH_DMA_IT_NIS | ETH_DMA_IT_R, ENABLE); // ʹ����̫�������ж�
      return 0;
   }
   return 1;
}
 
/********************************************************
 * �������ܣ��ͷ�ETH�ײ�����������ڴ�
 * ��    �Σ���
 * �� �� ֵ����
 ********************************************************/
static void eth_mem_free(void)
{
   vPortFree(DMARxDscrTab);
   vPortFree(DMATxDscrTab);
   vPortFree(pRxData);
   vPortFree(pTxData);
}
 
/********************************************************
 * �������ܣ�ΪETH�ײ����������ڴ�
 * ��    �Σ���
 * �� �� ֵ��0=�ɹ�������=ʧ��
 ********************************************************/
static unsigned char eth_mem_malloc(void)
{
   eth_mem_free(); // �˴��ͷ��Ǳ����ظ���ʼ��ʱ���ڴ�������
   DMARxDscrTab = pvPortMalloc(ETH_RXBUFNB * sizeof(ETH_DMADESCTypeDef)); // �����ڴ�
   DMATxDscrTab = pvPortMalloc(ETH_TXBUFNB * sizeof(ETH_DMADESCTypeDef)); // �����ڴ�
   pRxData = pvPortMalloc(ETH_RX_BUF_SIZE * ETH_RXBUFNB); // �����ڴ�
   pTxData = pvPortMalloc(ETH_TX_BUF_SIZE * ETH_TXBUFNB); // �����ڴ�
   if(!DMARxDscrTab)
   {
      vPortFree(DMARxDscrTab);
      return 1; // �ڴ�����ʧ��
   }
   if(!DMATxDscrTab)
   {
      vPortFree(DMATxDscrTab);
      return 2; // �ڴ�����ʧ��
   }
   if(!pRxData)
   {
      vPortFree(pRxData);
      return 3; // �ڴ�����ʧ��
   }
   if(!pTxData)
   {
      vPortFree(pTxData);
      return 4; // �ڴ�����ʧ��
   }
   return 0; // �ڴ�����ɹ�
}
 
/********************************************************
 * �������ܣ���̫����ʼ��
 * ��    �Σ���
 * �� �� ֵ��0=�ɹ�������=ʧ��
 ********************************************************/
static unsigned int eht_init(void)
{
   GPIO_InitTypeDef GPIO_InitStructure;
   // ʹ��GPIOʱ�� RMII�ӿ�
   RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOB | RCC_AHB1Periph_GPIOC, ENABLE);
   RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE); // ʹ��SYSCFGʱ��
   // MAC��PHY֮��ʹ��RMII�ӿ�
   SYSCFG_ETH_MediaInterfaceConfig(SYSCFG_ETH_MediaInterface_RMII);
// STM32F407VGTx ������������ RMII�ӿ�
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
// STM32F407VETx ������������ RMII�ӿ�
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
   // ���Ÿ��õ�����ӿ���
   GPIO_PinAFConfig(GPIOA, GPIO_PinSource1, GPIO_AF_ETH);
   GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_ETH);
   GPIO_PinAFConfig(GPIOA, GPIO_PinSource7, GPIO_AF_ETH);
   GPIO_PinAFConfig(GPIOB, GPIO_PinSource11, GPIO_AF_ETH);
   GPIO_PinAFConfig(GPIOB, GPIO_PinSource12, GPIO_AF_ETH);
   GPIO_PinAFConfig(GPIOB, GPIO_PinSource13, GPIO_AF_ETH);
   GPIO_PinAFConfig(GPIOC, GPIO_PinSource1, GPIO_AF_ETH);
   GPIO_PinAFConfig(GPIOC, GPIO_PinSource4, GPIO_AF_ETH);
   GPIO_PinAFConfig(GPIOC, GPIO_PinSource5, GPIO_AF_ETH);
   // ���� PA1 PA2 PA7
   GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_7;
   GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
   GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
   GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; // �������
   GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
   GPIO_Init(GPIOA, &GPIO_InitStructure);
   // ���� PB11 PB12 PB13
   GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13;
   GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
   GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
   GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; // �������
   GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
   GPIO_Init(GPIOB, &GPIO_InitStructure);
   // ���� PC1 PC4 PC5
   GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_4 | GPIO_Pin_5;
   GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
   GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
   GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; // �������
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
 
   // LAN8720 Ӳ����λ
   GPIO_SetBits(GPIOA, GPIO_Pin_6); // �رյ�Դ
   vTaskDelay(100);
   GPIO_ResetBits(GPIOA, GPIO_Pin_6); // �򿪵�Դ
   vTaskDelay(100);
   GPIO_ResetBits(GPIOC, GPIO_Pin_0); // Ӳ����λ
   vTaskDelay(100);
   GPIO_SetBits(GPIOC, GPIO_Pin_0); // ��λ����
   vTaskDelay(100);
 
   // �����ж����ȼ�
   NVIC_InitTypeDef NVIC_InitStructure;
   NVIC_InitStructure.NVIC_IRQChannel = ETH_IRQn; // ��̫���ж�
   NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 7; // �жϼĴ�����2������ȼ�
   NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
   NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
   NVIC_Init(&NVIC_InitStructure);
 
   // ��ETH�����ڴ�
   if(eth_mem_malloc())
   {
      return 1; // �ڴ�����ʧ��
   }
 
   // ����ETH��MAC��DMA
   if(eth_mac_dma_config() != 0)
   {
      return 2; // ETH����ʧ��
   }
 
   // ��LAN8720��31�żĴ����ж�ȡ�����ٶȺ�˫��ģʽ
   unsigned char speed = (unsigned char)ETH_ReadPHYRegister(LAN8720_PHY_ADDRESS, 31);
   speed = (unsigned char)((speed & 0x1C) >> 2);
 
   // 1:10M��˫��; 5:10Mȫ˫��; 2:100M��˫��; 6:100Mȫ˫��
   if(speed != 0x1 && speed != 0x2 && speed != 0x5 && speed != 0x6)
   {
      return 3; // LAN8720������ʼ��ʧ��
   }
 
   return 0;
}
 
/********************************************************
 * �������ܣ�����һ����̫�����ݰ�
 * ��    �Σ���
 * �� �� ֵ���������ݰ�֡�ṹ��
 ********************************************************/
static FrameTypeDef eth_rx_packet(void)
{
   unsigned int framelength = 0;
   FrameTypeDef frame = {0, 0};
 
   // ��鵱ǰ������,�Ƿ�����ETHERNET DMA(���õ�ʱ��)/CPU(��λ��ʱ��)
   if((DMARxDescToGet->Status & ETH_DMARxDesc_OWN) != (u32)RESET)
   {
      frame.length = ETH_ERROR;
      if ((ETH->DMASR & ETH_DMASR_RBUS) != (u32)RESET)
      {
         ETH->DMASR = ETH_DMASR_RBUS; // ���ETH DMA��RBUSλ
         ETH->DMARPDR = 0; // �ָ�DMA����
      }
      return frame; // ����,OWNλ��������
   }
 
   if((DMARxDescToGet->Status & ETH_DMARxDesc_ES) == (u32)RESET && (DMARxDescToGet->Status & ETH_DMARxDesc_LS) != (u32)RESET && (DMARxDescToGet->Status & ETH_DMARxDesc_FS) != (u32)RESET)
   {
      framelength = ((DMARxDescToGet->Status & ETH_DMARxDesc_FL) >> ETH_DMARxDesc_FrameLengthShift) - 4; // �õ����հ�֡����(������4�ֽ�CRC)
       frame.buffer = DMARxDescToGet->Buffer1Addr; // �õ����������ڵ�λ��
   }
   else
   {
      framelength = ETH_ERROR; // ����
   }
 
   frame.length = framelength;
   frame.descriptor = DMARxDescToGet;
 
   // ����ETH DMAȫ��Rx������Ϊ��һ��Rx��������Ϊ��һ��buffer��ȡ������һ��DMA Rx������
   DMARxDescToGet = (ETH_DMADESCTypeDef *)(DMARxDescToGet->Buffer2NextDescAddr);
   return frame;
}
 
/********************************************************
 * �������ܣ�����һ����̫�����ݰ�
 * ��    �Σ�frame_length�����ݰ�����
 * �� �� ֵ��0=�ɹ���1=ʧ��
 ********************************************************/
static unsigned int eth_tx_packet(unsigned short frame_length)
{
   //��鵱ǰ������,�Ƿ�����ETHERNET DMA(���õ�ʱ��)/CPU(��λ��ʱ��)
   if((DMATxDescToSet->Status & ETH_DMATxDesc_OWN) != (u32)RESET)
   {
      return 1; // ����,OWNλ��������
   }
 
  DMATxDescToSet->ControlBufferSize = (frame_length & ETH_DMATxDesc_TBS1); // ����֡����,bits[12:0]
  DMATxDescToSet->Status |= ETH_DMATxDesc_LS | ETH_DMATxDesc_FS; // �������һ���͵�һ��λ����λ(1������������һ֡)
  DMATxDescToSet->Status |= ETH_DMATxDesc_OWN; // ����Tx��������OWNλ,buffer�ع�ETH DMA
   if((ETH->DMASR & ETH_DMASR_TBUS) != (u32)RESET) // ��Tx Buffer������λ(TBUS)�����õ�ʱ��,������.�ָ�����
   {
      ETH->DMASR = ETH_DMASR_TBUS; // ����ETH DMA TBUSλ
      ETH->DMATPDR = 0; // �ָ�DMA����
   }
 
   // ����ETH DMAȫ��Tx������Ϊ��һ��Tx������
   // Ϊ��һ��buffer����������һ��DMA Tx������
   DMATxDescToSet = (ETH_DMADESCTypeDef *)(DMATxDescToSet->Buffer2NextDescAddr);
   return 0;
}
 
/********************************************************
 * �������ܣ���̫�����ݰ����͵ĵײ㺯��(ARP����)
 * ��    �Σ�netif�������ṹ��ָ��
             p��pbuf���ݽṹ��ָ��
 * �� �� ֵ��ERR_OK=����������ERR_MEM=�ڴ��쳣
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
      return ERR_MEM; // ���ش���״̬
   }
 
   return ERR_OK;
}
 
/********************************************************
 * �������ܣ���̫����������(��̫���жϺ�������)
 * ��    �Σ�netif�������ṹ��ָ��
 * �� �� ֵ��ERR_OK=����������ERR_MEM=�ڴ��쳣
 ********************************************************/
static err_t ethernetif_input(struct netif *netif)
{
   err_t err = ERR_OK;
   unsigned int length = 0;
   FrameTypeDef frame = eth_rx_packet();
   unsigned char *pdata=(unsigned char *)frame.buffer;
   struct pbuf *p = pbuf_alloc(PBUF_RAW, frame.length, PBUF_POOL); // ��pbufs�ڴ���и�pbuf�����ڴ�
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
      // �����ݷ��͸�Э��ջ
      if(netif->input(p, netif) != ERR_OK)
      {
         pbuf_free(p);
         p = NULL;
      }
   }
   else
   {
      err = ERR_MEM; // �ڴ��쳣
   }
 
   // ����Rx������OWNλ�����ݽ���ETH DMA
   frame.descriptor->Status = ETH_DMARxDesc_OWN;
   if((ETH->DMASR&ETH_DMASR_RBUS) != RESET)
   {
      ETH->DMASR=ETH_DMASR_RBUS; // ����ETH DMA RBUSλ
      ETH->DMARPDR = 0; // �ָ�DMA����
   }
 
   return err;
}
 
/********************************************************
 * �������ܣ���̫����ʼ����Э��ջ�ں˵��ã�
 * ��    �Σ�netif�������ṹ��ָ��
 * �� �� ֵ��ERR_OK=����������ERR_BUF=����ָ���쳣
 ********************************************************/
static err_t ethernetif_init(struct netif *netif)
{
   if(netif == NULL)
   {
      return ERR_BUF;
   }
 
#if LWIP_NETIF_HOSTNAME // LWIP_NETIF_HOSTNAME
   netif->hostname = "lwip"; // ��ʼ������
#endif
   netif->name[0] = 'e'; // ��ʼ������netif��name�ֶ�
   netif->name[1] = '0'; // ���ļ��ⶨ�����ﲻ�ù��ľ���ֵ
   netif->output = etharp_output; // IP�����ݰ����ͺ���
   netif->linkoutput = arp_output; // ARPģ�鷢�����ݰ�����
   netif->hwaddr_len = ETHARP_HWADDR_LEN; // ����MAC��ַ����,Ϊ6���ֽ�
          
  // ��ʼ��MAC��ַ,
   // ע��MAC��ַ�����������������豸��MAC��ַ��ͬ
   // ��ȡSTM32��ΨһID��ǰ32λ��ΪMAC��ַ���ĸ��ֽ�
   unsigned int id = *(unsigned int *)(0x1FFF7A10);
   netif->hwaddr[0] = 2;
   netif->hwaddr[1] = 0;
   netif->hwaddr[2] = (id >> 24) & 0xFF;
   netif->hwaddr[3] = (id >> 16) & 0xFF;
   netif->hwaddr[4] = (id >> 8) & 0xFF;
   netif->hwaddr[5] = (id >> 0) & 0xFF;
   netif->mtu = 1500; // ��������䵥Ԫ
 
   // ����������㲥��ARP���ܣ����Ҹ�����������Ӳ����·����
   netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;
 
   // ��STM32F4��MAC��ַ�Ĵ�����д��MAC��ַ
   ETH_MACAddressConfig(ETH_MAC_Address0, netif->hwaddr);
 
   // ��̫��DMA��������
   ETH_DMATxDescChainInit(DMATxDscrTab, pTxData, ETH_TXBUFNB);
   ETH_DMARxDescChainInit(DMARxDscrTab, pRxData, ETH_RXBUFNB);
 
#ifdef CHECKSUM_BY_HARDWARE // ʹ��Ӳ��֡У��
   // ʹ��TCP,UDP��ICMP�ķ���֡У��
   // TCP,UDP��ICMP�Ľ���֡У����DMA��������
   for(unsigned int i = 0; i < ETH_TXBUFNB; i++)
   {
      ETH_DMATxDescChecksumInsertionConfig(&DMATxDscrTab[i], ETH_DMATxDesc_ChecksumTCPUDPICMPFull);
   }
#endif
 
   ETH_Start(); // ����MAC��DMA    
   return ERR_OK;
}
 
/********************************************************
 * �������ܣ���̫��DMA�����жϷ�����
 * ��    �Σ���
 * �� �� ֵ����
 ********************************************************/
void ETH_IRQHandler(void)
{
   // �����ж��Ƿ��յ����ݰ�
   while(ETH_GetRxPktSize(DMARxDescToGet) != 0)
   {
      ethernetif_input(&eth0);
   }
   ETH_DMAClearITPendingBit(ETH_DMA_IT_R); // ���DMA�жϱ�־λ
   ETH_DMAClearITPendingBit(ETH_DMA_IT_NIS); // ���DMA�����жϱ�־λ
}
 
#if LWIP_DHCP // ���ʹ����DHCP
/********************************************************
 * �������ܣ�DHCP��������
 * ��    �Σ�psLwipDev��IP��Ϣ���ݽṹָ֮��
 * �� �� ֵ����
 ********************************************************/
static void dhcp_task(sLwipDev_t * const psLwipDev)
{
   dhcp_start(&eth0); // ����DHCP
   // ͨ��DHCP�����ȡIP��ַ�����ʧ���ҳ�������Դ������˳�
   while(eth0.dhcp->tries < 5U && psLwipDev != NULL)
   {
      if(eth0.ip_addr.addr != 0)
      {
         // ������ͨ��DHCP��ȡ�������ص�ַ
         psLwipDev->gateway[3] =(uint8_t)(eth0.gw.addr >> 24);
         psLwipDev->gateway[2] =(uint8_t)(eth0.gw.addr >> 16);
         psLwipDev->gateway[1] =(uint8_t)(eth0.gw.addr >> 8);
         psLwipDev->gateway[0] =(uint8_t)(eth0.gw.addr);
   
         // ����ͨ��DHCP��ȡ�������������ַ
         psLwipDev->mask[3] = (uint8_t)(eth0.netmask.addr >> 24);
         psLwipDev->mask[2] = (uint8_t)(eth0.netmask.addr >> 16);
         psLwipDev->mask[1] = (uint8_t)(eth0.netmask.addr >> 8);
         psLwipDev->mask[0] = (uint8_t)(eth0.netmask.addr);
   
         // ������ͨ��DHCP��ȡ����IP��ַ
         psLwipDev->localip[3] = (uint8_t)(eth0.ip_addr.addr >> 24);
         psLwipDev->localip[2] = (uint8_t)(eth0.ip_addr.addr >> 16);
         psLwipDev->localip[1] = (uint8_t)(eth0.ip_addr.addr >> 8);
         psLwipDev->localip[0] = (uint8_t)(eth0.ip_addr.addr);
         break;
      }
      vTaskDelay(50);
   }
   dhcp_stop(&eth0); // �ر�DHCP
}
#endif
 
/********************************************************
 * �������ܣ���ȡĬ��IP��Ϣ
 * ��    �Σ�psLwipDev��IP��Ϣ���ݽṹָ��
 * �� �� ֵ��0=�ɹ���1=����ָ��ΪNULL
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
 * �������ܣ���̫����Э��ջ�ں˵ȳ�ʼ��
 * ��    �Σ�psLwipDev��IP��Ϣ���ݽṹָ��
 * �� �� ֵ��0=��ʼ���ɹ�
             1=����ָ��ΪNULL
             2=��̫����ʼ��ʧ��
             3=����ע��ʧ��
 ********************************************************/
unsigned int eth_init(sLwipDev_t * const psLwipDev)
{
   if(psLwipDev == NULL)
   {
      return 1;
   }
 
   struct ip_addr mask; // ��������
   struct ip_addr gateway; // Ĭ������
   struct ip_addr localip; // ����IP��ַ
   // ��̫����ʼ��
   if(eht_init() != 0)
   {
      return 2;
   }
 
   // TCPIP�ں˳�ʼ��
   tcpip_init(NULL, NULL);
 
#if LWIP_DHCP // ���ʹ����DHCP
   dhcp_task(psLwipDev);
#else
   // ����Ĭ��IP��ַ��Ϣ
   IP4_ADDR(&mask, psLwipDev->mask[0], psLwipDev->mask[1], psLwipDev->mask[2], psLwipDev->mask[3]);
   IP4_ADDR(&gateway, psLwipDev->gateway[0], psLwipDev->gateway[1], psLwipDev->gateway[2], psLwipDev->gateway[3]);
   IP4_ADDR(&localip, psLwipDev->localip[0], psLwipDev->localip[1], psLwipDev->localip[2], psLwipDev->localip[3]);
#endif
 
   // �������б���ע��һ������
   if(netif_add(&eth0, &localip, &mask, &gateway, NULL, &ethernetif_init, &tcpip_input) == NULL)
   {
      return 3; // ����ע��ʧ��
   }
 
   // ������ӳɹ���,����netifΪĬ��ֵ,���Ҵ�netif����
   netif_set_default(&eth0); // ����netifΪĬ������
   netif_set_up(&eth0); // ��netif����
   return 0;
}

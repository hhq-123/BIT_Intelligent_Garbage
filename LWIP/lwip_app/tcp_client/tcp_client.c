#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "sys_eth.h"
#include "lwip/api.h"
#include "tcp_client.h"

// TCP�ͻ��˽������ݽṹ
#pragma pack(push, 1)
typedef struct _sTcpClientRxMsg
{
   unsigned char *pbuf;
   unsigned int length;
}sTcpClientRxMsg_t;
#pragma pack(pop)

static struct netconn *socket = NULL; // TCP�ͻ����׽���
static xQueueHandle TcpClientQueue = NULL; // ���������ж�
static unsigned char connect_flag = pdFALSE; // TCP�ͻ������ӱ�־
 

/********************************************************
 * �������ܣ�TCP�ͻ������ݽ����߳�
 * ��    �Σ�arg���߳��β�
 * �� �� ֵ����
 ********************************************************/
static void tcp_client_thread(void *parg)
{
   sLwipDev_t *psLwipDev = (sLwipDev_t *)parg;
 
   while(psLwipDev != NULL)
   {
      // ����һ���׽���
      socket = netconn_new(NETCONN_TCP);
      if(socket == NULL)
      {
         vTaskDelay(100);
         continue; // TCP���Ӵ���ʧ������´���
      }
  
      // ��IP�˿ڣ������ӷ�����
      ip_addr_t LocalIP = {0};
      ip_addr_t RemoteIP = {0};
      IP4_ADDR(&LocalIP, psLwipDev->localip[0], psLwipDev->localip[1], psLwipDev->localip[2], psLwipDev->localip[3]);
      IP4_ADDR(&RemoteIP, psLwipDev->remoteip[0], psLwipDev->remoteip[1], psLwipDev->remoteip[2], psLwipDev->remoteip[3]);
      netconn_bind(socket, &LocalIP, psLwipDev->localport); // �󶨱��ض˿ں�IP���󶨺�ʹ�ô�IP�Ͷ˿ڶ��ⷢ��
      if(netconn_connect(socket, &RemoteIP, psLwipDev->remoteport) != ERR_OK) // ���ӷ�����
      {
         netconn_delete(socket); // ����������ʧ�ܣ�ɾ������
         vTaskDelay(100);
         continue;
      }
      err_t recv_err;
      struct pbuf *q;
      struct netbuf *recvbuf;
      socket->recv_timeout = 10;
      connect_flag = pdTRUE; // ���ӱ�־��λ
      while(connect_flag == pdTRUE)
      {
         // ��������(ע��ʵ��ÿ�ν��յ����������Ϊ1460���ֽ�)
         recv_err = netconn_recv(socket, &recvbuf);
         if(recv_err == ERR_OK) // ���յ�����
         {
            unsigned int length = 0;
            sTcpClientRxMsg_t msg = {0};
    
            taskENTER_CRITICAL(); // �����ٽ���
            {
               length = recvbuf->p->tot_len;
               msg.pbuf = pvPortMalloc(length);
               if(msg.pbuf != NULL)
               {
                  for(q = recvbuf->p; q != NULL; q = q->next)
                  {
                     if((msg.length + q->len) > length)
                     {
                        break; // ���治����ֱ���˳�
                     }
       
                     unsigned char *pload = (unsigned char *)q->payload;
                     for(unsigned int i = 0; i < q->len; i++)
                     {
                        msg.pbuf[msg.length++] = pload[i];
                     }
                  }
      
                  netbuf_delete(recvbuf);
               }
            }
            taskEXIT_CRITICAL(); // �˳��ٽ���
            xQueueSend(TcpClientQueue, &msg, 0);
         }
         else if(recv_err == ERR_CLSD) // �����������ر�����
         {
            tcp_client_reconnect();
         }
      }
  
      // �����쳣
      netconn_close(socket); // �ر��׽���
      netconn_delete(socket); // �ͷ���Դ
   }
}

/********************************************************
 * �������ܣ�����TCP�ͻ���
 * ��    �Σ�ip_msg��IP��Ϣ���ݽṹָ��
 * �� �� ֵ��0=�ɹ�
             1=TCP�ͻ����̴߳���ʧ��
             2=TCP�ͻ��������жӴ���ʧ��
 ********************************************************/
unsigned int tcp_client_init(void *ip_msg)
{
   vPortFree(TcpClientQueue);
   TcpClientQueue = xQueueCreate(50, sizeof(sTcpClientRxMsg_t));
   if(TcpClientQueue == NULL)
   {
      return 2;
   }
 
   // �������񣬲��������������������ơ������ջ��� ��������βΡ��������ȼ���������
   if(xTaskCreate(tcp_client_thread, "tcp_client_thread", 256, ip_msg, 5, NULL) != pdPASS)
   {
      return 1; // ����ʧ��
   }
   return 0;
}

/********************************************************
 * �������ܣ���ȡTCP�ͻ��������жӾ��
 * ��    �Σ���
 * �� �� ֵ�������жӾ��
 ********************************************************/
void *tcp_client_queue(void)
{
   return TcpClientQueue;
}

/********************************************************
 * �������ܣ�TCP�ͻ�������
 * ��    �Σ���
 * �� �� ֵ��0=�ɹ�
             1=ʧ��
 ********************************************************/
unsigned int tcp_client_reconnect(void)
{
   connect_flag = pdFALSE; // ������ӱ�־
   if(connect_flag == pdFALSE)
   {
      return 0;
   }
   else
   {
      return 1;
   }
}

/********************************************************
 * �������ܣ���ȡTCP�ͻ�������״̬
 * ��    �Σ���
 * �� �� ֵ��0=��������
             1=�����쳣
 ********************************************************/
unsigned int tcp_client_connect_status_get(void)
{
   if(connect_flag == pdFALSE)
   {
      return 1;
   }
   else
   {
      return 0;
   }
}

/********************************************************
 * �������ܣ�TCP�ͻ��������ڷ�������
 * ��    �Σ�pbuf�����ݻ����ַ
             length�����������ֽ���
 * �� �� ֵ��0=�ɹ�
             1=���ݻ����ַΪNULL
             2=���ݳ��ȴ���
             3=�ͻ���δ����
             4=�����쳣
 ********************************************************/
unsigned int tcp_client_write(const unsigned char *pbuf, unsigned int length)
{
   unsigned char retry = 5;
 
   if(pbuf == NULL)
   {
      return 1;
   }
 
   if(length == 0)
   {
      return 2;
   }
 
   if(connect_flag == pdFALSE)
   {
      return 3;
   }
 
   while(netconn_write(socket, pbuf, length, NETCONN_COPY) != ERR_OK) // ��������
   {
      vTaskDelay(100);
      if(--retry == 0)
      {
         tcp_client_reconnect();
         return 4;
      }
   }
 
   return 0; // ���ͳɹ�
}

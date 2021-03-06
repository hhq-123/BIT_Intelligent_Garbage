#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "sys_eth.h"
#include "lwip/api.h"
#include "tcp_client.h"

// TCP客户端接收数据结构
#pragma pack(push, 1)
typedef struct _sTcpClientRxMsg
{
   unsigned char *pbuf;
   unsigned int length;
}sTcpClientRxMsg_t;
#pragma pack(pop)

static struct netconn *socket = NULL; // TCP客户端套接字
static xQueueHandle TcpClientQueue = NULL; // 接收数据列队
static unsigned char connect_flag = pdFALSE; // TCP客户端连接标志
 

/********************************************************
 * 函数功能：TCP客户端数据接收线程
 * 形    参：arg：线程形参
 * 返 回 值：无
 ********************************************************/
static void tcp_client_thread(void *parg)
{
   sLwipDev_t *psLwipDev = (sLwipDev_t *)parg;
 
   while(psLwipDev != NULL)
   {
      // 创建一个套接字
      socket = netconn_new(NETCONN_TCP);
      if(socket == NULL)
      {
         vTaskDelay(100);
         continue; // TCP连接创建失败则从新创建
      }
  
      // 绑定IP端口，并连接服务器
      ip_addr_t LocalIP = {0};
      ip_addr_t RemoteIP = {0};
      IP4_ADDR(&LocalIP, psLwipDev->localip[0], psLwipDev->localip[1], psLwipDev->localip[2], psLwipDev->localip[3]);
      IP4_ADDR(&RemoteIP, psLwipDev->remoteip[0], psLwipDev->remoteip[1], psLwipDev->remoteip[2], psLwipDev->remoteip[3]);
      netconn_bind(socket, &LocalIP, psLwipDev->localport); // 绑定本地端口和IP，绑定后使用此IP和端口对外发送
      if(netconn_connect(socket, &RemoteIP, psLwipDev->remoteport) != ERR_OK) // 连接服务器
      {
         netconn_delete(socket); // 服务器连接失败，删除连接
         vTaskDelay(100);
         continue;
      }
      err_t recv_err;
      struct pbuf *q;
      struct netbuf *recvbuf;
      socket->recv_timeout = 10;
      connect_flag = pdTRUE; // 连接标志置位
      while(connect_flag == pdTRUE)
      {
         // 接收数据(注：实测每次接收的数据量最大为1460个字节)
         recv_err = netconn_recv(socket, &recvbuf);
         if(recv_err == ERR_OK) // 接收到数据
         {
            unsigned int length = 0;
            sTcpClientRxMsg_t msg = {0};
    
            taskENTER_CRITICAL(); // 进入临界区
            {
               length = recvbuf->p->tot_len;
               msg.pbuf = pvPortMalloc(length);
               if(msg.pbuf != NULL)
               {
                  for(q = recvbuf->p; q != NULL; q = q->next)
                  {
                     if((msg.length + q->len) > length)
                     {
                        break; // 缓存不够，直接退出
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
            taskEXIT_CRITICAL(); // 退出临界区
            xQueueSend(TcpClientQueue, &msg, 0);
         }
         else if(recv_err == ERR_CLSD) // 服务器主动关闭连接
         {
            tcp_client_reconnect();
         }
      }
  
      // 连接异常
      netconn_close(socket); // 关闭套接字
      netconn_delete(socket); // 释放资源
   }
}

/********************************************************
 * 函数功能：创建TCP客户端
 * 形    参：ip_msg：IP信息数据结构指针
 * 返 回 值：0=成功
             1=TCP客户端线程创建失败
             2=TCP客户端数据列队创建失败
 ********************************************************/
unsigned int tcp_client_init(void *ip_msg)
{
   vPortFree(TcpClientQueue);
   TcpClientQueue = xQueueCreate(50, sizeof(sTcpClientRxMsg_t));
   if(TcpClientQueue == NULL)
   {
      return 2;
   }
 
   // 创建任务，参数：任务函数、任务名称、任务堆栈大� ⑷挝窈尾巍⑷挝裼畔燃丁⑷挝窬浔�
   if(xTaskCreate(tcp_client_thread, "tcp_client_thread", 256, ip_msg, 5, NULL) != pdPASS)
   {
      return 1; // 创建失败
   }
   return 0;
}

/********************************************************
 * 函数功能：获取TCP客户端数据列队句柄
 * 形    参：无
 * 返 回 值：数据列队句柄
 ********************************************************/
void *tcp_client_queue(void)
{
   return TcpClientQueue;
}

/********************************************************
 * 函数功能：TCP客户端重连
 * 形    参：无
 * 返 回 值：0=成功
             1=失败
 ********************************************************/
unsigned int tcp_client_reconnect(void)
{
   connect_flag = pdFALSE; // 清除连接标志
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
 * 函数功能：获取TCP客户端连接状态
 * 形    参：无
 * 返 回 值：0=连接正常
             1=连接异常
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
 * 函数功能：TCP客户端向网口发送数据
 * 形    参：pbuf：数据缓存地址
             length：发送数据字节数
 * 返 回 值：0=成功
             1=数据缓存地址为NULL
             2=数据长度错误
             3=客户端未启动
             4=连接异常
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
 
   while(netconn_write(socket, pbuf, length, NETCONN_COPY) != ERR_OK) // 发送数据
   {
      vTaskDelay(100);
      if(--retry == 0)
      {
         tcp_client_reconnect();
         return 4;
      }
   }
 
   return 0; // 发送成功
}

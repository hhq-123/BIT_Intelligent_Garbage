#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "sys_eth.h"
#include "lwip/api.h"
#include "tcp_client.h"

// TCP¿Í»§¶Ë½ÓÊÕÊı¾İ½á¹¹
#pragma pack(push, 1)
typedef struct _sTcpClientRxMsg
{
   unsigned char *pbuf;
   unsigned int length;
}sTcpClientRxMsg_t;
#pragma pack(pop)

static struct netconn *socket = NULL; // TCP¿Í»§¶ËÌ×½Ó×Ö
static xQueueHandle TcpClientQueue = NULL; // ½ÓÊÕÊı¾İÁĞ¶Ó
static unsigned char connect_flag = pdFALSE; // TCP¿Í»§¶ËÁ¬½Ó±êÖ¾
 

/********************************************************
 * º¯Êı¹¦ÄÜ£ºTCP¿Í»§¶ËÊı¾İ½ÓÊÕÏß³Ì
 * ĞÎ    ²Î£ºarg£ºÏß³ÌĞÎ²Î
 * ·µ »Ø Öµ£ºÎŞ
 ********************************************************/
static void tcp_client_thread(void *parg)
{
   sLwipDev_t *psLwipDev = (sLwipDev_t *)parg;
 
   while(psLwipDev != NULL)
   {
      // ´´½¨Ò»¸öÌ×½Ó×Ö
      socket = netconn_new(NETCONN_TCP);
      if(socket == NULL)
      {
         vTaskDelay(100);
         continue; // TCPÁ¬½Ó´´½¨Ê§°ÜÔò´ÓĞÂ´´½¨
      }
  
      // °ó¶¨IP¶Ë¿Ú£¬²¢Á¬½Ó·şÎñÆ÷
      ip_addr_t LocalIP = {0};
      ip_addr_t RemoteIP = {0};
      IP4_ADDR(&LocalIP, psLwipDev->localip[0], psLwipDev->localip[1], psLwipDev->localip[2], psLwipDev->localip[3]);
      IP4_ADDR(&RemoteIP, psLwipDev->remoteip[0], psLwipDev->remoteip[1], psLwipDev->remoteip[2], psLwipDev->remoteip[3]);
      netconn_bind(socket, &LocalIP, psLwipDev->localport); // °ó¶¨±¾µØ¶Ë¿ÚºÍIP£¬°ó¶¨ºóÊ¹ÓÃ´ËIPºÍ¶Ë¿Ú¶ÔÍâ·¢ËÍ
      if(netconn_connect(socket, &RemoteIP, psLwipDev->remoteport) != ERR_OK) // Á¬½Ó·şÎñÆ÷
      {
         netconn_delete(socket); // ·şÎñÆ÷Á¬½ÓÊ§°Ü£¬É¾³ıÁ¬½Ó
         vTaskDelay(100);
         continue;
      }
      err_t recv_err;
      struct pbuf *q;
      struct netbuf *recvbuf;
      socket->recv_timeout = 10;
      connect_flag = pdTRUE; // Á¬½Ó±êÖ¾ÖÃÎ»
      while(connect_flag == pdTRUE)
      {
         // ½ÓÊÕÊı¾İ(×¢£ºÊµ²âÃ¿´Î½ÓÊÕµÄÊı¾İÁ¿×î´óÎª1460¸ö×Ö½Ú)
         recv_err = netconn_recv(socket, &recvbuf);
         if(recv_err == ERR_OK) // ½ÓÊÕµ½Êı¾İ
         {
            unsigned int length = 0;
            sTcpClientRxMsg_t msg = {0};
    
            taskENTER_CRITICAL(); // ½øÈëÁÙ½çÇø
            {
               length = recvbuf->p->tot_len;
               msg.pbuf = pvPortMalloc(length);
               if(msg.pbuf != NULL)
               {
                  for(q = recvbuf->p; q != NULL; q = q->next)
                  {
                     if((msg.length + q->len) > length)
                     {
                        break; // »º´æ²»¹»£¬Ö±½ÓÍË³ö
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
            taskEXIT_CRITICAL(); // ÍË³öÁÙ½çÇø
            xQueueSend(TcpClientQueue, &msg, 0);
         }
         else if(recv_err == ERR_CLSD) // ·şÎñÆ÷Ö÷¶¯¹Ø±ÕÁ¬½Ó
         {
            tcp_client_reconnect();
         }
      }
  
      // Á¬½ÓÒì³£
      netconn_close(socket); // ¹Ø±ÕÌ×½Ó×Ö
      netconn_delete(socket); // ÊÍ·Å×ÊÔ´
   }
}

/********************************************************
 * º¯Êı¹¦ÄÜ£º´´½¨TCP¿Í»§¶Ë
 * ĞÎ    ²Î£ºip_msg£ºIPĞÅÏ¢Êı¾İ½á¹¹Ö¸Õë
 * ·µ »Ø Öµ£º0=³É¹¦
             1=TCP¿Í»§¶ËÏß³Ì´´½¨Ê§°Ü
             2=TCP¿Í»§¶ËÊı¾İÁĞ¶Ó´´½¨Ê§°Ü
 ********************************************************/
unsigned int tcp_client_init(void *ip_msg)
{
   vPortFree(TcpClientQueue);
   TcpClientQueue = xQueueCreate(50, sizeof(sTcpClientRxMsg_t));
   if(TcpClientQueue == NULL)
   {
      return 2;
   }
 
   // ´´½¨ÈÎÎñ£¬²ÎÊı£ºÈÎÎñº¯Êı¡¢ÈÎÎñÃû³Æ¡¢ÈÎÎñ¶ÑÕ»´óĞ ¢ÈÎÎñº¯ÊıĞÎ²Î¡¢ÈÎÎñÓÅÏÈ¼¶¡¢ÈÎÎñ¾ä±ú
   if(xTaskCreate(tcp_client_thread, "tcp_client_thread", 256, ip_msg, 5, NULL) != pdPASS)
   {
      return 1; // ´´½¨Ê§°Ü
   }
   return 0;
}

/********************************************************
 * º¯Êı¹¦ÄÜ£º»ñÈ¡TCP¿Í»§¶ËÊı¾İÁĞ¶Ó¾ä±ú
 * ĞÎ    ²Î£ºÎŞ
 * ·µ »Ø Öµ£ºÊı¾İÁĞ¶Ó¾ä±ú
 ********************************************************/
void *tcp_client_queue(void)
{
   return TcpClientQueue;
}

/********************************************************
 * º¯Êı¹¦ÄÜ£ºTCP¿Í»§¶ËÖØÁ¬
 * ĞÎ    ²Î£ºÎŞ
 * ·µ »Ø Öµ£º0=³É¹¦
             1=Ê§°Ü
 ********************************************************/
unsigned int tcp_client_reconnect(void)
{
   connect_flag = pdFALSE; // Çå³ıÁ¬½Ó±êÖ¾
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
 * º¯Êı¹¦ÄÜ£º»ñÈ¡TCP¿Í»§¶ËÁ¬½Ó×´Ì¬
 * ĞÎ    ²Î£ºÎŞ
 * ·µ »Ø Öµ£º0=Á¬½ÓÕı³£
             1=Á¬½ÓÒì³£
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
 * º¯Êı¹¦ÄÜ£ºTCP¿Í»§¶ËÏòÍø¿Ú·¢ËÍÊı¾İ
 * ĞÎ    ²Î£ºpbuf£ºÊı¾İ»º´æµØÖ·
             length£º·¢ËÍÊı¾İ×Ö½ÚÊı
 * ·µ »Ø Öµ£º0=³É¹¦
             1=Êı¾İ»º´æµØÖ·ÎªNULL
             2=Êı¾İ³¤¶È´íÎó
             3=¿Í»§¶ËÎ´Æô¶¯
             4=Á¬½ÓÒì³£
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
 
   while(netconn_write(socket, pbuf, length, NETCONN_COPY) != ERR_OK) // ·¢ËÍÊı¾İ
   {
      vTaskDelay(100);
      if(--retry == 0)
      {
         tcp_client_reconnect();
         return 4;
      }
   }
 
   return 0; // ·¢ËÍ³É¹¦
}

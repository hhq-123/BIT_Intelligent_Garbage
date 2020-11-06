#ifndef __SYS_ETH_H
#define __SYS_ETH_H
#pragma pack(push, 1)

typedef struct _sLwipDev
{
	unsigned char mask[4];
	unsigned char gateway[4];
	unsigned char localip[4];
	unsigned char remoteip[4];
	unsigned short localport;
	unsigned short remoteport;
}sLwipDev_t;
#pragma pack(pop)
 
/********************************************************
 * 函数功能：以太网、协议栈内核等初始化
 * 形    参：psLwipDev：IP信息数据结构指针
 * 返 回 值：0=初始化成功
             1=数据指针为NULL
             2=以太网初始化失败
             3=网卡注册失败
 ********************************************************/
unsigned int eth_init(sLwipDev_t * const psLwipDev);

/********************************************************
 * 函数功能：获取默认IP信息
 * 形    参：psLwipDev：IP信息数据结构指针
 * 返 回 值：0=成功，1=数据指针为NULL
 ********************************************************/
unsigned int eth_default_ip_get(sLwipDev_t * const psLwipDev);
 
#endif


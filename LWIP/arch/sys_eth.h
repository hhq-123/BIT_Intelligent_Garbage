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
 * �������ܣ���̫����Э��ջ�ں˵ȳ�ʼ��
 * ��    �Σ�psLwipDev��IP��Ϣ���ݽṹָ��
 * �� �� ֵ��0=��ʼ���ɹ�
             1=����ָ��ΪNULL
             2=��̫����ʼ��ʧ��
             3=����ע��ʧ��
 ********************************************************/
unsigned int eth_init(sLwipDev_t * const psLwipDev);

/********************************************************
 * �������ܣ���ȡĬ��IP��Ϣ
 * ��    �Σ�psLwipDev��IP��Ϣ���ݽṹָ��
 * �� �� ֵ��0=�ɹ���1=����ָ��ΪNULL
 ********************************************************/
unsigned int eth_default_ip_get(sLwipDev_t * const psLwipDev);
 
#endif


// cc.h属于LWIP TCP/IP协议栈一部分
// 作者: Adam Dunkels <adam@sics.se>

#ifndef __CC_H__
#define __CC_H__

#include "cpu.h"
#include "stdio.h"
#include "FreeRTOS.h"  //使用UCOS 要添加此头文件！/现改为FreeRTOS
#include "task.h"

//定义与平台无关的数据类型
typedef unsigned   char    u8_t;  	//无符号8位整数  
typedef signed     char    s8_t;   	//有符号8位整数 
typedef unsigned   short   u16_t;  	//无符号16位整数
typedef signed     short   s16_t;   //有符号16位整数
typedef unsigned   long    u32_t;   //无符号32位整数
typedef signed     long    s32_t;   //有符号32位整数
typedef u32_t mem_ptr_t;            //内存地址型数据
typedef int sys_prot_t;				//临界保护型数据

//使用操作系统时的临界区保护，这里以UCOS II为例
//当定义了OS_CRITICAL_METHOD时就说明使用了UCOS II
/*
#if OS_CRITICAL_METHOD == 1
#define SYS_ARCH_DECL_PROTECT(lev)
#define SYS_ARCH_PROTECT(lev)		CPU_INT_DIS()
#define SYS_ARCH_UNPROTECT(lev)		CPU_INT_EN()
#endif

#if OS_CRITICAL_METHOD == 3  
#define SYS_ARCH_DECL_PROTECT(lev)	u32_t lev
#define SYS_ARCH_PROTECT(lev)		lev = OS_CPU_SR_Save() 	//UCOS II中进入临界区,关中断
#define SYS_ARCH_UNPROTECT(lev)		OS_CPU_SR_Restore(lev)	//UCOS II中退出A临界区，开中断 
#endif
*/

//使用操作系统时的临界区保护
// SCB_ICSR_REG 定义的寄存器是用于区分当前是任务级还是中断级的，主要在Enter_Critical()与Exit_Critical()中使用
#define SCB_ICSR_REG  (*((volatile u32_t *)0xE000ED04))
// Enter_Critical()与Exit_Critical()这两个函数是在sys_arch.c中实现的，所以此处加extern声明为外部函数
extern u32_t Enter_Critical(void); // 用于声明进入保护临界区
extern void Exit_Critical(u32_t lev); // 声明退出保护临界区
#define SYS_ARCH_DECL_PROTECT(lev) u32_t lev
#define SYS_ARCH_PROTECT(lev)  lev=Enter_Critical()
#define SYS_ARCH_UNPROTECT(lev)  Exit_Critical(lev)


//根据不同的编译器定义一些符号
#if defined (__ICCARM__)

#define PACK_STRUCT_BEGIN
#define PACK_STRUCT_STRUCT 
#define PACK_STRUCT_END
#define PACK_STRUCT_FIELD(x) x
#define PACK_STRUCT_USE_INCLUDES

#elif defined (__CC_ARM)

#define PACK_STRUCT_BEGIN __packed
#define PACK_STRUCT_STRUCT 
#define PACK_STRUCT_END
#define PACK_STRUCT_FIELD(x) x

#elif defined (__GNUC__)

#define PACK_STRUCT_BEGIN
#define PACK_STRUCT_STRUCT __attribute__ ((__packed__))
#define PACK_STRUCT_END
#define PACK_STRUCT_FIELD(x) x

#elif defined (__TASKING__)

#define PACK_STRUCT_BEGIN
#define PACK_STRUCT_STRUCT
#define PACK_STRUCT_END
#define PACK_STRUCT_FIELD(x) x

#endif

//LWIP用printf调试时使用到的一些类型
#define U16_F "4d"
#define S16_F "4d"
#define X16_F "4x"
#define U32_F "8ld"
#define S32_F "8ld"
#define X32_F "8lx"

//宏定义
#ifndef LWIP_PLATFORM_ASSERT
#define LWIP_PLATFORM_ASSERT(x) \
    do \
    {   printf("Assertion \"%s\" failed at line %d in %s\r\n", x, __LINE__, __FILE__); \
    } while(0)
#endif

#ifndef LWIP_PLATFORM_DIAG
#define LWIP_PLATFORM_DIAG(x) do {printf x;} while(0)
#endif

#endif /* __CC_H__ */

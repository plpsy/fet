/****************************************************************************/
/*                                                                          */
/* 广州创龙电子科技有限公司                                                 */
/*                                                                          */
/* Copyright (C) 2014-2016 Guangzhou Tronlong Electronic Technology Co.,Ltd */
/*                                                                          */
/****************************************************************************/
/*
 *   - 希望缄默(bin wang)
 *   - bin@tronlong.com
 *   - DSP C665x 项目组
 *
 *   官网 www.tronlong.com
 *   论坛 51dsp.net
 *
 */

#ifndef GENERAL_H_
#define GENERAL_H_

// 设备
#include <c6x.h>

// C 语言
#include <stdio.h>

// XDCTools
#include <xdc/std.h>

#include <xdc/cfg/global.h>

#include <xdc/runtime/Types.h>
#include <xdc/runtime/System.h>
#include <xdc/runtime/Memory.h>
#include <xdc/runtime/IHeap.h>

#include <xdc/runtime/Error.h>

// SYS/BIOS
#include <ti/sysbios/BIOS.h>

#include <ti/sysbios/family/c64p/Hwi.h>
#include <ti/sysbios/family/c64p/TimestampProvider.h>
#include <ti/sysbios/family/c64p/Exception.h>

#include <ti/sysbios/family/c66/tci66xx/CpIntc.h>
#include <ti/sysbios/family/c66/Cache.h>

#include <ti/sysbios/knl/swi.h>
#include <ti/sysbios/knl/task.h>
#include <ti/sysbios/knl/Semaphore.h>

#include <ti/sysbios/heaps/HeapBuf.h>
#include <ti/sysbios/heaps/HeapMem.h>

#include <ti/ndk/inc/netmain.h>
#include <ti/ndk/inc/tools/console.h>
#include <ti/ndk/inc/tools/servers.h>

#include "resource_mgr.h"

// C 标准库
#include <string.h>

// 库
#include "Platform_C6678.h"

#include "soc_c665x.h"

#include "gpio.h"
#include "uart.h"
#include "srio.h"

// 开发板特定驱动
#include "BoardOS.h"

/****************************************************************************/
/*                                                                          */
/*              宏定义                                                      */
/*                                                                          */
/****************************************************************************/
// 调试输出
#define OUTPUT printf
#define DEBUGPRINT(format, ...)		\
     do {							\
             OUTPUT("[%16lld | %16s @ %16s, %4d] " format, _itoll(TSCH, TSCL), __func__, __FILE__, __LINE__, ##__VA_ARGS__ ); \
        } while (0)

// 串口终端命令字长
#define CmdSize 64

/****************************************************************************/
/*                                                                          */
/*              全局变量                                                    */
/*                                                                          */
/****************************************************************************/
// DSP 芯片型号
extern DSPType DSPChipType;

// 软件中断句柄
extern Swi_Handle KEYSwiHandle;

// 串口配置
extern BoardUART uart0cfg;
extern BoardUART uart1cfg;

/****************************************************************************/
/*                                                                          */
/*              函数声明                                                    */
/*                                                                          */
/****************************************************************************/
// 平台初始化
Void PlatformInit();

// 定时器（Timer）初始化
Void TimerInit();
// 按键初始化
Void KEYInit();
// 串口初始化
Void UartInit();
// SRIO 初始化
Void SRIOInit();
// PCIe 初始化
Void PCIeInit(char mode);
// 以太网初始化
Void SGMIISerDesInit();
Void SGMIIInit(unsigned int PortNum);

// 定时器中断服务函数
Void TimerIsr(UArg arg);
// 串口 1 中断服务函数
Void Uart1Isr(UArg arg);

// 任务线程（Task）初始化
Void TaskInit();
// 硬件中断线程（Hwi）初始化
Void HwiInit();
// 软件中断线程（Swi）初始化
Void SwiInit();

#endif

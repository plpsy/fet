/****************************************************************************/
/*                                                                          */
/* ���ݴ������ӿƼ����޹�˾                                                 */
/*                                                                          */
/* Copyright (C) 2014-2016 Guangzhou Tronlong Electronic Technology Co.,Ltd */
/*                                                                          */
/****************************************************************************/
/*
 *   - ϣ����Ĭ(bin wang)
 *   - bin@tronlong.com
 *   - DSP C665x ��Ŀ��
 *
 *   ���� www.tronlong.com
 *   ��̳ 51dsp.net
 *
 */

#ifndef GENERAL_H_
#define GENERAL_H_

// �豸
#include <c6x.h>

// C ����
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

// C ��׼��
#include <string.h>

// ��
#include "Platform_C6678.h"

#include "soc_c665x.h"

#include "gpio.h"
#include "uart.h"
#include "srio.h"

// �������ض�����
#include "BoardOS.h"

/****************************************************************************/
/*                                                                          */
/*              �궨��                                                      */
/*                                                                          */
/****************************************************************************/
// �������
#define OUTPUT printf
#define DEBUGPRINT(format, ...)		\
     do {							\
             OUTPUT("[%16lld | %16s @ %16s, %4d] " format, _itoll(TSCH, TSCL), __func__, __FILE__, __LINE__, ##__VA_ARGS__ ); \
        } while (0)

// �����ն������ֳ�
#define CmdSize 64

/****************************************************************************/
/*                                                                          */
/*              ȫ�ֱ���                                                    */
/*                                                                          */
/****************************************************************************/
// DSP оƬ�ͺ�
extern DSPType DSPChipType;

// ����жϾ��
extern Swi_Handle KEYSwiHandle;

// ��������
extern BoardUART uart0cfg;
extern BoardUART uart1cfg;

/****************************************************************************/
/*                                                                          */
/*              ��������                                                    */
/*                                                                          */
/****************************************************************************/
// ƽ̨��ʼ��
Void PlatformInit();

// ��ʱ����Timer����ʼ��
Void TimerInit();
// ������ʼ��
Void KEYInit();
// ���ڳ�ʼ��
Void UartInit();
// SRIO ��ʼ��
Void SRIOInit();
// PCIe ��ʼ��
Void PCIeInit(char mode);
// ��̫����ʼ��
Void SGMIISerDesInit();
Void SGMIIInit(unsigned int PortNum);

// ��ʱ���жϷ�����
Void TimerIsr(UArg arg);
// ���� 1 �жϷ�����
Void Uart1Isr(UArg arg);

// �����̣߳�Task����ʼ��
Void TaskInit();
// Ӳ���ж��̣߳�Hwi����ʼ��
Void HwiInit();
// ����ж��̣߳�Swi����ʼ��
Void SwiInit();

#endif

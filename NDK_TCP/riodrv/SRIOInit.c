/****************************************************************************/
/*                                                                          */
/* 广州创龙电子科技有限公司                                                 */
/*                                                                          */
/* Copyright (C) 2014-2016 Guangzhou Tronlong Electronic Technology Co.,Ltd */
/*                                                                          */
/****************************************************************************/
/****************************************************************************/
/*                                                                          */
/*              SRIO Direct I/O                                             */
/*                                                                          */
/*              2015年11月19日                                              */
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

// XDCTools
#include <xdc/std.h>

#include <xdc/runtime/System.h>

// 外设驱动库
#include "soc_C665x.h"

#include "psc.h"
#include "srio.h"

/****************************************************************************/
/*                                                                          */
/*              宏定义                                                      */
/*                                                                          */
/****************************************************************************/
// 软件断点
#define SW_BREAKPOINT   asm(" SWBP 0 ");

// SRIO 设备信息
#define DEVICE_VENDOR_ID            0x30
#define DEVICE_REVISION             0x0

#define DEVICE_ASSEMBLY_ID          0x0
#define DEVICE_ASSEMBLY_VENDOR_ID   0x30
#define DEVICE_ASSEMBLY_REVISION    0x0
#define DEVICE_ASSEMBLY_INFO        0x0100

/****************************************************************************/
/*                                                                          */
/*              全局变量                                                    */
/*                                                                          */
/****************************************************************************/
// 设备 ID
extern const unsigned int DEVICE_ID1_16BIT;
extern const unsigned int DEVICE_ID1_8BIT;
extern const unsigned int DEVICE_ID2_16BIT;
extern const unsigned int DEVICE_ID2_8BIT;

/****************************************************************************/
/*                                                                          */
/*              函数声明                                                    */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/*                                                                          */
/*              SRIO 初始化                                                 */
/*                                                                          */
/****************************************************************************/
void SRIOInit()
{
	// 使能外设
	PSCModuleControl(SOC_PSC_0_REGS, HW_PSC_SRIO, PSC_MDCTL_NEXT_ENABLE, PSC_POWERDOMAIN_SRIO, PSC_PDCTL_NEXT_ON);

    // 禁用 SRIO 及 SRIO Block
    SRIOGlobalDisable();

	int i;
   	for(i = 0; i <= 9; i++)
   	{
   		SRIOBlockDisable(i);
   	}

    // 设置 Boot Complete 为 0 以便可以修改 SRIO 所有寄存器包括只读（Read Only）寄存器
   	SRIOBootCompleteSet(SRIO_Disable);

    // 使能 SRIO 及 SRIO Block
    SRIOGlobalEnable();

   	for(i = 0; i <= 9; i++)
   	{
   		SRIOBlockEnable(i);
   	}

    // 配置 SRIO Lane 工作模式
   	SRIOModeSet(0, SRIO_Normal);
   	SRIOModeSet(1, SRIO_Normal);
   	SRIOModeSet(2, SRIO_Normal);
   	SRIOModeSet(3, SRIO_Normal);

	// 使能自动优先级提升
   	SRIOAutomaticPriorityPromotionEnable();

	// 设置 SRIO VBUS 预分频为 44.7 到 89.5
   	SRIOPrescalarSelectSet(0);

	// 解锁关键寄存器
	KickUnlock();

    // 配置 SRIO SerDes 时钟（250Mhz x 5 = 1.25GHz）
    SRIOSerDesPLLSet(0x29);

    // 配置 SRIO SerDes 发送 / 接收
    // 数据率 5Gbps（8B/10B）
    SRIOSerDesTxSet(0, 0x001C8F95);
    SRIOSerDesTxSet(1, 0x001C8F95);
    SRIOSerDesTxSet(2, 0x001C8F95);
    SRIOSerDesTxSet(3, 0x001C8F95);

    SRIOSerDesRxSet(0, 0x00468495);
    SRIOSerDesRxSet(1, 0x00468495);
    SRIOSerDesRxSet(2, 0x00468495);
    SRIOSerDesRxSet(3, 0x00468495);

    // 等待 SRIO SerDes 锁定
    while(!(SRIOSerDesPLLStatus() & 0x1));

	// 锁定关键寄存器
	KickLock();

    // 设置设备信息
   	SRIODeviceInfoSet(0, DEVICE_VENDOR_ID, DEVICE_REVISION);

    // 设置组织信息
   	SRIOAssemblyInfoSet(DEVICE_ASSEMBLY_ID, DEVICE_ASSEMBLY_VENDOR_ID, DEVICE_ASSEMBLY_REVISION, DEVICE_ASSEMBLY_INFO);

   	// PE 特性配置
    SRIOProcessingElementFeaturesSet(0x20000199);

    // 配置源及目标操作
    SRIODestinationOperationsSet(0x0004FDF4);
    SRIODestinationOperationsSet(0x0000FC04);

    // 设置 SRIO 设备 ID
    SRIODeviceIDSet(0xff, 0xffff);

    // 配置 TLM 基本路由信息
    SRIOTLMPortBaseRoutingSet(SRIO_Port0, 1, SRIO_Enable, SRIO_Enable, SRIO_Disable);
    SRIOTLMPortBaseRoutingPatternMatchSet(SRIO_Port0, 1, DEVICE_ID1_16BIT, 0xFFFF);

    // 配置端口 PLM
	// 配置 PLM 端口 Silence Timer
	SRIOPLMPortSilenceTimerSet(SRIO_Port0, 0x2);

	// 使能端口
	SRIOInputPortEnable(SRIO_Port0);
	SRIOOutputPortEnable(SRIO_Port0);

	// 配置 PLM 端口 Discovery Timer
	SRIOPLMPortDiscoveryTimerSet(SRIO_Port0, 0x2);

	// 配置端口 Write Reception Capture
	SRIOPortWriteRxCapture(SRIO_Port0, 0x0);

    // 配置端口连接超时
	SRIOPortLinkTimeoutSet(0x000FFF);

    // 端口 Master 使能4
	SRIOPortGeneralSet(SRIO_Enable, SRIO_Enable, SRIO_Disable);

    // 清除 Sticky Register 位
	SRIORegisterResetControlClear();

	// 设置端口写 ID
	SRIOPortWriteTargetDeviceID(0, 0, SRIO_ID_16Bit);

    // 设置数据流最大传输单元（MTU）
	SRIODataDtreamingLogicalLayerControl(64);

	// 配置端口路由模式
	SRIOPLMPathModeControl(SRIO_Port0, SRIO_Mode4_1_4x);

    // 设置 LLM Port IP 预分频
	SRIOServerClockPortIPPrescalar(0x1F);

	// DoorBell 中断配置
	SRIODoorBellInterruptRoutingControl(SRIO_DoorBell_Dedicated_INT);

	// DoorBell 中断路由配置
	SRIODoorBellInterruptConditionRoutingSet(SRIO_DoorBell0, SRIO_DoorBellInt0, SRIO_IntDst_0_16);
	SRIODoorBellInterruptConditionRoutingSet(SRIO_DoorBell0, SRIO_DoorBellInt1, SRIO_IntDst_0_16);
	SRIODoorBellInterruptConditionRoutingSet(SRIO_DoorBell0, SRIO_DoorBellInt2, SRIO_IntDst_0_16);
	SRIODoorBellInterruptConditionRoutingSet(SRIO_DoorBell0, SRIO_DoorBellInt3, SRIO_IntDst_0_16);

	// 使能外设
	SRIOPeripheralEnable();

    // 配置完成
   	SRIOBootCompleteSet(SRIO_Enable);
}

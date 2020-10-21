/****************************************************************************/
/*                                                                          */
/* ���ݴ������ӿƼ����޹�˾                                                 */
/*                                                                          */
/* Copyright (C) 2014-2016 Guangzhou Tronlong Electronic Technology Co.,Ltd */
/*                                                                          */
/****************************************************************************/
/****************************************************************************/
/*                                                                          */
/*              SRIO Direct I/O                                             */
/*                                                                          */
/*              2015��11��19��                                              */
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

// XDCTools
#include <xdc/std.h>

#include <xdc/runtime/System.h>

// ����������
#include "soc_C665x.h"

#include "psc.h"
#include "srio.h"

/****************************************************************************/
/*                                                                          */
/*              �궨��                                                      */
/*                                                                          */
/****************************************************************************/
// ����ϵ�
#define SW_BREAKPOINT   asm(" SWBP 0 ");

// SRIO �豸��Ϣ
#define DEVICE_VENDOR_ID            0x30
#define DEVICE_REVISION             0x0

#define DEVICE_ASSEMBLY_ID          0x0
#define DEVICE_ASSEMBLY_VENDOR_ID   0x30
#define DEVICE_ASSEMBLY_REVISION    0x0
#define DEVICE_ASSEMBLY_INFO        0x0100

/****************************************************************************/
/*                                                                          */
/*              ȫ�ֱ���                                                    */
/*                                                                          */
/****************************************************************************/
// �豸 ID
extern const unsigned int DEVICE_ID1_16BIT;
extern const unsigned int DEVICE_ID1_8BIT;
extern const unsigned int DEVICE_ID2_16BIT;
extern const unsigned int DEVICE_ID2_8BIT;

/****************************************************************************/
/*                                                                          */
/*              ��������                                                    */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/*                                                                          */
/*              SRIO ��ʼ��                                                 */
/*                                                                          */
/****************************************************************************/
void SRIOInit()
{
	// ʹ������
	PSCModuleControl(SOC_PSC_0_REGS, HW_PSC_SRIO, PSC_MDCTL_NEXT_ENABLE, PSC_POWERDOMAIN_SRIO, PSC_PDCTL_NEXT_ON);

    // ���� SRIO �� SRIO Block
    SRIOGlobalDisable();

	int i;
   	for(i = 0; i <= 9; i++)
   	{
   		SRIOBlockDisable(i);
   	}

    // ���� Boot Complete Ϊ 0 �Ա�����޸� SRIO ���мĴ�������ֻ����Read Only���Ĵ���
   	SRIOBootCompleteSet(SRIO_Disable);

    // ʹ�� SRIO �� SRIO Block
    SRIOGlobalEnable();

   	for(i = 0; i <= 9; i++)
   	{
   		SRIOBlockEnable(i);
   	}

    // ���� SRIO Lane ����ģʽ
   	SRIOModeSet(0, SRIO_Normal);
   	SRIOModeSet(1, SRIO_Normal);
   	SRIOModeSet(2, SRIO_Normal);
   	SRIOModeSet(3, SRIO_Normal);

	// ʹ���Զ����ȼ�����
   	SRIOAutomaticPriorityPromotionEnable();

	// ���� SRIO VBUS Ԥ��ƵΪ 44.7 �� 89.5
   	SRIOPrescalarSelectSet(0);

	// �����ؼ��Ĵ���
	KickUnlock();

    // ���� SRIO SerDes ʱ�ӣ�250Mhz x 5 = 1.25GHz��
    SRIOSerDesPLLSet(0x29);

    // ���� SRIO SerDes ���� / ����
    // ������ 5Gbps��8B/10B��
    SRIOSerDesTxSet(0, 0x001C8F95);
    SRIOSerDesTxSet(1, 0x001C8F95);
    SRIOSerDesTxSet(2, 0x001C8F95);
    SRIOSerDesTxSet(3, 0x001C8F95);

    SRIOSerDesRxSet(0, 0x00468495);
    SRIOSerDesRxSet(1, 0x00468495);
    SRIOSerDesRxSet(2, 0x00468495);
    SRIOSerDesRxSet(3, 0x00468495);

    // �ȴ� SRIO SerDes ����
    while(!(SRIOSerDesPLLStatus() & 0x1));

	// �����ؼ��Ĵ���
	KickLock();

    // �����豸��Ϣ
   	SRIODeviceInfoSet(0, DEVICE_VENDOR_ID, DEVICE_REVISION);

    // ������֯��Ϣ
   	SRIOAssemblyInfoSet(DEVICE_ASSEMBLY_ID, DEVICE_ASSEMBLY_VENDOR_ID, DEVICE_ASSEMBLY_REVISION, DEVICE_ASSEMBLY_INFO);

   	// PE ��������
    SRIOProcessingElementFeaturesSet(0x20000199);

    // ����Դ��Ŀ�����
    SRIODestinationOperationsSet(0x0004FDF4);
    SRIODestinationOperationsSet(0x0000FC04);

    // ���� SRIO �豸 ID
    SRIODeviceIDSet(0xff, 0xffff);

    // ���� TLM ����·����Ϣ
    SRIOTLMPortBaseRoutingSet(SRIO_Port0, 1, SRIO_Enable, SRIO_Enable, SRIO_Disable);
    SRIOTLMPortBaseRoutingPatternMatchSet(SRIO_Port0, 1, DEVICE_ID1_16BIT, 0xFFFF);

    // ���ö˿� PLM
	// ���� PLM �˿� Silence Timer
	SRIOPLMPortSilenceTimerSet(SRIO_Port0, 0x2);

	// ʹ�ܶ˿�
	SRIOInputPortEnable(SRIO_Port0);
	SRIOOutputPortEnable(SRIO_Port0);

	// ���� PLM �˿� Discovery Timer
	SRIOPLMPortDiscoveryTimerSet(SRIO_Port0, 0x2);

	// ���ö˿� Write Reception Capture
	SRIOPortWriteRxCapture(SRIO_Port0, 0x0);

    // ���ö˿����ӳ�ʱ
	SRIOPortLinkTimeoutSet(0x000FFF);

    // �˿� Master ʹ��4
	SRIOPortGeneralSet(SRIO_Enable, SRIO_Enable, SRIO_Disable);

    // ��� Sticky Register λ
	SRIORegisterResetControlClear();

	// ���ö˿�д ID
	SRIOPortWriteTargetDeviceID(0, 0, SRIO_ID_16Bit);

    // ��������������䵥Ԫ��MTU��
	SRIODataDtreamingLogicalLayerControl(64);

	// ���ö˿�·��ģʽ
	SRIOPLMPathModeControl(SRIO_Port0, SRIO_Mode4_1_4x);

    // ���� LLM Port IP Ԥ��Ƶ
	SRIOServerClockPortIPPrescalar(0x1F);

	// DoorBell �ж�����
	SRIODoorBellInterruptRoutingControl(SRIO_DoorBell_Dedicated_INT);

	// DoorBell �ж�·������
	SRIODoorBellInterruptConditionRoutingSet(SRIO_DoorBell0, SRIO_DoorBellInt0, SRIO_IntDst_0_16);
	SRIODoorBellInterruptConditionRoutingSet(SRIO_DoorBell0, SRIO_DoorBellInt1, SRIO_IntDst_0_16);
	SRIODoorBellInterruptConditionRoutingSet(SRIO_DoorBell0, SRIO_DoorBellInt2, SRIO_IntDst_0_16);
	SRIODoorBellInterruptConditionRoutingSet(SRIO_DoorBell0, SRIO_DoorBellInt3, SRIO_IntDst_0_16);

	// ʹ������
	SRIOPeripheralEnable();

    // �������
   	SRIOBootCompleteSet(SRIO_Enable);
}

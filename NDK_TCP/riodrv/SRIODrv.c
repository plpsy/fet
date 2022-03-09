/****************************************************************************/
/*                                                                          */
/* ���ݴ������ӿƼ����޹�˾                                                 */
/*                                                                          */
/* Copyright (C) 2014-2016 Guangzhou Tronlong Electronic Technology Co.,Ltd */
/*                                                                          */
/****************************************************************************/
/****************************************************************************/
/*                                                                          */
/*              SRIO Direct I/O(DoorBell)                                   */
/*                                                                          */
/*              2016��03��08��                                              */
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

#include <xdc/runtime/Error.h>
#include <xdc/runtime/System.h>
#include <xdc/runtime/Log.h>
#include <xdc/runtime/Diags.h>

// SYS/BIOS
#include <ti/sysbios/BIOS.h>

#include <ti/sysbios/knl/Task.h>

#include <ti/sysbios/family/c64p/hwi.h>
#include <ti/sysbios/family/c66/Cache.h>

#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Semaphore.h>

#include <xdc/runtime/IHeap.h>
#include <xdc/runtime/System.h>
#include <xdc/runtime/Memory.h>

#include <ti/sysbios/heaps/HeapBuf.h>
#include <ti/sysbios/heaps/HeapMem.h>

// ����������
#include "hw_types.h"

#include "soc_C665x.h"
#include "srio.h"

// TI C6000 DSP �������
#include <c6x.h>

// C ���Կ�
#include <stdio.h>
#include <string.h>

#include <usertype.h>
#include "../fet/mytypes.h"
#include "../fet/rfd.h"
#include "../fet/fetriod.h"

#include <ti/ndk/inc/netmain.h>

/****************************************************************************/
/*                                                                          */
/*              �궨��                                                      */
/*                                                                          */
/****************************************************************************/
// ����ϵ�
#define SW_BREAKPOINT   asm(" SWBP 0 ");

// CACHE_LINE_SIZE ��С
#define CACHE_LINE_SIZE 128

// ��������С 128KB
#define BUFFER_SIZE ((CACHE_LINE_SIZE / 4) * 1024)

/****************************************************************************/
/*                                                                          */
/*              ȫ�ֱ���                                                    */
/*                                                                          */
/****************************************************************************/
// ��������
UINT32 SRIOBuffer = 0x90000000;

// �豸 ID
const UINT32 DEVICE_ID1_16BIT = 0xffff;
const UINT32 DEVICE_ID1_8BIT  = 0xff;
const UINT32 DEVICE_ID2_16BIT = 0xffff;
const UINT32 DEVICE_ID2_8BIT  = 0xff;

// SRIO �����жϣ�DoorBell Interrupt����Ϣ����
typedef enum
{
    SRIO_DoorBell_Message_Notify00,
    SRIO_DoorBell_Message_Notify01,
    SRIO_DoorBell_Message_NRead_Finished,
    SRIO_DoorBell_Message_NWrite_Finished
} SRIODoorBellMessage;

// �ź���
Semaphore_Handle sem;
Semaphore_Handle sem1;

// DSP оƬ�ͺ�
DSPType DSPChipType;

/****************************************************************************/
/*                                                                          */
/*              ��������                                                    */
/*                                                                          */
/****************************************************************************/
extern void SRIOInit();

/****************************************************************************/
/*                                                                          */
/*              Ӳ���ж�                                                    */
/*                                                                          */
/****************************************************************************/
Void SRIODoorBellIsr(UArg arg)
{
	UINT32 status;

	status = SRIODoorBellInterruptGet(SRIO_DoorBell0);

	if( status  == (1 << SRIO_DoorBell_Message_Notify01))
	{
		// ��������жϱ�־λ
		SRIODoorBellInterruptClear(SRIO_DoorBell0, SRIO_DoorBell_Message_Notify01);

	    // �����ź���
	    Semaphore_post(sem1);
	}
	else if(status  == (1 << SRIO_DoorBell_Message_NWrite_Finished))
	{
		// ��������жϱ�־λ
		SRIODoorBellInterruptClear(SRIO_DoorBell0, SRIO_DoorBell_Message_NWrite_Finished);

		// �����ź���
		Semaphore_post(sem);
	}
}

/****************************************************************************/
/*                                                                          */
/*              ����                                                        */
/*                                                                          */
/****************************************************************************/
Void DoorBellIntTsk(UArg a0, UArg a1)
{
    // �����ź���
    Semaphore_pend(sem1, BIOS_WAIT_FOREVER);

	// �ȴ� LSU ����
	unsigned char LSU = 0;
	while(SRIOLSUFullCheck(LSU) != 0);

	// �����ж�֪ͨ
    Log_info0("Generating SRIO DoorBell Interrupt to Device 01......");
	SRIODoorBellInterruptGenerate(SRIO_Port0, LSU, SRIO_ID_16Bit, DEVICE_ID1_16BIT, SRIO_DoorBell_Message_Notify00);
}

Void smain1(UArg a0, UArg a1)
{
    // �����ź���
    Semaphore_pend(sem, BIOS_WAIT_FOREVER);

    Log_info0("Received data from Device 01");

	/* �������� */
	// ׼������
	Cache_inv((UINT32 *)SRIOBuffer, 4 * BUFFER_SIZE, Cache_Type_ALLD, TRUE);

    UINT32 i;
    for(i = 0; i < BUFFER_SIZE; i++)
    {
    	*(UINT32 *)(SRIOBuffer + i * 4) = i * 10;
    }

	Cache_wb((UINT32 *)SRIOBuffer, 4 * BUFFER_SIZE, Cache_Type_ALLD, TRUE);

	Log_info0("Send data to Device 01");
	Log_print2(Diags_INFO, "     Source Device ID 0x%04x Address = 0x%x", DEVICE_ID2_16BIT, SRIOBuffer);
	Log_print2(Diags_INFO, "Destination Device ID 0x%04x Address = 0x%x", DEVICE_ID1_16BIT, SRIOBuffer);

	// ʹ�õ� LSU ��Ԫ
	unsigned char LSU = 0;

	// ��������
	SRIOLSUConfig cfg;
	memset(&cfg, 0, sizeof(SRIOLSUConfig));

	cfg.Address.RapidIOAddress.LSB = SRIOBuffer;
	cfg.Address.DSPAddress = SRIOBuffer;
	cfg.ByteCount = BUFFER_SIZE * 4;
	cfg.ID.Port = SRIO_Port0;
	cfg.ID.Dest = DEVICE_ID1_16BIT;
	cfg.ID.Size = SRIO_ID_16Bit;
	cfg.PacketType = SRIO_NWrite;
	cfg.DoorBell.Enable = TRUE;
	cfg.DoorBell.Info = SRIO_DoorBell_Message_NWrite_Finished;

	// �ȴ� LSU ����
	while(SRIOLSUFullCheck(LSU) != 0);

	// ��������
	SRIODirectIOTransfer(LSU, &cfg);
}

int g_testOn = 0;
extern void QueryOrCfgIPAddr();
Void smain(UArg a0, UArg a1)
{
	UINT32 offset = 0;
	UINT32 data = 0;
	UINT32 isRead = 1;
	UINT32 ulDestID = 0xff;
	unsigned char uchHopCount = 0;

	QueryOrCfgIPAddr();

	while(1)
	{
		if(g_testOn)
		{
			if(isRead)
			{
				frdMaintRead(0, 0, ulDestID, uchHopCount, offset, ePRIORITY_M, &data, 1, 4, 0);
			}
			else
			{
				frdMaintWrite(0, 0, ulDestID, uchHopCount, offset, ePRIORITY_M, &data, 1, 4, 0);
			}
		}
		Task_sleep(1000);
	}
}

/****************************************************************************/
/*                                                                          */
/*              �����ʼ��                                                  */
/*                                                                          */
/****************************************************************************/
Void PeriphInit()
{
	// ������汾
	Version();

	// SRIO ��ʼ��
	Log_info0("Initializing SRIO Peripheral......");
	SRIOInit();

    // ���˿��Ƿ��������Զ��豸�������ӣ�
	Log_info0("Waiting for the other device ready......");
    while(SRIOPortOKCheck(SRIO_Port0) != TRUE);

	Log_info0("Port OK\n");
}

/****************************************************************************/
/*                                                                          */
/*              �̳߳�ʼ��                                                  */
/*                                                                          */
/****************************************************************************/
Void ThreadInit()
{
	/* Hwi �߳� */
    Hwi_Params hwiParams;

    Hwi_Params_init(&hwiParams);
    hwiParams.eventId = 20;
    Hwi_create(4, SRIODoorBellIsr, &hwiParams, NULL);

	/* Task �߳� */
    Task_create(smain, NULL, NULL);

    Task_create(DoorBellIntTsk, NULL, NULL);

	/* �ź��� */
    Semaphore_Params semParams;

    Semaphore_Params_init(&semParams);
    semParams.mode = Semaphore_Mode_BINARY;

    sem  = Semaphore_create(0, &semParams, NULL);
    sem1 = Semaphore_create(0, &semParams, NULL);
}

/****************************************************************************/
/*                                                                          */
/*              ������ʼ��                                                                                                                                                                                           */
/*                                                                          */
/****************************************************************************/
Int srioInit()
{
	Log_info0("SRIO Device 02 Test(with DoorBell Interrupt) Application......");

	//�ر�0xc0000000-0xc1000000����
	*(volatile UINT32*)(0x01848300) = 0;
	// �����ʼ��
	PeriphInit();

    // �̳߳�ʼ��
    ThreadInit();


    return(0);
}



// Local RIO Register Access
int frdReadRioReg(				UINT32 ulOffset,
								UINT32 ulSize,
								void* pData)
{
	UINT16 tmp16;
	UINT32 tmp32;
	switch (ulSize)
	{
	case 1:
		*(UINT8*)pData = *(volatile UINT8*)(0x290B000 + ulOffset);
		break;
	case 2:
		tmp16 = *(volatile UINT16*)(0x290B000 + ulOffset);
		*(UINT16*)pData = (tmp16);
		break;
	case 4:
		tmp32 = *(volatile UINT32*)(0x290B000 + ulOffset);
		*(UINT32*)pData = (tmp32);
		break;
	default:
		tmp32 = *(volatile UINT32*)(0x290B000 + ulOffset);
		*(UINT32*)pData = (tmp32);
	}

	return 0;
}
int frdWriteRioReg(				UINT32 ulOffset,
								UINT32 ulSize,
								void* pData)
{
	UINT16 tmp16;
	UINT32 tmp32;

	switch (ulSize)
	{
	case 1:
		*(volatile UINT8*)(0x290B000 + ulOffset) = *(UINT8*)pData;
		break;
	case 2:
		tmp16 = *(UINT16*)pData;
		*(volatile UINT16*)(0x290B000 + ulOffset) = (tmp16);
		break;
	case 4:
		tmp32 = *(UINT32*)pData;
		*(volatile UINT32*)(0x290B000 + ulOffset) = (tmp32);
		break;
	default:
		tmp32 = *(UINT32*)pData;
		*(volatile UINT32*)(0x290B000 + ulOffset) = (tmp32);
	}

	return 0;
}




int frdODB_Send(frdHandle hDBController,
								int bWait,								// Non-zero if requesting no return until Doorbell complete
								int nTimeoutMS,							// If waiting, how long to wait before timing out
								unsigned char bLargeTT,
								UINT32 ulDestID,
								ePRIORITY ePriority,
								unsigned short wInfoWord)
{

	return 0;
}


int frdODB_Busy(frdHandle hDBController)
{
	return 0;
}


int frdODB_Error(frdHandle hDBController)
{
	return 0;
}


int frdIDB_Busy(frdHandle hDBController)
{
	return 0;
}
int frdIDB_GetDoorbell(			frdHandle hDBController,
								unsigned short* pwSrcID,
								unsigned short* pwTgtID,
								unsigned short* pwInfo)
{
	return 0;
}

extern int rioHeap;
// RapidIO Transactions
int frdMaintRead(				frdHandle hMaintWindow,
								unsigned char bLargeTT,
								UINT32 ulDestID,
								unsigned char uchHopCount,
								UINT32 ulOffset,
								ePRIORITY ePriority,
								void* pData,
								UINT32 ulWordCount,
								UINT32 ulWordSize,
								UINT32 ulStride)
{
	// ʹ�õ� LSU ��Ԫ
	unsigned char LSU = 0;
    static UINT32* srioMaintReadBuffer = NULL;
    UINT32 uiCompletionCode = 0;
    IHeap_Handle heap = (IHeap_Handle)HeapBuf_Handle_upCast(rioHeap);

    if(NULL == srioMaintReadBuffer)
    {
    	srioMaintReadBuffer = Memory_alloc(heap, 128, 128, NULL);
    }

	// �ȴ� LSU ����
	while(SRIOLSUFullCheck(LSU) != 0);

	// ��������
	SRIOLSUConfig cfg;
	memset(&cfg, 0, sizeof(SRIOLSUConfig));

	cfg.Address.RapidIOAddress.LSB = ulOffset;
	cfg.Address.DSPAddress = (UINT32)srioMaintReadBuffer;
	cfg.Priority = ePriority;
	cfg.HopCount = uchHopCount;
	cfg.ByteCount = 4;
	cfg.ID.Port = SRIO_Port0;
	cfg.ID.Dest = (UINT16)ulDestID;
	cfg.ID.Size = bLargeTT ? SRIO_ID_16Bit : SRIO_ID_8Bit;
	cfg.PacketType = SRIO_MAINTRead;
	cfg.DoorBell.Enable = FALSE;
	cfg.DoorBell.Info = 0;


	// ��������
	SRIODirectIOTransfer(LSU, &cfg);

	uiCompletionCode = SRIOLSUStatusGet(LSU, &cfg);

//	Cache_inv((UINT32 *)srioMaintReadBuffer, 128, Cache_Type_ALLD, TRUE);

	*(UINT32*)pData = htonl(*srioMaintReadBuffer);

	return uiCompletionCode;
}


int frdMaintWrite(				frdHandle hMaintWindow,
								unsigned char bLargeTT,
								UINT32 ulDestID,
								unsigned char uchHopCount,
								UINT32 ulOffset,
								ePRIORITY ePriority,
								void* pData,
								UINT32 ulWordCount,
								UINT32 ulWordSize,
								UINT32 ulStride)
{
	// ʹ�õ� LSU ��Ԫ
	unsigned char LSU = 0;
    static UINT32* srioMaintWriteBuffer = NULL;
    UINT32 uiCompletionCode = 0;
    IHeap_Handle heap = (IHeap_Handle)HeapBuf_Handle_upCast(rioHeap);

    if(NULL == srioMaintWriteBuffer)
    {
    	srioMaintWriteBuffer = Memory_alloc(heap, 128, 128, NULL);
    }


    *srioMaintWriteBuffer = ntohl(*(UINT32*)pData);

//	Cache_wb((UINT32 *)srioMaintWriteBuffer, 128, Cache_Type_ALLD, TRUE);

	// �ȴ� LSU ����
	while(SRIOLSUFullCheck(LSU) != 0);

	// ��������
	SRIOLSUConfig cfg;
	memset(&cfg, 0, sizeof(SRIOLSUConfig));

	cfg.Address.RapidIOAddress.LSB = ulOffset;
	cfg.Address.DSPAddress = (UINT32)srioMaintWriteBuffer;
	cfg.Priority = ePriority;
	cfg.HopCount = uchHopCount;
	cfg.ByteCount = 4;
	cfg.ID.Port = SRIO_Port0;
	cfg.ID.Dest = (UINT16)ulDestID;
	cfg.ID.Size = bLargeTT ? SRIO_ID_16Bit : SRIO_ID_8Bit;
	cfg.PacketType = SRIO_MAINTWrite;
	cfg.DoorBell.Enable = FALSE;
	cfg.DoorBell.Info = 0;

	// ��������
	SRIODirectIOTransfer(LSU, &cfg);

	uiCompletionCode = SRIOLSUStatusGet(LSU, &cfg);

	return uiCompletionCode;
}


int frdRioRead(					frdHandle hWindow,
								unsigned char bLargeTT,
								UINT32 ulDestID,
								UINT32 ulRioAddressHi,
								UINT32 ulRioAddressLo,
								eRTYPE eReadType,
								ePRIORITY ePriority,
								unsigned char* puchData,
								UINT32 ulByteCount)
{

	// ʹ�õ� LSU ��Ԫ
	unsigned char LSU = 0;
    static UINT32* srioReadBuffer = NULL;
    UINT32 uiCompletionCode = 0;
    IHeap_Handle heap = HeapBuf_Handle_upCast(rioHeap);

    if(NULL == srioReadBuffer)
    {
    	srioReadBuffer = Memory_alloc(heap, 0x100000, 0x1000, NULL);
    }

    if(ulByteCount > 0x100000)
    {
    	return -1;
    }

	// �ȴ� LSU ����
	while(SRIOLSUFullCheck(LSU) != 0);

	// ��������
	SRIOLSUConfig cfg;
	memset(&cfg, 0, sizeof(SRIOLSUConfig));

	cfg.Address.RapidIOAddress.MSB = ulRioAddressHi;
	cfg.Address.RapidIOAddress.LSB = ulRioAddressLo;
	cfg.Address.DSPAddress = (UINT32)srioReadBuffer;
	cfg.Priority = ePriority;
	cfg.HopCount = 0xff;
	cfg.ByteCount = ulByteCount;
	cfg.ID.Port = SRIO_Port0;
	cfg.ID.Dest = (UINT16)ulDestID;
	cfg.ID.Size = bLargeTT ? SRIO_ID_16Bit : SRIO_ID_8Bit;
	cfg.PacketType = SRIO_NRead;
	cfg.DoorBell.Enable = FALSE;
	cfg.DoorBell.Info = 0;

	// ��������
	SRIODirectIOTransfer(LSU, &cfg);

	uiCompletionCode = SRIOLSUStatusGet(LSU, &cfg);

//	Cache_inv((UINT32 *)srioReadBuffer, ulByteCount, Cache_Type_ALLD, TRUE);
    memcpy(puchData, srioReadBuffer, ulByteCount);

	return uiCompletionCode;
}


int frdRioWrite(				frdHandle hWindow,
								unsigned char bLargeTT,
								UINT32 ulDestID,
								UINT32 ulRioAddressHi,
								UINT32 ulRioAddressLo,
								eWTYPE eWriteType,
								ePRIORITY ePriority,
								unsigned char* puchData,
								UINT32 ulByteCount)
{


	// ʹ�õ� LSU ��Ԫ
	unsigned char LSU = 0;
    static UINT32* srioWriteBuffer = NULL;
    UINT32 uiCompletionCode = 0;
    IHeap_Handle heap = HeapBuf_Handle_upCast(rioHeap);

    if(NULL == srioWriteBuffer)
    {
    	srioWriteBuffer = Memory_alloc(heap, 0x100000, 0x1000, NULL);
    }

    if(ulByteCount > 0x100000)
    {
    	return -1;
    }
    memcpy(srioWriteBuffer, puchData, ulByteCount);
//	Cache_wb((UINT32 *)srioWriteBuffer, ulByteCount, Cache_Type_ALLD, TRUE);

	// �ȴ� LSU ����
	while(SRIOLSUFullCheck(LSU) != 0);

	// ��������
	SRIOLSUConfig cfg;
	memset(&cfg, 0, sizeof(SRIOLSUConfig));

	cfg.Address.RapidIOAddress.MSB = ulRioAddressHi;
	cfg.Address.RapidIOAddress.LSB = ulRioAddressLo;
	cfg.Address.DSPAddress = (UINT32)srioWriteBuffer;
	cfg.Priority = ePriority;
	cfg.HopCount = 0xff;
	cfg.ByteCount = ulByteCount;
	cfg.ID.Port = SRIO_Port0;
	cfg.ID.Dest = (UINT16)ulDestID;
	cfg.ID.Size = bLargeTT ? SRIO_ID_16Bit : SRIO_ID_8Bit;
	cfg.PacketType = SRIO_NWrite;
	cfg.DoorBell.Enable = FALSE;
	cfg.DoorBell.Info = 0;

	// ��������
	SRIODirectIOTransfer(LSU, &cfg);
	uiCompletionCode = SRIOLSUStatusGet(LSU, &cfg);

	return uiCompletionCode;
}

// Error Management
UINT32 frdGetRioError(	UINT32* pulRioErrorMask,
								UINT32* apulRioErrorInfo[],
								char* apszRioErrorString[])
{

	return 0;
}


void frdClearRioError(void)
{


}

// Local Memory Access
int frdReadLocalMemory(			UINT32 ulAddrP,
								void* pData,
								UINT32 ulWordCount,
								UINT32 ulWordSize)
{

	return 0;
}


int frdWriteLocalMemory(		UINT32 ulAddrP,
								void* pData,
								UINT32 ulWordCount,
								UINT32 ulWordSize)
{


	return 0;
}



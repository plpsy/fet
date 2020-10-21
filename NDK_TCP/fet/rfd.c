///////////////////////////////////////////////////////////////////////////////
// $RCSfile: rfd.c,v $
// $Date: 2008/09/15 18:50:56 $
// $Author: Gilmour $
// $Revision: 1.11 $
// $Source: /cvsrepo/FET/Server/common/PQIII/rfd.c,v $
//
// Copyright (c) 2004-2008 Fabric Embedded Tools Corporation
//
///////////////////////////////////////////////////////////////////////////////

#include <usertype.h>
#include <serrno.h>
#include <stdlib.h>
#include <stdio.h>

#include <errno.h>
#include <string.h>

#include "mytypes.h"
#include "rfd.h"
#include "fetriod.h"
/* JuWi Added includ of MsgDbMon.h. */
/*#include "MsgDbMon.h"  sdr sealed 20081027*/

#define false 0
#define true 1
#define MAINTWINDOWSIZE	0x1000		// 4 kByte
#define READWRITEWINDOWSIZE 0x1000	// 4 kByte

#define PMC0				0x21018

// LOCAL PROCESSOR CLOCK SPEED //
unsigned long g_ulClockMHz	= 1000;//266 sdr 

// PRIVATE FUNCTIONS
static void DetermineClockSpeed(void);

// Configuration, Control and Status Register (CCSR) defines
#define PHYS_CCSRBASE	0xef000000		// Physical base address of the beginning of CCSR memory. sdr
#define CCSRSIZE		0x100000		// size of CCSR memory block

// Globals
unsigned long g_ulRFDVERSION = MAKEVERSION(2,8,1);
int g_bOpen = true;
static frdHandle g_hWinMaint;
static unsigned long* g_pulMaintWindow;
static frdHandle g_hMsgController;
static frdHandle g_hDBController;

static frdHandle g_hWinRW;
static unsigned long* g_pulRWWindow;

// handles used for data writes
static frdHandle g_hMemBuf_IO;						// handle for buffer memory
static void* g_pMemBuf;								// virtual address of contiguous memory that is DMA'd to/from
static frdHandle g_hDMA_IO;							// handle for DMA engine
static frdHandle g_hChain_IO;						// handle for DMA chain

#if 0
RFDSTATUS RFD_Open(void)
{
	int nRet;
	unsigned long ulSize;

        /* JuWi Removed unused variable. */
	/* RFDSTATUS rfds; */

	nRet = frdOpen(PHYS_CCSRBASE);
//	printf("frdOpen() returned %d\n", nRet);
	if (nRet < 0)
	{
		perror("Couldn't open FETrioD!");
		return eRFSRFD_ERR;
	}
	
	/* Map in a maintenance window. */ 
	ulSize = MAINTWINDOWSIZE;
	nRet = frdAllocOutboundWindow(&ulSize, &g_hWinMaint);
	printf("frdAllocOutboundWindow returned %d.  g_hWinMaint is %d, size is %x\n",
		nRet, g_hWinMaint, ulSize);
	if (nRet < 0)
	{
		perror("Couldn't map in maintenance window!");
	 	return eRFSRFD_ERR;
	}

	// get a virtual memory pointer to the mapped maintenance window
	g_pulMaintWindow = (unsigned long*)frdGetAddressV(g_hWinMaint);
	if (g_pulMaintWindow == NULL)
	{
		perror("Couldn't get address of maintenance window");
		return eRFSRFD_ERR;
	}

//===========================================

	/* Map in a window to use for various types of RIO reads and writes. */ 
	ulSize = READWRITEWINDOWSIZE;
	nRet = frdAllocOutboundWindow(&ulSize, &g_hWinRW);
	printf("frdAllocOutboundWindow returned %d.  g_hWinRW is %d, size is %x\n",
		nRet, g_hWinRW, ulSize);
	if (nRet < 0)
	{
		perror("Couldn't map in RW window!");
	 	return eRFSRFD_ERR;
	}

	// get a virtual memory pointer to the mapped RW window
	g_pulRWWindow = (unsigned long*)frdGetAddressV(g_hWinRW);
	if (g_pulRWWindow == NULL)
	{
		perror("Couldn't get address of RW window");
		return eRFSRFD_ERR;
	}

//==========================================

	// get an outbound message controller
	nRet = frdOMSG_Open(&g_hMsgController, OMSG_DIRECTMODE, 0, 4096);
	if (nRet < 0)
	{
		perror("Couldn't get outbound message controller handle!");
		return eRFSRFD_ERR;
	}

//==========================================

	// get an outbound doorbell controller
	nRet = frdODB_Open(&g_hDBController);
	if (nRet < 0)
	{
		perror("Couldn't get doorbell controller handle!");
		return eRFSRFD_ERR;
	}

//==========================================

	// get a DMA controller for data writes
	nRet = frdDMA_Open(&g_hDMA_IO);
	if (nRet != 0) 
	{
		perror("Couldn't get IO DMA controller handle!");
		return eRFSRFD_ERR;
	}
	printf("IO DMA handle is %d\n", g_hDMA_IO);

	////////////////////////////////////////////////
	// Allocate some real memory to DMA out of... //
	////////////////////////////////////////////////

	nRet = frdAllocMem(READWRITEWINDOWSIZE, &g_hMemBuf_IO);
	if (nRet != 0)
	{
	//sdr sealed	fprintf(stderr, "Could not allocate IO DMA source memory.  Error %d\n", errno);
		return eRFSRFD_ERR;
	}
	printf("Allocated %d bytes for local IO DMA source data.\n", READWRITEWINDOWSIZE);
//	printf("	Handle is %d\n", g_hMemBuf_IO);

	// get virtual address of buffer
	g_pMemBuf = (void*)frdGetAddressV(g_hMemBuf_IO);
	if (g_pMemBuf == NULL)
	{
	//sdr sealed	fprintf(stderr, "Could not determine virtual address of local DMA source memory.  Error %d\n", errno);
		return eRFSRFD_ERR;
	}
	printf("	Virtual Memory Address is  %08lx\n", g_pMemBuf);

	////////////////////////////////////////////////
	// allocate a DMA chain
	////////////////////////////////////////////////

	nRet = frdDMA_AllocChain(&g_hChain_IO, 1);
	if (nRet != 0)
	{
	//sdr sealed	perror("Unable to allocate DMA chain.");fflush(stderr);
		return eRFSRFD_ERR;
	}

//==========================================

	printf("RFD: RapidFET Driver V%d.%d using FETRIOD is open\n", MAJORVERSION(g_ulRFDVERSION), MINORVERSION(g_ulRFDVERSION));fflush(stdout);
	g_bOpen = true;

	return eRFSRFD_SUCCESS; 
}

RFDSTATUS RFD_Close(void)
{
	if (!g_bOpen)
	{
		return eRFSRFD_ERR_NOTOPEN;
	}

	frdOMSG_Close(g_hMsgController);
	frdODB_Close(g_hDBController);

	frdClose();
	g_bOpen = false;
	return eRFSRFD_SUCCESS;
}
#endif

RFDSTATUS RFD_Read(UINT32 ulOffset, UINT32 ulSize, void* pData)
{
	int nRet = 0;

	if (!g_bOpen)
	{
		return eRFSRFD_ERR_NOTOPEN;
	}

	nRet = frdReadRioReg(ulOffset, ulSize, pData);
	if (nRet != 0)
	{
		perror("frdReadRioReg failed");
	 	return eRFSRFD_ERR;
	}

	switch (ulSize)
	{
	case 1:
		printf("RFD: R Off:0x%lx Val: 0x%02x\n", ulOffset, *(unsigned char*)pData);
		break;
	case 2:
		printf("RFD: R Off:0x%lx Val: 0x%04x\n", ulOffset, *(unsigned short*)pData);
		break;
	case 4:
		printf("RFD: R Off:0x%lx Val: 0x%08x\n", ulOffset, *(unsigned long*)pData);
		break;
	}

	return eRFSRFD_SUCCESS;
}

RFDSTATUS RFD_Write(UINT32 ulOffset, UINT32 ulSize, void* pData)
{
	int nRet = 0;
	
	if (!g_bOpen)
	{
		return eRFSRFD_ERR_NOTOPEN;
	}

	nRet = frdWriteRioReg(ulOffset, ulSize, pData);

	if (nRet != 0)
	{
		perror("frdWriteRioReg failed");
	 	return eRFSRFD_ERR;
	}

	switch (ulSize)
	{
	case 1:
		printf("RFD: W Off:0x%lx Val: 0x%02x\n", ulOffset, *(unsigned char*)pData);
		break;
	case 2:
		printf("RFD: W Off:0x%lx Val: 0x%04x\n", ulOffset, *(unsigned short*)pData);
		break;
	case 4:
		printf("RFD: W Off:0x%lx Val: 0x%08x\n", ulOffset, *(unsigned long*)pData);
		break;
	}
	return eRFSRFD_SUCCESS;
}

RFDSTATUS RFD_MaintRead(unsigned char bLargeTT, 
						UINT32 ulDestID, 
						unsigned char uchHopCount, 
						UINT32 ulOffset, 
						void *pData, 
						UINT32 ulWordCount, 
						UINT32 ulWordSize, 
						UINT32 ulStride, 
						UINT32 ulPriority) 
{
	int nRet = 0;

	nRet = frdMaintRead(g_hWinMaint, bLargeTT, ulDestID, uchHopCount, ulOffset, ulPriority, pData, ulWordCount, ulWordSize, ulStride);
	if (nRet < 0)
	{
		perror("frdMaintRead failed");
		switch (errno)
		{
//		case EAGAIN:
//			return eRFSRFD_ERR_RIOOPEN;
		case EINVAL:
			return eRFSRFD_ERR_INVALID;
//		case EIO:
//			return eRFSRFD_ERR_RIO;
		default:
            return eRFSRFD_ERR;
		}
	}

	printf("RFD: MR %cDev:%04x HC:%d WC:%d Off:0x%lx STR:%d ", bLargeTT?'L':'S', ulDestID, uchHopCount, ulWordCount, ulOffset, ulStride);fflush(stdout);
	switch (ulWordSize)
	{
	case 1:
		printf("Data:%02lx %s\n", *(unsigned char*)pData, ulWordCount>1?"...":"");
		break;
	case 2:
		printf("Data:%04lx %s\n", *(unsigned short*)pData, ulWordCount>1?"...":"");
		break;
	case 4:
		printf("Data:%08lx %s\n", *(UINT32*)pData, ulWordCount>1?"...":"");
		break;
	}

	return eRFSRFD_SUCCESS; 
}

RFDSTATUS RFD_MaintWrite(unsigned char bLargeTT, 
						UINT32 ulDestID, 
						unsigned char uchHopCount, 
						UINT32 ulOffset, 
						void *pData, 
						UINT32 ulWordCount, 
						UINT32 ulWordSize, 
						UINT32 ulStride, 
						UINT32 ulPriority) 
{
	int nRet = 0;

	nRet = frdMaintWrite(g_hWinMaint, bLargeTT, ulDestID, uchHopCount, ulOffset, ulPriority, pData, ulWordCount, ulWordSize, ulStride);
	if (nRet < 0)
	{
		perror("frdMaintWrite failed");
		return eRFSRFD_ERR;
	}

	printf("RFD: MW %cDev:%04x HC:%d WC:%d Off:0x%lx STR:%d ", bLargeTT?'L':'S', ulDestID, uchHopCount, ulWordCount, ulOffset, ulStride);fflush(stdout);
	switch (ulWordSize)
	{
	case 1:
		printf("Data:%02lx %s\n", *(unsigned char*)pData, ulWordCount>1?"...":"");
		break;
	case 2:
		printf("Data:%04lx %s\n", *(unsigned short*)pData, ulWordCount>1?"...":"");
		break;
	case 4:
		printf("Data:%08lx %s\n", *(UINT32*)pData, ulWordCount>1?"...":"");
		break;
	}

	return eRFSRFD_SUCCESS; 
}

RFDSTATUS RFD_GetRioError(UINT32* pulRioErrorMask, UINT32* apulRioErrorInfo[], char* apszErrorString[]) 
{
	return 0;
}


UINT32 RFD_GetVersion()
{
	return g_ulRFDVERSION;
}

UINT32 RFD_GetTimeMarker(void)
{
	return 0;
}

UINT32 RFD_TimeSpanUS(UINT32 ulStartTimeMarker, UINT32 ulStopTimeMarker)
{
	UINT32 ulRet;
	static int bCalibrated=0;

	if (!bCalibrated)
	{
		DetermineClockSpeed();
		bCalibrated = 1;
	}

	if (ulStopTimeMarker < ulStartTimeMarker)	// handle cycle count roll-over
	{
		ulRet = (0xffffffff-ulStartTimeMarker+ulStopTimeMarker+1)/(g_ulClockMHz);
	}
	else
	{
		ulRet = (ulStopTimeMarker-ulStartTimeMarker)/(g_ulClockMHz);
	}

//	printf("The Delta is %d\n", ulRet);
	return ulRet;
}

static void DetermineClockSpeed(void)
{

	g_ulClockMHz = 266;  // approximate

}


#if 0
static void DetermineClockSpeed(void)
{
	struct timeval t0;
        /* JuWi Removed unused variables. */
	/* long todStart; */
	/* long todStop; */
	/* long todDelta; */
	int retval;
	int n;
	int nS, nUS;
	unsigned long ulPerfStart;
	unsigned long ulPerfStop;
	unsigned long ulPerfDelta;

	printf("Determining clock speed...");
	ulPerfStart = RFD_GetTimeMarker();
	//retval = gettimeofday(&t0, NULL); sdr  sealed 20081027
	nUS = t0.tv_usec + 100000;
	if (nUS > 1000000)
	{
		nS = t0.tv_sec + 1;
		nUS = nUS - 1000000;
	}
	else
	{
		nS = t0.tv_sec;
	}
	n=0;
	while(1)
	{
		ulPerfStop = RFD_GetTimeMarker();
		//retval = gettimeofday(&t0, NULL); sdr  sealed 20081027
		if ((t0.tv_sec == nS) && (t0.tv_usec >= nUS))
		{
			break;
		}
		n++;
	}

	ulPerfDelta = ulPerfStop-ulPerfStart;

	if(ulPerfDelta > 30000000)
	{
		printf("Delta = %lu cycles per sec => 333MHz clock \n",ulPerfDelta*10);
		g_ulClockMHz = 333;  // approximate
	}
	else
	{
		printf("delta = %lu cycles per sec => 266MHz clock \n",ulPerfDelta*10);
		g_ulClockMHz = 266;  // approximate
	}
}
#endif



RFDSTATUS RFD_RioRead(unsigned char bLargeTT, 
						UINT32 ulDestID,
						UINT32 ulRioAddressHi,
						UINT32 ulRioAddressLo,
						UINT32 ulXferType,
						UINT32 ulPriority,
						unsigned char *puchData,
						UINT32 ulByteCount)
{
	int nRet;
	ePRIORITY ePriority;
	eWTYPE eReadType;
	unsigned char *puchDst;
	unsigned long ulXferByteCount;
        /* JuWi Removed unused variable. */
        /* unsigned char *puchSrc; */
	UINT32 ulBytesToGo;
	UINT32 ulSaveRioAddressHi = ulRioAddressHi;
	UINT32 ulSaveRioAddressLo = ulRioAddressLo;

	switch (ulPriority)
	{
	case 0:
		ePriority = ePRIORITY_L;
//		printf("L");
		break;
	case 1:
		ePriority = ePRIORITY_M;
//		printf("M");
		break;
	case 2:
		ePriority = ePRIORITY_H;
//		printf("H");
		break;
	default:
		ePriority = ePRIORITY_L;
//		printf("L");
		break;
	}

	switch(ulXferType)
	{
		// no support for other types right now
	default:
		eReadType = eRTYPE_NREAD;
//		printf("N");
		break;
	}

	return eRFSRFD_SUCCESS; 
}

RFDSTATUS RFD_RioWrite(unsigned char bLargeTT, 
						UINT32 ulDestID,
						UINT32 ulRioAddressHi,
						UINT32 ulRioAddressLo,
						UINT32 ulXferType,
						UINT32 ulPriority,
						unsigned char *puchData,
						UINT32 ulByteCount)
{
	int nRet;
	ePRIORITY ePriority;
	eWTYPE eWriteType;
        /* JuWi Removed unused variables. */
	/* RFDSTATUS rfds; */
	/* int bNoTimer=0; */
	/* int nQueueLen = 0; */
	unsigned long ulXferByteCount;
	unsigned char *puchSrc;
	UINT32 ulBytesToGo;
	UINT32 ulSaveRioAddressHi = ulRioAddressHi;
	UINT32 ulSaveRioAddressLo = ulRioAddressLo;

	switch (ulPriority)
	{
	case 0:
		ePriority = ePRIORITY_L;
//		printf("L");
		break;
	case 1:
		ePriority = ePRIORITY_M;
//		printf("M");
		break;
	case 2:
		ePriority = ePRIORITY_H;
//		printf("H");
		break;
	default:
		ePriority = ePRIORITY_L;
//		printf("L");
		break;
	}

	switch(ulXferType)
	{
	case 0:
		eWriteType = eWTYPE_SWRITE;
//		printf("S");
		break;
	case 1:
		eWriteType = eWTYPE_NWRITE;
//		printf("N");
		break;
	case 2:
		eWriteType = eWTYPE_NWRITE_R;
//		printf("NR");
		break;
	default:
		eWriteType = eWTYPE_NWRITE;
//		printf("N");
		break;
	}

	return eRFSRFD_SUCCESS; 
}

RFDSTATUS RFD_RioMessageSend(
						unsigned char bLargeTT, 
						UINT32 ulDestID,
						UINT32 ulPriority, 
						UINT32 bMultiPacket,
						UINT32 ulMailboxNum,
						unsigned char *puchData,
						UINT32 ulPacketByteCount,
						UINT32 ulTotalByteCount)
{
	int nRet;
	ePRIORITY ePriority;

	switch (ulPriority)
	{
	case 0:
		ePriority = ePRIORITY_L;
		printf("L");
		break;
	case 1:
		ePriority = ePRIORITY_M;
		printf("M");
		break;
	case 2:
		ePriority = ePRIORITY_H;
		printf("H");
		break;
	default:
		ePriority = ePRIORITY_L;
		printf("L");
		break;
	}

	printf("RFD: TxMsg Q:%d %cDev:%04x %s %d %d %d FirstByte:%02x ",
		g_hMsgController,
		bLargeTT?'L':'S',
		ulDestID, 
		bMultiPacket?"Multi-packet":"Single Packet",
		ulMailboxNum,
		ulPacketByteCount,
		ulTotalByteCount,
		*puchData);

	return eRFSRFD_SUCCESS; 
}

RFDSTATUS RFD_RioFlushMessages(UINT32 ulWhichChannel)
{
	printf("RFD: RxMsg Flush Q:%d\n", ulWhichChannel);
//	MessageRxFlush(ulWhichChannel);
	return eRFSRFD_SUCCESS;
}

RFDSTATUS RFD_RioMessageGet(
						UINT32 ulWhichChannel,
						unsigned char* pbLargeTT, 
						UINT32* pulSourceID,
						UINT32* pulPriority, 
						UINT32* pulMailboxNum,
						UINT32* pulMessageLength,
						unsigned char* pbOverflowed,
						unsigned char *puchData)
{
	RFDSTATUS ret;
#if 0
	int nStatus = MessageRxGet(	ulWhichChannel,
                                        pbLargeTT,
                                        /* JuWi Added some type casting */
                                        (unsigned long*)pulSourceID,
                                        (unsigned long*)pulPriority, 
                                        (unsigned long*)pulMailboxNum,
                                        (unsigned long*)pulMessageLength,
                                        pbOverflowed,
                                        puchData);
#endif
    int nStatus = 0;
	switch (nStatus)
	{
		case 0:		// no error
			printf("RFD: RxMsg Q:%d %cDev:%04x Src:%d MBox:%d Len:%d FirstByte:%02x\n",
				ulWhichChannel,
				*pbLargeTT?'L':'S',
				*pulSourceID, 
				*pulMailboxNum,
				*pulMessageLength,
				*puchData);
			return eRFSRFD_SUCCESS;
		case -1:	// no input messages available
			printf("RFD: RxMsg Q:%d is empty.\n", ulWhichChannel);
			return eRFSRFD_SUCCESS;
		case -2:	// invalid parameter specified
			ret = eRFSRFD_ERR_INVALID;
			printf("RFD: RxMsg q:%d failed - invalid parameter\n");
			break;
		default:
			ret = eRFSRFD_ERR;
			printf("RFD: RxMsg q:%d failed - unknown error\n");
			break;
	}

	printf("RFD: RxMsg Error:%d\n", ret);

	return ret;			
}

RFDSTATUS RFD_RioDoorbellSend(
						unsigned char bLargeTT, 
						UINT32 ulDestID,
						UINT32 ulPriority, 
						UINT16 wInfoWord)
{
	int nRet;
	ePRIORITY ePriority;

	printf("RFD: TxDB %cDev:%04x Info:%04x Pri: ",
			bLargeTT?'L':'S',
			ulDestID, 
			wInfoWord);

	if (!g_bOpen)
	{
		printf("Failed\n	RFD not open\n");
		return eRFSRFD_ERR_RIOOPEN;
	}

	switch (ulPriority)
	{
	case 0:
		ePriority = ePRIORITY_L;
		printf("L ");
		break;
	case 1:
		ePriority = ePRIORITY_M;
		printf("M ");
		break;
	case 2:
		ePriority = ePRIORITY_H;
		printf("H ");
		break;
	default:
		ePriority = ePRIORITY_L;
		printf("L ");
		break;
	}

	if (ulDestID > 0xffff)
	{
		printf("Failed\n	Dest ID %08lx is too big\n", ulDestID);
		return eRFSRFD_ERR_INVALID;
	}

	return eRFSRFD_SUCCESS;
}

RFDSTATUS RFD_RioFlushDoorbells(UINT32 ulWhichChannel)
{
	printf("RFD: RxDB Flush Q:%d\n", ulWhichChannel);
	return eRFSRFD_SUCCESS;
}

RFDSTATUS RFD_RioDoorbellGet(
						UINT32 ulWhichChannel,
						unsigned char* pbValidDoorbell, 
						unsigned char* pbOverflowed,
						UINT16* pwInfoWord)
{
	RFDSTATUS ret;
//	int nStatus = DoorbellRxGet(ulWhichChannel, pwInfoWord, pbOverflowed, pbValidDoorbell);
	int nStatus = 0;
	switch (nStatus)
	{
		case 0:		// no error
			printf("RFD: RxDB Q:%d Info:%04x Valid:%c Overflow:%c\n",
				ulWhichChannel,
				*pwInfoWord,
				*pbValidDoorbell?'Y':'N',
				*pbOverflowed?'Y':'N'); 
			return eRFSRFD_SUCCESS;
		case -1:	// no input doorbells available
			printf("RFD: RxDB Q:%d is empty.\n", ulWhichChannel);
			return eRFSRFD_SUCCESS;
		case -2:	// invalid parameter specified
			printf("RFD: RxDB Q:%d failed - invalid parameter\n", ulWhichChannel);
			ret = eRFSRFD_ERR_INVALID;
			break;
		default:
			printf("RFD: RxDB Q:%d failed - unknown error\n", ulWhichChannel);
			ret = eRFSRFD_ERR;
	}

	return ret;			
}

RFDSTATUS RFD_ReadMem(UINT32 ulAddressHi, 
					  UINT32 ulAddressLo,
					  void *pData,
                      UINT32 ulWordCount, 
                      UINT32 ulWordSize)
{
//	int nRet = frdReadLocalMemory(ulAddressLo, pData, ulWordCount, ulWordSize);
	int nRet = 0;
	if (nRet < 0)
	{
		perror("frdReadLocalMemory failed");
		switch (errno)
		{
		case EINVAL:
			return eRFSRFD_ERR_INVALID;
		default:
			return eRFSRFD_ERR;
		}
	}
	return eRFSRFD_SUCCESS; 
}

RFDSTATUS RFD_WriteMem(UINT32 ulAddressHi,
					   UINT32 ulAddressLo,
                       void *pData,
                       UINT32 ulWordCount, 
                       UINT32 ulWordSize)
{
//	int nRet = frdWriteLocalMemory(ulAddressLo, pData, ulWordCount, ulWordSize);
	int nRet = 0;
	if (nRet < 0)
	{
		perror("frdWriteLocalMemory failed");
		switch (errno)
		{
		case EINVAL:
			return eRFSRFD_ERR_INVALID;
		default:
			return eRFSRFD_ERR;
		}
	}
	return eRFSRFD_SUCCESS; 
}





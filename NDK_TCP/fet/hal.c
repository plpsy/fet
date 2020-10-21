///////////////////////////////////////////////////////////////////////////////
// $RCSfile: hal.c,v $
// $Date: 2008/06/16 19:02:05 $
// $Author: Gilmour $
// $Revision: 1.30 $
// $Source: /cvsrepo/FET/Server/hal.c,v $
//
// Copyright (c) 2004-2007 Fabric Embedded Tools Corporation
//
///////////////////////////////////////////////////////////////////////////////
#include <usertype.h>
#include "mytypes.h"
#include "rio.h"
/* JuWi Changed include path. */
/* #include "include/feterr.h" */ 
#include "feterr.h"
#include "fethal.h"
/* JuWi Changed include path. */
/* #include "include/haldefs.h" */
//#include "haldefs.h"
#include <stdio.h>

#define HOST_REGS 0
#define ZERO_HOP 0

FETSTATUS RFDSTATUS_TO_FETSTATUS(RFDSTATUS rfds)
{
	switch (rfds)
	{
	case eRFSRFD_ERR_RIOOPEN:
		return eRFD_ERR_RIOOPEN;

	case eRFSRFD_ERR:
		return eRFD_ERR;

	case eRFSRFD_ERR_NOTOPEN:
		return eRFD_ERR_NOTOPEN;

	case eRFSRFD_ERR_INVALID:
		return eRFD_ERR_INVALID;

	case eRFSRFD_ERR_RIO:
		return eRFD_ERR_RIO;

	case eRFSRFD_ERR_NODG:
		return eRFD_ERR_NODG;

	case eRFSRFD_ERR_DGNOTOPEN:
		return eRFD_ERR_DGNOTINIT;

	case eRFSRFD_ERR_DGPROCERR:
		return eRFD_ERR_DGPROCERR;

	case eRFSRFD_ERR_DGRIOMEM:
		return eRFD_ERR_DGRIOMEM;

	case eRFSRFD_ERR_RIOTIMEOUT:
		return eRFD_ERR_RIOTIMEOUT;

	case eRFSRFD_SUCCESS:
	default:
		return eFET_SUCCESS;
	}
}

FETSTATUS rioGetNumLocalPorts (UINT32 *pulNumLocalPorts)
{
	UINT32 ulRet;
	FETSTATUS fsRet = eFET_SUCCESS;

	fsRet = RFDSTATUS_TO_FETSTATUS(RFD_Read(RIO_SPICAR, 4, (void*)&ulRet));
	if (fsRet == eFET_SUCCESS)
	{
		*pulNumLocalPorts = (ulRet>>8)&0xff;
//		printf("RapidFET HAL***:  rioGetNumLocalPorts returning %d\n", *pulNumLocalPorts);
	}
	else
	{
		printf("RapidFET HAL***:  rioGetNumLocalPorts failed with error %d\n", fsRet);fflush(stdout);
	}
	return fsRet;
}

FETSTATUS rioConfigurationRead (
							unsigned char uchLocalPort,
							unsigned char bLargeTT,
							UINT32 ulDestId,
							unsigned char uchHopCount,
							UINT32 ulOffset,
							void *pReadData,
							UINT32 ulWordCount,
							UINT32 ulWordSize,
							UINT32 ulStride,
							UINT32 ulPriority
							)
{
	FETSTATUS fsRet = eFET_SUCCESS;
	BYTE* pbyPtr = (BYTE*)pReadData;
	UINT32 ulOffsetTemp=ulOffset;
	UINT32 i;

//	printf("RapidFET HAL***: rioConfigurationRead(Port:%d, DestId:%d, HopCount:%d, Offset:%x Count:%d)\n",
//		uchLocalPort, ulDestId, uchHopCount, ulOffset, ulWordCount); fflush(stdout);
	if ((ulDestId == HOST_REGS) && (uchHopCount == ZERO_HOP))
	{
		for (i=0; i<ulWordCount; i++)
		{
//			printf("rioConfigurationRead doing local maint read at offset %08lx\n", ulOffsetTemp); fflush(stdout); 
			fsRet = RFDSTATUS_TO_FETSTATUS(RFD_Read(ulOffsetTemp, ulWordSize, (void*)pbyPtr));
			if (fsRet != eFET_SUCCESS)
			{
				printf("RapidFET HAL***: RFD_Read failed with error %d\n", fsRet);fflush(stdout);
				break;
			}

			ulOffsetTemp += ulStride;
			pbyPtr += ulWordSize;
		}
	}
	else
	{
		fsRet = RFDSTATUS_TO_FETSTATUS(RFD_MaintRead(bLargeTT, ulDestId, uchHopCount, ulOffsetTemp, pReadData, ulWordCount, ulWordSize, ulStride, ulPriority));
		if (fsRet != eFET_SUCCESS)
		{
			printf("RapidFET HAL***: RFD_MaintRead failed with error %d\n", fsRet);fflush(stdout);
		}
	}

//	printf("RapidFET HAL***: rioConfigurationRead(Port:%d, DestId:%d, HopCount:%d, Offset:%x Count:%d) returning %d\n",
//		uchLocalPort, ulDestId, uchHopCount, ulOffset, ulWordCount, fsRet);

	return fsRet;
}

FETSTATUS rioConfigurationWrite (
							unsigned char uchLocalPort,
							unsigned char bLargeTT,
							UINT32 ulDestId,
							unsigned char uchHopCount,
							UINT32 ulOffset,
							void *pWriteData,
							UINT32 ulWordCount,
							UINT32 ulWordSize,
							UINT32 ulStride,
							UINT32 ulPriority
							)
{
	FETSTATUS fsRet = eFET_SUCCESS;
	BYTE* pbyPtr = (BYTE*)pWriteData;
	UINT32 ulOffsetTemp=ulOffset;
	UINT32 i;

	if ((ulDestId == HOST_REGS) && (uchHopCount == ZERO_HOP))
	{
		for (i=0; i<ulWordCount; i++)
		{
//			printf("rioConfigurationWrite doing local maint write at offset %08lx\n", ulOffsetTemp); fflush(stdout); 
			fsRet = RFDSTATUS_TO_FETSTATUS(RFD_Write(ulOffsetTemp, ulWordSize, (void*)pbyPtr));
			if (fsRet != eFET_SUCCESS)
			{
				printf("RapidFET HAL***: rioConfigurationWrite failed with error %d\n", fsRet);fflush(stdout);
				break;
			}

			ulOffsetTemp += ulStride;
			pbyPtr += ulWordSize;
		}
	}
	else
	{
		fsRet = RFDSTATUS_TO_FETSTATUS(RFD_MaintWrite(bLargeTT, ulDestId, uchHopCount, ulOffsetTemp, pWriteData, ulWordCount, ulWordSize, ulStride, ulPriority));
		if (fsRet != eFET_SUCCESS)
		{
			printf("RapidFET HAL***: rioConfigurationWrite failed with error %d\n", fsRet);fflush(stdout);
		}
	}

//	printf("RapidFET HAL***: rioConfigurationWrite(Port:%d, DestId:%d, HopCount:%d, Offset:%x Count:%d) returning %d\n",
//		uchLocalPort, ulDestId, uchHopCount, ulOffset, ulWordCount, fsRet);
	return fsRet;
}

FETSTATUS rioDataRead (
					unsigned char uchLocalPort,
					unsigned char bLargeTT,
					UINT32 ulDestId,
					UINT32 ulRioAddressHi,
					UINT32 ulRioAddressLo,
					UINT32 ulXferType,
					UINT32 ulPriority,
					unsigned char *puchReadData,
					UINT32 ulByteCount
					)
{
	FETSTATUS fsRet = eFET_SUCCESS;

//	printf("RapidFET HAL***: rioDataRead(Port:%d, DestId:%d, Address:%x%08lx Count:%d)\n",
//		uchLocalPort, ulDestId, ulRioAddressHi, ulRioAddressLo, ulByteCount); fflush(stdout);

	fsRet = RFDSTATUS_TO_FETSTATUS(RFD_RioRead(bLargeTT, ulDestId, ulRioAddressHi, ulRioAddressLo, ulXferType, ulPriority, puchReadData, ulByteCount));
	if (fsRet != eFET_SUCCESS)
	{
		printf("RapidFET HAL***: rioDataRead failed with error %d\n", fsRet);fflush(stdout);
	}

//	printf("RapidFET HAL***: rioDataRead(Port:%d, DestId:%d, Address:%x%08lx Count:%d) returning %d\n",
//		uchLocalPort, ulDestId, ulRioAddressHi, ulRioAddressLo, ulByteCount, fsRet);

	return fsRet;
}

FETSTATUS rioDataWrite (
						unsigned char uchLocalPort,
						unsigned char bLargeTT,
						UINT32 ulDestId,
						UINT32 ulRioAddressHi,
						UINT32 ulRioAddressLo,
						UINT32 ulXferType,
						UINT32 ulPriority,
						unsigned char *puchWriteData,
						UINT32 ulByteCount
						)
{
	FETSTATUS fsRet = eFET_SUCCESS;

	fsRet = RFDSTATUS_TO_FETSTATUS(RFD_RioWrite(bLargeTT, ulDestId, ulRioAddressHi, ulRioAddressLo, ulXferType, ulPriority, puchWriteData, ulByteCount));
	if (fsRet != eFET_SUCCESS)
	{
		printf("RapidFET HAL***: rioWrite failed with error %d\n", fsRet);fflush(stdout);
	}

//	printf("RapidFET HAL***: rioWrite(Port:%d, DestId:%d, Address:%x%08lx Count:%d) returning %d\n",
//		uchLocalPort, ulDestId, ulRioAddressHi, ulRioAddressLo, ulByteCount, fsRet);

	return fsRet;
}

FETSTATUS rioMessageSend (
						unsigned char uchLocalPort,
						unsigned char bLargeTT,
						UINT32 ulDestId,
						UINT32 ulPriority, 
						UINT32 bMultiPacket,
						UINT32 ulMailboxNum,
						unsigned char *puchWriteData,
                        UINT32 ulPacketByteCount,
                        UINT32 ulTotalByteCount
						)
{
	FETSTATUS fsRet = eFET_SUCCESS;

	fsRet = RFDSTATUS_TO_FETSTATUS(RFD_RioMessageSend(bLargeTT, ulDestId, ulPriority, bMultiPacket, ulMailboxNum, puchWriteData, ulPacketByteCount, ulTotalByteCount));
	if (fsRet != eFET_SUCCESS)
	{
		printf("RapidFET HAL***: rioMessageSend failed with error %d\n", fsRet);fflush(stdout);
	}

//	printf("RapidFET HAL***: rioMessageSend(Port:%d, DestId:%d, %s Mailbox:%x PacketSize:%d TotalBytes:%d ) returning %d\n",
//		uchLocalPort, ulDestId, bMultiPacket?"Multi":"Single", ulMailboxNum, ulPacketByteCount, ulTotalByteCount, fsRet);

	return fsRet;
}

FETSTATUS rioDoorbellSend (
						unsigned char uchLocalPort,
						unsigned char bLargeTT,
						UINT32 ulDestId,
						UINT32 ulPriority,
						UINT16 wInfoWord
						)
{
	FETSTATUS fsRet = eFET_SUCCESS;

	fsRet = RFDSTATUS_TO_FETSTATUS(RFD_RioDoorbellSend(bLargeTT, ulDestId, ulPriority, wInfoWord));
	if (fsRet != eFET_SUCCESS)
	{
		printf("RapidFET HAL***: rioDoorbellSend failed with error %d\n", fsRet);fflush(stdout);
	}

//	printf("RapidFET HAL***: rioDoorbellSend(Port:%d, %s DestId:%d, Info:%x  ) returning %d\n",
//		uchLocalPort, bLargeTT?"Large":"Small", ulDestId, wInfoWord, fsRet);

	return fsRet;
}

FETSTATUS rioMessageGet (
						UINT32 ulWhichQueue,
						UINT32 ulCommand,
						unsigned char *pbLargeTT,
						unsigned char *pbOverflow,
						UINT32 *pulSourceID,
						UINT32 *pulPriority,
						UINT32 *pulMailboxNum,
						UINT32 *pulMessageLengthBytes,
						unsigned char *puchData)
{
	FETSTATUS fsRet = eFET_SUCCESS;

//	printf("RapidFET HAL***: rioMessageGet(Queue:%d Command:%s)\n", ulWhichQueue, ulCommand==1?"Get Message":"Flush Input Queue");

	if (ulCommand == 1)
	{
		fsRet = RFDSTATUS_TO_FETSTATUS(RFD_RioMessageGet(ulWhichQueue, pbLargeTT, pulSourceID, pulPriority, pulMailboxNum, pulMessageLengthBytes, pbOverflow, puchData));
		if (fsRet != eFET_SUCCESS)
		{
			printf("RapidFET HAL***: RFD_RioMessageGet failed with error %d\n", fsRet);fflush(stdout);
			*pbLargeTT = 0;
			*pbOverflow = 0;
			*pulSourceID = 0;
			*pulPriority = 0;
			*pulMailboxNum = 0;
			*pulMessageLengthBytes = 0;
		}
	}
	else if (ulCommand == 2)
	{
		fsRet = RFDSTATUS_TO_FETSTATUS(RFD_RioFlushMessages(ulWhichQueue));
		if (fsRet != eFET_SUCCESS)
		{
			printf("RapidFET HAL***: RFD_RioFlushMessages failed with error %d\n", fsRet);fflush(stdout);
		}
		*pbLargeTT = 0;
		*pbOverflow = 0;
		*pulSourceID = 0;
		*pulPriority = 0;
		*pulMailboxNum = 0;
		*pulMessageLengthBytes = 0;
	}
	else
	{
		return eRFD_ERR_INVALID;
	}


	return fsRet;
}

FETSTATUS rioDoorbellGet (
						UINT32 ulWhichQueue,
						UINT32 ulCommand,
						unsigned char *pbValidDoorbell,
						unsigned char *pbOverflow,
						UINT16 *pwInfoWord)
{
	FETSTATUS fsRet = eFET_SUCCESS;

//	printf("RapidFET HAL***: rioDoorbellGet(Queue:%d Command:%s)\n", ulWhichQueue, ulCommand==1?"Get Doorbell":"Flush Input Queue");

	if (ulCommand == 1)
	{
		fsRet = RFDSTATUS_TO_FETSTATUS(RFD_RioDoorbellGet(ulWhichQueue, pbValidDoorbell, pbOverflow, pwInfoWord));
		if (fsRet != eFET_SUCCESS)
		{
			printf("RapidFET HAL***: RFD_RioDoorbellGet failed with error %d\n", fsRet);fflush(stdout);
			*pbValidDoorbell = 0;
			*pbOverflow = 0;
			*pwInfoWord = 0;
		}
	}
	else if (ulCommand == 2)
	{
		fsRet = RFDSTATUS_TO_FETSTATUS(RFD_RioFlushDoorbells(ulWhichQueue));
		if (fsRet != eFET_SUCCESS)
		{
			printf("RapidFET HAL***: RFD_RioFlushDoorbells failed with error %d\n", fsRet);fflush(stdout);
		}
		*pbValidDoorbell = 0;
		*pbOverflow = 0;
		*pwInfoWord = 0;
	}
	else
	{
		return eRFD_ERR_INVALID;
	}

	return fsRet;
}

FETSTATUS rioGetRioError(UINT32* pulRioErrorMask, UINT32* apulRioErrorInfo[], char* aszErrorString[])
{
	FETSTATUS fsRet = eFET_SUCCESS;

	fsRet = RFDSTATUS_TO_FETSTATUS(RFD_GetRioError(pulRioErrorMask, apulRioErrorInfo, aszErrorString));
	return fsRet;
}

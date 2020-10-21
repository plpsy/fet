///////////////////////////////////////////////////////////////////////////////
// $RCSfile: fethal.h,v $
// $Date: 2008/07/04 14:14:38 $
// $Author: Gilmour $
// $Revision: 1.11 $
// $Source: /cvsrepo/FET/Server/fethal.h,v $
//
// Copyright (c) 2004-2007 Fabric Embedded Tools Corporation
//
///////////////////////////////////////////////////////////////////////////////
#ifndef hal__H
#define hal__H	

/* JuWi Changed include path */
/* #include "include/feterr.h" */
#include <usertype.h>
#include "feterr.h"

FETSTATUS rioGetNumLocalPorts(UINT32 *pulNumLocalPorts);
FETSTATUS rioConfigurationRead(
			unsigned char uchLocalPort,
			unsigned char uchLargeDestID,
			UINT32 ulDestId,
			unsigned char uchHopCount,
			UINT32 ulOffset,
			void *pReadData,
			UINT32 ulWordCount,
			UINT32 ulWordSize,
			UINT32 ulStride,
			UINT32 ulPriority
			);
FETSTATUS rioConfigurationWrite(
			unsigned char uchLocalPort,
			unsigned char uchLargeDestID,
			UINT32 ulDestId,
			unsigned char uchHopCount,
			UINT32 ulOffset,
			void *pWriteData,
			UINT32 ulWordCount,
			UINT32 ulWordSize,
			UINT32 ulStride,
			UINT32 ulPriority
			);
FETSTATUS rioDataRead (
			unsigned char uchLocalPort,
			unsigned char uchLargeDestID,
			UINT32 ulDestId,
			UINT32 ulRioAddressHi,
			UINT32 ulRioAddressLo,
			UINT32 ulXferType,
			UINT32 ulPriority,
			unsigned char *puchReadData,
			UINT32 ulByteCount
			);
FETSTATUS rioDataWrite (
			unsigned char uchLocalPort,
			unsigned char uchLargeDestID,
			UINT32 ulDestId,
			UINT32 ulRioAddressHi,
			UINT32 ulRioAddressLo,
			UINT32 ulXferType,
			UINT32 ulPriority,
			unsigned char *puchWriteData,
			UINT32 ulByteCount
			);
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
			);
FETSTATUS rioDoorbellSend (
			unsigned char uchLocalPort,
			unsigned char bLargeTT,
			UINT32 ulDestId,
			UINT32 ulPriority,
			UINT16 wInfoWord
			);
FETSTATUS rioMessageGet (
			UINT32 ulWhichQueue,
			UINT32 ulCommand,
			unsigned char *pbLargeTT,
			unsigned char *pbOverflow,
			UINT32 *pulSourceID,
			UINT32 *pulPriority,
			UINT32 *pulMailboxNum,
			UINT32 *pulMessageLengthBytes,
			unsigned char *puchData
			);
FETSTATUS rioDoorbellGet (
			UINT32 ulWhichQueue,
			UINT32 ulCommand,
			unsigned char *pbValidDoorbell,
			unsigned char *pbOverflow,
			UINT16 *pwInfoWord
			);
FETSTATUS rioGetRioError (
			UINT32* pulRioErrorMask, 
			UINT32* apulRioErrorInfo[], 
			char* aszErrorString[]
			);

#endif


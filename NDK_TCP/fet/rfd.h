///////////////////////////////////////////////////////////////////////////////
// $RCSfile: rfd.h,v $
// $Date: 2008/09/15 18:52:38 $
// $Author: Gilmour $
// $Revision: 1.24 $
// $Source: /cvsrepo/FET/Server/common/rfd.h,v $
//
// Copyright (c) 2004-2008 Fabric Embedded Tools Corporation
//
///////////////////////////////////////////////////////////////////////////////
#ifndef RFD__H
#define RFD__H	
//#include <types/vxTypesOld.h> //sdr added 20081027
typedef enum typeRFDSTATUS
{
	eRFSRFD_SUCCESS			=0,
	eRFSRFD_ERR_RIOOPEN		=-1,
	eRFSRFD_ERR				=-2,
	eRFSRFD_ERR_NOTOPEN		=-3,
	eRFSRFD_ERR_INVALID		=-4,
	eRFSRFD_ERR_RIO			=-5,
	eRFSRFD_ERR_NODG		=-6,
	eRFSRFD_ERR_DGNOTOPEN	=-7,
	eRFSRFD_ERR_DGPROCERR	=-8,
	eRFSRFD_ERR_DGRIOMEM	=-9,
	eRFSRFD_ERR_RIOTIMEOUT	=-10
} RFDSTATUS;

RFDSTATUS RFD_Open(void);
RFDSTATUS RFD_Close(void);
RFDSTATUS RFD_Read(UINT32 ulOffset, UINT32 ulSize, void *pData);
RFDSTATUS RFD_Write(UINT32 ulOffset, UINT32 ulSize, void* pData);
RFDSTATUS RFD_MaintRead(unsigned char bLargeTT, 
						UINT32 ulDestID, 
						unsigned char uchHopCount, 
						UINT32 ulOffset, 
						void *pData, 
						UINT32 ulWordCount,
						UINT32 ulWordSize,
						UINT32 ulStride,
						UINT32 ulPriority);
RFDSTATUS RFD_MaintWrite(unsigned char bLargeTT, 
						UINT32 ulDestID, 
						unsigned char uchHopCount, 
						UINT32 ulOffset, 
						void *pData, 
						UINT32 ulWordCount, 
						UINT32 ulWordSize,
						UINT32 ulStride,
						UINT32 ulPriority);
RFDSTATUS RFD_RioRead(unsigned char bLargeTT, 
						UINT32 ulDestID,
						UINT32 ulRioAddressHi,
						UINT32 ulRioAddressLo,
						UINT32 ulXferType,
						UINT32 ulPriority,
						unsigned char *puchData,
						UINT32 ulbyteCount);
RFDSTATUS RFD_RioWrite(unsigned char bLargeTT, 
						UINT32 ulDestID,
						UINT32 ulRioAddressHi,
						UINT32 ulRioAddressLo,
						UINT32 ulXferType,
						UINT32 ulPriority,
						unsigned char *puchData,
						UINT32 ulByteCount);
RFDSTATUS RFD_RioMessageSend(
						unsigned char bLargeTT, 
						UINT32 ulDestID,
						UINT32 ulPriority, 
						UINT32 bMultiPacket,
						UINT32 ulMailboxNum,
						unsigned char *puchData,
						UINT32 ulPacketByteCount,
						UINT32 ulTotalByteCount);
RFDSTATUS RFD_RioMessageGet(
						UINT32 ulWhichChannel,
						unsigned char* pbLargeTT, 
						UINT32* pulSourceID,
						UINT32* pulPriority, 
						UINT32* pulMailboxNum,
						UINT32* pulMessageLength,
						unsigned char* pbOverflowed,
						unsigned char *puchData);
RFDSTATUS RFD_RioFlushMessages(UINT32 ulWhichChannel);
RFDSTATUS RFD_RioDoorbellSend(unsigned char bLargeTT, 
						UINT32 ulDestID,
						UINT32 ulPriority, 
						UINT16 wInfoWord);
RFDSTATUS RFD_RioFlushDoorbells(UINT32 ulWhichChannel);
RFDSTATUS RFD_RioDoorbellGet(
						UINT32 ulWhichChannel,
						unsigned char* pbValidDoorbell, 
						unsigned char* pbOverflowed,
						UINT16* pwInfoWord);
RFDSTATUS RFD_RioDMAWrite(unsigned char bLargeTT, 
						UINT32 ulDestID,
						UINT32 ulRioAddressHi,
						UINT32 ulRioAddressLo,
						UINT32 ulDestBufferSizeBytes,
						UINT32 ulXferType,
						UINT32 ulPriority,
						unsigned char *puchData,
						UINT32 ulByteCount,
						UINT32 *pbTerminated);
RFDSTATUS RFD_RioDMARead(unsigned char bLargeTT, 
						UINT32 ulDestID,
						UINT32 ulRioAddressHi,
						UINT32 ulRioAddressLo,
						UINT32 ulDestBufferSizeBytes,
						UINT32 ulXferType,
						UINT32 ulPriority,
						unsigned char *puchData,
						UINT32 ulByteCount,
						UINT32 *pbTerminated);

#define MAXRIOERRORSTRINGLENGTH		128
#define MAXRIOERRORINFOWORDS		32
RFDSTATUS RFD_GetRioError(UINT32* pulRioErrorMask, UINT32* apulRioErrorInfo[], char* aszErrorString[]);
UINT32 RFD_GetTimeMarker(void);
UINT32 RFD_TimeSpanUS(UINT32 ulStartTimeMarker, UINT32 ulStopTimeMarker);
UINT32 RFD_GetVersion(void);

RFDSTATUS RFD_DGOpen(int nDGCtrlBytes, int nDGDataBytes, UINT32 ulRioAddress, RFDSTATUS (*pCallback)(void));
RFDSTATUS RFD_DGClose(void);
RFDSTATUS RFD_DGGetRemoteDGCtrlPtr(unsigned char bLargeTT, unsigned short uwDestID, unsigned long ulRioAddress, void** pp); 
RFDSTATUS RFD_DGGetLocalDGCtrlPtr(void** pp); 

RFDSTATUS RFD_ReadMem(UINT32 ulAddressHi, 
					  UINT32 ulAddressLo,
					  void *pData,
                      UINT32 ulWordCount, 
                      UINT32 ulWordSize);
RFDSTATUS RFD_WriteMem(UINT32 ulAddressHi,
					   UINT32 ulAddressLo,
                       void *pData,
                       UINT32 ulWordCount, 
                       UINT32 ulWordSize);

#define MAJORVERSION(x) (((x)>>24)&0xff)
#define MINORVERSION(x) (((x)>>16)&0xff)
#define BUILDVERSION(x) ((x)&0xffff)
#define MAKEVERSION(major, minor, build) ((((major)&0xff)<<24) + (((minor)&0xff)<<16) + ((build)&0xffff))

#endif	


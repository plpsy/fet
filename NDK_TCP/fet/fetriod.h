///////////////////////////////////////////////////////////////////////////////
// $RCSfile: fetriod.h,v $
// $Date: 2008/05/16 12:49:23 $
// $Author: Gilmour $
// $Revision: 1.14 $
// $Source: /cvsrepo/FET/FETrioD/fetriod.h,v $
//
// Copyright (c) 2005 Fabric Embedded Tools Corporation
//
///////////////////////////////////////////////////////////////////////////////

#define T printf("FETriod:  Trace Line %d in %s\n", __LINE__, __FILE__); fflush(stdout);


typedef enum typeRTYPE 
{
	eRTYPE_IOREADHOME=2,
	eRTYPE_NREAD=4,
	eRTYPE_MAINTENANCE=7,
	eRTYPE_ATOMICINCR=12,
	eRTYPE_ATOMICDECR=13,
	eRTYPE_ATOMICSET=14,
	eRTYPE_ATOMICCLEAR=15,
} eRTYPE;

typedef enum typeWTYPE 
{
	eWTYPE_FLUSHWITHDATA=1,
	eWTYPE_DOORBELL=2,
	eWTYPE_SWRITE=3,
	eWTYPE_NWRITE=4,
	eWTYPE_NWRITE_R=5,
	eWTYPE_MESSAGE=6,
	eWTYPE_MAINTENANCE=7,
} eWTYPE;

typedef enum typePRIORITY
{
	ePRIORITY_L=0,
	ePRIORITY_M=1,
	ePRIORITY_H=2,
} ePRIORITY;

typedef int frdHandle;

typedef enum typeIBW_RTYPE 
{
	eIBW_RTYPE_NOSNOOP=4,
	eIBW_RTYPE_SNOOP=5,
	eIBW_RTYPE_UNLOCKL2=7,
} eIBW_RTYPE;

typedef enum typeIBW_WTYPE 
{
	eIBW_WTYPE_NOSNOOP=4,
	eIBW_WTYPE_SNOOP=5,
	eIBW_WTYPE_ALLOCL2=6,
	eIBW_WTYPE_LOCKL2=7,
} eIBW_WTYPE;

int frdOpen(					UINT32 ulCCSRBAR_P);
int frdClose(					void);

// Local RIO Register Access
int frdReadRioReg(				UINT32 ulOffset,
								UINT32 ulSize,
								void* pData);
int frdWriteRioReg(				UINT32 ulOffset,
								UINT32 ulSize,
								void* pData);

// Window management
int frdAllocOutboundWindow(		UINT32* pulWindowSize,
								frdHandle* phWindow);
int frdConfigOutboundWindow(	frdHandle hWindow, 
								eRTYPE eReadType, 
								eWTYPE eWriteType, 
								ePRIORITY ePriority, 
								unsigned char bLargeTT,
								UINT32 ulDestID,
								UINT32 ulRapidIOAddressLo,
								UINT32 ulRapidIOAddressHi);
int frdConfigOutboundMaintWindow(frdHandle hWindow, 
								ePRIORITY ePriority, 
								UINT32 ulHopCount,
								unsigned char bLargeTT,
								UINT32 ulDestID,
								UINT32 ulOffset);
int frdFreeOutboundWindow(		frdHandle hWindow);
int frdAllocInboundWindow(		UINT32* pulWindowSize,
								frdHandle* phWindow, 
								frdHandle* phMemory);
int frdConfigInboundWindow(		frdHandle hWindow, 
								eIBW_RTYPE eReadType, 
								eIBW_WTYPE eWriteType, 
								UINT32 ulRapidIOAddressLo,
								UINT32 ulRapidIOAddressHi);
int frdFreeInboundWindow(		frdHandle hWindow);

// memory management
int frdAllocMem(				UINT32 ulBytes,
								frdHandle* phMemory);
int frdFreeMem(					frdHandle hMemory);
void* frdGetAddressV(			frdHandle hMemory);
void* frdGetAddressP(			frdHandle hMemory);
UINT32 frdGetSize(		frdHandle hMemory);

// DMA
int frdDMA_Open(				frdHandle* phDMA);						// open a dma channel
int frdDMA_Close(				frdHandle hDMA);						// close a dma channel

int frdDMA_AllocChain(			frdHandle* phChain, int nSize);			// allocate a DMA chain with nSize entries
int frdDMA_InitChain(			frdHandle hChain);						// initialize a DMA chain
int frdDMA_FreeChain(			frdHandle hChain);						// release memory associated with a dma chain

int frdDMA_AppendDirectWriteToRIO(frdHandle hChain,						// add a ATMU Bypass command to a dma chain
								frdHandle hMemory,
								UINT32 ulMemOffset,
								unsigned char bLargeTT,
								UINT32 ulDestID,
								UINT32 ulRapidIOAddressLo,
								UINT32 ulRapidIOAddressHi,
								UINT32 ulBytes,
								ePRIORITY ePriority,
								eWTYPE eWriteType
								);
int frdDMA_AppendDirectWriteToMemory(frdHandle hChain,					// the chain to be run
								frdHandle hMemorySrc,					// handle to source memory
								UINT32 ulMemOffsetSrc,			// offset into source memory of beginning of data to be DMA'd
								frdHandle hMemoryDst,					// handle to destination memory
								UINT32 ulMemOffsetDst,			// offset into destination memory where data is to be written
								UINT32 ulBytes					// number of bytes to be DMA'd
								);
int frdDMA_StartChainTransfer(	frdHandle hDMA,							// begin the DMA transfer
								frdHandle hChain,
								int bWait,
								int nTimeoutMS
								);
int frdDMA_Busy(				frdHandle hDMA);
int frdDMA_Abort(				frdHandle hDMA);
int frdDMA_Error(				frdHandle hDMA);

// Outbound Doorbells
int frdODB_Open(				frdHandle* phDBController);
/* JuWi Added prototype for frdODB_Close */
int frdODB_Close(                               frdHandle hDBController);
int frdODB_Send(				frdHandle hDBController,
								int bWait,								// Non-zero if requesting no return until Doorbell complete
								int nTimeoutMS,							// If waiting, how long to wait before timing out
								unsigned char bLargeTT, 
								UINT32 ulDestID,
								ePRIORITY ePriority, 
								unsigned short wInfoWord);
int frdODB_Busy(				frdHandle hDBController);
int frdODB_Error(				frdHandle hDBController);

// Inbound Doorbells
int frdIDB_Open(				frdHandle* phDBController, 
								UINT32 ulCirqFrameQueueSize);
int frdIDB_Close(				frdHandle hDBController);
int frdIDB_Busy(				frdHandle hDBController);
int frdIDB_GetDoorbell(			frdHandle hDBController, 
								unsigned short* pwSrcID, 
								unsigned short* pwTgtID, 
								unsigned short* pwInfo);

// Outbound Messaging
#define OMSG_DIRECTMODE			1
#define OMSG_CHAINEDMODE			0
int frdOMSG_Open(				frdHandle* phMsgController,				// open an outbound message controller
								int bDirectMode, 
								UINT32 ulChainLinks,
								UINT32 ulSizeBytes);
int frdOMSG_Close(				frdHandle hMsgController);				// close an outbound message controller
int frdOMSG_Busy(				frdHandle hMsgController);				// check to see if outbound message controller is busy
int frdOMSG_Error(				frdHandle hMsgController);				// check for error in outbound message controller
int frdOMSG_StartDirectTransfer(	frdHandle hMsgController,
								int bWait,								// Non-zero if requesting no return until message complete
								int nTimeoutMS,							// If waiting, how long to wait before timing out
								unsigned char bLargeTT, 
								UINT32 ulDestID,
								ePRIORITY ePriority, 
								UINT32 bMultiPacket,
								UINT32 ulMailboxNum,
								unsigned char* puchData, 
								UINT32 ulPacketByteCount,
								UINT32 ulTotalByteCount
								);
int frdOMSG_Enqueue(				frdHandle hMsgController,
								unsigned char bLargeTT, 
								UINT32 ulDestID,
								ePRIORITY ePriority,
								UINT32 bMultiPacket,
								UINT32 ulMailboxNum,
								unsigned char* puchData, 
								UINT32 ulPacketByteCount,
								UINT32 ulTotalByteCount
								);
int frdOMSG_StartChainTransfer(	frdHandle hMsgController);

// Inbound Messaging
int frdIMSG_Open(				frdHandle* phMsgController,				// open an inbound message controller
								int nRequestControllerID,				// set to non-negative to request specific controller
								UINT32 ulMsgFrameSize,
								UINT32 ulCirqFrameQueueSize);
int frdIMSG_Close(				frdHandle hMsgController);				// close an inbound message controller
int frdIMSG_Busy(				frdHandle hMsgController);				// check to see if inbound message controller is busy
int frdIMSG_Error(				frdHandle hMsgController);				// check for error in inbound message controller
int frdIMSG_GetMessage(			frdHandle hMsgController, 
								unsigned char* puchData);

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
								UINT32 ulStride);
int frdMaintWrite(				frdHandle hMaintWindow,
								unsigned char bLargeTT,
								UINT32 ulDestID,
								unsigned char uchHopCount,
								UINT32 ulOffset,
								ePRIORITY ePriority,
								void* pData,
								UINT32 ulWordCount,
								UINT32 ulWordSize,
								UINT32 ulStride);
int frdRioRead(					frdHandle hWindow, 
								unsigned char bLargeTT,
								UINT32 ulDestID,
								UINT32 ulRioAddressHi,
								UINT32 ulRioAddressLo,
								eRTYPE eReadType, 
								ePRIORITY ePriority, 
								unsigned char* puchData, 
								UINT32 ulByteCount);
int frdRioWrite(				frdHandle hWindow,
								unsigned char bLargeTT,
								UINT32 ulDestID,
								UINT32 ulRioAddressHi,
								UINT32 ulRioAddressLo,
								eWTYPE eWriteType, 
								ePRIORITY ePriority, 
								unsigned char* puchData, 
								UINT32 ulByteCount);

// Error Management
UINT32 frdGetRioError(	UINT32* pulRioErrorMask,
								UINT32* apulRioErrorInfo[],
								char* apszRioErrorString[]);
void frdClearRioError(			void);

// Local Memory Access
int frdReadLocalMemory(			UINT32 ulAddrP,
								void* pData,
								UINT32 ulWordCount,
								UINT32 ulWordSize);
int frdWriteLocalMemory(		UINT32 ulAddrP,
								void* pData,
								UINT32 ulWordCount,
								UINT32 ulWordSize);

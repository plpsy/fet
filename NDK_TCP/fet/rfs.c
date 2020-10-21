/////////////////////////////////////////////////////////////////////////////// 
// $RCSfile: rfs.c,v $ 
// $Date: 2008/09/15 18:48:13 $ 
// $Author: Gilmour $ 
// $Revision: 1.17 $ 
// $Source: /cvsrepo/FET/Server/STX8548AMC/STX8548AMC_LIN/rfs.c,v $ 
// 
// Copyright (c) 2004-2008 Fabric Embedded Tools Corporation 
// 
/////////////////////////////////////////////////////////////////////////////// 


#include <usertype.h>
#include <serrno.h>

#include "mytypes.h"


#include <stdlib.h>
#include <stdio.h> 
#include <signal.h>

#include <ti/sysbios/knl/Task.h>

#include <string.h> 
#include <errno.h> 
#include "rio.h" 
 
#include "fethal.h" 
/* JuWi Changed include path */
/* #include "include/feterr.h"  */
/* #include "include/fetmsg.h" */
#include "feterr.h"
#include "fetmsg.h"

#include "datagen.h"
#include "rfd.h"

#include <ti/ndk/inc/netmain.h>

static FETSTATUS DoMessageFET_HALGETNUMLOCALPORTS(FETMSG_HEADERSTRUCT *pHdr, SOCKET *psockClient); 
static FETSTATUS DoMessageFET_HALCONFIGREAD(FETMSG_HEADERSTRUCT *pHdr, SOCKET *psockClient); 
static FETSTATUS DoMessageFET_HALCONFIGWRITE(FETMSG_HEADERSTRUCT *pHdr, SOCKET *psockClient); 
static FETSTATUS DoMessageFET_CLOSE(FETMSG_HEADERSTRUCT *pHdr, SOCKET *psockClient); 
static FETSTATUS DoMessageFET_DATAGENCTRL (FETMSG_HEADERSTRUCT *pHdr, SOCKET *psockClient); 
static FETSTATUS DoMessageFET_IDENTIFY (FETMSG_HEADERSTRUCT *pHdr, SOCKET *psockClient); 
static FETSTATUS DoMessageFET_HALRIOREAD(FETMSG_HEADERSTRUCT *pHdr, SOCKET *psockClient); 
static FETSTATUS DoMessageFET_HALRIOWRITE(FETMSG_HEADERSTRUCT *pHdr, SOCKET *psockClient); 
static FETSTATUS DoMessageFET_HALRIOMESSAGERX(FETMSG_HEADERSTRUCT *pHdr, SOCKET *psockClient);
static FETSTATUS DoMessageFET_HALRIOMESSAGETX(FETMSG_HEADERSTRUCT *pHdr, SOCKET *psockClient);
static FETSTATUS DoMessageFET_HALRIODOORBELLTX(FETMSG_HEADERSTRUCT *pHdr, SOCKET *psockClient); 
static FETSTATUS DoMessageFET_HALRIODOORBELLRX(FETMSG_HEADERSTRUCT *pHdr, SOCKET *psockClient); 
static FETSTATUS DoMessageFET_HALGETRIOERROR(FETMSG_HEADERSTRUCT *pHdr, SOCKET *psockClient);
static FETSTATUS DoMessageFET_MEMREAD(FETMSG_HEADERSTRUCT *pHdr, SOCKET *psockClient);
static FETSTATUS DoMessageFET_MEMWRITE(FETMSG_HEADERSTRUCT *pHdr, SOCKET *psockClient);
static FETSTATUS DoMessageFET_GETSERVERCAPS(FETMSG_HEADERSTRUCT *pHdr, SOCKET *psockClient);
#ifdef SUPPORTS_FLASHREAD
static FETSTATUS DoMessageFET_FLASHREAD(FETMSG_HEADERSTRUCT *pHdr, SOCKET *psockClient);
#endif
#ifdef SUPPORTS_FLASHWRITE
static FETSTATUS DoMessageFET_FLASHWRITE(FETMSG_HEADERSTRUCT *pHdr, SOCKET *psockClient);
#endif
static FETSTATUS Send (SOCKET* psockClient, void* pData, int* pnBytes); 
static int 	Recv(SOCKET* psockClient, char* buf, int len);
static int IsPortOK(void);

void BuildFET_IDENTIFYRESP(FETMSG_HEADERSTRUCT* psHdr, FETMSG_IDENTIFYRESPSTRUCT* psData); 


// Globals 
int g_bConnected;
int g_bSupportsDG; 
int g_bSupportsMonitor;
UINT32 g_ulVERSION = MAKEVERSION(2,8,1); 

struct  sockaddr_in g_addrFrom;
int g_addrsize = sizeof(g_addrFrom);

extern FETSTATUS RFDSTATUS_TO_FETSTATUS(RFDSTATUS rfds); 

#if 0
/* JuWi Changed return type of main */
/* main(int argc, char** argv) */
int RFD_Init()
{ 
	FETSTATUS fs;


	printf("******************************************************\n"); 
	printf("**                  RapidFET Server                 **\n"); 
	printf("**                                                  **\n"); 
	printf("**               RFS Version %2d.%-2d(%04d)            **\n", MAJORVERSION(g_ulVERSION), MINORVERSION(g_ulVERSION), BUILDVERSION(g_ulVERSION)); 
	printf("**               RFD Version %2d.%-2d(%04d)            **\n", MAJORVERSION(RFD_GetVersion()), MINORVERSION(RFD_GetVersion()), BUILDVERSION(RFD_GetVersion())); 
	printf("******************************************************\n"); 


	// open the local RapidIO driver 
	fs = RFDSTATUS_TO_FETSTATUS(RFD_Open()); 
	if (fs != eFET_SUCCESS) 
	{ 
		exit(-1001); 
	}
	// start the data generator server 
	fs = DG_Init();
	if (fs != eFET_SUCCESS) 
	{ 
		g_bSupportsDG = 0; 
	} 
	else 
	{ 
		g_bSupportsDG = 1; 
	} 

	// start the inbound MSG/DB monitor
	fs = MsgDbMonOpen();
	if (fs != eFET_SUCCESS) 
	{ 
		//sdr sealed  //sdr sealed  fprintf(stderr, "Unable to start MSG/DB Monitor\n");; 
		g_bSupportsMonitor = 0; 
	} 
	else 
	{ 
		g_bSupportsMonitor = 1; 
	} 


	return 0;
}
#endif

 
FETSTATUS DoMessageFET_HALGETNUMLOCALPORTS (FETMSG_HEADERSTRUCT *pHdr, SOCKET *psockClient) 
{ 
	FETMSG_HEADERSTRUCT sRespHdr; 
	FETMSG_GETNUMLOCALPORTSRESPSTRUCT sRespData; 
 
	int nSent; 
	FETSTATUS fsRetVal; 
	UINT32 ulNumPorts; 
	FETSTATUS fsRet; 
 
//	printf("RFS: Rx FET_HALGETNUMLOCALPORTS\n"); 
 
	fsRet = rioGetNumLocalPorts(&ulNumPorts); 
 
	// response structure 
	sRespHdr.nMsgType = htonl(FET_HALGETNUMLOCALPORTSRESP); 
	sRespHdr.nMsgSize = htonl(sizeof(FETMSG_GETNUMLOCALPORTSRESPSTRUCT)); 

//	if ( !bSerialCommsSupport )
	{
		nSent = sizeof(sRespHdr); 
		fsRetVal = Send(psockClient, (void*)&sRespHdr, &nSent); 
		if (fsRetVal != eFET_SUCCESS) 
		{ 
			return fsRetVal; 
		} 
		if (nSent != sizeof(sRespHdr)) 
		{ 
			return eFET_ERR_SENDTOCLIENT; 
		} 

		sRespData.eStatus = (FETSTATUS)htonl(fsRet); 
		sRespData.ulNumPorts = htonl(ulNumPorts); 
 
		nSent = sizeof(sRespData); 
		fsRetVal = Send(psockClient, (void*)&sRespData, &nSent); 
		if (fsRetVal != eFET_SUCCESS) 
		{ 
			return fsRetVal; 
		} 
		if (nSent != sizeof(sRespData)) 
		{ 
			return eFET_ERR_SENDTOCLIENT; 
		} 
	}
	return eFET_SUCCESS; 
} 
 
FETSTATUS DoMessageFET_DATAGENCTRL (FETMSG_HEADERSTRUCT *pHdr, SOCKET *psockClient) 
{ 
	UINT32 i;
	FETMSG_DATAGENCTRLSTRUCT sData; 
	FETSTATUS fs; 
	FETMSG_HEADERSTRUCT sRespHdr; 
	FETMSG_DATAGENCTRLRESPSTRUCT sRespData; 
	int nSent; 
	FETSTATUS fsRetVal; 
 
//	printf("RFS: Rx FET_DATAGENCTRL\n"); 
 
	// make sure right number of bytes to follow 
	if (pHdr->nMsgSize != sizeof(FETMSG_DATAGENCTRLSTRUCT)) 
	{ 
		return eFET_ERR_WRONGSIZE; 
	} 
 
	// get a message payload from the RapidFET Client 
//	printf("RFS: reading message payload from client...\n");fflush(stdout); 
//	if ( !bSerialCommsSupport )
	{
		if (Recv(psockClient, (char *)&sData, sizeof(sData)) == -1)
		{
			perror("Unable to receive data from client"); 
			if (errno == ECONNRESET) 
			{ 
				// client has dropped the connection 
				fdClose(*psockClient);
				*psockClient = INVALID_SOCKET; 
				g_bConnected = 0; 
				printf("RFS: Waiting for connection...\n"); 
				DG_Stop();
				return eFET_SUCCESS; 
			} 
			printf("RFS: Errno is %d\n", errno);fflush(stdout); 
			return eFET_ERR_RECVPLD; 
		} 
	}
	// convert to local byte order and update global control structure 
	sData.eReqType = (eDGREQTYPE)ntohl(sData.eReqType);
	sData.ulRioTargetCtrlAddrHI = ntohl(sData.ulRioTargetCtrlAddrHI);
	sData.ulRioTargetCtrlAddrLO = ntohl(sData.ulRioTargetCtrlAddrLO);
	sData.eRioXferType = (eRIOXFERTYPE)ntohl(sData.eRioXferType);
	sData.bRepeat = ntohl(sData.bRepeat);
	sData.eRioXferPriority = (eRIOPRIORITY)ntohl(sData.eRioXferPriority);
	sData.ulRioPacketSize = ntohl(sData.ulRioPacketSize);
	sData.wRioTargetDestID = ntohs(sData.wRioTargetDestID);
	sData.ulRioTargetDestAddrHI = ntohl(sData.ulRioTargetDestAddrHI);
	sData.ulRioTargetDestAddrLO = ntohl(sData.ulRioTargetDestAddrLO);
	sData.ulRioTargetDestSize = ntohl(sData.ulRioTargetDestSize);
	sData.wRioHostDestID = ntohs(sData.wRioHostDestID);
	sData.ulDDGEntryCount = htonl(sData.ulDDGEntryCount);
	for (i = 0; i<MAXDDGENTRIES; i++)
	{
		sData.asDDGProfile[i].ulDDGDuration = htonl(sData.asDDGProfile[i].ulDDGDuration);
		sData.asDDGProfile[i].ulDDGXferRate = htonl(sData.asDDGProfile[i].ulDDGXferRate);
	}

	// update the data generator 
	if (g_bSupportsDG) 
	{ 
		fs = DG_Command(&sData,
						&sRespData.ulDDGEntryIndex,
						&sRespData.bLagging,
						&sRespData.b1ShotComplete); 
	} 
	else 
	{ 
		fs = eRFD_ERR_NODG; 
	} 
 
	// fill response header
	sRespHdr.nMsgType = htonl(FET_DATAGENCTRLRESP); 
	sRespHdr.nMsgSize = htonl(sizeof(FETMSG_DATAGENCTRLRESPSTRUCT)); 

		// fill response data
	sRespData.sCmd.eReqType = (eDGREQTYPE)htonl(sData.eReqType);
	sRespData.sCmd.ulRioTargetCtrlAddrHI = htonl(sData.ulRioTargetCtrlAddrHI);
	sRespData.sCmd.ulRioTargetCtrlAddrLO = htonl(sData.ulRioTargetCtrlAddrLO);
	sRespData.sCmd.eRioXferType = (eRIOXFERTYPE)htonl(sData.eRioXferType);
	sRespData.sCmd.bRepeat = htonl(sData.bRepeat);
	sRespData.sCmd.eRioXferPriority = (eRIOPRIORITY)htonl(sData.eRioXferPriority);
	sRespData.sCmd.ulRioPacketSize = htonl(sData.ulRioPacketSize);
	sRespData.sCmd.wRioTargetDestID = htons(sData.wRioTargetDestID);
	sRespData.sCmd.ulRioTargetDestAddrHI = htonl(sData.ulRioTargetDestAddrHI);
	sRespData.sCmd.ulRioTargetDestAddrLO = htonl(sData.ulRioTargetDestAddrLO);
	sRespData.sCmd.ulRioTargetDestSize = htonl(sData.ulRioTargetDestSize);
	sRespData.sCmd.wRioHostDestID = htons(sData.wRioHostDestID);
	sRespData.sCmd.ulDDGEntryCount = htonl(sData.ulDDGEntryCount);
	for (i = 0; i<MAXDDGENTRIES; i++)
	{
		sRespData.sCmd.asDDGProfile[i].ulDDGDuration = htonl(sData.asDDGProfile[i].ulDDGDuration);
		sRespData.sCmd.asDDGProfile[i].ulDDGXferRate = htonl(sData.asDDGProfile[i].ulDDGXferRate);
	}

	sRespData.eStatus = (FETSTATUS)htonl(fs); 
	sRespData.ulDDGEntryIndex = htonl(sRespData.ulDDGEntryIndex);
	sRespData.bLagging = htonl(sRespData.bLagging);
	sRespData.b1ShotComplete = htonl(sRespData.b1ShotComplete);

//	if ( !bSerialCommsSupport )
	{
		// send the response header
		nSent = sizeof(sRespHdr); 
		fsRetVal = Send(psockClient, (void*)&sRespHdr, &nSent); 
		if (fsRetVal != eFET_SUCCESS) 
		{ 
			return fsRetVal; 
		} 
		if (nSent != sizeof(sRespHdr)) 
		{ 
			return eFET_ERR_SENDTOCLIENT; 
		} 

		// send the response 
		nSent = sizeof(sRespData); 
		fsRetVal = Send(psockClient, (void*)&sRespData, &nSent); 
		if (fsRetVal != eFET_SUCCESS) 
		{ 
			return fsRetVal; 
		} 

		if (nSent != sizeof(sRespData)) 
		{ 
			perror("Unable to receive data from client"); 
			return eFET_ERR_SENDTOCLIENT; 
		} 
	}
	return eFET_SUCCESS; 
} 
 
FETSTATUS DoMessageFET_HALCONFIGREAD (FETMSG_HEADERSTRUCT *pHdr, SOCKET *psockClient) 
{ 
	FETMSG_CONFIGREADSTRUCT sData; 
	void* pReadData; 
	FETSTATUS eRetVal; 
	FETMSG_HEADERSTRUCT sRespHdr; 
	FETMSG_CONFIGREADRESPSTRUCT sRespData; 
	int nSent; 
	FETSTATUS fsRetVal; 
	UINT32 ulWord; 
	UINT32 ulNewTimeMarker;
	UINT32 ulElapsedTimeUS;
 
	
	ulNewTimeMarker = 0;
	ulElapsedTimeUS = 0;
//	printf("RFS: Rx FET_HALCONFIGREAD\n"); fflush(stdout);
 
	// make sure right number of bytes to follow 
	if (pHdr->nMsgSize != sizeof(FETMSG_CONFIGREADSTRUCT)) 
	{ 
		return eFET_ERR_WRONGSIZE; 
	} 
 
	// get a message payload from the RapidFET Client 
//	printf("RFS: reading message payload from client...\n"); 
	if (Recv(psockClient, (char *)&sData, sizeof(sData)) == -1)
	{
		perror("Unable to receive data from client"); 
		if (errno == ECONNRESET) 
		{ 
			// client has dropped the connection 
			fdClose(*psockClient);
			*psockClient = INVALID_SOCKET; 
			g_bConnected = 0; 
			printf("RFS: Waiting for connection...\n"); 
			DG_Stop();
			return eFET_SUCCESS; 
		} 
		printf("RFS: Errno is %d\n", errno); 
		return eFET_ERR_RECVPLD; 
	} 
 
	// convert to local byte order 
	sData.ulDestID = ntohl(sData.ulDestID); 
	sData.ulOffset = ntohl(sData.ulOffset); 
	sData.ulWordCount = ntohl(sData.ulWordCount); 
	sData.ulWordSize = ntohl(sData.ulWordSize); 
	sData.ulOldTimeMarker = ntohl(sData.ulOldTimeMarker);
	sData.ulStride = ntohl(sData.ulStride);
	sData.eRioXferPriority = (eRIOPRIORITY)ntohl(sData.eRioXferPriority);
 
	// allocate some memory to hold the read data 
	pReadData = (void*)malloc(sData.ulWordCount*sData.ulWordSize); 
	if (pReadData == NULL) 
	{ 
		return eFET_ERR_READNOMEM; 
	} 
 
	// call the HAL function 
	eRetVal = rioConfigurationRead( 
									sData.uchLocalPort, 
									sData.uchLongDestID,
									sData.ulDestID, 
									sData.uchHopCount, 
									sData.ulOffset,  
									pReadData, 
									sData.ulWordCount,
									sData.ulWordSize,
									sData.ulStride,
									sData.eRioXferPriority); 
	if (sData.uchWantTiming != 0)
	{
		ulNewTimeMarker = RFD_GetTimeMarker();
		ulElapsedTimeUS = RFD_TimeSpanUS(sData.ulOldTimeMarker, ulNewTimeMarker);
//		printf("ET:%d us\n", ulElapsedTimeUS);
	}
 
	// fill response header
	sRespHdr.nMsgType = htonl(FET_HALCONFIGREADRESP); 
	sRespHdr.nMsgSize = htonl(sizeof(FETMSG_CONFIGREADRESPSTRUCT)+(sData.ulWordCount*sData.ulWordSize)); 

	// send the header of the first part of the response 
	nSent = sizeof(sRespHdr); 
	fsRetVal = Send(psockClient, (void*)&sRespHdr, &nSent); 
	if (fsRetVal != eFET_SUCCESS) 
	{ 
		return fsRetVal; 
	} 
	if (nSent != sizeof(sRespHdr)) 
	{ 
		return eFET_ERR_SENDTOCLIENT; 
	} 
 
	// fill response data
	sRespData.sCmd.uchLocalPort = sData.uchLocalPort; 
	sRespData.sCmd.uchLongDestID = sData.uchLongDestID;
	sRespData.sCmd.ulDestID = htonl(sData.ulDestID); 
	sRespData.sCmd.uchHopCount = sData.uchHopCount; 
	sRespData.sCmd.ulOffset = htonl(sData.ulOffset); 
	sRespData.sCmd.ulWordCount = htonl(sData.ulWordCount); 
	sRespData.sCmd.ulWordSize = htonl(sData.ulWordSize); 
	sRespData.sCmd.ulStride = htonl(sData.ulStride);
	sRespData.sCmd.eRioXferPriority = (eRIOPRIORITY)htonl(sData.eRioXferPriority);
	sRespData.eStatus = (FETSTATUS)htonl(eRetVal); 
	sRespData.ulNewTimeMarker = htonl(ulNewTimeMarker);
	sRespData.ulElapsedTimeUS = htonl(ulElapsedTimeUS);
 
	// send the data of the first part of the response 
	nSent = sizeof(sRespData); 
	fsRetVal = Send(psockClient, (void*)&sRespData, &nSent); 
	if (fsRetVal != eFET_SUCCESS) 
	{ 
		return fsRetVal; 
	} 
	if (nSent != sizeof(sRespData)) 
	{ 
		return eFET_ERR_SENDTOCLIENT; 
	} 
 
	// convert to network byte order 
	switch (sData.ulWordSize)
	{
	case 1:
		break;
	case 2:
		{
			unsigned short* aData = (unsigned short*)pReadData;
			for (ulWord=0; ulWord<sData.ulWordCount; ulWord++) 
			{ 
				aData[ulWord] = htons(aData[ulWord]); 
			} 
		}
		break;
	case 4:
		{
			UINT32* aData = (UINT32*)pReadData;
			for (ulWord=0; ulWord<sData.ulWordCount; ulWord++) 
			{ 
				aData[ulWord] = htonl(aData[ulWord]); 
			} 
		}
		break;
	default:
		break;
	}
 
	// send the read data 
	nSent = sData.ulWordCount*sData.ulWordSize; 
	fsRetVal = Send(psockClient, pReadData, &nSent); 
	free(pReadData); 
	if (fsRetVal != eFET_SUCCESS) 
	{ 
		return fsRetVal; 
	} 
	if (nSent != (sData.ulWordCount*sData.ulWordSize)) 
	{ 
		return eFET_ERR_SENDTOCLIENT; 
	} 

	return eFET_SUCCESS; 
} 
 
FETSTATUS DoMessageFET_HALCONFIGWRITE (FETMSG_HEADERSTRUCT *pHdr, SOCKET *psockClient) 
{ 
	FETMSG_CONFIGWRITESTRUCT sData; 
	void* pWriteData; 
	FETSTATUS eRetVal; 
	FETMSG_HEADERSTRUCT sRespHdr; 
	FETMSG_CONFIGWRITERESPSTRUCT sRespData; 
	int nSent; 
	FETSTATUS fsRetVal; 
	UINT32 ulWord;
 
//	printf("RFS: Rx FET_HALCONFIGWRITE\n"); 
 
	// make sure right number of bytes to follow 
	if (pHdr->nMsgSize <= sizeof(FETMSG_CONFIGWRITESTRUCT)) 
	{ 
		return eFET_ERR_WRONGSIZE; 
	} 
 
	// get a message payload from the RapidFET Client 
//	printf("RFS: reading message payload from client...\n"); 
	if (Recv(psockClient, (char *)&sData, sizeof(sData)) == -1)
	{
		perror("Unable to receive data from client"); 
		if (errno == ECONNRESET) 
		{ 
			// client has dropped the connection 
			fdClose(*psockClient);
			*psockClient = INVALID_SOCKET; 
			g_bConnected = 0; 
			printf("RFS: Waiting for connection...\n"); 
			DG_Stop();
			return eFET_SUCCESS; 
		} 
		printf("RFS: Errno is %d\n", errno); 
		return eFET_ERR_RECVPLD; 
	} 
 
	// convert to local byte order 
	sData.ulDestID = ntohl(sData.ulDestID); 
	sData.ulOffset = ntohl(sData.ulOffset); 
	sData.ulWordCount = ntohl(sData.ulWordCount);
	sData.ulWordSize = ntohl(sData.ulWordSize);
	sData.ulStride = ntohl(sData.ulStride);
	sData.eRioXferPriority = (eRIOPRIORITY)ntohl(sData.eRioXferPriority);
 
	// allocate some memory to hold the write data 
	pWriteData = (void*)malloc(sData.ulWordCount*sData.ulWordSize); 
	if (pWriteData == NULL) 
	{ 
		return eFET_ERR_WRITENOMEM; 
	} 
 
	// get write data 
//	printf("Receiving %d bytes of data to be written\n", sData.ulWordCount*sData.ulWordSize); 
	if (Recv(psockClient, (char *)pWriteData, sData.ulWordCount*sData.ulWordSize) == -1)
	{
		perror("Unable to receive data from client"); 
		if (errno == ECONNRESET) 
		{ 
			// client has dropped the connection 
			fdClose(*psockClient);
			*psockClient = INVALID_SOCKET; 
			g_bConnected = 0; 
			printf("RFS: Waiting for connection...\n"); 
			DG_Stop();
			return eFET_SUCCESS; 
		} 
		printf("RFS: Errno is %d\n", errno); 
		return eFET_ERR_RECVPLD; 
	} 

	// convert to network byte order 
	switch (sData.ulWordSize)
	{
	case 1:
		break;
	case 2:
		{
			unsigned short* aData = (unsigned short*)pWriteData;
			for (ulWord=0; ulWord<sData.ulWordCount; ulWord++) 
			{ 
				aData[ulWord] = htons(aData[ulWord]); 
			} 
		}
		break;
	case 4:
		{
			UINT32* aData = (UINT32*)pWriteData;
			for (ulWord=0; ulWord<sData.ulWordCount; ulWord++) 
			{ 
				aData[ulWord] = htonl(aData[ulWord]); 
			} 
		}
		break;
	default:
		break;
	}

	// call the HAL function 
	eRetVal = rioConfigurationWrite( 
									sData.uchLocalPort,
									sData.uchLongDestID, 
									sData.ulDestID, 
									sData.uchHopCount, 
									sData.ulOffset,  
									pWriteData, 
									sData.ulWordCount,
									sData.ulWordSize,
									sData.ulStride,
									sData.eRioXferPriority); 
 
	free(pWriteData); 
 
 
	// fill response header
	sRespHdr.nMsgType = htonl(FET_HALCONFIGWRITERESP); 
	sRespHdr.nMsgSize = htonl(sizeof(FETMSG_CONFIGWRITERESPSTRUCT)); 

	// send response header
	nSent = sizeof(sRespHdr); 
	fsRetVal = Send(psockClient, (void*)&sRespHdr, &nSent); 
	if (fsRetVal != eFET_SUCCESS) 
	{ 
		return fsRetVal; 
	} 
	if (nSent != sizeof(sRespHdr)) 
	{ 
		return eFET_ERR_SENDTOCLIENT; 
	} 

	// fill response data
	sRespData.sCmd.uchLocalPort = sData.uchLocalPort; 
	sRespData.sCmd.uchLongDestID = sData.uchLongDestID;
	sRespData.sCmd.ulDestID = htonl(sData.ulDestID); 
	sRespData.sCmd.uchHopCount = sData.uchHopCount; 
	sRespData.sCmd.ulOffset = htonl(sData.ulOffset); 
	sRespData.sCmd.ulWordCount = htonl(sData.ulWordCount); 
	sRespData.sCmd.ulWordSize = htonl(sData.ulWordSize); 
	sRespData.sCmd.ulStride = htonl(sData.ulStride);
	sRespData.sCmd.eRioXferPriority = (eRIOPRIORITY)htonl(sData.eRioXferPriority);
	sRespData.eStatus = (FETSTATUS)htonl(eRetVal); 
 
	// send response data
	nSent = sizeof(sRespData); 
	fsRetVal = Send(psockClient, (void*)&sRespData, &nSent); 
	if (fsRetVal != eFET_SUCCESS) 
	{ 
		return fsRetVal; 
	} 
	if (nSent != sizeof(sRespData)) 
	{ 
		return eFET_ERR_SENDTOCLIENT; 
	} 
	return eFET_SUCCESS; 
} 
 
FETSTATUS DoMessageFET_CLOSE (FETMSG_HEADERSTRUCT *pHdr, SOCKET *psockClient) 
{ 
	// client is closing, so we will too 
	printf("RFS: Rx FET_CLOSE\n"); 
//if ( !bSerialCommsSupport )
{
	fdClose(*psockClient);
	*psockClient = INVALID_SOCKET; 
	g_bConnected = 0; 
}
	printf("RFS: Waiting for connection...\n"); 
	DG_Stop();
	return eFET_SUCCESS; 
} 
 
FETSTATUS DoMessageFET_IDENTIFY (FETMSG_HEADERSTRUCT *pHdr, SOCKET *psockClient) 
{ 
	FETMSG_HEADERSTRUCT sRespHdr; 
	FETMSG_IDENTIFYRESPSTRUCT sRespData; 
	int nSent; 
	FETSTATUS fsRetVal; 
 
	printf("RFS: Rx FET_IDENTIFY\n"); 
 
	// build a FET_IDENTIFYRESP message... 
	BuildFET_IDENTIFYRESP(&sRespHdr, &sRespData); 
 
//	if ( !bSerialCommsSupport )
	{
		// ...and send it back to where the request came from 
		nSent = sizeof(sRespHdr); 
		fsRetVal = Send(psockClient, (void*)&sRespHdr, &nSent); 
		if (fsRetVal != eFET_SUCCESS) 
		{ 
			return fsRetVal; 
		} 
		if (nSent != sizeof(sRespHdr)) 
		{ 
			return eFET_ERR_SENDTOCLIENT; 
		} 

		nSent = sizeof(sRespData); 
		fsRetVal = Send(psockClient, (void*)&sRespData, &nSent); 
		if (fsRetVal != eFET_SUCCESS) 
		{ 
			return fsRetVal; 
		} 
		if (nSent != sizeof(sRespData)) 
		{ 
			return eFET_ERR_SENDTOCLIENT; 
		}
	}
	return eFET_SUCCESS; 
} 
 
FETSTATUS Send (SOCKET* psockClient, void* pData, int* pnBytes) 
{ 
	int nSent; 
//	printf("RFS: send %d bytes to server using socket %08lx\n", *pnBytes, *psockClient); 
 
#ifdef NO_TCP
	nSent = sendto(*psockClient, ( char * )pData, *pnBytes, 0, (struct sockaddr*)&g_addrFrom, sizeof(g_addrFrom)); 
#else
	nSent = send(*psockClient, ( char * )pData, *pnBytes, 0); 
#endif
	if (nSent != *pnBytes) 
	{ 
		printf("RFS: Unable to send.  errno is %d\n", errno); 
		return eFET_ERR_SENDTOCLIENT; 
	} 
	return eFET_SUCCESS; 
} 
 
void BuildFET_IDENTIFYRESP(FETMSG_HEADERSTRUCT* psHdr, FETMSG_IDENTIFYRESPSTRUCT* psData) 
{ 
	UINT32 ul; 
 
	// build a FET_IDENTIFYRESP message... 
	// response structure 
	psHdr->nMsgType = htonl(FET_IDENTIFYRESP); 
	psHdr->nMsgSize = htonl(sizeof(FETMSG_IDENTIFYRESPSTRUCT)); 
	if (g_bConnected)
	{
		psData->m_uchStatus = RFSSTATUS_CONNECTED;
	}
	else 
	{
		psData->m_uchStatus = RFSSTATUS_LISTENING;
	}
	psData->m_ulFETMsgVersion = htonl(FETMSG_VERSION);
	psData->m_uchMajorServerRevision = (unsigned char)(MAJORVERSION(g_ulVERSION)); 
	psData->m_uchMinorServerRevision = (unsigned char)(MINORVERSION(g_ulVERSION)); 
	psData->m_ulMajorDriverRevision = (UINT32)(MAJORVERSION(RFD_GetVersion())); 
	psData->m_ulMinorDriverRevision = (UINT32)(MINORVERSION(RFD_GetVersion())); 
	psData->m_wServerRTOS = htons(RTOS_ID_DSPBIOS);
	psData->m_wServerProc = htons(PROC_ID_TI6678);
 
	// get device and vendor info from RapidIO 
	if (rioConfigurationRead (0, 0, 0xffffffff, 0, RIO_DIDCAR, &ul, 1, 4, 0, 0) != eFET_SUCCESS) 
	{ 
		printf("RFS: Unable to read RIO_DIDCAR\n"); 
		psData->m_wDeviceID = 0xffff; 
		psData->m_wVendorID = 0xffff; 
	} 
	else 
	{ 
		psData->m_wDeviceID = (unsigned short)(ul>>16)&0xffff; 
		psData->m_wVendorID = (unsigned short)(ul&0xffff); 
	} 
	// get the host ID 
	if (rioConfigurationRead (0, 0, 0xffffffff, 0, RIO_BDIDCSR, &ul, 1, 4, 0, 0) != eFET_SUCCESS) 
	{ 
		printf("RFS: Unable to read RIO_BDIDCSR\n"); 
		psData->m_wShortDestID = 0xffff;
		psData->m_wLongDestID = 0xffff; 
		psData->m_bSupportsLargeTransport = 0;
	} 
	else 
	{ 
		psData->m_wShortDestID = (unsigned short)((ul>>16)&0xff); 
		psData->m_wLongDestID = (unsigned short)(ul&0xffff);
	} 
	// find out if large transport is supported
	if (rioConfigurationRead(0, 0, 0xffffffff, 0, RIO_PEFCAR, &ul, 1, 4, 0, 0) != eFET_SUCCESS)
	{
		psData->m_bSupportsLargeTransport = 0;
	}
	else
	{
		psData->m_bSupportsLargeTransport = ((ul>>4)&1);
	}
	// get number of local ports from RapidIO 
	if (rioConfigurationRead (0, 0, 0xffffffff, 0, RIO_SPICAR, &ul, 1, 4, 0, 0) != eFET_SUCCESS) 
	{ 
		printf("RFS: Unable to read RIO_SPICAR\n"); 
		psData->m_ulNumLocalPorts = 0xff; 
	} 
	else 
	{ 
		psData->m_ulNumLocalPorts = (ul>>8)&0xff; 
	} 
	psData->m_wDeviceID = htons(psData->m_wDeviceID); 
	psData->m_wVendorID = htons(psData->m_wVendorID); 
	psData->m_wShortDestID = htons(psData->m_wShortDestID); 
	psData->m_wLongDestID = htons(psData->m_wLongDestID); 
	psData->m_bSupportsLargeTransport = htonl(psData->m_bSupportsLargeTransport); 
	psData->m_ulNumLocalPorts = htonl(psData->m_ulNumLocalPorts); 
	psData->m_ulMajorDriverRevision = htonl(psData->m_ulMajorDriverRevision);
	psData->m_ulMinorDriverRevision = htonl(psData->m_ulMinorDriverRevision);
} 


int Recv(SOCKET* psockClient, char* buf, int len)
{
	int nRecv;
	int status;
	char* p;

	p = buf;
	nRecv = 0;
	while (len > 0)
	{

		status = recv(*psockClient, p, len, 0);  
		if (status <= 0)
		{
			printf("recv error status=%d\n", status);
			break;
		}
		nRecv += status;
		len -= status;
		p += status;
	}
	return nRecv;
}

FETSTATUS DoMessageFET_HALRIOREAD (FETMSG_HEADERSTRUCT *pHdr, SOCKET *psockClient) 
{ 
	FETMSG_RIOREADSTRUCT sData; 
	unsigned char* puchReadData; 
	FETSTATUS eRetVal; 
	FETMSG_HEADERSTRUCT sRespHdr; 
	FETMSG_RIOREADRESPSTRUCT sRespData; 
	int nSent; 
	FETSTATUS fsRetVal; 
 
	
//	printf("RFS: Rx FET_HALRIOREAD\n"); 
 
	// make sure right number of bytes to follow 
	if (pHdr->nMsgSize != sizeof(FETMSG_RIOREADSTRUCT)) 
	{ 
		return eFET_ERR_WRONGSIZE; 
	} 
 
	// get a message payload from the RapidFET Client 
//	printf("RFS: reading message payload from client...\n"); 
	if (Recv(psockClient, (char *)&sData, sizeof(sData)) == -1)
	{
		perror("Unable to receive data from client"); 
		if (errno == ECONNRESET) 
		{ 
			// client has dropped the connection 
			fdClose(*psockClient);
			*psockClient = INVALID_SOCKET; 
			g_bConnected = 0; 
			printf("RFS: Waiting for connection...\n"); 
			DG_Stop();
			return eFET_SUCCESS; 
		} 
		printf("RFS: Errno is %d\n", errno); 
		return eFET_ERR_RECVPLD; 
	} 
 
	// convert to local byte order 
	sData.ulDestID = ntohl(sData.ulDestID); 
	sData.eRioXferType = (eRIOXFERTYPE)ntohl(sData.eRioXferType);
	sData.eRioXferPriority = (eRIOPRIORITY)ntohl(sData.eRioXferPriority);
	sData.ulRioAddressHi = ntohl(sData.ulRioAddressHi);
	sData.ulRioAddressLo = ntohl(sData.ulRioAddressLo);
	sData.ulByteCount = ntohl(sData.ulByteCount);
 
	// allocate some memory to hold the read data 
	puchReadData = (unsigned char*)malloc(sData.ulByteCount); 
	if (puchReadData == NULL) 
	{ 
		return eFET_ERR_READNOMEM; 
	} 
 
	// call the HAL function 
	eRetVal = rioDataRead(
						sData.uchLocalPort, 
						sData.uchLongDestID,
                        sData.ulDestID,
						sData.ulRioAddressHi,
						sData.ulRioAddressLo,
						sData.eRioXferType,
						sData.eRioXferPriority,
						puchReadData, 
						sData.ulByteCount); 
 
	// fill response header
	sRespHdr.nMsgType = htonl(FET_HALRIOREADRESP); 
	sRespHdr.nMsgSize = htonl(sizeof(FETMSG_RIOREADRESPSTRUCT)+(sData.ulByteCount)); 

	// send the header of the first part of the response 
	nSent = sizeof(sRespHdr); 
	fsRetVal = Send(psockClient, (void*)&sRespHdr, &nSent); 
	if (fsRetVal != eFET_SUCCESS) 
	{ 
		return fsRetVal; 
	} 
	if (nSent != sizeof(sRespHdr)) 
	{ 
		return eFET_ERR_SENDTOCLIENT; 
	} 
 
	// fill response data
	sRespData.sCmd.uchLocalPort = sData.uchLocalPort; 
	sRespData.sCmd.uchLongDestID = sData.uchLongDestID;
	sRespData.sCmd.ulDestID = htonl(sData.ulDestID); 
	sRespData.sCmd.eRioXferType = (eRIOXFERTYPE)htonl(sData.eRioXferType);
	sRespData.sCmd.eRioXferPriority = (eRIOPRIORITY)htonl(sData.eRioXferPriority);
	sRespData.sCmd.ulRioAddressHi = htonl(sData.ulRioAddressHi); 
	sRespData.sCmd.ulRioAddressLo = htonl(sData.ulRioAddressLo); 
	sRespData.sCmd.ulByteCount = htonl(sData.ulByteCount); 
	sRespData.eStatus = (FETSTATUS)htonl(eRetVal); 
 
	// send the data of the first part of the response 
	nSent = sizeof(sRespData); 
	fsRetVal = Send(psockClient, (void*)&sRespData, &nSent); 
	if (fsRetVal != eFET_SUCCESS) 
	{ 
		return fsRetVal; 
	} 
	if (nSent != sizeof(sRespData)) 
	{ 
		return eFET_ERR_SENDTOCLIENT; 
	} 
 
	// send the read data 
	nSent = sData.ulByteCount; 
	fsRetVal = Send(psockClient, (void*)puchReadData, &nSent); 
	free(puchReadData); 
	if (fsRetVal != eFET_SUCCESS) 
	{ 
		return fsRetVal; 
	} 
	if (nSent != sData.ulByteCount) 
	{ 
		return eFET_ERR_SENDTOCLIENT; 
	} 
 
	return eFET_SUCCESS; 
} 
 
FETSTATUS DoMessageFET_HALRIOWRITE (FETMSG_HEADERSTRUCT *pHdr, SOCKET *psockClient) 
{ 
	FETMSG_RIOWRITESTRUCT sData; 
	unsigned char* puchWriteData; 
	FETSTATUS eRetVal; 
	FETMSG_HEADERSTRUCT sRespHdr; 
	FETMSG_RIOWRITERESPSTRUCT sRespData; 
	int nSent; 
	FETSTATUS fsRetVal; 

//	printf("RFS: Rx FET_HALRIOWRITE\n"); 
 
	// make sure right number of bytes to follow 
	if (pHdr->nMsgSize <= sizeof(FETMSG_RIOWRITESTRUCT)) 
	{ 
		return eFET_ERR_WRONGSIZE; 
	} 
 
	// get a message payload from the RapidFET Client 
//	printf("RFS: reading message payload from client...\n"); 
	if (Recv(psockClient, (char *)&sData, sizeof(sData)) == -1)
	{
		perror("Unable to receive data from client"); 
		if (errno == ECONNRESET) 
		{ 
			// client has dropped the connection 
			fdClose(*psockClient);
			*psockClient = INVALID_SOCKET; 
			g_bConnected = 0; 
			printf("RFS: Waiting for connection...\n"); 
			DG_Stop();
			return eFET_SUCCESS; 
		} 
		printf("RFS: Errno is %d\n", errno); 
		return eFET_ERR_RECVPLD; 
	} 
//	printf("	read %d bytes\n", sizeof(sData));
 
	// convert to local byte order 
	sData.ulDestID = ntohl(sData.ulDestID); 
	sData.eRioXferType = (eRIOXFERTYPE)ntohl(sData.eRioXferType);
	sData.eRioXferPriority = (eRIOPRIORITY)ntohl(sData.eRioXferPriority);
	sData.ulRioAddressHi = ntohl(sData.ulRioAddressHi);
	sData.ulRioAddressLo = ntohl(sData.ulRioAddressLo);
	sData.ulByteCount = ntohl(sData.ulByteCount);
 
	// allocate some memory to hold the write data 
	puchWriteData = (unsigned char*)malloc(sData.ulByteCount); 
	if (puchWriteData == NULL) 
	{ 
		return eFET_ERR_WRITENOMEM; 
	} 
 
	// get write data 
//	printf("Receiving %d bytes of data to be written\n", sData.ulByteCount); 
	if (Recv(psockClient, (char *)puchWriteData, sData.ulByteCount) == -1)
	{
		perror("Unable to receive data from client"); 
		if (errno == ECONNRESET) 
		{ 
			// client has dropped the connection 
			fdClose(*psockClient);
			*psockClient = INVALID_SOCKET; 
			g_bConnected = 0; 
			printf("RFS: Waiting for connection...\n"); 
			DG_Stop();
			return eFET_SUCCESS; 
		} 
		printf("RFS: Errno is %d\n", errno); 
		return eFET_ERR_RECVPLD; 
	} 
 
	// call the HAL function 
	eRetVal = rioDataWrite(
        				sData.uchLocalPort,
						sData.uchLongDestID,
                        sData.ulDestID,
						sData.ulRioAddressHi,
						sData.ulRioAddressLo,
						sData.eRioXferType,
						sData.eRioXferPriority,
						puchWriteData,
                        sData.ulByteCount); 
 
	free(puchWriteData); 
 
 
	// fill response header
	sRespHdr.nMsgType = htonl(FET_HALRIOWRITERESP); 
	sRespHdr.nMsgSize = htonl(sizeof(FETMSG_RIOWRITERESPSTRUCT)); 

	// send response header
	nSent = sizeof(sRespHdr); 
//	printf("Sending %d bytes response header\n", nSent);
	fsRetVal = Send(psockClient, (void*)&sRespHdr, &nSent); 
	if (fsRetVal != eFET_SUCCESS) 
	{ 
		return fsRetVal; 
	} 
	if (nSent != sizeof(sRespHdr)) 
	{ 
		return eFET_ERR_SENDTOCLIENT; 
	} 

	// fill response data
	sRespData.sCmd.uchLocalPort = sData.uchLocalPort; 
	sRespData.sCmd.uchLongDestID = sData.uchLongDestID;
	sRespData.sCmd.ulDestID = htonl(sData.ulDestID); 
	sRespData.sCmd.eRioXferType = (eRIOXFERTYPE)htonl(sData.eRioXferType);
	sRespData.sCmd.eRioXferPriority = (eRIOPRIORITY)htonl(sData.eRioXferPriority);
	sRespData.sCmd.ulRioAddressHi = htonl(sData.ulRioAddressHi); 
	sRespData.sCmd.ulRioAddressLo = htonl(sData.ulRioAddressLo); 
	sRespData.sCmd.ulByteCount = htonl(sData.ulByteCount); 
	sRespData.eStatus = (FETSTATUS)htonl(eRetVal); 
 
	// send response data
	nSent = sizeof(sRespData); 
//	printf("Sending %d byte response payload\n", nSent);
	fsRetVal = Send(psockClient, (void*)&sRespData, &nSent); 
	if (fsRetVal != eFET_SUCCESS) 
	{ 
		return fsRetVal; 
	} 
	if (nSent != sizeof(sRespData)) 
	{ 
		return eFET_ERR_SENDTOCLIENT; 
	} 
	return eFET_SUCCESS; 
} 
 

FETSTATUS DoMessageFET_HALRIOMESSAGETX (FETMSG_HEADERSTRUCT *pHdr, SOCKET *psockClient) 
{ 
	FETMSG_RIOTXMESSAGESTRUCT sData; 
	unsigned char* puchWriteData; 
	FETSTATUS eRetVal; 
	FETMSG_HEADERSTRUCT sRespHdr; 
	FETMSG_RIOTXMESSAGERESPSTRUCT sRespData; 
	int nSent; 
	FETSTATUS fsRetVal; 

//	printf("RFS: Rx FET_HALRIOMESSAGETX\n"); 
 
	// make sure right number of bytes to follow 
	if (pHdr->nMsgSize <= sizeof(FETMSG_RIOTXMESSAGESTRUCT)) 
	{ 
		return eFET_ERR_WRONGSIZE; 
	} 
 
	// get a message payload from the RapidFET Client 
//	printf("RFS: reading message payload from client...\n"); 
	if (Recv(psockClient, (char *)&sData, sizeof(sData)) == -1)
	{
		perror("Unable to receive data from client"); 
		if (errno == ECONNRESET) 
		{ 
			// client has dropped the connection 
			fdClose(*psockClient);
			*psockClient = INVALID_SOCKET; 
			g_bConnected = 0; 
			printf("RFS: Waiting for connection...\n"); 
			DG_Stop();
			return eFET_SUCCESS; 
		} 
		printf("RFS: Errno is %d\n", errno); 
		return eFET_ERR_RECVPLD; 
	} 
//	printf("	read %d bytes\n", sizeof(sData));
 
	// convert to local byte order 
	sData.ulDestID = ntohl(sData.ulDestID); 
	sData.eRioXferPriority = (eRIOPRIORITY)ntohl(sData.eRioXferPriority);
	sData.bMultiPacket = ntohl(sData.bMultiPacket);
	sData.ulMailboxNum = ntohl(sData.ulMailboxNum);
	sData.ulPacketByteCount = ntohl(sData.ulPacketByteCount);
	sData.ulTotalByteCount = ntohl(sData.ulTotalByteCount);
 
	// allocate some memory to hold the write data 
	puchWriteData = (unsigned char*)malloc(sData.ulTotalByteCount); 
	if (puchWriteData == NULL) 
	{ 
		return eFET_ERR_WRITENOMEM; 
	} 
 
	// get write data 
//	printf("Receiving %d bytes of data to be written\n", sData.ulTotalByteCount); 
	if (Recv(psockClient, (char *)puchWriteData, sData.ulTotalByteCount) == -1)
	{
		perror("Unable to receive data from client"); 
		if (errno == ECONNRESET) 
		{ 
			// client has dropped the connection 
			fdClose(*psockClient);
			*psockClient = INVALID_SOCKET; 
			g_bConnected = 0; 
			printf("RFS: Waiting for connection...\n"); 
			DG_Stop();
			return eFET_SUCCESS; 
		} 
		printf("RFS: Errno is %d\n", errno); 
		return eFET_ERR_RECVPLD; 
	} 
 
	// call the HAL function 
	eRetVal = rioMessageSend(
        				sData.uchLocalPort,
						sData.uchLongDestID,
                        sData.ulDestID,
						sData.eRioXferPriority,
						sData.bMultiPacket,
						sData.ulMailboxNum,
						puchWriteData,
                        sData.ulPacketByteCount,
                        sData.ulTotalByteCount); 
 
	free(puchWriteData); 
 
 
	// fill response header
	sRespHdr.nMsgType = htonl(FET_HALRIOTXMESSAGERESP); 
	sRespHdr.nMsgSize = htonl(sizeof(FETMSG_RIOTXMESSAGERESPSTRUCT)); 

	// send response header
	nSent = sizeof(sRespHdr); 
//	printf("Sending %d bytes response header\n", nSent);
	fsRetVal = Send(psockClient, (void*)&sRespHdr, &nSent); 
	if (fsRetVal != eFET_SUCCESS) 
	{ 
		return fsRetVal; 
	} 
	if (nSent != sizeof(sRespHdr)) 
	{ 
		return eFET_ERR_SENDTOCLIENT; 
	} 

	// fill response data
	sRespData.sCmd.uchLocalPort = sData.uchLocalPort; 
	sRespData.sCmd.uchLongDestID = sData.uchLongDestID;
	sRespData.sCmd.ulDestID = htonl(sData.ulDestID); 
	sRespData.sCmd.eRioXferPriority = (eRIOPRIORITY)htonl(sData.eRioXferPriority);
	sRespData.sCmd.bMultiPacket = htonl(sData.bMultiPacket);
	sRespData.sCmd.ulMailboxNum = htonl(sData.ulMailboxNum); 
	sRespData.sCmd.ulPacketByteCount = htonl(sData.ulPacketByteCount); 
	sRespData.sCmd.ulTotalByteCount = htonl(sData.ulTotalByteCount); 
	sRespData.eStatus = (FETSTATUS)htonl(eRetVal); 
 
	// send response data
	nSent = sizeof(sRespData); 
//	printf("Sending %d byte response payload\n", nSent);
	fsRetVal = Send(psockClient, (void*)&sRespData, &nSent); 
	if (fsRetVal != eFET_SUCCESS) 
	{ 
		return fsRetVal; 
	} 
	if (nSent != sizeof(sRespData)) 
	{ 
		return eFET_ERR_SENDTOCLIENT; 
	} 
	return eFET_SUCCESS; 
} 
 
FETSTATUS DoMessageFET_HALRIOMESSAGERX (FETMSG_HEADERSTRUCT *pHdr, SOCKET *psockClient) 
{ 
	FETMSG_RIORXMESSAGESTRUCT sData; 
	unsigned char* puchMsgData; 
	FETSTATUS eRetVal; 
	FETMSG_HEADERSTRUCT sRespHdr; 
	FETMSG_RIORXMESSAGERESPSTRUCT sRespData; 
	int nSent; 
	FETSTATUS fsRetVal; 

	unsigned char bLargeTT;
	unsigned char bOverflow;
	UINT32 ulSourceID;
	UINT32 ulPriority;
	UINT32 ulMailboxNum;
	UINT32 ulMessageLengthBytes;
	int nBytesToGo;
	unsigned char* puch;
	
//	printf("RFS: Rx FET_HALRIORXMESSAGE\n"); 
 
	// make sure right number of bytes to follow 
	if (pHdr->nMsgSize != sizeof(FET_HALRIORXMESSAGE)) 
	{ 
		return eFET_ERR_WRONGSIZE; 
	} 
 
	// get a message payload from the RapidFET Client 
//	printf("RFS: reading message payload from client...\n"); 
	if (Recv(psockClient, (char *)&sData, sizeof(sData)) == -1)
	{
		perror("Unable to receive data from client"); 
		if (errno == ECONNRESET) 
		{ 
			// client has dropped the connection 
			fdClose(*psockClient);
			*psockClient = INVALID_SOCKET; 
			g_bConnected = 0; 
			printf("RFS: Waiting for connection...\n"); 
			DG_Stop();
			return eFET_SUCCESS; 
		} 
		printf("RFS: Errno is %d\n", errno); 
		return eFET_ERR_RECVPLD; 
	} 
 
	// convert to local byte order 
	sData.ulCommand = ntohl(sData.ulCommand); 
	sData.ulWhichQueue = (eRIOXFERTYPE)ntohl(sData.ulWhichQueue);
 
	// allocate some memory to hold the read data 
	puchMsgData = (unsigned char*)malloc(4096); 	// max message size is 4k
	if (puchMsgData == NULL) 
	{ 
		return eFET_ERR_READNOMEM; 
	} 
 
	// call the HAL function 
	eRetVal = rioMessageGet(
						sData.ulWhichQueue, 
						sData.ulCommand,
                        &bLargeTT,
						&bOverflow,
						&ulSourceID,
						&ulPriority,
						&ulMailboxNum,
						&ulMessageLengthBytes, 
						puchMsgData); 
 
	// fill response header
	sRespHdr.nMsgType = htonl(FET_HALRIORXMESSAGERESP); 
	sRespHdr.nMsgSize = htonl(sizeof(FETMSG_RIORXMESSAGERESPSTRUCT)+ulMessageLengthBytes); 

	// send the header of the first part of the response 
	nSent = sizeof(sRespHdr); 
	fsRetVal = Send(psockClient, (void*)&sRespHdr, &nSent); 
	if (fsRetVal != eFET_SUCCESS) 
	{ 
		return fsRetVal; 
	} 
	if (nSent != sizeof(sRespHdr)) 
	{ 
		return eFET_ERR_SENDTOCLIENT; 
	} 
 
	// fill response data
	sRespData.sCmd.ulCommand = htonl(sData.ulCommand);
	sRespData.sCmd.ulWhichQueue = htonl(sData.ulWhichQueue);
	switch(ulPriority)
	{
	case 0:
		sRespData.eRioXferPriority = (eRIOPRIORITY)htonl(eRIOPRIORITY_L);
		break;
	case 1:
		sRespData.eRioXferPriority = (eRIOPRIORITY)htonl(eRIOPRIORITY_M);
		break;
	default:
	case 2:
		sRespData.eRioXferPriority = (eRIOPRIORITY)htonl(eRIOPRIORITY_H);
		break;
	}
	sRespData.uchLargeTT = bLargeTT;
	sRespData.uchOverflow = bOverflow;
	sRespData.ulMailboxNum = ntohl(ulMailboxNum);
	sRespData.ulMessageLengthBytes = ntohl(ulMessageLengthBytes);
	sRespData.ulSourceID = htonl(ulSourceID);
	sRespData.eStatus = (FETSTATUS)htonl(eRetVal); 
 
	// send the response data
	nSent = sizeof(sRespData); 
	fsRetVal = Send(psockClient, (void*)&sRespData, &nSent); 
	if (fsRetVal != eFET_SUCCESS) 
	{ 
		return fsRetVal; 
	} 
	if (nSent != sizeof(sRespData)) 
	{ 
		return eFET_ERR_SENDTOCLIENT; 
	} 
 
	// send the message data in 512 byte chunks...
	nBytesToGo = ulMessageLengthBytes;
	puch = puchMsgData;
	while (nBytesToGo)
	{
		int nToSend = nSent = nBytesToGo>512?512:nBytesToGo; 
		fsRetVal = Send(psockClient, (void*)puch, &nSent); 
		if (fsRetVal != eFET_SUCCESS) 
		{ 
			printf("Send failed with error %d\n", fsRetVal);
			free(puchMsgData); 
			return fsRetVal; 
		} 
		if (nSent != nToSend) 
		{ 
			perror("Send failed");
			free(puchMsgData); 
			return eFET_ERR_SENDTOCLIENT; 
		} 
		nBytesToGo -= nSent;
		puch += nSent;
	};
	free(puchMsgData); 
	 
	return eFET_SUCCESS; 
}

FETSTATUS DoMessageFET_HALRIODOORBELLTX (FETMSG_HEADERSTRUCT *pHdr, SOCKET *psockClient) 
{ 
	FETMSG_RIOTXDOORBELLSTRUCT sData; 
	FETSTATUS eRetVal; 
	FETMSG_HEADERSTRUCT sRespHdr; 
	FETMSG_RIOTXDOORBELLRESPSTRUCT sRespData; 
	int nSent; 
	FETSTATUS fsRetVal; 

//	printf("RFS: Rx FET_HALDOORBELL\n"); 
 
	// make sure right number of bytes to follow 
	if (pHdr->nMsgSize != sizeof(FETMSG_RIOTXDOORBELLSTRUCT)) 
	{ 
		printf("Wrong message size:  %d <> %d\n", pHdr->nMsgSize, sizeof(FETMSG_RIOTXDOORBELLSTRUCT));
		return eFET_ERR_WRONGSIZE; 
	} 
 
	// get a doorbell payload from the RapidFET Client 
//	printf("RFS: reading doorbell payload from client...\n"); 
	if (Recv(psockClient, (char *)&sData, sizeof(sData)) == -1)
	{
		perror("Unable to receive data from client"); 
		if (errno == ECONNRESET) 
		{ 
			// client has dropped the connection 
			fdClose(*psockClient);
			*psockClient = INVALID_SOCKET; 
			g_bConnected = 0; 
			printf("RFS: Waiting for connection...\n"); 
			DG_Stop();
			return eFET_SUCCESS; 
		} 
		printf("RFS: Errno is %d\n", errno); 
		return eFET_ERR_RECVPLD; 
	} 
//	printf("	read %d bytes\n", sizeof(sData));
 
	// convert to local byte order 
	sData.ulDestID = ntohl(sData.ulDestID); 
	sData.eRioXferPriority = (eRIOPRIORITY)ntohl(sData.eRioXferPriority);
	sData.wInfoWord = ntohs(sData.wInfoWord);
 
	// call the HAL function 
	eRetVal = rioDoorbellSend(
        				sData.uchLocalPort,
						sData.uchLongDestID,
                        sData.ulDestID,
						sData.eRioXferPriority,
						sData.wInfoWord); 
 
	// fill response header
	sRespHdr.nMsgType = htonl(FET_HALRIOTXDOORBELLRESP); 
	sRespHdr.nMsgSize = htonl(sizeof(FETMSG_RIOTXDOORBELLRESPSTRUCT)); 

	// send response header
	nSent = sizeof(sRespHdr); 
//	printf("Sending %d bytes response header\n", nSent);
	fsRetVal = Send(psockClient, (void*)&sRespHdr, &nSent); 
	if (fsRetVal != eFET_SUCCESS) 
	{ 
		return fsRetVal; 
	} 
	if (nSent != sizeof(sRespHdr)) 
	{ 
		return eFET_ERR_SENDTOCLIENT; 
	} 

	// fill response data
	sRespData.sCmd.uchLocalPort = sData.uchLocalPort; 
	sRespData.sCmd.uchLongDestID = sData.uchLongDestID;
	sRespData.sCmd.ulDestID = htonl(sData.ulDestID); 
	sRespData.sCmd.eRioXferPriority = (eRIOPRIORITY)htonl(sData.eRioXferPriority);
	sRespData.sCmd.wInfoWord = htons(sData.wInfoWord);
	sRespData.eStatus = (FETSTATUS)htonl(eRetVal); 
 
	// send response data
	nSent = sizeof(sRespData); 
//	printf("Sending %d byte response payload\n", nSent);
	fsRetVal = Send(psockClient, (void*)&sRespData, &nSent); 
	if (fsRetVal != eFET_SUCCESS) 
	{ 
		return fsRetVal; 
	} 
	if (nSent != sizeof(sRespData)) 
	{ 
		return eFET_ERR_SENDTOCLIENT; 
	} 
	return eFET_SUCCESS; 
} 
 
FETSTATUS DoMessageFET_HALRIODOORBELLRX (FETMSG_HEADERSTRUCT *pHdr, SOCKET *psockClient) 
{
	FETMSG_RIORXDOORBELLSTRUCT sData; 
	FETSTATUS eRetVal; 
	FETMSG_HEADERSTRUCT sRespHdr; 
	FETMSG_RIORXDOORBELLRESPSTRUCT sRespData; 
	int nSent; 
	FETSTATUS fsRetVal; 

	unsigned char bValidDoorbell;
	unsigned char bOverflow;
	UINT16 wInfoWord;
	
//	printf("RFS: Rx FET_HALRIORXDOORBELL\n"); 
 
	// make sure right number of bytes to follow 
	if (pHdr->nMsgSize != sizeof(FET_HALRIORXDOORBELL)) 
	{ 
		return eFET_ERR_WRONGSIZE; 
	} 
 
	// get a doorbell payload from the RapidFET Client 
//	printf("RFS: reading doorbell payload from client...\n"); 
	if (Recv(psockClient, (char *)&sData, sizeof(sData)) == -1)
	{
		perror("Unable to receive data from client"); 
		if (errno == ECONNRESET) 
		{ 
			// client has dropped the connection 
			fdClose(*psockClient);
			*psockClient = INVALID_SOCKET; 
			g_bConnected = 0; 
			printf("RFS: Waiting for connection...\n"); 
			DG_Stop();
			return eFET_SUCCESS; 
		} 
		printf("RFS: Errno is %d\n", errno); 
		return eFET_ERR_RECVPLD; 
	} 
 
	// convert to local byte order 
	sData.ulCommand = ntohl(sData.ulCommand); 
	sData.ulWhichQueue = (eRIOXFERTYPE)ntohl(sData.ulWhichQueue);
 
	// call the HAL function 
	eRetVal = rioDoorbellGet(
						sData.ulWhichQueue, 
						sData.ulCommand,
                        &bValidDoorbell,
						&bOverflow,
						&wInfoWord); 
 
	// fill response header
	sRespHdr.nMsgType = htonl(FET_HALRIORXDOORBELLRESP); 
	sRespHdr.nMsgSize = htonl(sizeof(FETMSG_RIORXDOORBELLRESPSTRUCT)); 

	// send the header  
	nSent = sizeof(sRespHdr); 
	fsRetVal = Send(psockClient, (void*)&sRespHdr, &nSent); 
	if (fsRetVal != eFET_SUCCESS) 
	{ 
		return fsRetVal; 
	} 
	if (nSent != sizeof(sRespHdr)) 
	{ 
		return eFET_ERR_SENDTOCLIENT; 
	} 
 
	// fill response data
	sRespData.sCmd.ulCommand = htonl(sData.ulCommand);
	sRespData.sCmd.ulWhichQueue = htonl(sData.ulWhichQueue);
	sRespData.uchValidDoorbell = bValidDoorbell;
	sRespData.uchOverflow = bOverflow;
	sRespData.wInfoWord = htons(wInfoWord);
	sRespData.eStatus = (FETSTATUS)htonl(eRetVal); 
 
	// send the data of the first part of the response 
	nSent = sizeof(sRespData); 
	fsRetVal = Send(psockClient, (void*)&sRespData, &nSent); 
	if (fsRetVal != eFET_SUCCESS) 
	{ 
		return fsRetVal; 
	} 
	if (nSent != sizeof(sRespData)) 
	{ 
		return eFET_ERR_SENDTOCLIENT; 
	} 
 
	return eFET_SUCCESS; 
}

FETSTATUS DoMessageFET_HALGETRIOERROR (FETMSG_HEADERSTRUCT *pHdr, SOCKET *psockClient) 
{
	FETSTATUS eRetVal; 
	FETMSG_HEADERSTRUCT sRespHdr; 
	FETMSG_GETRIOERRORRESPHEADERSTRUCT sRespDataHeader;
	FETMSG_GETRIOERRORRESPERRORSTRUCT sRespDataError; 
	int nSent; 
	FETSTATUS fsRetVal; 
	UINT32 ulRioErrorMask=0;
	UINT32* apulRioErrorInfo[32];
	char* apszRioErrorString[32];
	UINT32 ulNumRioErrors;
	int i;
	
	printf("RFS: Rx FET_HALGETRIOERROR\n"); 
 
	// call the HAL function 
	eRetVal = rioGetRioError(&ulRioErrorMask, apulRioErrorInfo, apszRioErrorString);

 	// fill response header
	sRespHdr.nMsgType = htonl(FET_HALGETRIOERRORRESP); 
	sRespHdr.nMsgSize = htonl(sizeof(FETMSG_GETRIOERRORRESPHEADERSTRUCT)); 

	// send the header  
	nSent = sizeof(sRespHdr); 
	fsRetVal = Send(psockClient, (void*)&sRespHdr, &nSent); 
	if (fsRetVal != eFET_SUCCESS) 
	{ 
		return fsRetVal; 
	} 
	if (nSent != sizeof(sRespHdr)) 
	{ 
		return eFET_ERR_SENDTOCLIENT; 
	} 

	// send the response payload header
	ulNumRioErrors = 0;
	for (i=0; i<32; i++)
	{
		if (ulRioErrorMask & (1<<i))
		{
			ulNumRioErrors++;
		}
	}
	sRespDataHeader.ulNumRioErrors = htonl(ulNumRioErrors);
	sRespDataHeader.eStatus = (FETSTATUS)htonl(eRetVal);
 
	// send the data of the first part of the response 
	nSent = sizeof(sRespDataHeader); 
	fsRetVal = Send(psockClient, (void*)&sRespDataHeader, &nSent); 
	if (fsRetVal != eFET_SUCCESS) 
	{ 
		return fsRetVal; 
	} 
	if (nSent != sizeof(sRespDataHeader)) 
	{ 
		return eFET_ERR_SENDTOCLIENT; 
	} 
 
	// send the errors
	for (i=0; i<32; i++)
	{
		if (ulRioErrorMask & (1<<i))
		{
			sRespDataError.ulRioErrorValue = ntohl(*apulRioErrorInfo[i]);
			strncpy(sRespDataError.szRioErrorString, apszRioErrorString[i], 128);
 
			// send the data of the first part of the response 
			nSent = sizeof(sRespDataError); 
			fsRetVal = Send(psockClient, (void*)&sRespDataError, &nSent); 
			if (fsRetVal != eFET_SUCCESS) 
			{ 
				return fsRetVal; 
			} 
			if (nSent != sizeof(sRespDataError)) 
			{ 
				return eFET_ERR_SENDTOCLIENT; 
			} 
 		}
	}

	return eFET_SUCCESS; 
}

int IsPortOK(void)
{
	unsigned long ulVal;
	FETSTATUS fs;
	
	fs = rioConfigurationRead(0, 0, 0xffffffff, 0, 0x158, &ulVal, 1, 4, 4, 0);
	if (fs != eFET_SUCCESS)
	{
		//sdr sealed   fprintf(stderr, "Unable to read local port status.  Error %d\n", fs);
		return 0;
	}

	printf("Port Status: %08lx\n", ulVal);

	if (!(ulVal & 0x00000002))
	{
		//sdr sealed  fprintf(stderr, "***********************************************************************************\n");
		//sdr sealed  fprintf(stderr, "WARNING:  SRIO PORT IS CURRENTLY NOT TRAINED\n");
		//sdr sealed  fprintf(stderr, "***********************************************************************************\n");
		printf("***********************************************************************************\n");
		printf("WARNING:  SRIO PORT IS CURRENTLY NOT TRAINED\n");
		printf("***********************************************************************************\n");
		return 0;
	}

	if (ulVal & 0x00010100)
	{
		//sdr sealed  fprintf(stderr, "***********************************************************************************\n");
		//sdr sealed  fprintf(stderr, "WARNING:  SRIO PORT IS CURRENTLY IN A STOPPED STATE FOR INPUT AND/OR OUTPUT\n");
		//sdr sealed  fprintf(stderr, "***********************************************************************************\n");
		printf("***********************************************************************************\n");
		printf("WARNING:  SRIO PORT IS CURRENTLY IN A STOPPED STATE FOR INPUT AND/OR OUTPUT\n");
		printf("***********************************************************************************\n");
		return 0;
	}

	return 1;
}

static FETSTATUS DoMessageFET_MEMREAD(FETMSG_HEADERSTRUCT *pHdr, SOCKET *psockClient)
{
	FETMSG_MEMREADSTRUCT sData; 
	unsigned char* puchReadData; 
	FETSTATUS eRetVal;
	FETMSG_HEADERSTRUCT sRespHdr; 
	FETMSG_MEMREADRESPSTRUCT sRespData; 
	int nSent; 
	FETSTATUS fsRetVal; 
 
	
	printf("RFS: Rx FET_MEMREAD\n"); 
 
	// make sure right number of bytes to follow 
	if (pHdr->nMsgSize != sizeof(FETMSG_MEMREADSTRUCT)) 
	{ 
		return eFET_ERR_WRONGSIZE; 
	} 
 
	// get a message payload from the RapidFET Client 
//	printf("RFS: reading message payload from client...\n"); 
	if (Recv(psockClient, (char *)&sData, sizeof(sData)) == -1)
	{
		perror("Unable to receive data from client"); 
		if (errno == ECONNRESET) 
		{ 
			// client has dropped the connection 
			fdClose(*psockClient);
			*psockClient = INVALID_SOCKET; 
			g_bConnected = 0; 
			printf("RFS: Waiting for connection...\n"); 
			DG_Stop();
			return eFET_SUCCESS; 
		} 
		printf("RFS: Errno is %d\n", errno); 
		return eFET_ERR_RECVPLD; 
	} 
 
	// convert to local byte order 
	sData.ulAddressHi = ntohl(sData.ulAddressHi);
	sData.ulAddressLo = ntohl(sData.ulAddressLo);
	sData.ulWordCount = ntohl(sData.ulWordCount);
	sData.ulWordSize = ntohl(sData.ulWordSize);
 
	// allocate some memory to hold the read data 
	puchReadData = (unsigned char*)malloc(sData.ulWordCount * sData.ulWordSize); 
	if (puchReadData == NULL) 
	{ 
		return eFET_ERR_READNOMEM; 
	} 
 
	// Do the memory read
	eRetVal = RFDSTATUS_TO_FETSTATUS(RFD_ReadMem(sData.ulAddressHi, sData.ulAddressLo, (void*)puchReadData, sData.ulWordCount, sData.ulWordSize));

	// fill response header
	sRespHdr.nMsgType = htonl(FET_MEMREADRESP); 
	sRespHdr.nMsgSize = htonl(sizeof(FETMSG_MEMREADRESPSTRUCT)+(sData.ulWordCount * sData.ulWordSize)); 

	// send the header of the first part of the response 
	nSent = sizeof(sRespHdr); 
	fsRetVal = Send(psockClient, (void*)&sRespHdr, &nSent); 
	if (fsRetVal != eFET_SUCCESS) 
	{ 
		return fsRetVal; 
	} 
	if (nSent != sizeof(sRespHdr)) 
	{ 
		return eFET_ERR_SENDTOCLIENT; 
	} 
 
	// fill response data
	sRespData.sCmd.ulAddressHi = htonl(sData.ulAddressHi); 
	sRespData.sCmd.ulAddressLo = htonl(sData.ulAddressLo); 
	sRespData.sCmd.ulWordCount = htonl(sData.ulWordCount); 
	sRespData.sCmd.ulWordSize = htonl(sData.ulWordSize); 
	sRespData.eStatus = (FETSTATUS)htonl(eRetVal); 
 
	// send the data of the first part of the response 
	nSent = sizeof(sRespData); 
	fsRetVal = Send(psockClient, (void*)&sRespData, &nSent); 
	if (fsRetVal != eFET_SUCCESS) 
	{ 
		return fsRetVal; 
	} 
	if (nSent != sizeof(sRespData)) 
	{ 
		return eFET_ERR_SENDTOCLIENT; 
	} 
 
	// send the read data 
	nSent = sData.ulWordCount * sData.ulWordSize; 
	fsRetVal = Send(psockClient, (void*)puchReadData, &nSent); 
	free(puchReadData); 
	if (fsRetVal != eFET_SUCCESS) 
	{ 
		return fsRetVal; 
	} 
	if (nSent != (sData.ulWordCount * sData.ulWordSize)) 
	{ 
		return eFET_ERR_SENDTOCLIENT; 
	} 
 
	return eFET_SUCCESS; 
}

static FETSTATUS DoMessageFET_MEMWRITE(FETMSG_HEADERSTRUCT *pHdr, SOCKET *psockClient)
{
	FETMSG_MEMWRITESTRUCT sData; 
	unsigned char* puchWriteData; 
	FETSTATUS eRetVal;
	FETMSG_HEADERSTRUCT sRespHdr; 
	FETMSG_MEMWRITERESPSTRUCT sRespData; 
	int nSent; 
	FETSTATUS fsRetVal; 

	printf("RFS: Rx FET_HALMEMWRITE\n"); 
 
	// make sure right number of bytes to follow 
	if (pHdr->nMsgSize <= sizeof(FETMSG_MEMWRITESTRUCT)) 
	{ 
		return eFET_ERR_WRONGSIZE; 
	} 
 
	// get a message payload from the RapidFET Client 
//	printf("RFS: reading message payload from client...\n"); 
	if (Recv(psockClient, (char *)&sData, sizeof(sData)) == -1)
	{
		perror("Unable to receive data from client"); 
		if (errno == ECONNRESET) 
		{ 
			// client has dropped the connection 
			fdClose(*psockClient);
			*psockClient = INVALID_SOCKET; 
			g_bConnected = 0; 
			printf("RFS: Waiting for connection...\n"); 
			DG_Stop();
			return eFET_SUCCESS; 
		} 
		printf("RFS: Errno is %d\n", errno); 
		return eFET_ERR_RECVPLD; 
	} 
//	printf("	read %d bytes\n", sizeof(sData));
 
	// convert to local byte order 
	sData.ulAddressHi = ntohl(sData.ulAddressHi);
	sData.ulAddressLo = ntohl(sData.ulAddressLo);
	sData.ulWordCount = ntohl(sData.ulWordCount);
	sData.ulWordSize = ntohl(sData.ulWordSize);
 
	// allocate some memory to hold the write data 
	puchWriteData = (unsigned char*)malloc(sData.ulWordCount * sData.ulWordSize); 
	if (puchWriteData == NULL) 
	{ 
		return eFET_ERR_WRITENOMEM; 
	} 
 
	// get write data 
//	printf("Receiving %d bytes of data to be written\n", sData.ulByteCount); 
	if (Recv(psockClient, (char *)puchWriteData, sData.ulWordCount * sData.ulWordSize) == -1)
	{
		perror("Unable to receive data from client"); 
		if (errno == ECONNRESET) 
		{ 
			// client has dropped the connection 
			fdClose(*psockClient);
			*psockClient = INVALID_SOCKET; 
			g_bConnected = 0; 
			printf("RFS: Waiting for connection...\n"); 
			DG_Stop();
			return eFET_SUCCESS; 
		} 
		printf("RFS: Errno is %d\n", errno); 
		return eFET_ERR_RECVPLD; 
	} 
 
	// do the memory write
	eRetVal = RFDSTATUS_TO_FETSTATUS(RFD_WriteMem(sData.ulAddressHi, sData.ulAddressLo, (void*)puchWriteData, sData.ulWordCount, sData.ulWordSize));
	free(puchWriteData); 
 
 
	// fill response header
	sRespHdr.nMsgType = htonl(FET_MEMWRITERESP); 
	sRespHdr.nMsgSize = htonl(sizeof(FETMSG_MEMWRITERESPSTRUCT)); 

	// send response header
	nSent = sizeof(sRespHdr); 
//	printf("Sending %d bytes response header\n", nSent);
	fsRetVal = Send(psockClient, (void*)&sRespHdr, &nSent); 
	if (fsRetVal != eFET_SUCCESS) 
	{ 
		return fsRetVal; 
	} 
	if (nSent != sizeof(sRespHdr)) 
	{ 
		return eFET_ERR_SENDTOCLIENT; 
	} 

	// fill response data
	sRespData.sCmd.ulAddressHi = htonl(sData.ulAddressHi); 
	sRespData.sCmd.ulAddressLo = htonl(sData.ulAddressLo); 
	sRespData.sCmd.ulWordCount = htonl(sData.ulWordCount); 
	sRespData.sCmd.ulWordSize = htonl(sData.ulWordSize); 
	sRespData.eStatus = (FETSTATUS)htonl(eRetVal); 
 
	// send response data
	nSent = sizeof(sRespData); 
//	printf("Sending %d byte response payload\n", nSent);
	fsRetVal = Send(psockClient, (void*)&sRespData, &nSent); 
	if (fsRetVal != eFET_SUCCESS) 
	{ 
		return fsRetVal; 
	} 
	if (nSent != sizeof(sRespData)) 
	{ 
		return eFET_ERR_SENDTOCLIENT; 
	} 
	return eFET_SUCCESS; 
}

static FETSTATUS DoMessageFET_GETSERVERCAPS(FETMSG_HEADERSTRUCT *pHdr, SOCKET *psockClient)
{
	FETMSG_HEADERSTRUCT sRespHdr; 
	FETMSG_GETSERVERCAPSRESPSTRUCT sRespData; 
	int nSent; 
	int x;
	FETSTATUS fsRetVal; 

	printf("RFS: Rx FET_GETSERVERCAPS\n"); 
 
	// make sure right number of bytes to follow 
	if (pHdr->nMsgSize != 0) 
	{ 
		return eFET_ERR_WRONGSIZE; 
	} 
 
	// fill response header
	sRespHdr.nMsgType = htonl(FET_GETSERVERCAPSRESP); 
	sRespHdr.nMsgSize = htonl(sizeof(FETMSG_GETSERVERCAPSRESPSTRUCT)); 

	// send response header
	nSent = sizeof(sRespHdr); 
//	printf("Sending %d bytes response header\n", nSent);
	fsRetVal = Send(psockClient, (void*)&sRespHdr, &nSent); 
	if (fsRetVal != eFET_SUCCESS) 
	{ 
		return fsRetVal; 
	} 
	if (nSent != sizeof(sRespHdr)) 
	{ 
		return eFET_ERR_SENDTOCLIENT; 
	} 

	// fill response data
	sRespData.ulServerCaps1 = 0;
	sRespData.ulServerCaps1 |= SERVERCAP_MEMREAD;
	sRespData.ulServerCaps1 |= SERVERCAP_MEMWRITE;
#if 0
	sRespData.ulServerCaps1 |= SERVERCAP_LOCALTRAFFICGEN;
	sRespData.ulServerCaps1 |= SERVERCAP_REMOTETRAFFICGEN;
	sRespData.ulServerCaps1 |= SERVERCAP_DOORBELLTX;
	sRespData.ulServerCaps1 |= SERVERCAP_DOORBELLRX;
	sRespData.ulServerCaps1 |= SERVERCAP_MESSAGETX;
	sRespData.ulServerCaps1 |= SERVERCAP_MESSAGERX;
#endif
#ifdef SUPPORTS_FLASHREAD
	sRespData.ulServerCaps1 |= SERVERCAP_FLASHREAD;
#endif
#ifdef SUPPORTS_FLASHWRITE
	sRespData.ulServerCaps1 |= SERVERCAP_FLASHWRITE;
#endif
	for (x=0; x<31; x++)
	{
        sRespData.aulReserved[x] = 0;
	}
 
	// convert to network byte order
	sRespData.ulServerCaps1 = htonl(sRespData.ulServerCaps1);
 
	// send response data
	nSent = sizeof(sRespData); 
//	printf("Sending %d byte response payload\n", nSent);
	fsRetVal = Send(psockClient, (void*)&sRespData, &nSent); 
	if (fsRetVal != eFET_SUCCESS) 
	{ 
		return fsRetVal; 
	} 
	if (nSent != sizeof(sRespData)) 
	{ 
		return eFET_ERR_SENDTOCLIENT; 
	} 
	return eFET_SUCCESS; 
}

#ifdef SUPPORTS_FLASHREAD
static FETSTATUS DoMessageFET_FLASHREAD(FETMSG_HEADERSTRUCT *pHdr, SOCKET *psockClient)
{
	FETMSG_FLASHREADSTRUCT sData; 
	unsigned char* puchReadData; 
	FETSTATUS eRetVal;
	FETMSG_HEADERSTRUCT sRespHdr; 
	FETMSG_FLASHREADRESPSTRUCT sRespData; 
	int nSent; 
	FETSTATUS fsRetVal; 
 
	
	printf("RFS: Rx FET_FLASHREAD\n"); 
 
	// make sure right number of bytes to follow 
	if (pHdr->nMsgSize != sizeof(FETMSG_FLASHREADSTRUCT)) 
	{ 
		return eFET_ERR_WRONGSIZE; 
	} 
 
	// get a message payload from the RapidFET Client 
//	printf("RFS: reading message payload from client...\n"); 
	if (Recv(psockClient, (char *)&sData, sizeof(sData)) == -1)
	{
		perror("Unable to receive data from client"); 
		if (errno == ECONNRESET) 
		{ 
			// client has dropped the connection 
			fdClose(*psockClient); 
			*psockClient = INVALID_SOCKET; 
			g_bConnected = 0; 
			printf("RFS: Waiting for connection...\n"); 
			DG_Stop();
			return eFET_SUCCESS; 
		} 
		printf("RFS: Errno is %d\n", errno); 
		return eFET_ERR_RECVPLD; 
	} 
 
	// convert to local byte order 
	sData.ulAddressHi = ntohl(sData.ulAddressHi);
	sData.ulAddressLo = ntohl(sData.ulAddressLo);
	sData.ulByteCount = ntohl(sData.ulByteCount);
 
	// allocate some memory to hold the read data 
	puchReadData = (unsigned char*)malloc(sData.ulByteCount); 
	if (puchReadData == NULL) 
	{ 
		return eFET_ERR_READNOMEM; 
	} 
 
	// Do the flash read
	eRetVal = RFDSTATUS_TO_FETSTATUS(RFD_ReadFlash(sData.ulAddressHi, sData.ulAddressLo, (void*)puchReadData, sData.ulByteCount));

	// fill response header
	sRespHdr.nMsgType = htonl(FET_FLASHREADRESP); 
	sRespHdr.nMsgSize = htonl(sizeof(FETMSG_FLASHREADRESPSTRUCT)+sData.ulByteCount); 

	// send the header of the first part of the response 
	nSent = sizeof(sRespHdr); 
	fsRetVal = Send(psockClient, (void*)&sRespHdr, &nSent); 
	if (fsRetVal != eFET_SUCCESS) 
	{ 
		return fsRetVal; 
	} 
	if (nSent != sizeof(sRespHdr)) 
	{ 
		return eFET_ERR_SENDTOCLIENT; 
	} 
 
	// fill response data
	sRespData.sCmd.ulAddressHi = htonl(sData.ulAddressHi); 
	sRespData.sCmd.ulAddressLo = htonl(sData.ulAddressLo); 
	sRespData.sCmd.ulByteCount = htonl(sData.ulByteCount); 
	sRespData.eStatus = (FETSTATUS)htonl(eRetVal); 
 
	// send the data of the first part of the response 
	nSent = sizeof(sRespData); 
	fsRetVal = Send(psockClient, (void*)&sRespData, &nSent); 
	if (fsRetVal != eFET_SUCCESS) 
	{ 
		return fsRetVal; 
	} 
	if (nSent != sizeof(sRespData)) 
	{ 
		return eFET_ERR_SENDTOCLIENT; 
	} 
 
	// send the read data 
	nSent = sData.ulByteCount; 
	fsRetVal = Send(psockClient, (void*)puchReadData, &nSent); 
	free(puchReadData); 
	if (fsRetVal != eFET_SUCCESS) 
	{ 
		return fsRetVal; 
	} 
	if (nSent != sData.ulByteCount) 
	{ 
		return eFET_ERR_SENDTOCLIENT; 
	} 
 
	return eFET_SUCCESS; 
}
#endif

#ifdef SUPPORTS_FLASHWRITE
static FETSTATUS DoMessageFET_FLASHWRITE(FETMSG_HEADERSTRUCT *pHdr, SOCKET *psockClient)
{
	FETMSG_FLASHWRITESTRUCT sData; 
	unsigned char* puchWriteData; 
	FETSTATUS eRetVal;
	FETMSG_HEADERSTRUCT sRespHdr; 
	FETMSG_FLASHWRITERESPSTRUCT sRespData; 
	int nSent; 
	FETSTATUS fsRetVal; 

	printf("RFS: Rx FET_FLASHWRITE\n"); 
 
	// make sure right number of bytes to follow 
	if (pHdr->nMsgSize <= sizeof(FETMSG_FLASHWRITESTRUCT)) 
	{ 
		return eFET_ERR_WRONGSIZE; 
	} 
 
	// get a message payload from the RapidFET Client 
//	printf("RFS: reading message payload from client...\n"); 
	if (Recv(psockClient, (char *)&sData, sizeof(sData)) == -1)
	{
		perror("Unable to receive data from client"); 
		if (errno == ECONNRESET) 
		{ 
			// client has dropped the connection 
			fdClose(*psockClient); 
			*psockClient = INVALID_SOCKET; 
			g_bConnected = 0; 
			printf("RFS: Waiting for connection...\n"); 
			DG_Stop();
			return eFET_SUCCESS; 
		} 
		printf("RFS: Errno is %d\n", errno); 
		return eFET_ERR_RECVPLD; 
	} 
//	printf("	read %d bytes\n", sizeof(sData));
 
	// convert to local byte order 
	sData.ulAddressHi = ntohl(sData.ulAddressHi);
	sData.ulAddressLo = ntohl(sData.ulAddressLo);
	sData.ulByteCount = ntohl(sData.ulByteCount);
 
	printf("Write %d bytes to address %08x\n", sData.ulByteCount, sData.ulAddressLo);
	// allocate some memory to hold the write data 
	puchWriteData = (unsigned char*)malloc(sData.ulByteCount); 
	if (puchWriteData == NULL) 
	{ 
		return eFET_ERR_WRITENOMEM; 
	} 
 
	// get write data 
//	printf("Receiving %d bytes of data to be written\n", sData.ulByteCount); 

	if (Recv(psockClient, (char *)puchWriteData, sData.ulByteCount) <= 0)
	{
		perror("Unable to receive data from client"); 
		if (errno == ECONNRESET) 
		{ 
			// client has dropped the connection 
			fdClose(*psockClient); 
			*psockClient = INVALID_SOCKET; 
			g_bConnected = 0; 
			printf("RFS: Waiting for connection...\n"); 
			DG_Stop();
			return eFET_SUCCESS; 
		} 
		printf("RFS: Errno is %d\n", errno); 
		return eFET_ERR_RECVPLD; 
	} 
 
	// do the flash write
	eRetVal = RFDSTATUS_TO_FETSTATUS(RFD_WriteFlash(sData.ulAddressHi, sData.ulAddressLo, (void*)puchWriteData, sData.ulByteCount));
	free(puchWriteData); 
 
 
	// fill response header
	sRespHdr.nMsgType = htonl(FET_FLASHWRITERESP); 
	sRespHdr.nMsgSize = htonl(sizeof(FETMSG_FLASHWRITERESPSTRUCT)); 

	// send response header
	nSent = sizeof(sRespHdr); 
//	printf("Sending %d bytes response header\n", nSent);
	fsRetVal = Send(psockClient, (void*)&sRespHdr, &nSent); 
	if (fsRetVal != eFET_SUCCESS) 
	{ 
		return fsRetVal; 
	} 
	if (nSent != sizeof(sRespHdr)) 
	{ 
		return eFET_ERR_SENDTOCLIENT; 
	} 

	// fill response data
	sRespData.sCmd.ulAddressHi = htonl(sData.ulAddressHi); 
	sRespData.sCmd.ulAddressLo = htonl(sData.ulAddressLo); 
	sRespData.sCmd.ulByteCount = htonl(sData.ulByteCount); 
	sRespData.eStatus = (FETSTATUS)htonl(eFET_SUCCESS); 
 
	// send response data
	nSent = sizeof(sRespData); 
//	printf("Sending %d byte response payload\n", nSent);
	fsRetVal = Send(psockClient, (void*)&sRespData, &nSent); 
	if (fsRetVal != eFET_SUCCESS) 
	{ 
		return fsRetVal; 
	} 
	if (nSent != sizeof(sRespData)) 
	{ 
		return eFET_ERR_SENDTOCLIENT; 
	} 
	return eFET_SUCCESS; 
}
#endif

int fetTcpSocket(SOCKET* psockClient, UINT32 unused)
{
    struct timeval to;
	FETMSG_HEADERSTRUCT sHdr;
	int nRecv;

    // 配置超时时间 5s
    to.tv_sec  = 5;
    to.tv_usec = 0;
    setsockopt(*psockClient, SOL_SOCKET, SO_SNDTIMEO, &to, sizeof(to));
//    setsockopt(*psockClient, SOL_SOCKET, SO_RCVTIMEO, &to, sizeof(to));

    for(;;)
    {

        	/* get a message header from the RapidFET Client */
        	printf("RFS: reading message header from client...\n");
        	nRecv = Recv(psockClient, ( char * )&sHdr, sizeof(sHdr));
        	printf("RFS: Rx %d bytes\n", nRecv);
        	if (nRecv <= 0)
        	{
        		// client has dropped the connection
        		printf("RFS: Detected dropped connection, nRecv=%d\n", nRecv);
        		fdClose(*psockClient);
        		*psockClient = INVALID_SOCKET;
        		g_bConnected = 0;
        		DG_Stop();
        		break;
        	}
        	// convert to local byte order
        	sHdr.nMsgType = ntohl(sHdr.nMsgType);
        	sHdr.nMsgSize = ntohl(sHdr.nMsgSize);

#if 0
        	// set TCP_CORK to force header and payload to go out together
        	{
        		int on = 1;
        		setsockopt(*psockClient, SOL_TCP, TCP_CORK, &on, sizeof (on)); /* cork */
        	}
#endif
        	switch (sHdr.nMsgType)
        	{
        	case FET_DATAGENCTRL:
        		DoMessageFET_DATAGENCTRL(&sHdr, psockClient);
        		break;
        	case FET_HALGETNUMLOCALPORTS:
        		DoMessageFET_HALGETNUMLOCALPORTS(&sHdr, psockClient);
        		break;
        	case FET_HALCONFIGREAD:
        		DoMessageFET_HALCONFIGREAD(&sHdr, psockClient);
        		break;
        	case FET_HALCONFIGWRITE:
        		DoMessageFET_HALCONFIGWRITE(&sHdr, psockClient);
        		break;
        	case FET_CLOSE:
        		DoMessageFET_CLOSE(&sHdr, psockClient);
        		break;
        	case FET_IDENTIFY:
        		DoMessageFET_IDENTIFY(&sHdr, psockClient);
        		break;
        	case FET_HALRIOREAD:
        		DoMessageFET_HALRIOREAD(&sHdr, psockClient);
        		break;
        	case FET_HALRIOWRITE:
        		DoMessageFET_HALRIOWRITE(&sHdr, psockClient);
        		break;
        	case FET_HALRIOTXMESSAGE:
        		DoMessageFET_HALRIOMESSAGETX(&sHdr, psockClient);
        		break;
        	case FET_HALRIORXMESSAGE:
        		DoMessageFET_HALRIOMESSAGERX(&sHdr, psockClient);
        		break;
        	case FET_HALRIOTXDOORBELL:
        		DoMessageFET_HALRIODOORBELLTX(&sHdr, psockClient);
        		break;
        	case FET_HALRIORXDOORBELL:
        		DoMessageFET_HALRIODOORBELLRX(&sHdr, psockClient);
        		break;
        	case FET_HALGETRIOERROR:
        		DoMessageFET_HALGETRIOERROR(&sHdr, psockClient);
        		break;
        	case FET_MEMREAD:
        		DoMessageFET_MEMREAD(&sHdr, psockClient);
        		break;
        	case FET_MEMWRITE:
        		DoMessageFET_MEMWRITE(&sHdr, psockClient);
        		break;
        	case FET_GETSERVERCAPS:
        		DoMessageFET_GETSERVERCAPS(&sHdr, psockClient);
        		break;
        #ifdef SUPPORTS_FLASHREAD
        	case FET_FLASHREAD:
        		DoMessageFET_FLASHREAD(&sHdr, psockClient);
        		break;
        #endif
        #ifdef SUPPORTS_FLASHWRITE
        	case FET_FLASHWRITE:
        		DoMessageFET_FLASHWRITE(&sHdr, psockClient);
        		break;
        #endif
        	default:
        		printf("RFS: Unknown message type %d, size %d\n", sHdr.nMsgType, sHdr.nMsgSize);
        		break;
        	}

    }

    return(0);
}


int fetTcpDaemon(SOCKET s, UINT32 unused)
{
	SOCKET sockClient = s;

	return fetTcpSocket(&sockClient, unused);
}


SOCKET g_identUdpSendSocket = NULL;
SOCKET getUdpSendSocket()
{
	struct  sockaddr_in addrIdentifyResp;

	if(NULL == g_identUdpSendSocket)
	{
		/* set up a socket to reply to FET_IDENTIFY broadcasts */
	//	printf("RFS: creating identify response socket\n");
		if ((g_identUdpSendSocket = socket(AF_INET, SOCK_DGRAM, 0)) == NULL)
		{
			perror("Unable to create idenification response socket");
			return NULL;
		}

		memset(&addrIdentifyResp, 0, sizeof(addrIdentifyResp));
		addrIdentifyResp.sin_family = AF_INET;
		addrIdentifyResp.sin_addr.s_addr = htonl(INADDR_ANY);
		addrIdentifyResp.sin_port = htons(FET_BROADCASTRESPPORT_SERVER);


		/* bind the socket to FET_BROADCASTRESPPORT */
	//	printf("RFS: binding identify response socket to FET_BROADCASTRESPPORT\n");
		if (bind(g_identUdpSendSocket, (struct sockaddr *) &addrIdentifyResp, sizeof(addrIdentifyResp)) == -1) {
			perror("Unable to bind socket to identification response  port");
			close(g_identUdpSendSocket);
			return NULL;
		}
	}
	return g_identUdpSendSocket;
}


int fetUdpDaemon( SOCKET s, UINT32 unused )
{
	struct timeval to;
	FETMSG_HEADERSTRUCT sHdr;
	int nRecv;

	SOCKET sockSendUDP = -1;

    // 配置超时时间 5s
    to.tv_sec  = 5;
    to.tv_usec = 0;
    setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, &to, sizeof(to));
//    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &to, sizeof(to));

    sockSendUDP = getUdpSendSocket();
    for(;;)
    {
    	// broadcast data ready to be read
    //	printf("RFS: ready to read broadcast data\n");
    	nRecv = recvfrom(s, ( char * )&sHdr, sizeof(sHdr), 0, (struct sockaddr*)&g_addrFrom, (unsigned int*)&g_addrsize);
    	if (nRecv != sizeof(sHdr))
    	{
    		printf("RFS: Error:  Looking for %d bytes, read %d bytes\n", sizeof(sHdr), nRecv);
    	}
    	else
    	{
    		sHdr.nMsgType = ntohl(sHdr.nMsgType);
    		sHdr.nMsgSize = ntohl(sHdr.nMsgSize);
    		if ((sHdr.nMsgType == FET_IDENTIFY) && (sHdr.nMsgSize == 0))
    		{
    			struct {
    				FETMSG_HEADERSTRUCT sHdr;
    				FETMSG_IDENTIFYRESPSTRUCT sData;
    			} sResp;
    			int nSent;

    			printf("RFS: Rx FET_IDENTIFY from %d.%d.%d.%d (%08lx)\n",
    				(ntohl(g_addrFrom.sin_addr.s_addr)>>24)&0xff,
    				(ntohl(g_addrFrom.sin_addr.s_addr)>>16)&0xff,
    				(ntohl(g_addrFrom.sin_addr.s_addr)>>8)&0xff,
    				(ntohl(g_addrFrom.sin_addr.s_addr)>>0)&0xff,
    				ntohl(g_addrFrom.sin_addr.s_addr));

    			// build a FET_IDENTIFYRESP message...
    			BuildFET_IDENTIFYRESP(&sResp.sHdr, &sResp.sData);

    			// ...and send it back to where the request came from
    			printf("RFS: sending FET_IDENTIFYRESP\n");
    			g_addrFrom.sin_port = htons(FET_BROADCASTRESPPORT);
    			g_addrFrom.sin_family = AF_INET;
    			nSent = sendto(sockSendUDP,
    					(char*)&sResp,
    					sizeof(sResp),
    					0,
    					(struct sockaddr*)&g_addrFrom,
    					sizeof(g_addrFrom));
    			if (nSent != sizeof(sResp))
    			{
    				printf("RFS: Unable to send FET_IDENTIFYRESP\n");
    				printf("RFS:   Sent %d bytes instead of %d\n", nSent, sizeof(sResp));
    			}
    		}
    		else
    		{
    			printf("RFS: recevied unknown message %d(size:%d) from %08lx\n",
    			  sHdr.nMsgType,
    			  sHdr.nMsgSize,
    			  ntohl(g_addrFrom.sin_addr.s_addr));
    		}
    	}
    }
}

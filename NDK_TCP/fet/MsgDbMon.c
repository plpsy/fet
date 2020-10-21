/////////////////////////////////////////////////////////////////////////////// 
// $RCSfile: MsgDbMon.c,v $ 
// $Date: 2008/06/16 19:22:38 $ 
// $Author: Gilmour $ 
// $Revision: 1.3 $ 
// $Source: /cvsrepo/FET/Server/common/PQIII/MsgDbMon.c,v $ 
// 
// Copyright (c) 2006 Fabric Embedded Tools Corporation 
// 
/////////////////////////////////////////////////////////////////////////////// 
#if 0

#include <usertype.h>
#include "mytypes.h"
//#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <errno.h>
#include "fetriod.h"
#include "rfd.h"
#include "rio.h"
#include "fethal.h" 
/* JuWi Changed include path. */
/* #include "include/feterr.h" */
#include "feterr.h" 
/* JuWi Added include of unistd.h and MsgDbMon.h */
//#include <unistd.h>
/*#include "MsgDbMon.h"  sdr sealed 20081027*/

static void MonitorMain ( void *ptr );
/* JuWi Removed undefined function. */ 
/* static void MonitorAlarm (int sig); */
static pthread_t threadMon;
static int g_bWantMonitor = false;			// an indication that we want the monitor to be running
static int g_bMonitorRunning = false;		// an indication that the monitor IS running
static frdHandle g_hMsgController0;
static frdHandle g_hMsgController1;
static frdHandle g_hDBController;

#define MSGQSIZE	512
#define DBQSIZE		512

// structure for Rx buffer
typedef struct {
	unsigned long ulOverflowFlag;
	unsigned short wInfoWord;
} RXDOORBELL;

// Circular buffer for received doorbells.  Calls to DoorbellRx extract from this buffer.
#define MAXRXDOORBELLQUEUES 1
#define RXDOORBELLBUFFERSIZE 32					// Increase this if you want to be able to queue up more
												// doorbells in the circular input buffer.
#define BUFFERSIZE_BYTES		4096
static RXDOORBELL asRxDoorbells[MAXRXDOORBELLQUEUES][RXDOORBELLBUFFERSIZE];
// head and tail indexes for secondary buffer
static int nRxDoorbellHead[MAXRXDOORBELLQUEUES];
static int nRxDoorbellTail[MAXRXDOORBELLQUEUES];
// overflow flag
static int g_bRxDoorbellOverflow = 0;

// structure for secondary Rx buffer
typedef struct {
	unsigned long ulSourceID;
	unsigned long ulPriority; 
	unsigned long ulTT;
	unsigned long ulMailboxNum;
	unsigned long ulMessageLength;
	unsigned long ulOverflowFlag;
	unsigned long aulData[BUFFERSIZE_BYTES];
} RXMESSAGE;

// secondary buffer for received messages.  Calls to MessageRxGet extract messages from this buffer.
#define MAXRXMSGQUEUES			2
#define RXSECONDARYBUFFERSIZE	32		// Increase this if you want to be able to queue up more
										// messages in the circular secondary input buffer.
static RXMESSAGE asRxMsgs[MAXRXMSGQUEUES][RXSECONDARYBUFFERSIZE];
// head and tail indexes for secondary buffer
static int nRxMsgHead[MAXRXMSGQUEUES];
static int nRxMsgTail[MAXRXMSGQUEUES];
// overflow flag
static int g_bRxMsgOverflow = 0;

// global that keeps track of whether or not large TT is supported
static int g_bSupportsLargeTransport = 0;

static void DoorbellRx(int nQueue, unsigned short wInfoWord);
static void MessageRx(int nQueue, unsigned char* auchData, int nNumBytes);
//static void DumpRxMsg(RXMESSAGE* psMsg);

RFDSTATUS MsgDbMonOpen()
{
	int nRet;
	int i;
        /* JuWi Removed unused variable. */
        /*	unsigned long ulSize; */
	unsigned long ul;

	printf("MsgDbMonOpen()\n");

	//////////////////////////////////////////////
	// find out if large transport is supported
	//////////////////////////////////////////////
	if (rioConfigurationRead(0, 0, 0xffffffff, 0, RIO_PEFCAR, &ul, 1, 4, 0, 0) != eFET_SUCCESS)
	{
		g_bSupportsLargeTransport = 0;
	}
	else
	{
		g_bSupportsLargeTransport = ((ul>>4)&1);
	}

	//////////////////////////////////////////////
	// set up secondary RX buffer for doorbells //
	//////////////////////////////////////////////
	// Circular Queue Policy:  Head==Tail means empty, Head==(Tail-1) means full
	//   Enqueuing routine inserts at head, then advances pointer iff it is not already full
	//   Dequeueing routine extracts from tail unless empty, then advances pointer
	for (i=0; i<MAXRXDOORBELLQUEUES; i++)
	{
		nRxDoorbellHead[i] = nRxDoorbellTail[i] = 0;	// empty
	}

	/////////////////////////////////////////////
	// set up secondary RX buffer for messages //
	/////////////////////////////////////////////
	// Circular Queue Policy:  Head==Tail means empty, Head==(Tail-1) means full
	//   Enqueuing routine inserts at head, then advances pointer iff it is not already full
	//   Dequeueing routine extracts from tail unless empty, then advances pointer
	for (i=0; i<MAXRXMSGQUEUES; i++)
	{
		nRxMsgHead[i] = nRxMsgTail[i] = 0;	// empty
	}

	// get an inbound message controller
	nRet = frdIMSG_Open(&g_hMsgController0, 1, 4096, MSGQSIZE);
	if (nRet < 0)
	{
		perror("Couldn't get inbound message controller handle 1!");
		return eRFSRFD_ERR;
	}

	// get an inbound message controller
	nRet = frdIMSG_Open(&g_hMsgController1, 0, 4096, MSGQSIZE);
	if (nRet < 0)
	{
		perror("Couldn't get inbound message controller handle 1!");
		return eRFSRFD_ERR;
	}

	// get an inbound doorbell controller
	nRet = frdIDB_Open(&g_hDBController, DBQSIZE);
	if (nRet < 0)
	{
		perror("Couldn't get inbound message controller handle!");
		return eRFSRFD_ERR;
	}

	// start the monitor thread
	g_bWantMonitor = true;
	g_bMonitorRunning = false;
	pthread_create(&threadMon, NULL, (void*)&MonitorMain, (void*)NULL);

	// wait for the thread to get going
	for (i=0; i<5; i++)
	{
		if (!g_bMonitorRunning)
		{
			sleep(1);
		}
	}

	// make sure thread is running
	if (!g_bMonitorRunning)
	{
		// thread failed to start
	//sdr sealed	fprintf(stderr, "MsgDbMonOpen: Monitor failed to start\n");fflush(stderr);
		g_bWantMonitor = false;
		return eRFSRFD_ERR_DGPROCERR;
	}

	printf("MsgDbMonOpen: Message/Doorbell Monitor is running\n");fflush(stdout);

	return eRFSRFD_SUCCESS;
}

RFDSTATUS MsgDbMonClose()
{
	int i;

	if (!g_bWantMonitor)
	{
		return eRFSRFD_ERR_DGNOTOPEN;
	}
	g_bWantMonitor = false;
	printf("Waiting for MSG/DB monitor thread to terminate");fflush(stdout);
	// wait for the thread to terminate
	for (i=0; i<5; i++)
	{
		printf(".");fflush(stdout);
		if (g_bMonitorRunning)
		{
			sleep(1);
		}
	}
	printf("\n");

	// make sure thread is not running
	if (g_bMonitorRunning)
	{
		// thread failed to die
		printf("MSG/DB monitor thread did not terminate\n");
		frdIMSG_Close(g_hMsgController0);
		frdIMSG_Close(g_hMsgController1);
		frdIDB_Close(g_hDBController);
		return eRFSRFD_ERR_DGPROCERR;
	}

	frdIMSG_Close(g_hMsgController0);
	frdIMSG_Close(g_hMsgController1);
	frdIDB_Close(g_hDBController);
	return eRFSRFD_SUCCESS;
}


// The main Monitor thread routine.  Handles receipt of MSGs and DBs.
void MonitorMain ( void *ptr )
{
	int nRet;
	unsigned char auchData[4096];
	unsigned short wSrcID, wDstID, wInfo;

	printf("MSG/DB Monitor thread started\n");fflush(stdout);
	g_bMonitorRunning = true;

	while (g_bWantMonitor)
	{
		usleep(10000);	// sleep 10 ms 

		// process messages on channel 0
		while ((nRet = frdIMSG_GetMessage(g_hMsgController0, auchData)) > 0)
		{
		//sdr sealed	fprintf(stderr, "MSG0(%02x %02x %02x %02x ...) ", auchData[0], auchData[1], auchData[2], auchData[3]);fflush(stderr);
			MessageRx(0, auchData, 4096);
		//sdr 	fprintf(stderr, "\n");
		}
		if (nRet < 0)
		{
		//sdr	fprintf(stderr, "MSG0 Rx Error\n");
			continue;
		}
		if (nRet == 0)
		{
//			fprintf(stderr, "No MSG0\n");
		}

		// process messages on channel 1
		while ((nRet = frdIMSG_GetMessage(g_hMsgController1, auchData)) > 0)
		{
		//sdr	fprintf(stderr, "MSG1(%02x %02x %02x %02x ...) ", auchData[0], auchData[1], auchData[2], auchData[3]);fflush(stderr);
			MessageRx(1, auchData, 4096);
		//sdr	fprintf(stderr, "\n");
		}
		if (nRet < 0)
		{
		//sdr	fprintf(stderr, "MSG1 Rx Error\n");
			continue;
		}
		if (nRet == 0)
		{
//			fprintf(stderr, "No MSG1\n");
		}

		// process doorbells
		while ((nRet = frdIDB_GetDoorbell(g_hDBController, &wSrcID, &wDstID, &wInfo)) > 0)
		{
		//sdr	fprintf(stderr, "DB(From %04x to %04x Info:%04x) ", wSrcID, wDstID, wInfo);fflush(stderr);
			DoorbellRx(0, wInfo);
		//sdr	fprintf(stderr, "\n");
		}
		if (nRet < 0)
		{
		//sdr	fprintf(stderr, "DB Rx Error\n");
			continue;
		}
		if (nRet == 0)
		{
//			fprintf(stderr, "No DB\n");
		}

	}

	printf("MSG/DB Monitor thread terminated\n");fflush(stdout);

	g_bMonitorRunning = false;
	pthread_exit(NULL);
}

//void DumpRxMsg(RXMESSAGE* psMsg)
//{
//	printf("Msg @ 0x%08lx : Src:%04lx Pri:%d TT:%d Mailbox:%04x Len:%d Overflow:%d\n",  
//		psMsg,
//		psMsg->ulSourceID,
//		psMsg->ulPriority, 
//		psMsg->ulTT,
//		psMsg->ulMailboxNum,
//		psMsg->ulMessageLength,
//		psMsg->ulOverflowFlag);
//}

static void MessageRx(int nQueue, unsigned char* auchData, int nNumBytes)
{
	// find out if the queue is already full (Head = Tail-1)
	int nNextHead = nRxMsgHead[nQueue] + 1;
	if (nNextHead >= RXSECONDARYBUFFERSIZE)
	{
		nNextHead = 0;
	}
	if (nNextHead != nRxMsgTail[nQueue])
	{
		// there is room in the queue
		RXMESSAGE *psMsg;
		unsigned char* puchSrc;
		unsigned char* puchDst;
		int i;

		psMsg = &asRxMsgs[nQueue][nRxMsgHead[nQueue]];
		psMsg->ulMailboxNum = 0xff;											// unknown
		psMsg->ulMessageLength = nNumBytes/8;								// longwords
		psMsg->ulPriority = 0;												// unknown
		psMsg->ulSourceID = 0xffff;											// unknown
		psMsg->ulTT = g_bSupportsLargeTransport?1:0;

		puchSrc = auchData;
		puchDst = (unsigned char*)psMsg->aulData; 

		for (i=0; i<psMsg->ulMessageLength; i++)
		{
			*puchDst++ = *puchSrc++;
		}

		if (g_bRxMsgOverflow)
		{
			psMsg->ulOverflowFlag = 1;
			g_bRxMsgOverflow = 0;
		}
		else
		{
			psMsg->ulOverflowFlag = 0;
		}

//		DumpRxMsg(psMsg);
		nRxMsgHead[nQueue] = nNextHead;
//sdr		fprintf(stderr, "QUEUED");
	}
	else
	{
		// discard the message and indicate error
		g_bRxMsgOverflow = 1;
//sdr		fprintf(stderr, "DROPPED");
	}
}

int MessageRxGet(	int nQueue,
					unsigned char* pbLargeTT, 
					unsigned long* pulSourceID,
					unsigned long* pulPriority, 
					unsigned long* pulMailboxNum,
					unsigned long* pulMessageLengthBytes,
					unsigned char* pbOverflowed,
					unsigned char *puchData)
{
	RXMESSAGE *psMsg;
	int i;
	unsigned char *puchSrc;
	unsigned char *puchDst;

	if (nRxMsgHead[nQueue] == nRxMsgTail[nQueue])
	{
		// empty
		*pbLargeTT = 0; 
		*pulSourceID = 0;
		*pulPriority = 0;
		*pulMailboxNum = 0;
		*pulMessageLengthBytes = 0;
		*pbOverflowed = 0;
		return -1;
	}

	psMsg = &asRxMsgs[nQueue][nRxMsgTail[nQueue]];
//	DumpRxMsg(psMsg);
	*pbLargeTT = psMsg->ulTT?1:0;
	*pulSourceID = psMsg->ulSourceID;
	*pulMailboxNum = psMsg->ulMailboxNum;
	*pulPriority = psMsg->ulPriority;
	*pulMessageLengthBytes = psMsg->ulMessageLength * 8;
	*pbOverflowed = psMsg->ulOverflowFlag?1:0;
	puchSrc = (unsigned char*)psMsg->aulData;
	puchDst = puchData;
	for (i=0; i<*pulMessageLengthBytes; i++)
	{
		*puchDst++ = *puchSrc++;
	}

	// advance tail pointer
	nRxMsgTail[nQueue]++;
	if (nRxMsgTail[nQueue] >= RXSECONDARYBUFFERSIZE)
	{
		nRxMsgTail[nQueue] = 0;
	}


	return 0;
}

// Called via RFS to discard any messages that are currently in the circular buffer.
void MessageRxFlush(int nQueue)
{
	nRxMsgTail[nQueue] = nRxMsgHead[nQueue];
	g_bRxMsgOverflow = 0;
}

// Called from monitor loop to add doorbell to head of circular buffer
void DoorbellRx(int nQueue, unsigned short wInfoWord)
{    
	// find out if the queue is already full (Head = Tail-1)
	int nNextHead = nRxDoorbellHead[nQueue] + 1;
	if (nNextHead >= RXDOORBELLBUFFERSIZE)
	{
		nNextHead = 0;
	}
	if (nNextHead != nRxDoorbellTail[nQueue])
	{
		// there is room in the queue
		RXDOORBELL *psDoorbell;

		psDoorbell = &asRxDoorbells[nQueue][nRxDoorbellHead[nQueue]];

		psDoorbell->wInfoWord = wInfoWord;

		if (g_bRxDoorbellOverflow)
		{
			psDoorbell->ulOverflowFlag = 1;
			g_bRxDoorbellOverflow = 0;
		}
		else
		{
			psDoorbell->ulOverflowFlag = 0;
		}

		nRxDoorbellHead[nQueue] = nNextHead;
	//sdr	fprintf(stderr, "QUEUED");
	}
	else
	{
		// discard the doorbell and indicate error
		g_bRxDoorbellOverflow = 1;
	//sdr sealed	fprintf(stderr, "DROPPED");
	}
}

// called via RFS to retrieve next available doorbell from tail of circular buffer.
int DoorbellRxGet(int nQueue, unsigned short* pwInfoWord, unsigned char* pbOverflowed, unsigned char* pbValidDoorbell)
{
	RXDOORBELL *psDoorbell;

	if (nRxDoorbellHead[nQueue] == nRxDoorbellTail[nQueue])
	{
		// empty
		*pbValidDoorbell = 0; 
		*pbOverflowed = 0;
		*pwInfoWord = 0;
		return -1;
	}

	psDoorbell = &asRxDoorbells[nQueue][nRxDoorbellTail[nQueue]];
	*pbValidDoorbell = 1;
	*pbOverflowed = psDoorbell->ulOverflowFlag?1:0;
	*pwInfoWord = psDoorbell->wInfoWord;

	// advance tail pointer
	nRxDoorbellTail[nQueue]++;
	if (nRxDoorbellTail[nQueue] >= RXDOORBELLBUFFERSIZE)
	{
		nRxDoorbellTail[nQueue] = 0;
	}
	return 0;
}

// Called via RFS to discard any doorbells that are currently in the circular buffer.
void DoorbellRxFlush(int nQueue)
{
	nRxDoorbellTail[nQueue] = nRxDoorbellHead[nQueue];
	g_bRxDoorbellOverflow = 0;
}

#endif

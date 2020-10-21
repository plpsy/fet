///////////////////////////////////////////////////////////////////////////////
// $RCSfile: datagen.c,v $
// $Date: 2008/09/15 18:50:35 $
// $Author: Gilmour $
// $Revision: 1.6 $
// $Source: /cvsrepo/FET/Server/common/PQIII/datagen.c,v $
//
// Copyright (c) 2004-2008 Fabric Embedded Tools Corporation
//
///////////////////////////////////////////////////////////////////////////////
#include <usertype.h>
#include "mytypes.h"
#include <stdio.h>
#include "rio.h"
/* JuWi Changed include path. */
/* #include "include/feterr.h" */
/* #include "include/fetmsg.h" */
#include "feterr.h"
#include "fetmsg.h" 
#include "datagen.h"
#include "fethal.h"

#include <ti/ndk/inc/netmain.h>

#define DGMAJORREV	2
#define DGMINORREV	8
#define DGBUILD		001

#define WATCHDOGCOUNT	5

extern FETSTATUS RFDSTATUS_TO_FETSTATUS(RFDSTATUS rfds); 

typedef struct 
{
	unsigned char bRemoteDG;
	unsigned char uchDGStatus;
	unsigned char bLargeTT;
	unsigned short wDestID;
	unsigned long ulCtrlStructAddrHI;
	unsigned long ulCtrlStructAddrLO;
} DGSTATUSSTRUCT;

DGSTATUSSTRUCT g_sDGStatus[256];
#define DGINACTIVE	0x00
#define DGACTIVE	0x01

UINT32 g_bInitialized=0;

#define DATASIZE				0x00400000	// Size of data transfer buffer:	4 MB
#define CTRLSIZE				0x00001000	// Size of control structure:		4 kB
#define RAPIDIO_ADDRESS			0x04000000	// start of RapidIO address space for DG memory

unsigned char g_bLargeTT = 0;

#define DGCMD_IDLE		0			// no command in progress
#define DGCMD_RESET		1			// request for DG to reset
#define DGCMD_DMA		2			// request DG to do output via DMA

#define DGRESP_NULLRESP	0			// no response yet
#define DGRESP_OK		1			// request completed successfully
#define DGRESP_ERROR	2			// request failed

#define DGSTATUS_INITIALIZING	0	// DG is currently initializing
#define DGSTATUS_READY			1	// DG is ready to accept a new command
#define DGSTATUS_WORKING		2	// DG is currently processing a command

// used to identify the presence of a RapidFET Data Generator
char achSignature[16] = { 'R', 'a', 'p', 'i', 'd', 'F', 'E', 'T', '_', 'D', 'A', 'T', 'A', 'G', 'E', 'N' };

// Data Generator Control Structure - used to control both local and remote DGs via shared memory
// NOTE:  All data must be stored in this structure in BIGENDIAN byte order.  Maintain long word alignment so that 
//        structure remains packed automatically without the need to use pragmas.
typedef struct
{
	char achDGSig[16];					// a 'signature' string so that we can tell if there's a data
										// generator running or not

	UINT32 ulDGRev;						// revision indicator for the data generator

	unsigned char uchFlags;				// flags.  See defines below
	unsigned char uchStatus;			// current status of the data generator
	unsigned char uchCommand;			// request to data generator from host PE
	unsigned char bRepeat;				// indicates whether the sequence repeats or not

	unsigned char uchResponse;			// response to request
	unsigned char uchRioXferType;		// the type of transfer to be made
	unsigned char uchRioXferPriority;	// priority level to use for the transfer
	unsigned char uchLongDestID;		// set to nonzero if uwTgtDestID contains a large transport (16 bit) value

	UINT32 ulRioPacketSize;				// maximum size of RapidIO packet (1 - 256)

	UINT32 ulTgtDestAddrHI;				// address on target PE where to write data

	UINT32 ulTgtDestAddrLO;				// address on target PE where to write data

	UINT32 ulTgtDestSize;				// size of available buffer memory to write into on target PE

	UINT16 uwTgtDestID;					// where the data is to be sent to or read from
	UINT16 uwSrcDestID;					// Destination ID of this PE

	char chWatchDogCounter;				// DG counts this down at 1Hz and resets if reaches 0.
	char achUnused[3];

	UINT32 ulDDGEntryCount;				// number of entries in the DDG profile array

	UINT32 ulDDGEntryIndex;				// the current index into the DDG profile array

	UINT32 ulDDGEntryTime;				// keeps track of where we are in the current DDG entry (seconds)

	struct
	{
		UINT32 ulDDGDuration;	// length (seconds)
		UINT32 ulDDGXferRate;	// output rate (bytes/second)
	} asDDGProfile[MAXDDGENTRIES];		// array that describes the dynamic data generation profile
} DGCONTROLSTRUCT;

#define DGFLAG_WATCHDOGTIMEOUT	0x01	// flag set if watchdog counter hits 0
#define DGFLAG_LAGGING			0x02	// flag set if DMA operation took > 1 second
#define DGFLAG_1SHOTCOMPLETE	0x04	// flag set when 1shot cycle is complete

unsigned short g_uwMyDestID;				// our own destination id

FETSTATUS ResetWatchdogTimers(void);
int ValidDG(DGCONTROLSTRUCT* psDG);
void SetActive(unsigned char bRemoteDG, unsigned char bLargeTT, unsigned short wDestID, unsigned long ulRioTargetCtrlAddrHI, unsigned long ulRioTargetCtrlAddrLO);
void SetInactive(unsigned short wDestID);

// This function is called by the data generator thread once per second
RFDSTATUS DG_Callback(void)
{
	void* pLocalDGMemPtr;
	DGCONTROLSTRUCT* psDGLocal;
	RFDSTATUS rfds;
	FETSTATUS fs;
	UINT32 bTerminated;

	// start by resetting watchdog timers on all active DGs
	fs = ResetWatchdogTimers();
	if (fs != eFET_SUCCESS)
	{
		// dag: TODO: save this error in some status variable for later reporting to the client
	}

	if (!g_bInitialized)
	{
		printf("Callback aborting: Not initialized\n");
		return eRFSRFD_SUCCESS;
	}

	// get pointer to local data generator memory
	rfds = RFD_DGGetLocalDGCtrlPtr(&pLocalDGMemPtr);
	if (rfds != eRFSRFD_SUCCESS)
	{
		printf("Callback unabled to get local DG memory pointer\n");
		return rfds;
	}
	psDGLocal = (DGCONTROLSTRUCT*)pLocalDGMemPtr;

	switch (psDGLocal->uchCommand)
	{
	case DGCMD_IDLE:
		//printf("I");fflush(stdout);
		break;
	case DGCMD_DMA:
		printf("DMA ");fflush(stdout);
		psDGLocal->uchFlags = 0;
		if (--psDGLocal->chWatchDogCounter <= 0)
		{
			printf("Watchdog timeout.  Data Generator is resetting.\n");fflush(stdout);
			// reset
			psDGLocal->uchResponse = DGRESP_OK;				// indicate successful completion of command
			psDGLocal->uchStatus = DGSTATUS_READY;			// indicate that we're ready to accept a new command
			psDGLocal->uchCommand = DGCMD_IDLE;				// go back to to idle state
			psDGLocal->uchFlags |= DGFLAG_WATCHDOGTIMEOUT;	// indicate timeout has occurred
			break;
		}

//		printf("Index:%d of %d, Time:%d Dur'n %d\n", ntohl(psDGLocal->ulDDGEntryIndex), 
//													ntohl(psDGLocal->ulDDGEntryCount),
//													ntohl(psDGLocal->ulDDGEntryTime),
//													ntohl(psDGLocal->asDDGProfile[ntohl(psDGLocal->ulDDGEntryIndex)].ulDDGDuration));
		fflush(stdout);

		psDGLocal->uchStatus = DGSTATUS_WORKING;	// indicate that we're working on the command

		// increment seconds...
		psDGLocal->ulDDGEntryTime = htonl(ntohl(psDGLocal->ulDDGEntryTime)+1);
//		printf("Incr time to %d\n", ntohl(psDGLocal->ulDDGEntryTime));fflush(stdout);
		// ...then check to see if we're done the current entry in the profile
		if (ntohl(psDGLocal->ulDDGEntryTime) >= ntohl(psDGLocal->asDDGProfile[ntohl(psDGLocal->ulDDGEntryIndex)].ulDDGDuration))
		{
			psDGLocal->ulDDGEntryIndex = htonl(ntohl(psDGLocal->ulDDGEntryIndex)+1);
			psDGLocal->ulDDGEntryTime = 0;
//			printf("Incr index to %d, reset time\n", ntohl(psDGLocal->ulDDGEntryIndex));fflush(stdout);
			// now check to see if we're done the sequence...
			if (ntohl(psDGLocal->ulDDGEntryIndex) >= ntohl(psDGLocal->ulDDGEntryCount))
			{
				// do we have to repeat the sequence?
				if (psDGLocal->bRepeat)
				{
					// yes, so go back to the beginning
					psDGLocal->ulDDGEntryIndex = 0;
//					printf("Sequence is done.  Reset index and start again\n");fflush(stdout);
				}
				else
				{
					printf("1Shot Sequence is done.  Reset DG\n");fflush(stdout);
					// no, so reset the data generator
					psDGLocal->uchResponse = DGRESP_OK;			// indicate successful completion of command
					psDGLocal->uchStatus = DGSTATUS_READY;		// indicate that we're ready to accept a new command
					psDGLocal->uchCommand = DGCMD_IDLE;			// go back to to idle state
					psDGLocal->uchFlags |= DGFLAG_1SHOTCOMPLETE;	// indicate that single cycle is complete
					psDGLocal->ulDDGEntryIndex = 0;
					break;
				}
			}
		}
//		printf("Index:%d\n", psDGLocal->ulDDGEntryIndex);fflush(stdout);
		if (psDGLocal->uchRioXferType == 3)
		{
			rfds = RFD_RioDMARead(
						psDGLocal->uchLongDestID, 
						ntohs(psDGLocal->uwTgtDestID),
						ntohl(psDGLocal->ulTgtDestAddrHI),
						ntohl(psDGLocal->ulTgtDestAddrLO),
						ntohl(psDGLocal->ulTgtDestSize),
						(UINT32)psDGLocal->uchRioXferType,
						(UINT32)psDGLocal->uchRioXferPriority,
						0,
						ntohl(psDGLocal->asDDGProfile[ntohl(psDGLocal->ulDDGEntryIndex)].ulDDGXferRate),
						&bTerminated);
		}
		else
		{
			rfds = RFD_RioDMAWrite(
						psDGLocal->uchLongDestID, 
						ntohs(psDGLocal->uwTgtDestID),
						ntohl(psDGLocal->ulTgtDestAddrHI),
						ntohl(psDGLocal->ulTgtDestAddrLO),
						ntohl(psDGLocal->ulTgtDestSize),
						(UINT32)psDGLocal->uchRioXferType,
						(UINT32)psDGLocal->uchRioXferPriority,
						0,
						ntohl(psDGLocal->asDDGProfile[ntohl(psDGLocal->ulDDGEntryIndex)].ulDDGXferRate),
						&bTerminated);
		}
		if (bTerminated)
		{
			psDGLocal->uchFlags |= DGFLAG_LAGGING;
		}
//		printf("DMA is done.\n");fflush(stdout);
		psDGLocal->uchResponse = DGRESP_OK;			// indicate successful completion of command
		break;
	case DGCMD_RESET:
		printf("DG RESET\n");fflush(stdout);
		psDGLocal->uchResponse = DGRESP_OK;			// indicate successful completion of command
		psDGLocal->uchStatus = DGSTATUS_READY;		// indicate that we're ready to accept a new command
		psDGLocal->uchCommand = DGCMD_IDLE;			// go back to to idle state
		break;
	default:
		printf("Unknown command:%x ", psDGLocal->uchCommand);fflush(stdout);
		break;
	}


	return eRFSRFD_SUCCESS;
}

// Called to initialize data generator routines
FETSTATUS DG_Init(void)
{
	FETSTATUS fs;
	UINT32 ulPE;
	void* pLocalDGMemPtr;
	DGCONTROLSTRUCT* psDGLocal;
	int i;
	UINT32 ulTemp;

	printf("DG_Init\n");fflush(stdout);

	// initialize remote DG status
	for (ulPE=0; ulPE<256; ulPE++)
	{
		g_sDGStatus[ulPE].uchDGStatus = DGINACTIVE;
	}

	printf("======================================================\n");
	printf("=   Traffic Generation Information                   =\n");
	printf("======================================================\n");
	printf("=   Input Buffer:             %08x               =\n", (UINT32)RAPIDIO_ADDRESS);
	printf("=   Input Buffer Size:        %08x               =\n", DATASIZE);
	printf("=   Control Structure:        %08x               =\n", (UINT32)RAPIDIO_ADDRESS+DATASIZE);
	printf("======================================================\n");

	// do device specific initialization 
	fs = RFDSTATUS_TO_FETSTATUS(RFD_DGOpen(CTRLSIZE, DATASIZE, RAPIDIO_ADDRESS, &DG_Callback)); 
	if (fs != eFET_SUCCESS)
	{
	//sdr 	fprintf(stderr, "RFD_DGOpen returned %d\n", fs);
		return fs;
	}

	// get pointer to local data generator memory
	fs = RFDSTATUS_TO_FETSTATUS(RFD_DGGetLocalDGCtrlPtr(&pLocalDGMemPtr)); 
	if (fs != eFET_SUCCESS)
	{
	//sdr 	fprintf(stderr, "RFD_DGGetLocalDGCtrlPtr returned %d\n", fs);
		return fs;
	}
	psDGLocal = (DGCONTROLSTRUCT*)pLocalDGMemPtr;

	// Initialize signature
	for (i=0; i<16; i++)
	{
		psDGLocal->achDGSig[i] = achSignature[i];
	}

	psDGLocal->uchStatus = DGSTATUS_INITIALIZING;	// set current state to "initializeing"
	psDGLocal->uchCommand = DGCMD_IDLE;			// initialize command to "do nothing"
	psDGLocal->uchResponse = DGRESP_NULLRESP;		// clear response */
	psDGLocal->ulDGRev = htonl(MAKEVERSION(DGMAJORREV,DGMINORREV,DGBUILD));
	psDGLocal->uchStatus = DGSTATUS_READY;			// local data generator is ready to go
	psDGLocal->uchFlags = 0;

	// get the host ID 
	if (rioConfigurationRead(0, 0, 0xffffffff, 0, RIO_BDIDCSR, &ulTemp, 1, 4, 0, 0) != eFET_SUCCESS) 
	{ 
		printf("RFS: Unable to read RIO_BDIDCSR\n"); 
		g_uwMyDestID = (UINT16)0xffff; 
		psDGLocal->uwSrcDestID = htons(g_uwMyDestID);
		printf("My Destination ID is %d\n", g_uwMyDestID);
	} 
	else 
	{ 
		g_uwMyDestID = (unsigned short)((ulTemp>>16)&0xffff); 
		psDGLocal->uwSrcDestID = htons(g_uwMyDestID); 
	} 

//	DumpATMUs();

	g_bInitialized = 1;
	return eFET_SUCCESS;
}

// Process a data generation command from the RapidFET Client
FETSTATUS DG_Command(FETMSG_DATAGENCTRLSTRUCT* psCmd,
					 UINT32* pulDDGEntryIndex,
					 UINT32* pbLagging,
					 UINT32* pb1ShotComplete)
{
	void* pDGMemPtr;
	DGCONTROLSTRUCT* psDG;
	FETSTATUS fs, retval;
	UINT32 i;
	UINT32 ulTemp;
	int bRemoteDG = 0;
	unsigned char uwDGID;

	g_bLargeTT = psCmd->uchLongDestID;

	bRemoteDG = psCmd->wRioHostDestID == 0xffff ? 0 : 1;

	if (!bRemoteDG)		// this PE
	{
		// get our own host ID just in case it has been updated since DG was initialized
		fs = rioConfigurationRead (0, 0, 0xffffffff, 0, RIO_BDIDCSR, &ulTemp, 1, 4, 0, 0);
		if (fs != eFET_SUCCESS) 
		{ 
			printf("RFS: Unable to read RIO_BDIDCSR\n"); 
			return fs;
		} 
		uwDGID = g_uwMyDestID = (unsigned char)((ulTemp>>16)&0xffff); 

		// get pointer to local data generator memory
		printf("Local PG is source\n");
		fs = RFDSTATUS_TO_FETSTATUS(RFD_DGGetLocalDGCtrlPtr(&pDGMemPtr)); 
	}
	else
	{
		// keep track of DG source's ID
		uwDGID = (unsigned char)(psCmd->wRioHostDestID & 0xffff); 

		// get pointer to remote data generator memory
		printf("RemotePE %d is source\n", psCmd->wRioHostDestID);
		fs = RFDSTATUS_TO_FETSTATUS(RFD_DGGetRemoteDGCtrlPtr(g_bLargeTT, (unsigned char)(psCmd->wRioHostDestID&0xffff), psCmd->ulRioTargetCtrlAddrLO, &pDGMemPtr)); 
	}
	if (fs != eFET_SUCCESS)
	{
		printf("Failed to get pointer to DG control memory\n");
		return fs;
	}
	psDG = (DGCONTROLSTRUCT*)pDGMemPtr;

	// make sure there is a data generator available on the specified PE
	if (!ValidDG(psDG))
	{
		return eDG_ERR_NOREMOTEDG;
	}

	// make sure the remote DG is compatible
	if (psDG->ulDGRev != MAKEVERSION(DGMAJORREV,DGMINORREV,DGBUILD))
	{
		printf("	Incompatible DG:  Looking for %08lx, found %08lx\n", MAKEVERSION(DGMAJORREV,DGMINORREV,DGBUILD), psDG->ulDGRev);
		return eDG_ERR_INCOMPATIBLE;
	}

	psDG->uwSrcDestID = htons(uwDGID); 
	printf("Data Generator's DestID is %d\n", ntohs(psDG->uwSrcDestID));

	retval = eFET_SUCCESS;

	switch(psCmd->eReqType) {
	case eDGREQTYPE_ENABLE:
		printf("DG_Command(Enable)\n");
		{
			UINT32 i;

			if (psDG->uchStatus != DGSTATUS_READY)
			{
				printf("Data generator is not ready\n");
				retval = eDG_ERR_DGNOTREADY;
				break;
			}
			psDG->uchResponse = DGRESP_NULLRESP;	// initialize so that we know when it's done
			psDG->uchRioXferType = psCmd->eRioXferType;
			psDG->uchRioXferPriority = psCmd->eRioXferPriority;
			psDG->ulRioPacketSize = htonl(psCmd->ulRioPacketSize);
			psDG->uchLongDestID = psCmd->uchLongDestID;
			psDG->uwTgtDestID = htons((psCmd->wRioTargetDestID&0xffff));
			psDG->ulTgtDestAddrHI = htonl(psCmd->ulRioTargetDestAddrHI);
			psDG->ulTgtDestAddrLO = htonl(psCmd->ulRioTargetDestAddrLO);
			psDG->ulTgtDestSize = htonl(psCmd->ulRioTargetDestSize);
			printf("Setting DG ID %d to active\n", ntohs(psDG->uwSrcDestID));
			SetActive(	bRemoteDG,
						psCmd->uchLongDestID,
						ntohs(psDG->uwSrcDestID), 
						psCmd->ulRioTargetCtrlAddrHI,
						psCmd->ulRioTargetCtrlAddrLO);
			psDG->chWatchDogCounter = (char)WATCHDOGCOUNT;
			psDG->uchFlags &= ~DGFLAG_WATCHDOGTIMEOUT;
			psDG->ulDDGEntryCount = htonl(psCmd->ulDDGEntryCount);
			psDG->ulDDGEntryIndex = 0;
			psDG->ulDDGEntryTime = 0;
			for (i=0; i<psCmd->ulDDGEntryCount; i++)
			{
				psDG->asDDGProfile[i].ulDDGDuration = htonl(psCmd->asDDGProfile[i].ulDDGDuration);
				psDG->asDDGProfile[i].ulDDGXferRate = htonl(psCmd->asDDGProfile[i].ulDDGXferRate);
			}
			psDG->bRepeat = psCmd->bRepeat;

			psDG->uchCommand = DGCMD_DMA;		// request DMA
			printf("Enabled DMA\n");
		}
		break;
	case eDGREQTYPE_DISABLE:
		printf("DG_Command(Disable)\n");
		{
			psDG->uchResponse = DGRESP_NULLRESP;	// initialize so that we know when it's done
			psDG->uchCommand = DGCMD_RESET;			// stop the current action
			SetInactive(ntohs(psDG->uwSrcDestID));
		}
		break;
	case eDGREQTYPE_UPDATE:
		printf("DG_Command(Update)\n");
		{
			// update the profile
			psDG->ulDDGEntryIndex = 0;
			psDG->ulDDGEntryTime = 0;
			for (i=0; i<ntohl(psDG->ulDDGEntryCount); i++)
			{
				psDG->asDDGProfile[i].ulDDGDuration = htonl(psCmd->asDDGProfile[i].ulDDGDuration);
				psDG->asDDGProfile[i].ulDDGXferRate = htonl(psCmd->asDDGProfile[i].ulDDGXferRate);
			}
		}
		break;
	case eDGREQTYPE_GETSTATUS:
		printf("DG_Command(HeartBeat)\n");
		{
			// test for watchdog timeout
			if (psDG->uchFlags & DGFLAG_WATCHDOGTIMEOUT)
			{
				retval = eDG_ERR_WATCHDOGTIMEOUT;
				break;
			}
			*pbLagging = (psDG->uchFlags & DGFLAG_LAGGING)?1:0;
			*pb1ShotComplete = (psDG->uchFlags & DGFLAG_1SHOTCOMPLETE)?1:0;
			*pulDDGEntryIndex = ntohl(psDG->ulDDGEntryIndex);
			printf("%s %08lx\n", *pbLagging?"Lagging":"Not Lagging", *pulDDGEntryIndex);
			fflush(stdout);
		}
		break;
	case eDGREQTYPE_CHECKIF:
		printf("DG_Command(Check Interface)\n");
		// nothing else to do - ValidDG has already been called and we wouldn't get here unless the interface is valid
		break;
	default:
		printf("DG_Command(%d) is unknown\n", psCmd->eReqType);
		retval = eDG_ERR_INVALIDREQUEST;
		break;
	}
 

	return retval;
}

FETSTATUS ResetWatchdogTimers(void)
{
	int i;
	void* pDGMemPtr;
	DGCONTROLSTRUCT* psDG;
	FETSTATUS fs;

	for (i=0; i<256; i++)
	{
		if (g_sDGStatus[i].uchDGStatus != DGACTIVE)
		{
			continue;
		}

		if (!g_sDGStatus[i].bRemoteDG)		// this PE
		{
			// get pointer to local data generator memory
//			printf("\nResetting local watchdog timer %d\n", i);
			fs = RFDSTATUS_TO_FETSTATUS(RFD_DGGetLocalDGCtrlPtr(&pDGMemPtr)); 
		}
		else
		{
			// get pointer to remote data generator memory
//			printf("\nResetting remote watchdog timer %d\n", i);
			fs = RFDSTATUS_TO_FETSTATUS(RFD_DGGetRemoteDGCtrlPtr(g_sDGStatus[i].bLargeTT,
																g_sDGStatus[i].wDestID,
																g_sDGStatus[i].ulCtrlStructAddrLO,
																&pDGMemPtr)); 
		}
		if (fs != eFET_SUCCESS)
		{
			printf("Failed to get pointer to DG control memory\n");
			return fs;
		}
		psDG = (DGCONTROLSTRUCT*)pDGMemPtr;

		// make sure there is a data generator available on the specified PE
		if (!ValidDG(psDG))
		{
			g_sDGStatus[i].uchDGStatus = DGINACTIVE;
			continue;
		}

		// reset the watchdog counter
		psDG->chWatchDogCounter = (char)WATCHDOGCOUNT;
//		printf("Rst %d ", uwDestID);
	}
	return eFET_SUCCESS;
}

FETSTATUS DG_Stop(void)
{
	int i;
	void* pDGMemPtr;
	DGCONTROLSTRUCT* psDG;
	FETSTATUS fs;

	printf("DG_Stop()\n");

	for (i=0; i<256; i++)
	{
		if (g_sDGStatus[i].uchDGStatus != DGACTIVE)
		{
			continue;
		}

		if (!g_sDGStatus[i].bRemoteDG)		// this PE
		{
			// get pointer to local data generator memory
			fs = RFDSTATUS_TO_FETSTATUS(RFD_DGGetLocalDGCtrlPtr(&pDGMemPtr)); 
		}
		else
		{
			// get pointer to remote data generator memory
			fs = RFDSTATUS_TO_FETSTATUS(RFD_DGGetRemoteDGCtrlPtr(g_sDGStatus[i].bLargeTT,
																g_sDGStatus[i].wDestID,
																g_sDGStatus[i].ulCtrlStructAddrLO,
																&pDGMemPtr)); 
		}
		if (fs != eFET_SUCCESS)
		{
			printf("Failed to get pointer to DG control memory\n");
			return fs;
		}
		psDG = (DGCONTROLSTRUCT*)pDGMemPtr;

		// make sure there is a data generator available on the specified PE
		if (!ValidDG(psDG))
		{
			g_sDGStatus[i].uchDGStatus = DGINACTIVE;
			continue;
		}

		// reset the DG
		printf("Reset DG @ DestID %d\n", g_sDGStatus[i].wDestID);
		psDG->uchResponse = DGRESP_NULLRESP;	// initialize so that we know when it's done
		psDG->uchCommand = DGCMD_RESET;			// stop the current action
		g_sDGStatus[i].uchDGStatus = DGINACTIVE;
	}
	return eFET_SUCCESS;
}

int ValidDG(DGCONTROLSTRUCT* psDG)
{
	int i;
	char chVal;
	int nRetries;

	for (i=0; i<16; i++)
	{
		for (nRetries = 5; nRetries>0; nRetries--)
		{
			chVal = psDG->achDGSig[i];
			if (chVal == achSignature[i])
			{
				break;
			}
			printf("	Bad DG Signature at Signature[%d] (Got %x, wanted %x)\n", i, chVal, achSignature[i] );
			printf("	psDG:%08lx\n", psDG);
		}
		if (!nRetries)
		{
			printf("	Too many retries\n");
			return 0;
		}
	}
	return 1;
}

void SetActive(	unsigned char bRemoteDG,
				unsigned char bLargeTT,
				unsigned short wDestID, 
				unsigned long ulRioTargetCtrlAddrHI,
				unsigned long ulRioTargetCtrlAddrLO)
{
	int i;
	for (i=0; i<256; i++)
	{
		if ((g_sDGStatus[i].uchDGStatus == DGACTIVE) && (g_sDGStatus[i].wDestID == wDestID))
		{
			// already there
			return;
		}
	}

	for (i=0; i<256; i++)
	{
		if (g_sDGStatus[i].uchDGStatus == DGINACTIVE)
		{
			g_sDGStatus[i].bRemoteDG = bRemoteDG;
			g_sDGStatus[i].bLargeTT = bLargeTT;
			g_sDGStatus[i].wDestID = wDestID;
			g_sDGStatus[i].ulCtrlStructAddrHI = ulRioTargetCtrlAddrHI;
			g_sDGStatus[i].ulCtrlStructAddrLO = ulRioTargetCtrlAddrLO;
			g_sDGStatus[i].uchDGStatus = DGACTIVE;
			return;
		}
	}
}

void SetInactive(unsigned short wDestID)
{
	int i;
	for (i=0; i<256; i++)
	{
		if ((g_sDGStatus[i].uchDGStatus == DGACTIVE) && (g_sDGStatus[i].wDestID == wDestID))
		{
			g_sDGStatus[i].uchDGStatus = DGINACTIVE;
			return;
		}
	}
}

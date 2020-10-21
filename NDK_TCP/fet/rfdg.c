///////////////////////////////////////////////////////////////////////////////
// $RCSfile: rfdg.c,v $
// $Date: 2008/09/15 18:47:52 $
// $Author: Gilmour $
// $Revision: 1.6 $
// $Source: /cvsrepo/FET/Server/STX8548AMC/STX8548AMC_LIN/rfdg.c,v $
//
// Copyright (c) 2004-2008 Fabric Embedded Tools Corporation
//
///////////////////////////////////////////////////////////////////////////////



#include <usertype.h>
#include <stdlib.h>
#include "mytypes.h"

#include <signal.h>
#include <stdio.h>
#include <errno.h>
#include "fetriod.h"
#include "rfd.h"

RFDSTATUS RFD_DGOpen(int nDGCtrlBytes, int nDGDataBytes, UINT32 ulRioAddress, RFDSTATUS (*pCallback)(void))
{
	return 0;
}


RFDSTATUS RFD_RioDMARead(
						unsigned char bLargeTT,
						UINT32 ulDestID,
						UINT32 ulRioAddressHi,
						UINT32 ulRioAddressLo,
						UINT32 ulDestBufferSizeBytes,
						UINT32 ulXferType,
						UINT32 ulPriority,
						unsigned char *puchData,	// not implemented
						UINT32 ulByteCount,
						UINT32 *pbTerminated)
{
	return 0;
}

RFDSTATUS RFD_RioDMAWrite(
						unsigned char bLargeTT,
						UINT32 ulDestID,
						UINT32 ulRioAddressHi,
						UINT32 ulRioAddressLo,
						UINT32 ulDestBufferSizeBytes,
						UINT32 ulXferType,
						UINT32 ulPriority,
						unsigned char *puchData,	// not implemented
						UINT32 ulByteCount,
						UINT32 *pbTerminated)
{
	return 0;
}

RFDSTATUS RFD_DGGetLocalDGCtrlPtr(void** pp)
{

	return eRFSRFD_SUCCESS;
}

RFDSTATUS RFD_DGGetRemoteDGCtrlPtr(unsigned char bLargeTT, unsigned short uwDestID, unsigned long ulRioAddress, void** pp)
{
	return eRFSRFD_SUCCESS;
}

#if 0

RFDSTATUS RFD_DGOpen(int nDGCtrlBytes, int nDGDataBytes, UINT32 ulRioAddress, RFDSTATUS (*pCallback)(void))
{
	int nRet;
	int i;
	unsigned long ulSize;

	printf("RFDG_Open(%08lx, %08lx, %08lx)\n", nDGCtrlBytes, nDGDataBytes, ulRioAddress);

	g_nDGDataBytes = nDGDataBytes;
#if defined(MPC8548)
	g_nDGTxWindowBytes = nDGDataBytes;
#endif
	g_nDGCtrlBytes = nDGCtrlBytes;
	g_ulRioAddress = ulRioAddress;
	g_pCallback = pCallback;

	// figure out how much memory we'll have to allocate
	ulSize = g_nDGDataBytes+g_nDGCtrlBytes;

	/////////////////////////////////////////////////////////////////////////////
	// map an inbound window for other devices to be able to access our memory //
	/////////////////////////////////////////////////////////////////////////////

	nRet = frdAllocInboundWindow(&ulSize, &g_hIBWin, &g_hIBMem);
	printf("frdAllocInboundWindow returned %d.  g_hIBWin is %d, g_hIBMem is %d, size is %x\n", nRet, g_hIBWin, g_hIBMem, ulSize);
	if (nRet != 0)
	{
		//sdr sealed  fprintf(stderr, "Could not allocate inbound window.  Error %d\n", errno);
		return eRFSRFD_ERR;
	}
	printf("Allocated %08lx bytes for local control access.\n", ulSize);

	// get virtual address of buffer
	g_pDGLocalVbase = frdGetAddressV(g_hIBWin);
	if (g_pDGLocalVbase == NULL)
	{
		//sdr sealed  fprintf(stderr, "Could not get pointer to inbound window.  Error %d\n", errno);
		return eRFSRFD_ERR;
	}
	printf("	Virtual Base Memory Address is     %08lx - %08lx\n", g_pDGLocalVbase, g_pDGLocalVbase+(ulSize*2)-1);

	// get physical address of buffer
	g_pDGLocalPbase = frdGetAddressP(g_hIBWin);
	if (g_pDGLocalPbase == NULL)
	{
		//sdr sealed  fprintf(stderr, "Could not determine physical address of buffer memory.  Error %d\n", errno);
		return eRFSRFD_ERR;
	}
	printf("	Physical Base Memory Address is    %08lx - %08lx\n", g_pDGLocalPbase, g_pDGLocalPbase+(ulSize*2)-1);

	// configure the inbound window
	nRet = frdConfigInboundWindow(g_hIBWin, eIBW_RTYPE_NOSNOOP, eIBW_WTYPE_NOSNOOP, g_ulRioAddress, 0);
//	printf("frdConfigInboundWindow returned %d.  \n", nRet);
	if (nRet != 0)
	{
		//sdr sealed  fprintf(stderr, "Could not configure inbound window.  Error %d\n", errno);
		return eRFSRFD_ERR;
	}
	printf("Inbound window at RIO address %08lx now points to %08lx\n", g_ulRioAddress, g_pDGLocalPbase);

#if defined(MPC8548)
	////////////////////////////////////////////////////////////////
	// map an outbound window so that we can DMA to other devices //
	////////////////////////////////////////////////////////////////

	// allocate the window
	ulSize = g_nDGTxWindowBytes;
	nRet = frdAllocOutboundWindow(&ulSize, &g_hOBWinData);
	g_nDGTxWindowBytes = ulSize;
	printf("frdAllocOutboundWindow returned %d.  g_hOBWinData is %d, size is %x\n", nRet, g_hOBWinData, g_nDGTxWindowBytes);
	if (nRet != 0)
	{
		//sdr sealed  fprintf(stderr, "Could not allocate outbound window for data.  Error %d\n", errno);
		return eRFSRFD_ERR;
	}
	printf("Allocated %d bytes of RIO space for DMAing to remote devices.\n", g_nDGTxWindowBytes);
	printf("g_hOBWinData is %d, size is %x, VAddr is %08lx, PAddr is %08lx\n", 
		g_hOBWinData, 
		frdGetSize(g_hOBWinData),
		frdGetAddressV(g_hOBWinData),
		frdGetAddressP(g_hOBWinData));
#endif

	//////////////////////////////////////////////////////////////////////////////////
	// map an outbound window so that we can access control memory on other devices //
	//////////////////////////////////////////////////////////////////////////////////

	// allocate the window
	ulSize = g_nDGCtrlBytes;
	nRet = frdAllocOutboundWindow(&ulSize, &g_hOBWinCtrl);
	g_nDGCtrlBytes = ulSize;
//	printf("frdAllocOutboundWindow returned %d.  g_hOBWinCtrl is %d, size is %x\n",
//		nRet, g_hOBWinCtrl, g_nDGCtrlBytes);
	if (nRet != 0)
	{
		//sdr sealed  fprintf(stderr, "Could not allocate outbound window for control.  Error %d\n", errno);
		return eRFSRFD_ERR;
	}
	printf("Allocated %d bytes for remote control access.\n", g_nDGCtrlBytes);

	// determine the virtual address of the window
	g_pDGRemote_Ctrl = frdGetAddressV(g_hOBWinCtrl);
	if (g_pDGRemote_Ctrl == NULL)
	{
		//sdr sealed  fprintf(stderr, "Could not get address of outbound window for control.  Error %d\n", errno);
		return eRFSRFD_ERR;
	}
	printf("	Memory Ptr is    %08lx\n", g_pDGRemote_Ctrl);

	////////////////////////////////////////////////
	// Allocate some real memory to DMA out of... //
	////////////////////////////////////////////////

	nRet = frdAllocMem(g_nDGDataBytes, &g_hMemBuf);
	if (nRet != 0)
	{
		//sdr sealed  fprintf(stderr, "Could not allocate DMA source memory.  Error %d\n", errno);
		return eRFSRFD_ERR;
	}
	printf("Allocated %d bytes for local DMA source data.\n", g_nDGDataBytes);
//	printf("	Handle is %d\n", g_hMemBuf);

	// get virtual address of buffer
	g_aulLocalBufferV = (unsigned long*)frdGetAddressV(g_hMemBuf);
	if (g_aulLocalBufferV == NULL)
	{
		//sdr sealed  fprintf(stderr, "Could not determine virtual address of local DMA source memory.  Error %d\n", errno);
		return eRFSRFD_ERR;
	}
	printf("	Virtual Memory Address is  %08lx\n", g_aulLocalBufferV);

	// get physical address of buffer
	g_aulLocalBufferP = (unsigned long*)frdGetAddressP(g_hMemBuf);
	if (g_aulLocalBufferP == NULL)
	{
		//sdr sealed  fprintf(stderr, "Could not determine physical address of local DMA source memory.  Error %d\n", errno);
		return eRFSRFD_ERR;
	}
	printf("	Physical Memory Address is %08lx\n", g_aulLocalBufferP);

	// initialize local buffer memory 
	for (i=0; i<g_nDGDataBytes/sizeof(unsigned long); i++)
	{
		if (i&1)
		{
			g_aulLocalBufferV[i]=0x64464554;		// dFET
		}
		else
		{
			g_aulLocalBufferV[i]=0x52617069;		// Rapi
		}
	}
	printf("RFD_DGOpen:Memory has been initialized\n");fflush(stdout);

	////////////////////////////////////////////////
	// allocate a DMA chain
	////////////////////////////////////////////////

	nRet = frdDMA_AllocChain(&g_hChain, MAXCHAINLINKS);
	if (nRet != 0)
	{
	//sdr sealed	perror("Unable to allocate DMA chain.");fflush(stderr);
		return eRFSRFD_ERR;
	}

	// initialize a mutex to permit safe access to variables shared between
	// the server thread and the data generator thread
	pthread_mutex_init(&mutexDG, NULL);

	// start the data generator thread
	g_bWantDGServer = true;
	g_bDGServerRunning = false;
	pthread_create(&threadDG, NULL, (void*)&DataGeneratorMain, (void*)NULL);

	// wait for the thread to get going
	for (i=0; i<5; i++)
	{
		if (!g_bDGServerRunning)
		{
			sleep(1);
		}
	}

	// make sure thread is running
	if (!g_bDGServerRunning)
	{
		// thread failed to start
		//sdr sealed  fprintf(stderr, "RFDG_Open:DG Server failed to start\n");fflush(stderr);
		g_bWantDGServer = false;
		return eRFSRFD_ERR_DGPROCERR;
	}

	nRet = frdDMA_Open(&g_hDMA);
	if (nRet != 0) 
	{
		return eRFSRFD_ERR;
	}

	printf("RFDG_Open:DG Server is running\n");fflush(stdout);

	return eRFSRFD_SUCCESS;
}

RFDSTATUS RFD_DGClose()
{
	int i;

	if (!g_bWantDGServer)
	{
		return eRFSRFD_ERR_DGNOTOPEN;
	}
	g_bWantDGServer = false;
	printf("Waiting for DG thread to terminate");fflush(stdout);
	// wait for the thread to terminate
	for (i=0; i<5; i++)
	{
		printf(".");fflush(stdout);
		if (g_bDGServerRunning)
		{
			sleep(1);
		}
	}
	printf("\n");

	// make sure thread is not running
	if (g_bDGServerRunning)
	{
		// thread failed to die
		printf("DG thread did not terminate\n");
		return eRFSRFD_ERR_DGPROCERR;
	}

	frdFreeMem(g_hMemBuf);
	frdFreeOutboundWindow(g_hOBWinCtrl);
	frdFreeInboundWindow(g_hIBWin);

	return eRFSRFD_SUCCESS;
}

RFDSTATUS RFD_DGGetRemoteDGCtrlPtr(unsigned char bLargeTT, unsigned short uwDestID, unsigned long ulRioAddress, void** pp)
{
	int nRet;

	// configure the outbound window to map to remote PE's buffer
	nRet = frdConfigOutboundWindow(g_hOBWinCtrl, eRTYPE_NREAD, eWTYPE_NWRITE, ePRIORITY_L, bLargeTT, uwDestID, ulRioAddress, 0);
//	printf("frdConfigOutboundWindow returned %d.  \n", nRet);
	if (nRet != 0)
	{
		//sdr sealed  fprintf(stderr, "Could not configure outbound control window.  Error %d\n", errno);fflush(stderr);
		return eRFSRFD_ERR;
	}

	printf("Configured remote control window: %cDev=%d, %08lx bytes starting at address %08lx\n",
									bLargeTT?'L':'S',
									uwDestID,
									g_nDGDataBytes,
									ulRioAddress);

	*pp = g_pDGRemote_Ctrl;
	return eRFSRFD_SUCCESS;
}

RFDSTATUS RFD_DGGetLocalDGCtrlPtr(void** pp)
{
//	pthread_mutex_lock(&mutexDG);
	*pp = g_pDGLocalVbase+g_nDGDataBytes;
//	printf("Local DG Control Address: %08lx\n", *pp);
	return eRFSRFD_SUCCESS;
}

///////////////////////////////////////////////////////////////////
// ALL FOLLOWING ROUTINES ARE PART OF THE DATA GENERATION THREAD //
///////////////////////////////////////////////////////////////////

int g_bTick;

// The main Data Generator thread routine.  Handles control of data generation
// operations on this processor.
void DataGeneratorMain ( void *ptr )
{
	printf("Data Generator thread started\n");fflush(stdout);
	g_bDGServerRunning = true;

	// set up to receive alarm signals...
	struct sigaction sact;
	sigemptyset(&sact.sa_mask);
	sact.sa_flags = 0;
	sact.sa_handler = DataGeneratorAlarm;
	sigaction(SIGALRM, &sact, NULL);

	// schedule the first alarm to go off in 1 second...
	alarm(1);

	while (g_bWantDGServer)
	{
		g_bTick=0;		// flag to detect when we're falling behind

		// invoke the callback function
	g_pCallback();
		if (!g_bTick)
		{
            pause();	// wait for next alarm
		}
		else
		{
			printf("DG Lag! ");fflush(stdout);
		}
	}

	printf("Data Generator thread terminated\n");fflush(stdout);

	g_bDGServerRunning = false;
	pthread_exit(NULL);
}

// schedules the next alarm
void DataGeneratorAlarm (int sig)
{
	g_bTick=1;
		alarm(1);	// reschedule the alarm to go off again 1 second hence...
}

RFDSTATUS RFD_RioDMAWrite(
						unsigned char bLargeTT,
						UINT32 ulDestID,
						UINT32 ulRioAddressHi,
						UINT32 ulRioAddressLo,
						UINT32 ulDestBufferSizeBytes,
						UINT32 ulXferType,
						UINT32 ulPriority,
						unsigned char *puchData,	// not implemented
						UINT32 ulByteCount,
						UINT32 *pbTerminated)
{
	int nRet;
	int nChainSize;
	ePRIORITY ePriority;
	eWTYPE eWriteType;
        /* JuWi Removed unused variables. */
 	/* RFDSTATUS rfds; */
	/* int bNoTimer=0; */
	int nQueueLen = 0;
	unsigned long ulXferByteCount;

	*pbTerminated = 0;

	printf("RFD_DGDoDMA: %d to %d...", ulByteCount, ulDestID);fflush(stdout);

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

	switch(ulXferType)
	{
	case 0:
		eWriteType = eWTYPE_SWRITE;
		printf("Sw");
		break;
	case 1:
		eWriteType = eWTYPE_NWRITE;
		printf("Nw");
		break;
	case 2:
		eWriteType = eWTYPE_NWRITE_R;
		printf("NwR");
		break;
	default:
		eWriteType = eWTYPE_NWRITE;
		printf("Nw");
		break;
	}
	fflush(stdout);

	// check to see if previous DMA transfer is still ongoing
	if (frdDMA_Busy(g_hDMA) == 1)
	{
		// and terminate it if it is
		printf("Aborting current DMA operation...");fflush(stdout);
		*pbTerminated = 1;
		frdDMA_Abort(g_hDMA);
		while (frdDMA_Busy(g_hDMA) == 1)
		{
			printf("w");fflush(stdout);
		}
		printf("\n");
	}

	// determine number of links needed in chain, and limit it to the maximum allowed
	nChainSize = (ulByteCount / g_nDGDataBytes) + 1;
	if (nChainSize > MAXCHAINLINKS)
	{
		nChainSize = MAXCHAINLINKS;
	}

	// build the chain
	nRet = frdDMA_InitChain(g_hChain);
	if (nRet != 0)
	{
		perror("Could not initialize chain");
		exit(__LINE__);
	}

	while (ulByteCount)
	{
#if defined(MPC8540) || defined (MPC8560)
		if (ulByteCount > g_nDGDataBytes)
		{
			ulXferByteCount = g_nDGDataBytes;
		}
#elif defined(MPC8548)
		if (ulByteCount > g_nDGTxWindowBytes)
		{
			ulXferByteCount = g_nDGTxWindowBytes;
		}
#endif
		else
		{
			ulXferByteCount = ulByteCount;
		}

		if (ulXferByteCount > ulDestBufferSizeBytes)
		{
			ulXferByteCount = ulDestBufferSizeBytes;
		}

#if defined(MPC8540) || defined (MPC8560)
		nRet = frdDMA_AppendDirectWriteToRIO(g_hChain, g_hMemBuf, 0, bLargeTT, ulDestID, ulRioAddressLo, ulRioAddressHi, ulXferByteCount, ePriority, eWriteType);
#elif defined(MPC8548)
		nRet = frdDMA_AppendDirectWriteToMemory(g_hChain, g_hMemBuf, 0, g_hOBWinData, 0, ulXferByteCount);
#endif
		if (nRet != 0)
		{
			if (errno == E2BIG)	// no more chain links available
			{
				// this can happen if we've been asked to send too large an amount of data, or if the destination
				// memory buffer is too small, or both
				break;
			}
			else
			{
			perror("Could not add to DMA chain.");
			exit(__LINE__);
			}
		}
//		printf("	DMA %08lx bytes from buffer %d to RIO address %%lx08lx\n", ulXferByteCount, g_hMemBuf, ulRioAddressHi, ulRioAddressLo);
			
		nQueueLen++;
		ulByteCount -= ulXferByteCount;
	}
	printf(" QL:%d...", nQueueLen);fflush(stdout);

#if defined(MPC8548)
	// Configure the outbound window...
	nRet = frdConfigOutboundWindow(g_hOBWinData, eRTYPE_NREAD, eWriteType, ePriority, bLargeTT, ulDestID, ulRioAddressLo, ulRioAddressHi);

#endif

	// start the DMA transfer
	nRet = frdDMA_StartChainTransfer(g_hDMA, g_hChain, 0, 5000);
	if (nRet != 0)
	{
		//sdr sealed  fprintf(stderr, "Could not start DMA.  Error %d\n", errno);fflush(stderr);
		exit(__LINE__);
	}

	// check for errors 
	nRet = frdDMA_Error(g_hDMA);
	if (nRet == -1)
	{
		//sdr sealed  fprintf(stderr, "Could not start DMA.  Error %d\n", errno);fflush(stderr);
		exit(__LINE__);
	}
	if (nRet == 1)
	{
		//sdr sealed  fprintf(stderr, "DMA Error detected\n");fflush(stderr);
	}

	printf("Done\n");	

	return eRFSRFD_SUCCESS;
}

RFDSTATUS RFD_RioDMARead(
						unsigned char bLargeTT,
						UINT32 ulDestID,
						UINT32 ulRioAddressHi,
						UINT32 ulRioAddressLo,
						UINT32 ulDestBufferSizeBytes,
						UINT32 ulXferType,
						UINT32 ulPriority,
						unsigned char *puchData,	// not implemented
						UINT32 ulByteCount,
						UINT32 *pbTerminated)
{
	int nRet;
	int nChainSize;
	ePRIORITY ePriority;
	eRTYPE eReadType;
        /* JuWi Removed unused variables. */
 	/* RFDSTATUS rfds; */
	/* int bNoTimer=0; */
	int nQueueLen = 0;
	unsigned long ulXferByteCount;

	*pbTerminated = 0;

	printf("RFD_DGDoDMA: %d from %d...", ulByteCount, ulDestID);fflush(stdout);

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

	switch(ulXferType)
	{
	case 3:
		eReadType = eRTYPE_NREAD;
		printf("Nr");
		break;
	default:
		eReadType = eRTYPE_NREAD;
		printf("Nr");
		break;
	}
	fflush(stdout);

	// check to see if previous DMA transfer is still ongoing
	if (frdDMA_Busy(g_hDMA) == 1)
	{
		// and terminate it if it is
		printf("Aborting current DMA operation...");fflush(stdout);
		*pbTerminated = 1;
		frdDMA_Abort(g_hDMA);
		while (frdDMA_Busy(g_hDMA) == 1)
		{
			printf("w");fflush(stdout);
		}
		printf("\n");
	}

	// determine number of links needed in chain, and limit it to the maximum allowed
	nChainSize = (ulByteCount / g_nDGDataBytes) + 1;
	if (nChainSize > MAXCHAINLINKS)
	{
		nChainSize = MAXCHAINLINKS;
	}

	// build the chain
	nRet = frdDMA_InitChain(g_hChain);
	if (nRet != 0)
	{
		perror("Could not initialize chain");
		exit(__LINE__);
	}

	while (ulByteCount)
	{
#if defined(MPC8540) || defined (MPC8560)
		if (ulByteCount > g_nDGDataBytes)
		{
			ulXferByteCount = g_nDGDataBytes;
		}
#elif defined(MPC8548)
		if (ulByteCount > g_nDGTxWindowBytes)
		{
			ulXferByteCount = g_nDGTxWindowBytes;
		}
#endif
		else
		{
			ulXferByteCount = ulByteCount;
		}

		if (ulXferByteCount > ulDestBufferSizeBytes)
		{
			ulXferByteCount = ulDestBufferSizeBytes;
		}

#if defined(MPC8540) || defined (MPC8560)
		//sdr sealed  fprintf(stderr, "DMA Read not currently supported in FETRioD for this processor type.\n");
		exit(__LINE__);
#elif defined(MPC8548)
		nRet = frdDMA_AppendDirectWriteToMemory(g_hChain, g_hOBWinData, 0, g_hMemBuf, 0, ulXferByteCount);
#endif
		if (nRet != 0)
		{
			if (errno == E2BIG)	// no more chain links available
			{
				// this can happen if we've been asked to send too large an amount of data, or if the destination
				// memory buffer is too small, or both
				break;
			}
			else
			{
				perror("Could not add to DMA chain.");
				exit(__LINE__);
			}
		}
//		printf("	DMA %08lx bytes from buffer %d to RIO address %%lx08lx\n", ulXferByteCount, g_hMemBuf, ulRioAddressHi, ulRioAddressLo);
			
		nQueueLen++;
		ulByteCount -= ulXferByteCount;
	}
	printf(" QL:%d...", nQueueLen);fflush(stdout);

#if defined(MPC8548)
	// Configure the outbound window...
	nRet = frdConfigOutboundWindow(g_hOBWinData, eReadType, eWTYPE_NWRITE, ePriority, bLargeTT, ulDestID, ulRioAddressLo, ulRioAddressHi);

#endif

	// start the DMA transfer
	nRet = frdDMA_StartChainTransfer(g_hDMA, g_hChain, 0, 5000);
	if (nRet != 0)
	{
		//sdr sealed  fprintf(stderr, "Could not start DMA.  Error %d\n", errno);fflush(stderr);
		exit(__LINE__);
	}

	// check for errors 
	nRet = frdDMA_Error(g_hDMA);
	if (nRet == -1)
	{
		//sdr sealed  fprintf(stderr, "Could not start DMA.  Error %d\n", errno);fflush(stderr);
		exit(__LINE__);
	}
	if (nRet == 1)
	{
		//sdr sealed  fprintf(stderr, "DMA Error detected\n");fflush(stderr);
	}

	printf("Done\n");	

	return eRFSRFD_SUCCESS;
}

#endif

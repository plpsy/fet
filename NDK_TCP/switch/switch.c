///////////////////////////////////////////////////////////////////////////////
// switch function
// DSP----(p10)(SW20)(p5)-----(p10)(SW40)
// SW20 nodeid(0,12) SW40 nodeid(100+0, 100+12)
// DSP ID = 10
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

unsigned char gAllNodes[256] = {0}

Void switchMain(UArg a0, UArg a1)
{
	Task_sleep(1000);
	cfgSWPaths();

	while(1)
	{
		scanNewNodes();
		Task_sleep(1000);
	}
}



Void switchInit()
{
	/* Task 线程 */
    Task_create(switchMain, NULL, NULL);
}


void cfgSW20Route(unsigned char destid, unsigned char port)
{
	UINT32 data;
	unsigned char uchHopCount = 0;
	data = destid << 24;
	frdMaintWrite(0, 0, 20, uchHopCount, 0x70, ePRIORITY_M, data, 1, 4, 0);

	data = port << 24;
	frdMaintWrite(0, 0, 20, uchHopCount, 0x74, ePRIORITY_M, data, 1, 4, 0);
}

/*
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
								
*/

void cfgSW40Route(unsigned char destid, unsigned char port)
{
	UINT32 data;
	unsigned char uchHopCount = 1;
	data = destid << 24;
	frdMaintWrite(0, 0, 40, uchHopCount, 0x70, ePRIORITY_M, data, 1, 4, 0);

	data = port << 24;
	frdMaintWrite(0, 0, 40, uchHopCount, 0x74, ePRIORITY_M, data, 1, 4, 0);

}

void cfgSWPaths()
{
	unsigned char port = 0;
	unsigned char destid = 0;

	// cfg node route connected to SW20
	for(port = 0; port < 12; port++)
	{
		destid = port
		cfgSW20Route(destid, port);
	}
	// cfg DSP route to SW40
	cfgSW20Route(40, 5);	
	// cfg nodes route connected to SW20 via SW20(PORT5)
	for(destid = 100; destid < 100+12; destid++)
	{
		port = 5
		cfgSW20Route(destid, port);
	}

	// cfg nodes route connected to SW40
	for(port = 0; port < 12; port++)
	{
		destid = 100 + port
		cfgSW40Route(destid, port);
	}	

	// cfg nodes route connected to SW20 via SW40(PORT10)
	for(destid = 0; destid < 12; destid++)
	{
		port = 10
		cfgSW20Route(destid, port);
	}
}

void scanNewNodes()
{
	unsigned char port = 0;
	unsigned char destid = 0;
	unsigned char uchHopCount = 0;
	UINT32 data;	
	// scan SW20
	for(port = 0; port < 12; port++)
	{
		if(port == 5 || port == 10)
		{
			continue;
		}
		destid = port;
		uchHopCount = 0;
		frdMaintRead(0, 0, 20, uchHopCount, 0x158 + 0x20*port, &data, 1, 4, 0);
		if(data&0x2)
		{
			if(!gAllNodes[destid])
			{
				cfgSW20Route(0xff, port);
				data = destid<<16|destid;
				uchHopCount = 1;
				frdMaintWrite(0, 0, 0xff, uchHopCount, 0x60, ePRIORITY_M, data, 1, 4, 0);
				gAllNodes[destid] = 1;
			}
		}
	}

	cfgSW20Route(0xff, 5);
	// scan SW40
	for(port = 0; port < 12; port++)
	{
		if(port == 10)
		{
			continue;
		}
		destid = 100 + port;
		uchHopCount = 1;
		frdMaintRead(0, 0, 20, uchHopCount, 0x158 + 0x20*port, &data, 1, 4, 0);
		if(data&0x2)
		{
			if(!gAllNodes[destid])
			{
				cfgSW40Route(0xff, port);
				data = destid<<16|destid;
				uchHopCount = 2;
				frdMaintWrite(0, 0, 0xff, uchHopCount, 0x60, ePRIORITY_M, data, 1, 4, 0);
				gAllNodes[destid] = 1;
			}
		}
	}
}

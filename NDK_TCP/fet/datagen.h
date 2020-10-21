///////////////////////////////////////////////////////////////////////////////
// $RCSfile: datagen.h,v $
// $Date: 2007/05/22 20:36:44 $
// $Author: Gilmour $
// $Revision: 1.2 $
// $Source: /cvsrepo/FET/Server/STX8548AMC/STX8548AMC_LIN/datagen.h,v $
//
// Copyright (c) 2004 Fabric Embedded Tools Corporation
//
///////////////////////////////////////////////////////////////////////////////
#ifndef datagen__H
#define datagen__H	

FETSTATUS DG_Init(void);
FETSTATUS DG_Command(FETMSG_DATAGENCTRLSTRUCT* psCmd,
					 UINT32* pulDDGEntryIndex,
					 UINT32* pbLagging,
					 UINT32* pb1ShotComplete); 
FETSTATUS DG_ResetAll(void);
FETSTATUS DG_Reset(unsigned char uchWhichPE);
FETSTATUS DG_Stop(void);

#endif

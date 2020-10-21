///////////////////////////////////////////////////////////////////////////////
// $RCSfile: rio.h,v $
// $Date: 2008/06/16 19:06:12 $
// $Author: Gilmour $
// $Revision: 1.6 $
// $Source: /cvsrepo/FET/Server/rio.h,v $
//
// Copyright (c) 2004-2008 Fabric Embedded Tools Corporation
//
///////////////////////////////////////////////////////////////////////////////

#define RIO_DIDCAR		0x00
#define RIO_DICAR		0x04
#define RIO_AIDCAR		0x08
#define RIO_AICAR		0x0C
#define RIO_PEFCAR		0x10
#define RIO_SPICAR		0x14
#define RIO_SOCAR		0x18
#define RIO_DOCAR		0x1C
#define RIO_MCSR		0x40
#define RIO_WPDCSR		0x44
#define RIO_PELLCCSR	0x4C
#define RIO_LCSBA0CSR	0x58
#define RIO_LCSBA1CSR	0x5C
#define RIO_BDIDCSR		0x60
#define RIO_HBDIDLCSR	0x68
#define RIO_CTCSR		0x6C

/* JuWi Changed include path */
/* #include "common/rfd.h" */
#include "rfd.h"

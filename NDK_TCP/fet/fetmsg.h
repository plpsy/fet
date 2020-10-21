///////////////////////////////////////////////////////////////////////////////
// $RCSfile: fetmsg.h,v $
// $Date: 2008/09/15 18:34:14 $
// $Author: Gilmour $
// $Revision: 1.32 $
// $Source: /cvsrepo/FET/Common/include/fetmsg.h,v $
//
// Copyright (c) 2004-2008 Fabric Embedded Tools Corporation
//
///////////////////////////////////////////////////////////////////////////////

#ifndef fetmsg__H
#define fetmsg__H	

#ifdef _WIN32
#pragma once
#endif

#define FETMSG_VERSION	0x20071021

#define FET_MSGPORT						15000 // [TCP] Server listens and RapidFET client connects to server on this port. 
#define FET_BROADCASTPORT				15001 // [UDP] Client broadcasts a FET_IDENTIFY message on this port when user presses "find targets" button.
#define FET_BROADCASTRESPPORT			15002 // [UDP] Server uses this as it's "From" port when responding to a  FET_IDENTIFY request
#define FET_BROADCASTRESPPORT_SERVER	15003 // [UDP] Server sends FET_IDENTIFY responses to the client on this port

// RapidFET Message Types
#define FET_HALGETNUMLOCALPORTS			1
#define FET_HALGETNUMLOCALPORTSRESP		2
#define FET_HALCONFIGREAD				3
#define FET_HALCONFIGREADRESP			4
#define FET_HALCONFIGWRITE				5
#define FET_HALCONFIGWRITERESP			6
#define FET_CLOSE						7
#define FET_IDENTIFY					9
#define FET_IDENTIFYRESP				10
#define FET_DATAGENCTRL					11
#define FET_DATAGENCTRLRESP				12
#define FET_HALCONFIGRWMULTI			13
#define FET_HALCONFIGRWMULTIRESP		14
#define FET_HALRIOREAD					15
#define FET_HALRIOREADRESP				16
#define FET_HALRIOWRITE					17
#define FET_HALRIOWRITERESP				18
#define FET_HALGETRIOERROR				19
#define FET_HALGETRIOERRORRESP			20
#define FET_HALRIOTXMESSAGE				21
#define FET_HALRIOTXMESSAGERESP			22
#define FET_HALRIOTXDOORBELL			23
#define FET_HALRIOTXDOORBELLRESP		24
#define FET_HALRIORXMESSAGE				25
#define FET_HALRIORXMESSAGERESP			26
#define FET_HALRIORXDOORBELL			27
#define FET_HALRIORXDOORBELLRESP		28
#define FET_MEMREAD						29
#define FET_MEMREADRESP					30
#define FET_MEMWRITE					31
#define FET_MEMWRITERESP				32
#define FET_GETSERVERCAPS				33
#define FET_GETSERVERCAPSRESP			34
#define FET_FLASHREAD					35
#define FET_FLASHREADRESP				36
#define FET_FLASHWRITE					37
#define FET_FLASHWRITERESP				38

// RapidFET Message Header Structure
typedef struct FETMSG_HEADER
{
	UINT32 nMsgType;
	UINT32 nMsgSize;
} FETMSG_HEADERSTRUCT;

////////////////////////////////////////
// RapidFET Message Payload Stuctures //
////////////////////////////////////////

typedef enum {
	// NOTE:  DO NOT CHANGE ANY OF THESE VALUES OR THE RFD API WILL BE INVALIDATED
	eRIOXFERTYPE_SWRITE=0,		// SWRITE
	eRIOXFERTYPE_NWRITE=1,		// NWRITE
	eRIOXFERTYPE_NWRITE_R=2,	// NWRITE_R
	eRIOXFERTYPE_NREAD=3, 		// NREAD
	eRIOXFERTYPE_DOORBELL=10,	// Doorbell
	eRIOXFERTYPE_MESSAGE=11		// Message (Mailbox)
} eRIOXFERTYPE;

typedef enum {
	// NOTE:  DO NOT CHANGE ANY OF THESE VALUES OR THE RFD API WILL BE INVALIDATED
	eRIOPRIORITY_L=0,			// Low priority transfer requested
	eRIOPRIORITY_M=1,			// Medium priority transfer requested
	eRIOPRIORITY_H=2			// High priority transfer requested
} eRIOPRIORITY;

typedef enum {
	eDGREQTYPE_DISABLE=0,
	eDGREQTYPE_UPDATE=1,
	eDGREQTYPE_ENABLE=2,
	eDGREQTYPE_GETSTATUS=3,
	eDGREQTYPE_CHECKIF=4
} eDGREQTYPE;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Payload for a FET_DATAGENCTRL message.  Controls the RapidFET Data Generator.
// Client -> Server only
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define MAXDDGENTRIES 20
typedef struct FETMSG_DATAGENCTRL
{
	eDGREQTYPE eReqType;				// type of request being made
	eRIOXFERTYPE eRioXferType;			// the type of transfer to be made
	eRIOPRIORITY eRioXferPriority;		// priority level to use for the transfer
	UINT32 bRepeat;						// indicates whether sequence repeats or is just output once
	UINT32 ulRioPacketSize;				// maximum size of RapidIO packet (1 - 256)
	unsigned char uchPad1;
	unsigned char uchPad2;
	unsigned char uchPad3;
	unsigned char uchLongDestID;		// nonzero indicates wRioTargetDestID contains large transport (16 bit) value
	unsigned short wRioTargetDestID;	// the DestID of the PE where the data is to be sent to / read from
	unsigned short wRioHostDestID;		// the DestID of the PE that will do the DG
	UINT32 ulRioTargetDestAddrHI;		// the RIO address of where on the target PE the data is to be written
	UINT32 ulRioTargetDestAddrLO;		// the RIO address of where on the target PE the data is to be written
	UINT32 ulRioTargetDestSize;			// the size of the available buffer on the target PE
	UINT32 ulRioTargetCtrlAddrHI;		// the RIO address of the control structure on the PE (remote DG only)
	UINT32 ulRioTargetCtrlAddrLO;		// the RIO address of the control structure on the PE (remote DG only)
	UINT32 ulDDGEntryCount;				// number of entries in the following array
	struct
	{
		UINT32 ulDDGDuration;			// length (seconds)
		UINT32 ulDDGXferRate;			// output rate (bytes/second)
	} asDDGProfile[MAXDDGENTRIES];		// array that describes the dynamic data generation profile
} FETMSG_DATAGENCTRLSTRUCT;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Payload for a FET_DATAGENCTRLRESP message.
// Server -> Client only
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct FETMSG_DATAGENCTRLRESP
{
	FETMSG_DATAGENCTRLSTRUCT sCmd;
	FETSTATUS eStatus;					// Success status
	UINT32 ulDDGEntryIndex;		// the current index into the asDDGProfile table
	UINT32 bLagging;				// a flag that indicates if the DG is lagging real time
	UINT32 b1ShotComplete;		// a flag that indicates that the one-shot cycle is done
} FETMSG_DATAGENCTRLRESPSTRUCT;


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Payload for a FET_IDENTIFYRESP message.  Contains information describing the PE that is
// responding to a FET_IDENTIFY message.
// Server -> Client only
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct FETMSG_IDENTIFYRESP
{
	UINT32 m_ulFETMsgVersion;				// identifies the FET message interface revision supported by the server
	unsigned short m_wDeviceID;				// identifies the type of PE - vendor assigned
	unsigned short m_wVendorID;				// identifies the vendor of the PE - RTA assigned
	unsigned short m_wShortDestID;			// 8-bit DestID for the PE - small transport type systems
	unsigned short m_wLongDestID;			// 16-bit DestID for the PE - large transport type systems
	UINT32	m_bSupportsLargeTransport;		// non-zero if long dest id is valid, i.e. large DestIDs are supported
	unsigned char m_uchStatus;				// see RFSSTATUS defines below
	unsigned char reserved1[3];				// pad
	UINT32 m_ulNumLocalPorts;				// number of RIO ports on the PE
	unsigned char m_uchMajorServerRevision;	// Major Revision ID for the RapidFET Server
	unsigned char m_uchMinorServerRevision;	// Minor Revision ID for the RapidFET Server
	unsigned char reserved2[2];				// pad
	UINT32 m_ulMajorDriverRevision;			// Major Revision ID for the RapidFET Driver Interface
	UINT32 m_ulMinorDriverRevision;			// Minor Revision ID for the RapidFET Driver Interface
	unsigned short m_wServerRTOS;			// Indicates type of RTOS
	unsigned short m_wServerProc;			// Indicates type of processor
	char m_szServerID[48];					// Text string identifying server
	char m_szDriverID[48];					// Text string identifying driver interface
} FETMSG_IDENTIFYRESPSTRUCT;

#define RFSSTATUS_UNKNOWN	0
#define RFSSTATUS_LISTENING	1				// Ready for a TCP connection
#define RFSSTATUS_CONNECTED	2				// Already connected to a RapidFET client
#define RFSSTATUS_ERROR		3				// In an error state.  Unable to connect.
#define RFSSTATUS_READYUDP	4				// Ready for a UDP connection (TCP not supported)

#define MAJORVERSION(x) (((x)>>24)&0xff)
#define MINORVERSION(x) (((x)>>16)&0xff)
#define BUILDVERSION(x) ((x)&0xffff)
#define MAKEVERSION(major, minor, build) ((((major)&0xff)<<24) + (((minor)&0xff)<<16) + ((build)&0xffff))

#define PROC_ID_SIM			0	// RapidFET simulator
#define PROC_ID_PQIII		1	// FreeScale 8540 and 8560
#define PROC_ID_TI6455		2	// TI TMS320C6455
#define PROC_ID_TI6678		3	// TI TMS320C6455

#define RTOS_ID_SIM			0	// RapidFET simulator only
#define RTOS_ID_LINUX		1	// Linux
#define RTOS_ID_NEUTRINO	2	// QNX Neutrino
#define RTOS_ID_DSPBIOS		3	// TI DSP/BIOS

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Payload for a FET_HALGETNUMLOCALPORTSRESP message.  Contains the results of the transaction requested via
// the FET_HALGETNUMLOCALPORTS message.
// Server -> Client only
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct FETMSG_GETNUMLOCALPORTSRESP
{
	FETSTATUS eStatus;				// Success status
	UINT32 ulNumPorts;		// The number of ports
} FETMSG_GETNUMLOCALPORTSRESPSTRUCT;


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Payload for a FET_HALCONFIGREAD message.  Performs a configuration read transaction from CAR and/or 
// CSR register(s) belonging to a local or remote RapidIO device.  Results are returned in a
// FET_HALCONFIGREADRESP message.
// Client -> Server only
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct FETMSG_CONFIGREAD
{
	unsigned char uchLocalPort;		// Local Port Number
	unsigned char uchPad1;			// unused
	unsigned char uchPad2;			// unused
	unsigned char uchLongDestID;	// nonzero indicates ulDestID contains large transport (16 bit) value
	UINT32 ulDestID;				// Destination ID of the target device
	unsigned char uchHopCount;		// Hop count
	unsigned char uchPad4;			// unused
	unsigned char uchPad5;			// unused
	unsigned char uchWantTiming;	// set to nonzero if want timing information
	UINT32 ulOldTimeMarker;			// Time marker from a previous read, used for calculating time since 
									//  that last read	
	UINT32 ulOffset;				// Word-aligned (four byte boundary) offset - in bytes - of the CAR or CSR
	UINT32 ulWordCount;				// the number of words to be read
	UINT32 ulWordSize;				// number of bytes per word
	UINT32 ulStride;				// the number of bytes to increment ulOffset by with each read
	eRIOPRIORITY eRioXferPriority;	// priority level to use for the transfer
} FETMSG_CONFIGREADSTRUCT;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Payload for a FET_HALCONFIGREADRESP message.  Contains the results of the read transaction requested via
// the FET_HALCONFIGREAD message.
// Server -> Client only
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct FETMSG_CONFIGREADRESP
{
	FETMSG_CONFIGREADSTRUCT sCmd;	// Contents of the request payload
	UINT32 ulElapsedTimeUS;	// Elapsed time since "ulOldTimeMarker" in microseconds
	UINT32 ulNewTimeMarker;	// Time marker for this read
	FETSTATUS eStatus;				// Success status
} FETMSG_CONFIGREADRESPSTRUCT;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Payload for a FET_HALCONFIGWRITE message.  Performs a configuration write transaction toa CAR and/or 
// CSR register(s) belonging to a local or remote RapidIO device.  Results are returned in a
// FET_HALCONFIGWRITERESP message.
// Client -> Server only
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct FETMSG_CONFIGWRITE
{
	unsigned char uchLocalPort;		// Local Port Number
	unsigned char uchPad1;			// unused
	unsigned char uchPad2;			// unused
	unsigned char uchLongDestID;	// nonzero indicates ulDestID contains large transport (16 bit) value
	UINT32 ulDestID;			// Destination ID of the target device
	unsigned char uchHopCount;		// Hop count
	unsigned char uchPad4;			// unused
	unsigned char uchPad5;			// unused
	unsigned char uchPad6;			// unused
	UINT32 ulOffset;			// Word-aligned (four byte boundary) offset - in bytes - of the CAR or CSR
	UINT32 ulWordCount;		// The number of words to be written
	UINT32 ulWordSize;				// number of bytes per word
	UINT32 ulStride;			// the number of bytes to increment ulOffset by with each write
	eRIOPRIORITY eRioXferPriority;	// priority level to use for the transfer
} FETMSG_CONFIGWRITESTRUCT;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Payload for a FET_HALCONFIGWRITERESP message.  Contains the results of the write transaction requested via
// the FET_HALCONFIGWRITE message.
// Server -> Client only
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct FETMSG_CONFIGWRITERESP
{
	FETMSG_CONFIGWRITESTRUCT sCmd;	// Contents of the request payload
	FETSTATUS eStatus;				// Success status
} FETMSG_CONFIGWRITERESPSTRUCT;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Payload for a FET_HALCONFIGRWMULTI message.  Performs configuration read and/or write transactions
// from CAR and/or CSR register(s) belonging to a local or remote RapidIO device.  Results are returned in a
// FET_HALCONFIGRWMULTIRESP message.
// Client -> Server only
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct CONFIGMULTICMD
{
	UINT32 bRead;			// 1=Config Read, 0=Config Write
	UINT32 ulOffset;			// Word-aligned (four byte boundary) offset - in bytes - of the CAR or CSR
} CONFIGMULTICMDSTRUCT;
#define MAXMULTICMDS	100
typedef struct FETMSG_CONFIGRWMULTI
{
	unsigned char uchLocalPort;		// Local Port Number
	unsigned char uchPad1;			// unused
	unsigned char uchPad2;			// unused
	unsigned char uchPad3;			// unused
	UINT32 ulDestID;			// Destination ID of the target device
	unsigned char uchHopCount;		// Hop count
	unsigned char uchPad4;			// unused
	unsigned char uchPad5;			// unused
	UINT32 ulCommandCount;	// the number of words to be read
	CONFIGMULTICMDSTRUCT asCmds[MAXMULTICMDS];
} FETMSG_CONFIGRWMULTISTRUCT;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Payload for a FET_HALCONFIGREADRESP message.  Contains the results of the read/write transactions requested via
// the FET_HALCONFIGRWMULTI message.
// Server -> Client only
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct CONFIGMULTIRESP
{
	FETSTATUS eStatus;				// Success status
	UINT32 ulOffset;			// Word-aligned (four byte boundary) offset - in bytes - of the CAR or CSR
} CONFIGMULTIRESPSTRUCT;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct FETMSG_CONFIGRWMULTIRESP
{
	FETMSG_CONFIGREADSTRUCT sCmd;	// Contents of the request payload
	CONFIGMULTIRESPSTRUCT asResp[MAXMULTICMDS];
} FETMSG_CONFIGRWMULTIRESPSTRUCT;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Payload for a FET_HALRIOREAD message.  Performs a data read transaction from memory belonging to a remote RapidIO 
// device.  Results are returned in a FET_HALRIOREADRESP message.
// Client -> Server only
typedef struct FETMSG_RIOREAD
{
	unsigned char uchLocalPort;		// Local Port Number
	unsigned char uchPad1;			// unused
	unsigned char uchPad2;			// unused
	unsigned char uchLongDestID;	// nonzero indicates ulDestID contains large transport (16 bit) value
	UINT32 ulDestID;			// Destination ID of the target device
	eRIOXFERTYPE eRioXferType;		// the type of transfer to be made
	eRIOPRIORITY eRioXferPriority;	// priority level to use for the transfer
	UINT32 ulRioAddressHi;	// upper bits of RIO address
	UINT32 ulRioAddressLo;	// lower bits of RIO address
	UINT32 ulByteCount;		// the number of words to be read
	UINT32 unused;
} FETMSG_RIOREADSTRUCT;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Payload for a FET_HALRIOREADRESP message.  Contains the results of the read transaction requested via
// the FET_HALRIOREAD message.
// Server -> Client only
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct FETMSG_RIOREADRESP
{
	FETMSG_RIOREADSTRUCT sCmd;	// Contents of the request payload
	FETSTATUS eStatus;				// Success status
} FETMSG_RIOREADRESPSTRUCT;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Payload for a FET_HALRIOWRITE message.  Performs a configuration write transaction to memory belonging to a remote 
// RapidIO device.  Results are returned in a FET_HALRIOWRITERESP message.
// Client -> Server only
typedef struct FETMSG_RIOWRITE
{
	unsigned char uchLocalPort;		// Local Port Number
	unsigned char uchPad1;			// unused
	unsigned char uchPad2;			// unused
	unsigned char uchLongDestID;	// nonzero indicates ulDestID contains large transport (16 bit) value
	UINT32 ulDestID;			// Destination ID of the target device
	eRIOXFERTYPE eRioXferType;		// the type of transfer to be made
	eRIOPRIORITY eRioXferPriority;	// priority level to use for the transfer
	UINT32 ulRioAddressHi;	// upper bits of RIO address
	UINT32 ulRioAddressLo;	// lower bits of RIO address
	UINT32 ulByteCount;		// The number of words to be written
	UINT32 unused;
} FETMSG_RIOWRITESTRUCT;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Payload for a FET_HALRIOWRITERESP message.  Contains the results of the write transaction requested via
// the FET_HALRIOWRITE message.
// Server -> Client only
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct FETMSG_RIOWRITERESP
{
	FETMSG_RIOWRITESTRUCT sCmd;	// Contents of the request payload
	FETSTATUS eStatus;				// Success status
} FETMSG_RIOWRITERESPSTRUCT;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Payload for a FET_HALRIOTXMESSAGE message.  Performs a message transaction to remote RapidIO device.  Results are 
// returned in a FET_HALRIOTXMESSAGERESP message.
// Client -> Server only
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct FETMSG_RIOTXMESSAGE
{
	unsigned char uchLocalPort;			// Local Port Number
	unsigned char uchPad1;				// unused
	unsigned char uchPad2;				// unused
	unsigned char uchLongDestID;		// nonzero indicates ulDestID contains large transport (16 bit) value
	UINT32 bMultiPacket;				// nonzero indicates multi-packet message request
	UINT32 ulDestID;					// Destination ID of the target device
	eRIOPRIORITY eRioXferPriority;		// priority level to use for the transfer
	UINT32 ulMailboxNum;				// mailbox number
	UINT32 ulPacketByteCount;			// number of bytes per message packet
	UINT32 ulTotalByteCount;			// total number of bytes to send via message packets
} FETMSG_RIOTXMESSAGESTRUCT;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Payload for a FET_HALRIOTXMESSAGERESP message.  Contains the results of the message transaction requested via
// the FET_HALRIOTXMESSAGE message.
// Server -> Client only
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct FETMSG_RIOTXMESSAGERESP
{
	FETMSG_RIOTXMESSAGESTRUCT sCmd;		// Contents of the request payload
	FETSTATUS eStatus;					// Success status
} FETMSG_RIOTXMESSAGERESPSTRUCT;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Payload for a FET_HALRIOTXDOORBELL message.  Performs a message transaction to remote RapidIO device.  Results are 
// returned in a FET_HALRIOTXDOORBELLRESP message.
// Client -> Server only
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct FETMSG_RIOTXDOORBELL
{
	unsigned char uchLocalPort;			// Local Port Number
	unsigned char uchPad1;				// unused
	unsigned char uchPad2;				// unused
	unsigned char uchLongDestID;		// nonzero indicates ulDestID contains large transport (16 bit) value
	UINT32 ulDestID;					// Destination ID of the target device
	eRIOPRIORITY eRioXferPriority;		// priority level to use for the transfer
	UINT16 wInfoWord;					// Information word contained within doorbell packet
	unsigned char uchPad3;				// unused
	unsigned char uchPad4;				// unused
} FETMSG_RIOTXDOORBELLSTRUCT;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Payload for a FET_HALRIOTXDOORBELLRESP message.  Contains the results of the message transaction requested via
// the FET_HALRIOTXDOORBELL message.
// Server -> Client only
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct FETMSG_RIOTXDOORBELLRESP
{
	FETMSG_RIOTXDOORBELLSTRUCT sCmd;	// Contents of the request payload
	FETSTATUS eStatus;					// Success status
} FETMSG_RIOTXDOORBELLRESPSTRUCT;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Payload for a FET_HALGETRIOERRORRESP message.  Consists of one of the header structures followed by "N" of the
// data structures.
// Server -> Client only
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct FETMSG_GETRIOERRORRESPHEADER
{
	UINT32 ulNumRioErrors;				// count of the number of errors structures to follow
	FETSTATUS eStatus;					// Success status
} FETMSG_GETRIOERRORRESPHEADERSTRUCT;
typedef struct FETMSG_GETRIOERRORRESPERROR
{
	UINT32 ulRioErrorValue;				// Hardware specific error info	
	char szRioErrorString[128];			// Human readable string describing the error
} FETMSG_GETRIOERRORRESPERRORSTRUCT;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Payload for a FET_HALRIORXMESSAGE message.  Retrieve a message from the host's inbound message queue.  Results are 
// returned in a FET_HALRIOMRXMESSAGERESP message.
// Client -> Server only
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define RIORXMESSAGE_CMD_GETMESSAGE	1
#define RIORXMESSAGE_CMD_FLUSH		2
typedef struct FETMSG_RIORXMESSAGE
{
	UINT32 ulWhichQueue;
	UINT32 ulCommand;				// 1=Retrieve a message, 2=Flush input buffer
} FETMSG_RIORXMESSAGESTRUCT;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Payload for a FET_HALRIORXMESSAGERESP message.  Contains the results of the message transaction requested via
// the FET_HALRIORXMESSAGE message.
// Server -> Client only
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct FETMSG_RIORXMESSAGERESP
{
	FETMSG_RIORXMESSAGESTRUCT sCmd;		// Contents of the request payload
	unsigned char uchOverflow;			// nonzero means there were dropped messages preceding this one
	unsigned char uchLargeTT;			// nonzero indicates ulSourceID contains large transport (16 bit) value
	unsigned char uchPad1;				// unused
	unsigned char uchPad2;				// unused
	UINT32 ulSourceID;					// Destination ID of the target device
	eRIOPRIORITY eRioXferPriority;		// priority level that the message was sent with
	UINT32 ulMailboxNum;				// mailbox number that the message was sent to
	UINT32 ulMessageLengthBytes;		// total number of message payload bytes to follow
	FETSTATUS eStatus;					// Success status
} FETMSG_RIORXMESSAGERESPSTRUCT;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Payload for a FET_HALRIORXDOORBELL message.  Retrieve a doorbell from the host's inbound doorbell queue.  Results are 
// returned in a FET_HALRIOMRXDOORBELLERESP message.
// Client -> Server only
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define RIORXDOORBELL_CMD_GETDOORBELL	1
#define RIORXDOORBELL_CMD_FLUSH			2
typedef struct FETMSG_RIORXDOORBELL
{
	UINT32 ulWhichQueue;
	UINT32 ulCommand;				// 1=Retrieve a doorbell, 2=Flush input buffer
} FETMSG_RIORXDOORBELLSTRUCT;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Payload for a FET_HALRIORXDOORBELLRESP message.  Contains the info word of the doorbell transaction requested via
// the FET_HALRIORXDOORBELL message.
// Server -> Client only
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct FETMSG_RIORXDOORBELLRESP
{
	FETMSG_RIORXDOORBELLSTRUCT sCmd;		// Contents of the request payload
	unsigned char uchOverflow;			// nonzero means there were dropped messages preceding this one
	unsigned char uchValidDoorbell;		// nonzero indicates wInfoWord contains valid data
	UINT16 wInfoWord;					// info word from the doorbell transaction
	FETSTATUS eStatus;					// Success status
} FETMSG_RIORXDOORBELLRESPSTRUCT;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Payload for a FET_MEMREAD message.  Performs a read of the local host PE's memory.
// Results are returned in a FET_MEMREADRESP message.
// Client -> Server only
typedef struct FETMSG_MEMREAD
{
	UINT32 ulAddressHi;			// upper bits memory address
	UINT32 ulAddressLo;			// lower bits of memory address
	UINT32 ulWordCount;			// The number of words to be written
	UINT32 ulWordSize;			// The number of bytes per word
	UINT32 unused;
} FETMSG_MEMREADSTRUCT;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Payload for a FET_MEMREADRESP message.  Contains the results of the memory read transaction requested via
// the FET_MEMREAD message.
// Server -> Client only
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct FETMSG_MEMREADRESP
{
	FETMSG_MEMREADSTRUCT sCmd;	// Contents of the request payload
	FETSTATUS eStatus;			// Success status
} FETMSG_MEMREADRESPSTRUCT;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Payload for a FET_MEMWRITE message.  Performs a write to the local host PE's memory
// Results are returned in a FET_MEMWRITERESP message.
// Client -> Server only
typedef struct FETMSG_MEMWRITE
{
	UINT32 ulAddressHi;			// upper bits of memory address
	UINT32 ulAddressLo;			// lower bits of memory address
	UINT32 ulWordCount;			// The number of words to be written
	UINT32 ulWordSize;			// The number of bytes per word
	UINT32 unused;
} FETMSG_MEMWRITESTRUCT;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Payload for a FET_HALMEMWRITERESP message.  Contains the results of the memory write transaction requested via
// the FET_HALMEMWRITE message.
// Server -> Client only
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct FETMSG_MEMWRITERESP
{
	FETMSG_MEMWRITESTRUCT sCmd;	// Contents of the request payload
	FETSTATUS eStatus;			// Success status
} FETMSG_MEMWRITERESPSTRUCT;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Payload for a FET_GETSERVERCAPSRESP message.  Contains server capability flags requested via
// the FET_GETSERVERCAP message.
// Server -> Client only
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct FETMSG_GETSERVERCAPSRESP
{
	UINT32	ulServerCaps1;		// unused flags must be set to zero
	UINT32	aulReserved[31];	// reserved for future use - must contain zeros
} FETMSG_GETSERVERCAPSRESPSTRUCT;

#define SERVERCAP_MEMREAD			1
#define SERVERCAP_MEMWRITE			2
#define SERVERCAP_LOCALTRAFFICGEN	4
#define SERVERCAP_REMOTETRAFFICGEN	8
#define SERVERCAP_DOORBELLTX		0x10
#define SERVERCAP_DOORBELLRX		0x20
#define SERVERCAP_MESSAGETX			0x40
#define SERVERCAP_MESSAGERX			0x80
#define SERVERCAP_FLASHREAD			0x100
#define SERVERCAP_FLASHWRITE		0x200

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Payload for a FET_FLASHREAD message.  Performs a read of the local host PE's flash memory.
// Results are returned in a FET_FLASHREADRESP message.
// Client -> Server only
typedef struct FETMSG_FLASHREAD
{
	UINT32 ulAddressHi;			// upper bits of flash memory address
	UINT32 ulAddressLo;			// lower bits of flash memory address
	UINT32 ulByteCount;			// The number of bytes to be written
	UINT32 unused;
} FETMSG_FLASHREADSTRUCT;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Payload for a FET_FLASHREADRESP message.  Contains the results of the flash read transaction requested via
// the FET_FLASHREAD message.
// Server -> Client only
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct FETMSG_FLASHREADRESP
{
	FETMSG_FLASHREADSTRUCT sCmd;	// Contents of the request payload
	FETSTATUS eStatus;			// Success status
} FETMSG_FLASHREADRESPSTRUCT;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Payload for a FET_FLASHWRITE message.  Performs a write to the local host PE's flash.
// Results are returned in a FET_FLASHWRITERESP message.
// Client -> Server only
typedef struct FETMSG_FLASHWRITE
{
	UINT32 ulAddressHi;			// upper bits of flash memory address
	UINT32 ulAddressLo;			// lower bits of flash memory address
	UINT32 ulByteCount;			// The number of bytes to be written
	UINT32 unused;
} FETMSG_FLASHWRITESTRUCT;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Payload for a FET_HALFLASHWRITERESP message.  Contains the results of the flash write transaction requested via
// the FET_HALFLASHWRITE message.
// Server -> Client only
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct FETMSG_FLASHWRITERESP
{
	FETMSG_FLASHWRITESTRUCT sCmd;	// Contents of the request payload
	FETSTATUS eStatus;			// Success status
} FETMSG_FLASHWRITERESPSTRUCT;

#endif


///////////////////////////////////////////////////////////////////////////////
// $RCSfile: feterr.h,v $
// $Date: 2008/06/13 18:29:37 $
// $Author: Gilmour $
// $Revision: 1.28 $
// $Source: /cvsrepo/FET/Common/include/feterr.h,v $
//
// Copyright (c) 2004-2007 Fabric Embedded Tools Corporation
//
///////////////////////////////////////////////////////////////////////////////

#ifndef feterr__H
#define feterr__H	

#ifdef _WIN32
#pragma once
#endif

typedef enum {
	eFET_SUCCESS					=0,		// no error
	eFET_ERR_CONNECT				=-1,	// failure by server to connect to client
	eFET_ERR_RECVHDR				=-2,	// failure attempting to recv a header from the client
	eFET_ERR_SENDCLOSERESP			=-3,	// failure attempting to send a FET_CLOSERESP message
	eFET_ERR_SOCKUDPRECV			=-4,	// failed to create UDP receive socket
	eFET_ERR_BINDUDPRECV			=-5,	// failed to bind UDP receive socket
	eFET_ERR_SOCKUDPSEND			=-6,	// failed to create UDP transmit socket
	eFET_ERR_BINDUDPSEND			=-7,	// failed to bind UDP transmit socket
	eFET_ERR_SOCKLISTEN				=-8,	// failed to create TCP listen socket
	eFET_ERR_BINDLISTEN				=-9,	// failed to bind TCP listen socket
	eFET_ERR_LISTEN					=-10,	// failure attempting to listen
	eFET_ERR_NOSOCKET				=-11,	// the FET socket was found to be invalid (not connected)
	eFET_ERR_SENDGNLP				=-12,	// failure attempting to send a FET_HALGETNUMLOCALPORTS message
	eFET_ERR_SENDTOCLIENT			=-13,	// failed to send data from server to client
	eFET_ERR_RECVGNLPR				=-14,	// failure attempting to receive a FET_HALGETNUMLOCALPORTSRESP message
	eFET_ERR_WRONGRESPONSE			=-15,	// received the wrong type of message from the server
	eFET_ERR_SENDCREAD				=-16,	// failure attempting to send a FET_HALCONFIGREAD message
	eFET_ERR_RECVCREAD				=-17,	// failure attempting to receive a FET_HALCONFIGREADRESP message
	eFET_ERR_RESPMISMATCH			=-18,	// response from server did not match request from client
	eFET_ERR_SENDCWRITE				=-19,	// failure attempting to send a FET_HALCONFIGWRITE message
	eFET_ERR_RECVCWRITE				=-20,	// failure attempting to receive a FET_HALCONFIGWRITERESP message
	eFET_ERR_WRONGSIZE				=-21,	// receieve a message having a wrong message size value
	eFET_ERR_RECVPLD				=-22,	// failure attempting to recv a payload from the client
	eFET_ERR_RIOOPEN				=-23,	// failed to open local RIO device
	eFET_ERR_RIOREAD				=-24,	// read failed on local RIO device
	eFET_ERR_RIOWRITE				=-25,	// write failed on local RIO device
	eFET_ERR_READNOMEM				=-26,	// malloc failed when creating buffer to hold config read data
	eFET_ERR_WRITENOMEM				=-27,	// malloc failed when creating buffer to hold config write data
	eFET_ERR_NOVALIDROUTE			=-28,	// there is no valid route to a PE using the current routing tables
	eFET_ERR_SENDDGCMD				=-29,	// failure attempting to send a FET_DATAGENCTRL message
	eFET_ERR_RECVDGCMND				=-30,	// failure attempting to receive a FET_DATAGENCTRLRESP message
	eFET_ERR_FLASH					=-31,	// a flash write operation failed - data read <> data written
	eFET_ERR_FLASHTIMEOUT			=-32,	// a flash write operation timed out
	eFET_ERR_FLASHNOTFOUND			=-33,	// no flash device found
	eFET_ERR_FLASHWRITEPROTECT		=-34,	// flash device is write protected
	eFET_ERR_SENDFI					=-35,	// failure attempting to send a FET_IDENTIFY message
	eFET_ERR_RECVFIR				=-36,	// failure attempting to receive a FET_IDENTIFYRESP message
	eFET_ERR_DEVICEUNKNOWN			=-37,	// unknown RIO device
	eFET_ERR_NOTCONNECTED			=-38,	// RapidFET is not connected to a host PE
	eFET_ERR_BADPARAM				=-39,	// an invalid parameter was specified
	eFET_ERR_AUTOMATION_MEMORY		=-40,	// a memory allocation error occured within RapidFET
	eFET_ERR_SENDRIOWRITE			=-41,	// failure attempting to send a FET_HALRIOWRITE message
	eFET_ERR_RECVRIOWRITE			=-42,	// failure attempting to receive a FET_HALRIOWRITERESP message
	eFET_ERR_SENDRIOREAD			=-43,	// failure attempting to send a FET_HALRIOREAD message
	eFET_ERR_RECVRIOREAD			=-44,	// failure attempting to receive a FET_HALRIOREADRESP message
	eFET_ERR_RIOTIMEOUT				=-45,	// timeout on RapidIO transaction
	eFET_ERR_TOOMANYSWITCHES		=-46,	// there are too many switches in the network
	eFET_ERR_NOTSUPPORTED			=-47,	// a requested operation is not supported for the specified type of PE
	eFET_ERR_SENDRIOMESSAGE			=-48,	// failure attempting to send a FET_HALMESSAGE message
	eFET_ERR_RECVRIOMESSAGE			=-49,	// failure attempting to receive a FET_HALMESSAGERESP message
	eFET_ERR_SENDRIODOORBELL		=-50,	// failure attempting to send a FET_HALDOORBELL message
	eFET_ERR_RECVRIODOORBELL		=-51,	// failure attempting to receive a FET_HALDOORBELLRESP message
	eFET_ERR_SENDGETRIOERROR		=-52,	// failure attempting to send a FET_HALGETRIOERROR message
	eFET_ERR_RECVGETRIOERROR		=-53,	// failure attempting to receive a FET_HALGETRIOERRORRESP message
	eFET_ERR_SENDMEMWRITE			=-54,	// failure attempting to send a FET_MEMWRITE message
	eFET_ERR_RECVMEMWRITE			=-55,	// failure attempting to receive a FET_MEMWRITERESP message
	eFET_ERR_SENDMEMREAD			=-56,	// failure attempting to send a FET_MEMREAD message
	eFET_ERR_RECVMEMREAD			=-57,	// failure attempting to receive a FET_MEMREADRESP message
	eFET_ERR_SENDFLASHWRITE			=-58,	// failure attempting to send a FET_FLASHWRITE message
	eFET_ERR_RECVFLASHWRITE			=-59,	// failure attempting to receive a FET_FLASHWRITERESP message
	eFET_ERR_SENDFLASHREAD			=-60,	// failure attempting to send a FET_FLASHREAD message
	eFET_ERR_RECVFLASHREAD			=-61,	// failure attempting to receive a FET_FLASHREADRESP message
	eFET_ERR_LINKREQFAILED			=-62,	// link status request failed
	eFET_ERR_RESETACKIDSFAILED		=-63,	// Unable to reset ACK IDs
	eFET_ERR_NOTAPROBE				=-64,	// Target is not a RapidFET Probe
	eFET_ERR_NOTOFF					=-65,	// Not off.
	eFET_ERR_NOTON					=-66,	// Not on.
	eFET_ERR_PROBECONFIGERROR		=-67,	// RapidFET Probe configuration error.
	eFET_ERR_SERVERCAPS				=-68,	// RapidFET Server does not support this operation.
	eFET_ERR_NOMEM					=-69,	// Not enough memory to complete operation
	eFET_ERR_USERABORT				=-70,	// User aborted the operation

	// RapidIO HAL Errors
	eRIO_ERR_NOSUPPORT				=-101,	// feature not supported yet
	eRIO_WARN_INCONSISTENT			=-102,	// Used by rioRouteGetEntry - indicates that the route port
											//  number returned is not the same for all ports
	eRIO_ERR_SLAVE					=-103,	// Another host has a higher priority
	eRIO_ERR_INVALID_PARAMETER		=-104,	// One or more input parameters had an invalid value
	eRIO_ERR_RIO					=-105,	// The RapidIO fabric returned a Response Packet with ERROR status reported
	eRIO_ERR_ACCESS					=-106,	// A device-specific hardware interface was unable to
											//  generate a maintenance transaction and reported an error
	eRIO_ERR_LOCK					=-107,	// Another host already acquired the specified PE
	eRIO_ERR_NO_DEVICE_SUPPORT		=-108,	// Device Access Routine does not provide service for this device
	eRIO_ERR_INSUFFICIENT_RESOURCES	=-109,	// Insufficient storage available in DAR private storage area
	eRIO_ERR_ROUTE_ERROR			=-110,	// Switch cannot suport requested routing
	eRIO_ERR_NO_SWITCH				=-111,	// Target device is not a switch
	eRIO_ERR_FEATURE_NOT_SUPPORTED	=-112,	// Target device is not capable of per-input-port routing

	// License Validation Errors
	eLIC_ERR_NOKEY					=-200,	// Software protection key not found
	eLIC_ERR_NOLIC					=-201,	// Software license not activated
	eLIC_ERR_NOTFETKEY				=-202,	// Key is not a FET key
	eLIC_ERR_EXPIRED				=-203,	// License has expired
	eLIC_ERR_ADJUSTEDTIME			=-204,	// Detected backwards adjustement of system clock
	eLIC_ERR_PLATFORMUNK			=-205,	// An unknown combination of RTOS/Processor was found

	// RapidFET Driver Errors
	eRFD_ERR_RIOOPEN				=-301,	// failed to configure the RapidIO Device
	eRFD_ERR						=-302,	// general, unspecified error
	eRFD_ERR_NOTOPEN				=-303,	// Driver is not open
	eRFD_ERR_INVALID				=-304,	// Invalid parameter detected
	eRFD_ERR_RIO					=-305,	// RapidIO error
	eRFD_ERR_NODG					=-306,	// Data Generation is not supported by this RFD
	eRFD_ERR_DGNOTINIT				=-307,	// Attempt to access Data Gen without initializing it first
	eRFD_ERR_DGPROCERR				=-308,	// Data Generator processing error detected
	eRFD_ERR_DGRIOMEM				=-309,	// RapidIO Memory error detected
	eRFD_ERR_RIOTIMEOUT				=-310,	// RapidIO Timeout detected

	// Data Generator Errors
	eDG_ERR_INVALIDREQUEST			=-400,	// An unknown DG request ID was received by RFS
	eDG_ERR_DGNOTREADY				=-401,	// Attempt to command a DG that is not ready to accept a command
	eDG_ERR_NOREMOTEDG				=-402,	// Attempt to command a remote DG that does not appear to be present
	eDG_ERR_WATCHDOGTIMEOUT			=-403,	// Watchdog timeout occurred in DG
	eDG_ERR_INCOMPATIBLE			=-404,	// The remote Data Generator is not compatible with this version of RapidFET

	// Automation Errors
	eAUTO_ERR_NODENOTFOUND			=-10000	// node not found

} FETSTATUS;


#endif

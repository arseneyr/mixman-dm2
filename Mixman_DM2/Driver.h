/*++

Module Name:

    driver.h

Abstract:

    This file contains the driver definitions.

Environment:

    Kernel-mode Driver Framework

--*/

#include <ntddk.h>
#include <wdf.h>
#include <usb.h>
#include <usbdlib.h>
#include <wdfusb.h>
#include <initguid.h>
#include <windef.h>
#include <ks.h>

#include "device.h"
#include "queue.h"
#include "trace.h"

EXTERN_C_START

//
// WDFDRIVER Events
//

#define DM2_POOL_TAG 'd2mD'

DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_UNLOAD MixmanDM2EvtDriverUnload;

EXTERN_C_END

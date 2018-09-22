/*++

Module Name:

    driver.h

Abstract:

    This file contains the driver definitions.

Environment:

    Kernel-mode Driver Framework

--*/

#ifndef DRIVER_H
#define DRIVER_H

#include <ntifs.h>
#include <wdf.h>
#include <usb.h>
#include <wdfusb.h>
#include <initguid.h>
#include <portcls.h>
#include <wdfminiport.h>
#define _NEW_DELETE_OPERATORS_
#include <stdunk.h>

EXTERN_C_START
#include <usbdlib.h>
EXTERN_C_END

#include "trace.h"

#pragma pack(push, 1)
typedef struct _DM2_MIDI_PACKET {
    UINT8 Status;
    UINT8 Data1;
    UINT8 Data2;
} DM2_MIDI_PACKET, *PDM2_MIDI_PACKET;
#pragma pack(pop)

typedef struct _DM2_DEVICE_CONTEXT {
    WDFDEVICE WdfDevice;
} DM2_DEVICE_CONTEXT, *PDM2_DEVICE_CONTEXT;

NTSTATUS
MixmanDM2AddDevice(
    PDRIVER_OBJECT  DriverObject,
    PDEVICE_OBJECT  PhysicalDeviceObject
);

PVOID operator new
(
    size_t          iSize,
    _When_((poolType & NonPagedPoolMustSucceed) != 0,
        __drv_reportError("Must succeed pool allocations are forbidden. "
            "Allocation failures cause a system crash"))
    POOL_TYPE       poolType
    );

PVOID operator new
(
    size_t          iSize,
    _When_((poolType & NonPagedPoolMustSucceed) != 0,
        __drv_reportError("Must succeed pool allocations are forbidden. "
            "Allocation failures cause a system crash"))
    POOL_TYPE       poolType,
    ULONG           tag
    );

/*++

Routine Description:

    Array delete() operator.

Arguments:

    pVoid -
        The memory to free.

Return Value:

    None

--*/
void
__cdecl
operator delete[](
    PVOID pVoid
    );

/*++

Routine Description:

    Sized delete() operator.

Arguments:

    pVoid -
        The memory to free.

    size -
        The size of the memory to free.

Return Value:

    None

--*/
void __cdecl operator delete
(
    void *pVoid,
    size_t /*size*/
    );

/*++

Routine Description:

    Sized delete[]() operator.

Arguments:

    pVoid -
        The memory to free.

    size -
        The size of the memory to free.

Return Value:

    None

--*/
void __cdecl operator delete[]
(
    void *pVoid,
    size_t /*size*/
    );

//
// WDFDRIVER Events
//

#define DM2_POOL_TAG 'd2mD'

EVT_WDF_DRIVER_UNLOAD MixmanDM2EvtDriverUnload;

#endif

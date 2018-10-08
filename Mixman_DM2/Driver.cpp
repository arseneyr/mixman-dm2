/*++

Module Name:

    driver.c

Abstract:

    This file contains the driver entry points and callbacks.

Environment:

    Kernel-mode Driver Framework

--*/

#include "driver.h"
#include "driver.tmh"
extern "C" DRIVER_INITIALIZE DriverEntry;

PVOID operator new
(
    size_t          iSize,
    _When_((poolType & NonPagedPoolMustSucceed) != 0,
        __drv_reportError("Must succeed pool allocations are forbidden. "
            "Allocation failures cause a system crash"))
    POOL_TYPE       poolType
    )
{
    PVOID result = ExAllocatePoolWithTag(poolType, iSize, 'wNCK');

    if (result) {
        RtlZeroMemory(result, iSize);
    }

    return result;
}

PVOID operator new
(
    size_t          iSize,
    _When_((poolType & NonPagedPoolMustSucceed) != 0,
        __drv_reportError("Must succeed pool allocations are forbidden. "
            "Allocation failures cause a system crash"))
    POOL_TYPE       poolType,
    ULONG           tag
    )
{
    PVOID result = ExAllocatePoolWithTag(poolType, iSize, tag);

    if (result) {
        RtlZeroMemory(result, iSize);
    }

    return result;
}

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
    )
{
    if (pVoid)
    {
        ExFreePool(pVoid);
    }
}

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
    )
{
    if (pVoid)
    {
        ExFreePool(pVoid);
    }
}

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
    )
{
    if (pVoid)
    {
        ExFreePool(pVoid);
    }
}

/*void __cdecl operator delete
(
    PVOID pVoid
    )
{
    if (pVoid) {
        ExFreePool(pVoid);
    }
}*/

extern "C"

static PDRIVER_UNLOAD originalUnload;

NTSTATUS
PnpDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
)
{
    PIO_STACK_LOCATION ioStack;

    ioStack = IoGetCurrentIrpStackLocation(Irp);

    if (ioStack->MinorFunction == IRP_MN_REMOVE_DEVICE) {
        MixmanDM2OnDeviceRemove(DeviceObject);
    }

    return PcDispatchIrp(DeviceObject, Irp);
}

VOID DriverUnload(
    PDRIVER_OBJECT Driver
)
{
    WPP_CLEANUP(Driver);
    WdfDriverMiniportUnload(WdfGetDriver());
    originalUnload(Driver);
}

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
)
/*++

Routine Description:
    DriverEntry initializes the driver and is the first routine called by the
    system after the driver is loaded. DriverEntry specifies the other entry
    points in the function driver, such as EvtDevice and DriverUnload.

Parameters Description:

    DriverObject - represents the instance of the function driver that is loaded
    into memory. DriverEntry must initialize members of DriverObject before it
    returns to the caller. DriverObject is allocated by the system before the
    driver is loaded, and it is released by the system after the system unloads
    the function driver from memory.

    RegistryPath - represents the driver specific path in the Registry.
    The function driver can use the path to store driver related data between
    reboots. The path does not store hardware instance specific data.

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise.

--*/
{
    WDF_DRIVER_CONFIG config;
    NTSTATUS status;

    //
    // Initialize WPP Tracing
    //
    WPP_INIT_TRACING(DriverObject, RegistryPath);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Entry");

    //
    // Register a cleanup callback so that we can call WPP_CLEANUP when
    // the framework driver object is deleted during driver unload.
    //

    WDF_DRIVER_CONFIG_INIT(&config,
        NULL
    );

    config.DriverInitFlags = WdfDriverInitNoDispatchOverride;

    status = WdfDriverCreate(DriverObject,
        RegistryPath,
        WDF_NO_OBJECT_ATTRIBUTES,
        &config,
        WDF_NO_HANDLE
    );

    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "WdfDriverCreate failed %!STATUS!", status);
        goto Cleanup;
    }

    status = PcInitializeAdapterDriver(DriverObject, RegistryPath, MixmanDM2AddDevice);
    if (!NT_SUCCESS(status)) {
        goto Cleanup;
    }

    DriverObject->MajorFunction[IRP_MJ_PNP] = PnpDispatch;
    originalUnload = DriverObject->DriverUnload;
    DriverObject->DriverUnload = DriverUnload;

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Exit");

Cleanup:
    if (!NT_SUCCESS(status)) {
        WPP_CLEANUP(DriverObject);
    }

    return status;
}
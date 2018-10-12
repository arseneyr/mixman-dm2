/*++

Module Name:

    queue.c

Abstract:

    This file contains the queue entry points and callbacks.

Environment:

    Kernel-mode Driver Framework

--*/

#include "driver.h"
#include <usbioctl.h>
#include <usb.h>
#include "queue.tmh"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, MixmanFilterQueueInitialize)
#endif

USB_DEVICE_DESCRIPTOR TARGET_DEVICE_DESCRIPTOR = {
    0x12,
    0x01,
    0x0110,
    0xff,
    0xff,
    0xff,
    0x08,
    0x0665,
    0x0301,
    0x0003,
    0x00,
    0x00,
    0x00,
    0x01
};

#pragma pack(push, 1)
typedef struct _MIXMAN_FILTER_CONFIGURATION_DESCRIPTOR {
    USB_CONFIGURATION_DESCRIPTOR ConfigDescriptor;
    USB_INTERFACE_DESCRIPTOR InterfaceDescriptor;
    USB_ENDPOINT_DESCRIPTOR EndpointDescriptors[2];
} MIXMAN_FILTER_CONFIGURATION_DESCRIPTOR, *PMIXMAN_FILTER_CONFIGURATION_DESCRIPTOR;
#pragma pack(pop)

MIXMAN_FILTER_CONFIGURATION_DESCRIPTOR TARGET_CONFIGURATION_DESCRIPTOR = {
    {   // Config descriptor
        0x09,   // bLength
        0x02,   // bDescriptorType
        0x0020, // wTotalLength
        0x01,   // bNumInterfaces
        0x01,   // bConfigurationValue
        0x00,   // iConfiguration
        0x80,   // bmAttributes
        0x00    // MaxPower
    },
    {   // Interface descriptor
        0x09,   // bLength
        0x04,   // bDescriptorType
        0x00,   // bInterfaceNumber
        0x00,   // bAlternateSetting
        0x02,   // bNumEndpoints
        0xff,   // bInterfaceClass
        0xff,   // bInterfaceSubClass
        0xff,   // bInterfaceProtocol
        0x00,   // iInterface
    },
    {
        {   // Endpoint Descriptor (In)
            0x07,   // bLength
            0x05,   // bDescriptorType
            0x81,   // bEndpointAddress
            0x03,   // bmAttributes
            0x08,   // wMaxPacketSize
            0x0a    // bInterval
        },
        {   // Endpoint Descriptor (Out)
            0x07,   // bLength
            0x05,   // bDescriptorType
            0x02,   // bEndpointAddress
            0x02,   // bmAttributes (INVALID)
            0x08,   // wMaxPacketSize
            0x00    // bInterval    (INVALID)
        }
    }
};

NTSTATUS
MixmanFilterQueueInitialize(
    _In_ WDFDEVICE Device
)
/*++

Routine Description:

     The I/O dispatch callbacks for the frameworks device object
     are configured in this function.

     A single default I/O Queue is configured for parallel request
     processing, and a driver context memory allocation is created
     to hold our structure QUEUE_CONTEXT.

Arguments:

    Device - Handle to a framework device object.

Return Value:

    VOID

--*/
{
    NTSTATUS status;
    WDF_IO_QUEUE_CONFIG queueConfig;

    PAGED_CODE();

    //
    // Configure a default queue so that requests that are not
    // configure-fowarded using WdfDeviceConfigureRequestDispatching to goto
    // other queues get dispatched here.
    //
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(
        &queueConfig,
        WdfIoQueueDispatchParallel
    );

    queueConfig.EvtIoInternalDeviceControl = MixmanFilterEvtIoDeviceControl;

    status = WdfIoQueueCreate(
        Device,
        &queueConfig,
        WDF_NO_OBJECT_ATTRIBUTES,
        WDF_NO_HANDLE
    );

    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_QUEUE, "WdfIoQueueCreate failed %!STATUS!", status);
        return status;
    }

    return status;
}

VOID
MixmanFilterOnDescriptorRequestCompletion(
    IN WDFREQUEST                  Request,
    IN WDFIOTARGET                 Target,
    PWDF_REQUEST_COMPLETION_PARAMS CompletionParams,
    IN WDFCONTEXT                  Context
)
{
    WDF_REQUEST_PARAMETERS parameters;
    PUSB_DEFAULT_PIPE_SETUP_PACKET setupPacket;
    SIZE_T comparizonSize;
    BOOLEAN isDeviceDescriptor;
    PVOID pipeHandle;
    PVOID transferBuffer;
    ULONG transferBufferLength;
    PURB urb;

    UNREFERENCED_PARAMETER(Target);
    UNREFERENCED_PARAMETER(Context);

    if (!NT_SUCCESS(CompletionParams->IoStatus.Status)) {
        goto Cleanup;
    }

    WDF_REQUEST_PARAMETERS_INIT(&parameters);
    WdfRequestGetParameters(Request, &parameters);
    urb = (PURB)parameters.Parameters.Others.Arg1;
    if (!urb) {
        goto Cleanup;
    }

    if (urb->UrbHeader.Function == URB_FUNCTION_CONTROL_TRANSFER_EX) {
        setupPacket = (PUSB_DEFAULT_PIPE_SETUP_PACKET)urb->UrbControlTransferEx.SetupPacket;
        transferBuffer = urb->UrbControlTransferEx.TransferBuffer;
        transferBufferLength = urb->UrbControlTransferEx.TransferBufferLength;
        pipeHandle = urb->UrbControlTransferEx.PipeHandle;
    }
    else if (urb->UrbHeader.Function == URB_FUNCTION_CONTROL_TRANSFER) {
        setupPacket = (PUSB_DEFAULT_PIPE_SETUP_PACKET)urb->UrbControlTransfer.SetupPacket;
        transferBuffer = urb->UrbControlTransfer.TransferBuffer;
        transferBufferLength = urb->UrbControlTransfer.TransferBufferLength;
        pipeHandle = urb->UrbControlTransfer.PipeHandle;
    }
    else {
        goto Cleanup;
    }

    if (setupPacket->wValue.W == USB_DESCRIPTOR_MAKE_TYPE_AND_INDEX(USB_DEVICE_DESCRIPTOR_TYPE, 0)) {
        isDeviceDescriptor = TRUE;
        comparizonSize = sizeof(TARGET_DEVICE_DESCRIPTOR);
    }
    else if (setupPacket->wValue.W == USB_DESCRIPTOR_MAKE_TYPE_AND_INDEX(USB_CONFIGURATION_DESCRIPTOR_TYPE, 0)) {
        isDeviceDescriptor = FALSE;
        comparizonSize = sizeof(TARGET_CONFIGURATION_DESCRIPTOR);
    }
    else {
        goto Cleanup;
    }

    if (transferBufferLength < comparizonSize) {
        goto Cleanup;
    }

    if (isDeviceDescriptor &&
        RtlCompareMemory(
            transferBuffer,
            &TARGET_DEVICE_DESCRIPTOR,
            comparizonSize) == comparizonSize) {
        //
        // We've found a DM2 device, so save the pipe handle
        // to match the configuration descriptor
        //
        ((PDEVICE_CONTEXT)Context)->PipeHandle = pipeHandle;
    }
    else if (!isDeviceDescriptor &&
        RtlCompareMemory(
            transferBuffer,
            &TARGET_CONFIGURATION_DESCRIPTOR,
            comparizonSize) == comparizonSize &&
        pipeHandle == ((PDEVICE_CONTEXT)Context)->PipeHandle) {
        ((PMIXMAN_FILTER_CONFIGURATION_DESCRIPTOR)transferBuffer)->EndpointDescriptors[1].bmAttributes = 0x03;
        ((PMIXMAN_FILTER_CONFIGURATION_DESCRIPTOR)transferBuffer)->EndpointDescriptors[1].bInterval = 0x0a;
    }
    else if (pipeHandle == ((PDEVICE_CONTEXT)Context)->PipeHandle) {
        //
        // A non-matching device used the same pipe handle. We don't
        // want to use it anymore for comparison purposes.
        //
        ((PDEVICE_CONTEXT)Context)->PipeHandle = NULL;
    }
    else {
        goto Cleanup;
    }

Cleanup:
    WdfRequestComplete(Request, CompletionParams->IoStatus.Status);
}

VOID
MixmanFilterEvtIoDeviceControl(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode
)
/*++

Routine Description:

    This event is invoked when the framework receives IRP_MJ_DEVICE_CONTROL request.

Arguments:

    Queue -  Handle to the framework queue object that is associated with the
             I/O request.

    Request - Handle to a framework request object.

    OutputBufferLength - Size of the output buffer in bytes

    InputBufferLength - Size of the input buffer in bytes

    IoControlCode - I/O control code.

Return Value:

    VOID

--*/
{
    NTSTATUS status;
    WDF_REQUEST_SEND_OPTIONS options;
    WDF_REQUEST_PARAMETERS parameters;
    PURB urb;
    PUSB_DEFAULT_PIPE_SETUP_PACKET setupPacket;

    TraceEvents(TRACE_LEVEL_INFORMATION,
        TRACE_QUEUE,
        "%!FUNC! Queue 0x%p, Request 0x%p OutputBufferLength %d InputBufferLength %d IoControlCode %d",
        Queue, Request, (int)OutputBufferLength, (int)InputBufferLength, IoControlCode);

    WDF_REQUEST_SEND_OPTIONS_INIT(&options, WDF_REQUEST_SEND_OPTION_SEND_AND_FORGET);

    WDF_REQUEST_PARAMETERS_INIT(&parameters);
    WdfRequestGetParameters(Request, &parameters);
    urb = (PURB)parameters.Parameters.Others.Arg1;

    WdfRequestFormatRequestUsingCurrentType(Request);

    if (parameters.Parameters.Others.IoControlCode == IOCTL_INTERNAL_USB_SUBMIT_URB &&
        (urb->UrbHeader.Function == URB_FUNCTION_CONTROL_TRANSFER_EX ||
            urb->UrbHeader.Function == URB_FUNCTION_CONTROL_TRANSFER) &&
            (urb->UrbControlTransfer.TransferFlags & USBD_TRANSFER_DIRECTION_IN))
    {
        if (urb->UrbHeader.Function == URB_FUNCTION_CONTROL_TRANSFER_EX) {
            setupPacket = (PUSB_DEFAULT_PIPE_SETUP_PACKET)urb->UrbControlTransferEx.SetupPacket;
        }
        else {
            setupPacket = (PUSB_DEFAULT_PIPE_SETUP_PACKET)urb->UrbControlTransfer.SetupPacket;
        }
        if (setupPacket->bmRequestType.Dir == BMREQUEST_DEVICE_TO_HOST &&
            setupPacket->bmRequestType.Type == BMREQUEST_STANDARD &&
            setupPacket->bmRequestType.Recipient == BMREQUEST_TO_DEVICE &&
            setupPacket->bRequest == USB_REQUEST_GET_DESCRIPTOR &&
            (setupPacket->wValue.W == USB_DESCRIPTOR_MAKE_TYPE_AND_INDEX(USB_DEVICE_DESCRIPTOR_TYPE, 0) ||
                setupPacket->wValue.W == USB_DESCRIPTOR_MAKE_TYPE_AND_INDEX(USB_CONFIGURATION_DESCRIPTOR_TYPE, 0))
            ) {
            WdfRequestSetCompletionRoutine(
                Request,
                MixmanFilterOnDescriptorRequestCompletion,
                DeviceGetContext(WdfIoQueueGetDevice(Queue)));

            options.Flags = 0;
        }
    }

    if (!WdfRequestSend(Request, WdfDeviceGetIoTarget(WdfIoQueueGetDevice(Queue)), &options)) {
        status = WdfRequestGetStatus(Request);
        KdPrint(("WdfRequestSend failed: 0x%x\n", status));
        WdfRequestComplete(Request, status);
    }
}
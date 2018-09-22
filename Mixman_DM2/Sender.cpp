#include "Sender.h"

void
MixmanDM2Sender::CompletionRoutine(
    WDFREQUEST Request,
    WDFIOTARGET Target,
    PWDF_REQUEST_COMPLETION_PARAMS Params,
    WDFCONTEXT Context
)
{
    UNREFERENCED_PARAMETER(Target);
    UNREFERENCED_PARAMETER(Params);
    UNREFERENCED_PARAMETER(Context);

    WdfObjectDelete(Request);
}

void
MixmanDM2Sender::Init(
    WDFUSBPIPE Pipe
)
{
    m_Pipe = Pipe;
}

NTSTATUS
MixmanDM2Sender::SetLeds(
    UINT8 Left,
    UINT8 Right
)
{
    NTSTATUS status;
    WDF_OBJECT_ATTRIBUTES attributes;
    WDFREQUEST request = NULL;
    WDFMEMORY memory;
    PUINT8 buffer;

    status = WdfRequestCreate(WDF_NO_OBJECT_ATTRIBUTES, WdfUsbTargetPipeGetIoTarget(m_Pipe), &request);
    if (!NT_SUCCESS(status)) {
        goto Cleanup;
    }

    WdfRequestSetCompletionRoutine(request, MixmanDM2Sender::CompletionRoutine, NULL);

    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = request;
    status = WdfMemoryCreate(&attributes, NonPagedPoolNx, DM2_POOL_TAG, DM2_SENDER_BUFFER_LENGTH, &memory, (PVOID*)&buffer);
    if (!NT_SUCCESS(status)) {
        goto Cleanup;
    }

    buffer[0] = ~Left;
    buffer[1] = ~Right;
    buffer[2] = 0xff;
    buffer[3] = 0xff;

    status = WdfUsbTargetPipeFormatRequestForWrite(m_Pipe, request, memory, NULL);
    if (!NT_SUCCESS(status)) {
        goto Cleanup;
    }

    if (!WdfRequestSend(request, WdfUsbTargetPipeGetIoTarget(m_Pipe), NULL)) {
        status = STATUS_IO_DEVICE_ERROR;
        goto Cleanup;
    }

    status = STATUS_SUCCESS;

Cleanup:
    if (!NT_SUCCESS(status)) {
        if (request != NULL) {
            WdfObjectDelete(request);
        }
    }
    else {
        m_Leds[0] = Left;
        m_Leds[1] = Right;
    }

    return status;
}

NTSTATUS
MixmanDM2Sender::SetSingleLed(
    UINT8 LedNumber,
    BOOLEAN TurnOn
)
{
    UINT8 ledCopy[2];
    if (LedNumber >= 16) {
        return STATUS_INVALID_PARAMETER;
    }

    RtlCopyMemory(ledCopy, m_Leds, sizeof(ledCopy));
    if (TurnOn) {
        *(PUINT16)ledCopy |= 1 << LedNumber;
    }
    else {
        *(PUINT16)ledCopy &= ~(1 << LedNumber);
    }

    return SetLeds(ledCopy[0], ledCopy[1]);
}
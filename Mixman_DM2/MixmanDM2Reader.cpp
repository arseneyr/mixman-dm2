#include "MixmanDM2Reader.h"

#pragma warning(disable: 100 101)

_When_((PoolType&NonPagedPoolMustSucceed) != 0,
    __drv_reportError("Must succeed pool allocations are forbidden. "
        "Allocation failures cause a system crash"))
    NTSTATUS CreateMixmanDM2Reader(
        PUNKNOWN        *Unknown,
        REFCLSID        ClassID,
        PUNKNOWN        UnknownOuter,
        POOL_TYPE       PoolType,
        CMiniportDM2    *Miniport
    )
{
    NTSTATUS ntStatus;

    UNREFERENCED_PARAMETER(ClassID);

    MixmanDM2Reader *p = new(PoolType, DM2_POOL_TAG) MixmanDM2Reader(UnknownOuter, Miniport);
    if (p) {
        *Unknown = PUNKNOWN((PMINIPORTMIDISTREAM)(p));
        (*Unknown)->AddRef();
        ntStatus = STATUS_SUCCESS;
    }
    else {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    return ntStatus;
}

void
MixmanDM2Reader::ReadCompletedCallback(
    WDFUSBPIPE Pipe,
    WDFMEMORY Buffer,
    size_t NumBytesTransferred,
    WDFCONTEXT Context
)
{
    UNREFERENCED_PARAMETER(Pipe);

    reinterpret_cast<MixmanDM2Reader*>(Context)->OnReadCompleted(Buffer, NumBytesTransferred);
}

void
MixmanDM2Reader::OnReadCompleted(
    WDFMEMORY Buffer,
    size_t NumBytesTransferred
)
{
    PDM2_DATA_FORMAT current;

    if (NumBytesTransferred != sizeof(DM2_DATA_FORMAT)) {
        return;
    }

    current = (PDM2_DATA_FORMAT)WdfMemoryGetBuffer(Buffer, NULL);

    if (RtlCompareMemory(current, &m_Previous, sizeof(*current)) == sizeof(*current)) {
        return;
    }

    m_RingBuffer.Lock();
    HandleButtons(current->Buttons);
    m_RingBuffer.Unlock();

    m_Miniport->Notify();
    m_Previous = *current;
}

void
MixmanDM2Reader::HandleButtons(
    UINT32 Current
)
{
    UINT32 buttonDiff;

    buttonDiff = Current ^ m_Previous.Buttons;
    for (UINT8 i = 0; i < 32; ++i) {
        if (buttonDiff & 1 << i) {
            m_RingBuffer.Insert({
                DM2_MIDI_BUTTON_STATUS,
                i,
                Current & 1 << 1 ? DM2_MIDI_NOTE_ON : DM2_MIDI_NOTE_OFF });
        }
    }
}

NTSTATUS
MixmanDM2Reader::Init(
    WDFUSBPIPE Pipe
)
{
    NTSTATUS status;
    WDF_USB_CONTINUOUS_READER_CONFIG config;

    WDF_USB_CONTINUOUS_READER_CONFIG_INIT(
        &config,
        MixmanDM2Reader::ReadCompletedCallback,
        this,
        16);

    status = WdfUsbTargetPipeConfigContinuousReader(Pipe, &config);
    if (NT_SUCCESS(status)) {
        m_IoTarget = WdfUsbTargetPipeGetIoTarget(Pipe);
    }

    return status;
}

MixmanDM2Reader::~MixmanDM2Reader()
{
}

STDMETHODIMP_(NTSTATUS)
MixmanDM2Reader::SetFormat(
    PKSDATAFORMAT DataFormat
)
{
    return STATUS_SUCCESS;
}

STDMETHODIMP_(NTSTATUS)
MixmanDM2Reader::SetState(
    KSSTATE State
)
{
    switch (State) {
    case KSSTATE_RUN:
        return WdfIoTargetStart(m_IoTarget);
    case KSSTATE_STOP:
        WdfIoTargetStop(m_IoTarget, WdfIoTargetCancelSentIo);
        break;
    }

    return STATUS_SUCCESS;
}

STDMETHODIMP_(NTSTATUS)
MixmanDM2Reader::Read(
    PVOID BufferAddress,
    ULONG BufferLength,
    PULONG BytesRead
)
{
    if (BufferLength < sizeof(DM2_MIDI_PACKET)) {
        __debugbreak();
        return STATUS_BUFFER_TOO_SMALL;
    }

    m_RingBuffer.Lock();
    if (NT_SUCCESS(m_RingBuffer.Remove((PDM2_MIDI_PACKET)BufferAddress))) {
        *BytesRead = sizeof(DM2_MIDI_PACKET);
    }
    else {
        *BytesRead = 0;
    }
    m_RingBuffer.Unlock();
    return STATUS_SUCCESS;
}

STDMETHODIMP_(NTSTATUS)
MixmanDM2Reader::Write(
    PVOID BufferAddress,
    ULONG BytesToWrite,
    PULONG BytesWritten
)
{
    return STATUS_INVALID_DEVICE_REQUEST;
}

NTSTATUS
MixmanDM2RingBuffer::Insert(
    DM2_MIDI_PACKET Packet
)
{
    if ((m_In + 1) % RING_SIZE == m_Out) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    m_Ring[m_In] = Packet;
    m_In = (m_In + 1) % RING_SIZE;
    return STATUS_SUCCESS;
}

NTSTATUS
MixmanDM2RingBuffer::Remove(
    PDM2_MIDI_PACKET Packet
)
{
    if (m_In == m_Out) {
        return STATUS_NO_MORE_ENTRIES;
    }

    *Packet = m_Ring[m_Out];
    m_Out = (m_Out + 1) % RING_SIZE;
    return STATUS_SUCCESS;
}

STDMETHODIMP_(NTSTATUS)
MixmanDM2Reader::NonDelegatingQueryInterface
(
    _In_ REFIID  Interface,
    _COM_Outptr_   PVOID * Object
)
{
    PAGED_CODE();

    ASSERT(Object);

    if (IsEqualGUIDAligned(Interface, IID_IUnknown))
    {
        *Object = PVOID(PUNKNOWN(PMINIPORT(this)));
    }
    else if (IsEqualGUIDAligned(Interface, IID_IMiniportMidiStream))
    {
        *Object = PVOID(PMINIPORTMIDISTREAM(this));
    }
    else
    {
        *Object = NULL;
    }

    if (*Object)
    {
        //
        // We reference the interface for the caller.
        //
        PUNKNOWN((PMINIPORTMIDISTREAM)*Object)->AddRef();
        return STATUS_SUCCESS;
    }

    return STATUS_INVALID_PARAMETER;
}
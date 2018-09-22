#include "MixmanDM2Reader.h"
#include "Device.h"

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

    MixmanDM2Stream *p = new(PoolType, DM2_POOL_TAG) MixmanDM2Stream(UnknownOuter, Miniport);
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

    if (NumBytesTransferred != sizeof(DM2_DATA_FORMAT) ||
        !m_FirstBufferSkipped) {
        //
        // First buffer is uninteresting
        //
        m_FirstBufferSkipped = TRUE;
        return;
    }

    current = (PDM2_DATA_FORMAT)WdfMemoryGetBuffer(Buffer, NULL);

    if (RtlCompareMemory(current, &m_Previous, sizeof(*current)) == sizeof(*current)) {
        return;
    }

    HandleButtons(current->Buttons);

    if (m_MidiPacketsCount) {
        m_Miniport->Notify(m_MidiPackets, m_MidiPacketsCount);
        m_MidiPacketsCount = 0;
    }

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
            AddMidiPacket({
                DM2_MIDI_BUTTON_STATUS,
                i,
                Current & 1 << i ? DM2_MIDI_NOTE_ON : DM2_MIDI_NOTE_OFF });
        }
    }
}

MixmanDM2Reader::~MixmanDM2Reader()
{
    if (m_IoTarget) {
        WdfIoTargetStop(m_IoTarget, WdfIoTargetCancelSentIo);
    }
}

NTSTATUS
MixmanDM2Reader::Init(WDFUSBPIPE InPipe)
{
    NTSTATUS status;
    WDF_USB_CONTINUOUS_READER_CONFIG config;
    WDFIOTARGET target;

    WDF_USB_CONTINUOUS_READER_CONFIG_INIT(
        &config,
        MixmanDM2Reader::ReadCompletedCallback,
        this,
        sizeof(DM2_DATA_FORMAT));

    status = WdfUsbTargetPipeConfigContinuousReader(InPipe, &config);
    if (!NT_SUCCESS(status)) {
        return status;
    }
    target = WdfUsbTargetPipeGetIoTarget(InPipe);
    status = WdfIoTargetStart(target);
    if (NT_SUCCESS(status)) {
        m_IoTarget = target;
    }

    return status;
}

MixmanDM2Stream::~MixmanDM2Stream() {
    m_Miniport->RemoveStream(this);
}

NTSTATUS
MixmanDM2Stream::AddPackets(
    PDM2_MIDI_PACKET Packets,
    UINT8 PacketCount,
    BOOLEAN AtDpc
)
{
    NTSTATUS status = STATUS_SUCCESS;

    if (m_State != KSSTATE_RUN) {
        return status;
    }

    m_RingBuffer.Lock(AtDpc);
    for (UINT8 i = 0; i < PacketCount; ++i) {
        status = m_RingBuffer.Insert(Packets + i);
        if (!NT_SUCCESS(status)) {
            break;
        }
    }
    m_RingBuffer.Unlock(AtDpc);
    return status;
}

STDMETHODIMP_(NTSTATUS)
MixmanDM2Stream::SetFormat(
    PKSDATAFORMAT DataFormat
)
{
    return STATUS_SUCCESS;
}

STDMETHODIMP_(NTSTATUS)
MixmanDM2Stream::SetState(
    KSSTATE State
)
{
    m_State = State;
    return STATUS_SUCCESS;
}

STDMETHODIMP_(NTSTATUS)
MixmanDM2Stream::Read(
    PVOID BufferAddress,
    ULONG BufferLength,
    PULONG BytesRead
)
{
    if (BufferLength < sizeof(DM2_MIDI_PACKET)) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    m_RingBuffer.Lock(FALSE);
    if (NT_SUCCESS(m_RingBuffer.Remove((PDM2_MIDI_PACKET)BufferAddress))) {
        *BytesRead = sizeof(DM2_MIDI_PACKET);
    }
    else {
        *BytesRead = 0;
    }

    m_RingBuffer.Unlock(FALSE);
    return STATUS_SUCCESS;
}

STDMETHODIMP_(NTSTATUS)
MixmanDM2Stream::Write(
    PVOID BufferAddress,
    ULONG BytesToWrite,
    PULONG BytesWritten
)
{
    return STATUS_INVALID_DEVICE_REQUEST;
}

NTSTATUS
MixmanDM2RingBuffer::Insert(
    PDM2_MIDI_PACKET Packet
)
{
    if ((m_In + 1) % RING_SIZE == m_Out) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    m_Ring[m_In] = *Packet;
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
MixmanDM2Stream::NonDelegatingQueryInterface
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
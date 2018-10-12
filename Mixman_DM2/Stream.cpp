#include "Stream.h"
#include "Miniport.h"

_When_((PoolType&NonPagedPoolMustSucceed) != 0,
    __drv_reportError("Must succeed pool allocations are forbidden. "
        "Allocation failures cause a system crash"))
    NTSTATUS CreateMiniportDM2Stream(
        PUNKNOWN        *Unknown,
        REFCLSID        ClassID,
        PUNKNOWN        UnknownOuter,
        POOL_TYPE       PoolType,
        CMiniportDM2    *Miniport,
        BOOLEAN         Capture
    )
{
    NTSTATUS ntStatus;

    UNREFERENCED_PARAMETER(ClassID);

    CMiniportDM2Stream *p = new(PoolType, DM2_POOL_TAG) CMiniportDM2Stream(UnknownOuter, Miniport, Capture);
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

CMiniportDM2Stream::~CMiniportDM2Stream() {
    m_Miniport->RemoveStream(this);
}

NTSTATUS
CMiniportDM2Stream::AddPackets(
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
CMiniportDM2Stream::SetFormat(
    PKSDATAFORMAT DataFormat
)
{
    UNREFERENCED_PARAMETER(DataFormat);

    return STATUS_SUCCESS;
}

STDMETHODIMP_(NTSTATUS)
CMiniportDM2Stream::SetState(
    KSSTATE State
)
{
    m_State = State;
    return STATUS_SUCCESS;
}

STDMETHODIMP_(NTSTATUS)
CMiniportDM2Stream::Read(
    PVOID BufferAddress,
    ULONG BufferLength,
    PULONG BytesRead
)
{
    if (!m_Capture) {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

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
CMiniportDM2Stream::Write(
    PVOID BufferAddress,
    ULONG BytesToWrite,
    PULONG BytesWritten
)
{
    PDM2_MIDI_PACKET packet;

    if (m_Capture) {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    *BytesWritten = BytesToWrite;
    if (BytesToWrite != sizeof(DM2_MIDI_PACKET)) {
        return STATUS_SUCCESS;
    }

    packet = (PDM2_MIDI_PACKET)BufferAddress;
    if (packet->Status == 0x90 &&
        packet->Data1 < 16 &&
        (packet->Data2 == 0x00 ||
            packet->Data2 == 0x7F)) {
        m_Miniport->SetSingleLed(packet->Data1, packet->Data2 == 0x7F);
    }

    return STATUS_SUCCESS;
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
CMiniportDM2Stream::NonDelegatingQueryInterface
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
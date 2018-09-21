#pragma once

#include "Driver.h"
#include "Device.h"

constexpr UINT8 DM2_MIDI_NOTE_ON = 0x7F;
constexpr UINT8 DM2_MIDI_NOTE_OFF = 0x00;
constexpr UINT8 DM2_MIDI_BUTTON_STATUS = 0x90;
constexpr UINT8 DM2_MIDI_STATUS_MASK = 0x80;

#pragma pack(push, 1)
typedef struct _DM2_DATA_FORMAT {
    UINT32 Buttons;
    UINT8 Reserved;
    UINT8 Sliders[3];
    UINT8 Wheels[2];
} DM2_DATA_FORMAT, *PDM2_DATA_FORMAT;

typedef struct _DM2_MIDI_PACKET {
    UINT8 Status;
    UINT8 Data1;
    UINT8 Data2;
} DM2_MIDI_PACKET, *PDM2_MIDI_PACKET;
#pragma pack(pop)

_When_((PoolType&NonPagedPoolMustSucceed) != 0,
    __drv_reportError("Must succeed pool allocations are forbidden. "
        "Allocation failures cause a system crash"))
    NTSTATUS CreateMixmanDM2Reader(
        PUNKNOWN        *Unknown,
        REFCLSID        ClassID,
        PUNKNOWN        UnknownOuter,
        POOL_TYPE       PoolType,
        CMiniportDM2    *Miniport,
        WDFUSBPIPE      InPipe
    );

class MixmanDM2RingBuffer {
private:
    static constexpr UINT8 RING_SIZE = 64;
    DM2_MIDI_PACKET m_Ring[RING_SIZE];
    UINT8 m_In;
    UINT8 m_Out;
    KSPIN_LOCK m_Lock;
    KIRQL m_Irql;

public:
    MixmanDM2RingBuffer() {
        KeInitializeSpinLock(&m_Lock);
    }

    void Lock() {
        NT_ASSERT(m_Irql == 0);

        KeAcquireSpinLock(&m_Lock, &m_Irql);
    }

    void Unlock() {
        NT_ASSERT(m_Irql != 0);

        KeReleaseSpinLock(&m_Lock, m_Irql);
        m_Irql = 0;
    }

    BOOLEAN IsEmpty() {
        return m_In == m_Out;
    }

    NTSTATUS Insert(DM2_MIDI_PACKET Packet);
    NTSTATUS Remove(PDM2_MIDI_PACKET Packet);
};

class MixmanDM2Reader
    : public IMiniportMidiStream,
    public CUnknown
{
private:

    CMiniportDM2 *m_Miniport;
    WDFUSBPIPE m_Pipe;
    WDFIOTARGET m_IoTarget;
    DM2_DATA_FORMAT m_Previous;
    MixmanDM2RingBuffer m_RingBuffer;
    BOOLEAN m_FirstBufferSkipped;

    static EVT_WDF_USB_READER_COMPLETION_ROUTINE ReadCompletedCallback;
    void OnReadCompleted(WDFMEMORY Buffer, size_t NumBytesTransferred);
    void HandleButtons(UINT32 Current);
public:
    DECLARE_STD_UNKNOWN();

    MixmanDM2Reader(PUNKNOWN OuterUnknown, CMiniportDM2 *Miniport, WDFUSBPIPE InPipe)
        : CUnknown(OuterUnknown), m_Miniport(Miniport), m_Pipe(InPipe) {}

    NTSTATUS Init();
    ~MixmanDM2Reader();

    // Inherited via IMiniportMidiStream
    STDMETHODIMP_(NTSTATUS) SetFormat(PKSDATAFORMAT DataFormat);
    STDMETHODIMP_(NTSTATUS) SetState(KSSTATE State);
    STDMETHODIMP_(NTSTATUS) Read(PVOID BufferAddress, ULONG BufferLength, PULONG BytesRead);
    STDMETHODIMP_(NTSTATUS) Write(PVOID BufferAddress, ULONG BytesToWrite, PULONG BytesWritten);
};

#pragma once

#include "Driver.h"

class CMiniportDM2;

constexpr UINT8 DM2_MIDI_NOTE_ON = 0x7F;
constexpr UINT8 DM2_MIDI_NOTE_OFF = 0x00;
constexpr UINT8 DM2_MIDI_BUTTON_STATUS = 0x90;
constexpr UINT8 DM2_MIDI_STATUS_MASK = 0x80;
constexpr UINT8 DM2_MIDI_MAX_BUTTONS = 32;
constexpr UINT8 DM2_MIDI_MAX_SLIDERS = 3;
constexpr UINT8 DM2_MIDI_MAX_WHEELS = 2;

_When_((PoolType&NonPagedPoolMustSucceed) != 0,
    __drv_reportError("Must succeed pool allocations are forbidden. "
        "Allocation failures cause a system crash"))
    NTSTATUS CreateMixmanDM2Reader(
        PUNKNOWN        *Unknown,
        REFCLSID        ClassID,
        PUNKNOWN        UnknownOuter,
        POOL_TYPE       PoolType,
        CMiniportDM2    *Miniport
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

    void Lock(BOOLEAN AtDpc) {
        if (AtDpc) {
            KeAcquireSpinLockAtDpcLevel(&m_Lock);
        }
        else {
            NT_ASSERT(m_Irql == 0);
            KeAcquireSpinLock(&m_Lock, &m_Irql);
        }
    }

    void Unlock(BOOLEAN AtDpc) {
        if (AtDpc) {
            KeReleaseSpinLockFromDpcLevel(&m_Lock);
        }
        else {
            NT_ASSERT(m_Irql != 0);

            KeReleaseSpinLock(&m_Lock, m_Irql);
            m_Irql = 0;
        }
    }

    BOOLEAN IsEmpty() {
        return m_In == m_Out;
    }

    NTSTATUS Insert(PDM2_MIDI_PACKET Packet);
    NTSTATUS Remove(PDM2_MIDI_PACKET Packet);
};

class MixmanDM2Reader
{
    CMiniportDM2 *m_Miniport;
    WDFIOTARGET m_IoTarget;
    DM2_DATA_FORMAT m_Previous;
    BOOLEAN m_FirstBufferSkipped;
    DM2_MIDI_PACKET m_MidiPackets[DM2_MIDI_MAX_BUTTONS + DM2_MIDI_MAX_SLIDERS + DM2_MIDI_MAX_WHEELS];
    UINT8 m_MidiPacketsCount;

    static EVT_WDF_USB_READER_COMPLETION_ROUTINE ReadCompletedCallback;
    void OnReadCompleted(WDFMEMORY Buffer, size_t NumBytesTransferred);
    void AddMidiPacket(DM2_MIDI_PACKET Packet) { m_MidiPackets[m_MidiPacketsCount++] = Packet; }
    void HandleButtons(UINT32 Current);

public:
    MixmanDM2Reader(CMiniportDM2 *Miniport)
        : m_Miniport(Miniport) {}

    ~MixmanDM2Reader();

    NTSTATUS Init(WDFUSBPIPE InPipe);
};

class MixmanDM2Stream
    : public IMiniportMidiStream,
    public CUnknown
{
private:
    MixmanDM2RingBuffer m_RingBuffer;
    KSSTATE m_State;
    CMiniportDM2 *m_Miniport;

public:
    DECLARE_STD_UNKNOWN();

    MixmanDM2Stream(PUNKNOWN OuterUnknown, CMiniportDM2 *Miniport)
        : CUnknown(OuterUnknown), m_Miniport(Miniport) {}

    ~MixmanDM2Stream();

    NTSTATUS AddPackets(PDM2_MIDI_PACKET Packets, UINT8 PacketCount, BOOLEAN AtDpc);

    // Inherited via IMiniportMidiStream
    STDMETHODIMP_(NTSTATUS) SetFormat(PKSDATAFORMAT DataFormat);
    STDMETHODIMP_(NTSTATUS) SetState(KSSTATE State);
    STDMETHODIMP_(NTSTATUS) Read(PVOID BufferAddress, ULONG BufferLength, PULONG BytesRead);
    STDMETHODIMP_(NTSTATUS) Write(PVOID BufferAddress, ULONG BytesToWrite, PULONG BytesWritten);
};

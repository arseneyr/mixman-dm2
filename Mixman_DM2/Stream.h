#pragma once
#include "Driver.h"

class CMiniportDM2;

_When_((PoolType&NonPagedPoolMustSucceed) != 0,
    __drv_reportError("Must succeed pool allocations are forbidden. "
        "Allocation failures cause a system crash"))
    NTSTATUS CreateMiniportDM2Stream(
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
    UINT8 m_In = 0;
    UINT8 m_Out = 0;
    KSPIN_LOCK m_Lock;
    KIRQL m_Irql = 0;

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

class CMiniportDM2Stream
    : public IMiniportMidiStream,
    public CUnknown
{
private:
    MixmanDM2RingBuffer m_RingBuffer;
    KSSTATE m_State;
    CMiniportDM2 *m_Miniport;

public:
    DECLARE_STD_UNKNOWN();

    CMiniportDM2Stream(PUNKNOWN OuterUnknown, CMiniportDM2 *Miniport)
        : CUnknown(OuterUnknown), m_Miniport(Miniport) {}

    ~CMiniportDM2Stream();

    NTSTATUS AddPackets(PDM2_MIDI_PACKET Packets, UINT8 PacketCount, BOOLEAN AtDpc);

    // Inherited via IMiniportMidiStream
    STDMETHODIMP_(NTSTATUS) SetFormat(PKSDATAFORMAT DataFormat);
    STDMETHODIMP_(NTSTATUS) SetState(KSSTATE State);
    STDMETHODIMP_(NTSTATUS) Read(PVOID BufferAddress, ULONG BufferLength, PULONG BytesRead);
    STDMETHODIMP_(NTSTATUS) Write(PVOID BufferAddress, ULONG BytesToWrite, PULONG BytesWritten);
};

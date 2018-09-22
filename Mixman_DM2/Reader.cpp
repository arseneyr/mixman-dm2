#include "Miniport.h"
#include "Reader.h"

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

    current = (PDM2_DATA_FORMAT)WdfMemoryGetBuffer(Buffer, NULL);

    if (NumBytesTransferred != sizeof(DM2_DATA_FORMAT)) {
        return;
    }

    if (RtlCompareMemory(current, &m_Previous, sizeof(*current)) == sizeof(*current)) {
        return;
    }

    HandleButtons(current->Buttons);
    for (UINT8 i = 0; i < DM2_MIDI_NUM_SLIDERS; ++i) {
        if (m_Sliders[i].Update(current->Sliders[i], m_MidiPackets + m_MidiPacketsCount)) {
            m_MidiPacketsCount += 1;
        }
    }

    for (UINT8 i = 0; i < DM2_MIDI_NUM_WHEELS; ++i) {
        if (current->Wheels[i] != m_Previous.Wheels[i]) {
            HandleWheel(DM2_MIDI_NUM_SLIDERS + i, current->Wheels[i]);
        }
    }

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

void MixmanDM2Reader::HandleWheel(UINT8 WheelNum, UINT8 Current)
{
    INT8 clampedCurrent = Current;

    clampedCurrent = (clampedCurrent > 64 ? 64 : clampedCurrent < -63 ? -63 : clampedCurrent);
    AddMidiPacket({ 0xB0, WheelNum, (UINT8)(64 - clampedCurrent) });
}

void
MixmanDM2Reader::Calibrate()
{
    NTSTATUS status;
    DM2_DATA_FORMAT buffer;
    WDF_MEMORY_DESCRIPTOR wdfMemory;
    ULONG bytesRead = 0;
    UINT8 led = 0;

    status = STATUS_SUCCESS;
    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&wdfMemory, &buffer, sizeof(buffer));

    for (UINT8 i = 0; i < 100; ++i) {
        if (i % 12 == 0) {
            led |= 0x80 >> (i / 12);
            m_Miniport->SetLeds(led, led);
        }

        status = WdfUsbTargetPipeReadSynchronously(m_Pipe, NULL, NULL, &wdfMemory, &bytesRead);
    }

    m_Miniport->SetLeds(0, 0);

    if (NT_SUCCESS(status) && bytesRead == sizeof(buffer)) {
        for (UINT8 i = 0; i < DM2_MIDI_NUM_SLIDERS; ++i) {
            m_Sliders[i].Reset(buffer.Sliders[i]);
        }
    }
}

NTSTATUS
MixmanDM2Reader::Init(WDFUSBPIPE InPipe)
{
    NTSTATUS status;
    WDF_USB_CONTINUOUS_READER_CONFIG config;

    WDF_USB_CONTINUOUS_READER_CONFIG_INIT(
        &config,
        MixmanDM2Reader::ReadCompletedCallback,
        this,
        sizeof(DM2_DATA_FORMAT));

    status = WdfUsbTargetPipeConfigContinuousReader(InPipe, &config);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    m_Pipe = InPipe;
    m_IoTarget = WdfUsbTargetPipeGetIoTarget(InPipe);
    Calibrate();
    return status;
}

void
MixmanDM2Reader::IncrementStreams()
{
    if (++m_ActiveStreams == 1) {
        WdfIoTargetStart(m_IoTarget);
    }
}

void
MixmanDM2Reader::DecrementStreams()
{
    if (--m_ActiveStreams == 0) {
        WdfIoTargetStop(m_IoTarget, WdfIoTargetCancelSentIo);
    }
}
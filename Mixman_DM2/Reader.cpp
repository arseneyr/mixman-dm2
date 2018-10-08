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
    UINT8 led = 0;

    current = (PDM2_DATA_FORMAT)WdfMemoryGetBuffer(Buffer, NULL);

    if (NumBytesTransferred != sizeof(DM2_DATA_FORMAT)) {
        return;
    }
    if (m_InitCounter > 0) {
        m_InitCounter -= 1;
        if (m_InitCounter == 0) {
            for (UINT8 i = 0; i < DM2_MIDI_NUM_SLIDERS; ++i) {
                m_Sliders[i].Reset(current->Sliders[i]);
            }

            m_Miniport->SetLeds(0, 0);
        }
        else if (m_InitCounter % 12 == 0) {
            led = 0xFF << ((m_InitCounter / 12) - 1);
            m_Miniport->SetLeds(led, led);
        }
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

NTSTATUS
MixmanDM2Reader::Init(WDFUSBPIPE InPipe)
{
    NTSTATUS status;
    WDF_USB_CONTINUOUS_READER_CONFIG config;

    m_Pipe = InPipe;

    WDF_USB_CONTINUOUS_READER_CONFIG_INIT(
        &config,
        MixmanDM2Reader::ReadCompletedCallback,
        this,
        sizeof(DM2_DATA_FORMAT));

    status = WdfUsbTargetPipeConfigContinuousReader(m_Pipe, &config);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    m_IoTarget = WdfUsbTargetPipeGetIoTarget(InPipe);
    WdfIoTargetStart(m_IoTarget);
    return status;
}

void MixmanDM2Reader::OnD0Exit()
{
    WdfIoTargetStop(m_IoTarget, WdfIoTargetCancelSentIo);
}
#pragma once

#include "Driver.h"
#include "Slider.h"

class CMiniportDM2;

constexpr UINT8 DM2_MIDI_NOTE_ON = 0x7F;
constexpr UINT8 DM2_MIDI_NOTE_OFF = 0x00;
constexpr UINT8 DM2_MIDI_BUTTON_STATUS = 0x90;
constexpr UINT8 DM2_MIDI_STATUS_MASK = 0x80;
constexpr UINT8 DM2_MIDI_NUM_BUTTONS = 32;
constexpr UINT8 DM2_MIDI_NUM_SLIDERS = 3;
constexpr UINT8 DM2_MIDI_NUM_WHEELS = 2;

#pragma pack(push, 1)
typedef struct _DM2_DATA_FORMAT {
    UINT32 Buttons;
    UINT8 Reserved;
    UINT8 Sliders[DM2_MIDI_NUM_SLIDERS];
    UINT8 Wheels[DM2_MIDI_NUM_WHEELS];
} DM2_DATA_FORMAT, *PDM2_DATA_FORMAT;
#pragma pack(pop)

class MixmanDM2Reader
{
    CMiniportDM2 *m_Miniport = NULL;
    WDFIOTARGET m_IoTarget = NULL;
    WDFUSBPIPE m_Pipe = NULL;
    DM2_DATA_FORMAT m_Previous = { 0 };
    DM2_MIDI_PACKET m_MidiPackets[DM2_MIDI_NUM_BUTTONS + DM2_MIDI_NUM_SLIDERS + DM2_MIDI_NUM_WHEELS];
    UINT8 m_MidiPacketsCount = 0;
    UINT32 m_ActiveStreams = 0;
    UINT32 m_InitCounter = 100;

    MixmanDM2Slider m_Sliders[DM2_MIDI_NUM_SLIDERS] = { {0, TRUE}, {1, FALSE}, {2, FALSE} };

    static EVT_WDF_USB_READER_COMPLETION_ROUTINE ReadCompletedCallback;
    void OnReadCompleted(WDFMEMORY Buffer, size_t NumBytesTransferred);
    void AddMidiPacket(DM2_MIDI_PACKET Packet) { m_MidiPackets[m_MidiPacketsCount++] = Packet; }
    void HandleButtons(UINT32 Current);
    void HandleWheel(UINT8 WheelNum, UINT8 Current);

public:
    MixmanDM2Reader(CMiniportDM2 *Miniport)
        : m_Miniport(Miniport) {}

    NTSTATUS Init(WDFUSBPIPE InPipe);
    void OnD0Exit();
};

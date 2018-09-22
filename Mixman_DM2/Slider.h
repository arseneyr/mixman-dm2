#pragma once
#include "Driver.h"

class MixmanDM2Slider
{
    UINT8 m_SliderNumber;

    BOOLEAN m_Invert;

    // 5 units on either side of the middle point
    UINT8 m_Deadzone = 5;

    UINT8 m_Mid;
    UINT8 m_Min;
    UINT8 m_Max;

    UINT8 m_LowerRangeSize = 1;
    UINT8 m_UpperRangeSize = 1;

    UINT8 m_Previous;
public:
    MixmanDM2Slider(UINT8 SliderNumber, BOOLEAN Invert)
        : m_SliderNumber(SliderNumber), m_Invert(Invert) {}

    void Reset(UINT8 Value);

    BOOLEAN Update(UINT8 Value, PDM2_MIDI_PACKET OutPacket);
};

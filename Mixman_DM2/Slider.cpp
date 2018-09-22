#include "Slider.h"

void
MixmanDM2Slider::Reset(
    UINT8 Value
)
{
    if (m_Invert) {
        Value = ~Value;
    }

    m_Mid = Value;
    m_Min = Value - m_Deadzone - 1;
    m_Max = Value + m_Deadzone + 1;

    // Midpoint is defined as 64
    m_Previous = 64;
}

BOOLEAN
MixmanDM2Slider::Update(
    UINT8 Value,
    PDM2_MIDI_PACKET OutPacket
)
{
    UINT8 output;

    if (m_Invert) {
        Value = ~Value;
    }

    if (Value < m_Min) {
        m_Min = Value;
        m_LowerRangeSize = m_Mid - m_Deadzone - m_Min;
    }
    else if (Value > m_Max) {
        m_Max = Value;
        m_UpperRangeSize = m_Max - m_Mid - m_Deadzone;
    }

    if (Value < m_Mid - m_Deadzone) {
        output = ((Value - m_Min) * 64) / m_LowerRangeSize;
        NT_ASSERT(output <= 64);
    }
    else if (Value > m_Mid + m_Deadzone) {
        output = (((Value - m_Mid - m_Deadzone) * 63) / m_UpperRangeSize) + 64;
        NT_ASSERT(output >= 64 && output <= 127);
    }
    else {
        output = 64;
    }

    if (output != m_Previous) {
        *OutPacket = { 0xB0, m_SliderNumber, output };
        m_Previous = output;
        return TRUE;
    }

    return FALSE;
}
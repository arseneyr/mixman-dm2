#pragma once
#include "Driver.h"

class MixmanDM2Sender
{
private:
    static constexpr UINT8 DM2_SENDER_BUFFER_LENGTH = 4;

    WDFUSBPIPE m_Pipe = NULL;
    UINT8 m_Leds[2] = { 0, 0 };

    static void CompletionRoutine(
        WDFREQUEST Request,
        WDFIOTARGET Target,
        PWDF_REQUEST_COMPLETION_PARAMS Params,
        WDFCONTEXT Context);

public:
    void Init(WDFUSBPIPE Pipe);
    NTSTATUS SetLeds(UINT8 Left, UINT8 Right);
    NTSTATUS SetSingleLed(UINT8 LedNumber, BOOLEAN TurnOn);
};

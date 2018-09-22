/*++

Module Name:

    device.h

Abstract:

    This file contains the device definitions.

Environment:

    Kernel-mode Driver Framework

--*/

#ifndef DEVICE_H
#define DEVICE_H

#include "Driver.h"
#include "Reader.h"
#include "Stream.h"
#include "Sender.h"

class CMiniportDM2Stream;

class CMiniportDM2
    : public IMiniportMidi,
    public IPowerNotify,
    public CUnknown
{
private:

    MixmanDM2Reader m_Reader{ this };
    MixmanDM2Sender m_Sender;
    WDFDEVICE       m_WdfDevice = NULL;
    WDFUSBDEVICE    m_UsbDevice = NULL;
    WDFUSBINTERFACE m_UsbInterface = NULL;
    PPORTMIDI       m_Port = NULL;
    PSERVICEGROUP   m_ServiceGroup = NULL;
    LIST_ENTRY      m_StreamListHead;
    KSPIN_LOCK      m_StreamListLock;

    static KSDATARANGE_MUSIC MidiDataRanges[];
    static PKSDATARANGE MidiDataRangePointers[];
    static KSDATARANGE BridgeDataRanges[];
    static PKSDATARANGE BridgeDataRangePointers[];
    static PCPIN_DESCRIPTOR PinDescriptors[];
    static PCNODE_DESCRIPTOR Nodes[];
    static PCCONNECTION_DESCRIPTOR Connections[];
    static GUID MiniportCategories[];
    static PCFILTER_DESCRIPTOR FilterDescriptor;

    typedef struct _STREAM_LIST_ENTRY {
        LIST_ENTRY ListEntry;
        CMiniportDM2Stream *Stream;
    } STREAM_LIST_ENTRY, *PSTREAM_LIST_ENTRY;

    NTSTATUS AddStream(CMiniportDM2Stream *Stream);
    NTSTATUS InitSender(WDFUSBPIPE OutPipe);

public:
    DECLARE_STD_UNKNOWN();

    CMiniportDM2(PUNKNOWN OuterUnknown, WDFDEVICE WdfDevice)
        :CUnknown(OuterUnknown)
    {
        InitializeListHead(&m_StreamListHead);
        KeInitializeSpinLock(&m_StreamListLock);
        m_WdfDevice = WdfDevice;
    }

    virtual ~CMiniportDM2();

    void RemoveStream(CMiniportDM2Stream *stream);
    void Notify(PDM2_MIDI_PACKET Packets, UINT8 PacketCount);
    NTSTATUS SetLeds(UINT8 Left, UINT8 Right);
    NTSTATUS SetSingleLed(UINT8 LedNumber, BOOLEAN TurnOn);

    // Inherited via IMiniportMidi
    STDMETHODIMP_(NTSTATUS) Init(PUNKNOWN UnknownAdapter, PRESOURCELIST ResourceList, PPORTMIDI Port, PSERVICEGROUP * ServiceGroup);
    STDMETHODIMP_(void) Service(void);
    STDMETHODIMP_(NTSTATUS) NewStream(PMINIPORTMIDISTREAM * Stream, PUNKNOWN OuterUnknown, POOL_TYPE PoolType, ULONG Pin, BOOLEAN Capture, PKSDATAFORMAT DataFormat, PSERVICEGROUP * ServiceGroup);
    STDMETHODIMP_(void) PowerChangeNotify(POWER_STATE PowerState);
    STDMETHODIMP_(NTSTATUS) GetDescription(PPCFILTER_DESCRIPTOR *Description);

    // Inherited via IPowerNotify
    STDMETHODIMP_(NTSTATUS) DataRangeIntersection(ULONG PinId, PKSDATARANGE DataRange, PKSDATARANGE MatchingDataRange, ULONG OutputBufferLength, PVOID ResultantFormat, PULONG ResultantFormatLength);
};

//
// The device context performs the same job as
// a WDM device extension in the driver frameworks
//
typedef struct _DEVICE_CONTEXT
{
    WDFUSBDEVICE UsbDevice;
    WDFUSBINTERFACE UsbInterface;
    WDFUSBPIPE InPipe;
    WDFUSBPIPE OutPipe;
} DEVICE_CONTEXT, *PDEVICE_CONTEXT;

NTSTATUS
MixmanDM2StartDevice(
    PDEVICE_OBJECT  DeviceObject,
    PIRP  Irp,
    PRESOURCELIST  ResourceList
);

#endif
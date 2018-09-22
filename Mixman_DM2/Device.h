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
#include "public.h"
#include "MixmanDM2Reader.h"

class MixmanDM2Stream;

class CMiniportDM2
    : public IMiniportMidi,
    public IPowerNotify,
    public CUnknown
{
private:

    MixmanDM2Reader m_Reader{ this };
    WDFDEVICE       m_WdfDevice;
    WDFUSBDEVICE    m_UsbDevice;
    WDFUSBINTERFACE m_UsbInterface;
    PPORTMIDI       m_Port;
    PSERVICEGROUP   m_ServiceGroup;
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
        MixmanDM2Stream *Stream;
    } STREAM_LIST_ENTRY, *PSTREAM_LIST_ENTRY;

    NTSTATUS AddStream(MixmanDM2Stream *Stream);

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

    void RemoveStream(MixmanDM2Stream *stream);
    void Notify(PDM2_MIDI_PACKET Packets, UINT8 PacketCount);

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

//
// Function to initialize the device's queues and callbacks
//
NTSTATUS
MixmanDM2CreateDevice(
    PKSDEVICE Device
);

//
// Function to select the device's USB configuration and get a WDFUSBDEVICE
// handle
//
NTSTATUS
MixmanDM2DispatchPnpStart(
    PKSDEVICE Device,
    PIRP Irp,
    PCM_RESOURCE_LIST TranslatedResourceList,
    PCM_RESOURCE_LIST UntranslatedResourceList
);

EVT_WDF_DEVICE_D0_ENTRY MixmanDM2EvtDeviceD0Entry;

#endif
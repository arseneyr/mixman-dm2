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
#include <wdfminiport.h>

class CMiniportDM2
    : public IMiniportMidi,
    public IPowerNotify,
    public CUnknown
{
private:
    friend class MixmanDM2Reader;

    PUNKNOWN        m_Reader;
    WDFDEVICE       m_WdfDevice;
    WDFUSBDEVICE    m_UsbDevice;
    WDFUSBINTERFACE m_UsbInterface;
    PPORTMIDI       m_Port;

    static KSDATARANGE_MUSIC MidiDataRanges[];
    static PKSDATARANGE MidiDataRangePointers[];
    static KSDATARANGE BridgeDataRanges[];
    static PKSDATARANGE BridgeDataRangePointers[];
    static PCPIN_DESCRIPTOR PinDescriptors[];
    static PCNODE_DESCRIPTOR Nodes[];
    static PCCONNECTION_DESCRIPTOR Connections[];
    static GUID MiniportCategories[];
    static PCFILTER_DESCRIPTOR FilterDescriptor;

    void Notify() { m_Port->Notify(NULL); }

public:
    DECLARE_STD_UNKNOWN();

    CMiniportDM2(PUNKNOWN OuterUnknown, WDFDEVICE WdfDevice)
        :CUnknown(OuterUnknown)
    {
        m_WdfDevice = WdfDevice;
    }

    virtual ~CMiniportDM2();

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

//
// This macro will generate an inline function called DeviceGetContext
// which will be used to get a pointer to the device context memory
// in a type safe manner.
//
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_CONTEXT, DeviceGetContext)

NTSTATUS
MixmanDM2AddDevice(
    PDRIVER_OBJECT  DriverObject,
    PDEVICE_OBJECT  PhysicalDeviceObject
);

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
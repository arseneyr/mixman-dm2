/*++

Module Name:

    device.c - Device handling events for example driver.

Abstract:

   This file contains the device entry points and callbacks.

Environment:

    Kernel-mode Driver Framework

--*/

#include "device.h"
#include "device.tmh"

#include "MixmanDM2Reader.h"

#pragma warning(disable: 100 101)

KSDATARANGE_MUSIC CMiniportDM2::MidiDataRanges[] = { {
        {
            sizeof(KSDATARANGE_MUSIC),
            0,
            0,
            0,
            STATICGUIDOF(KSDATAFORMAT_TYPE_MUSIC),
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_MIDI),
            STATICGUIDOF(KSDATAFORMAT_SPECIFIER_NONE)
        },
        STATICGUIDOF(KSMUSIC_TECHNOLOGY_PORT),
        0,
        0,
        0xFFFF
    } };

PKSDATARANGE CMiniportDM2::MidiDataRangePointers[] = {
    (PKSDATARANGE)&CMiniportDM2::MidiDataRanges[0]
};

KSDATARANGE CMiniportDM2::BridgeDataRanges[] = { {
            sizeof(KSDATARANGE),
            0,
            0,
            0,
            STATICGUIDOF(KSDATAFORMAT_TYPE_MUSIC),
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_MIDI_BUS),
            STATICGUIDOF(KSDATAFORMAT_SPECIFIER_NONE)
    } };

PKSDATARANGE CMiniportDM2::BridgeDataRangePointers[] = {
    (PKSDATARANGE)&CMiniportDM2::BridgeDataRanges[0]
};

PCPIN_DESCRIPTOR CMiniportDM2::PinDescriptors[]
{
    {
        1,1,1,  // InstanceCount
        NULL,   // AutomationTable
        {       // KsPinDescriptor
            0,                                          // InterfacesCount
            NULL,                                       // Interfaces
            0,                                          // MediumsCount
            NULL,                                       // Mediums
            SIZEOF_ARRAY(MidiDataRangePointers),                   // DataRangesCount
            MidiDataRangePointers,                                 // DataRanges
            KSPIN_DATAFLOW_OUT,                         // DataFlow
            KSPIN_COMMUNICATION_SINK,                   // Communication
            (GUID *)&KSCATEGORY_AUDIO,                  // Category
            NULL,                                       // Name
            0                                           // Reserved
        }
    },
    {
        0,0,0,  // InstanceCount
        NULL,   // AutomationTable
        {       // KsPinDescriptor
            0,                                          // InterfacesCount
            NULL,                                       // Interfaces
            0,                                          // MediumsCount
            NULL,                                       // Mediums
            SIZEOF_ARRAY(BridgeDataRangePointers),      // DataRangesCount
            BridgeDataRangePointers,                    // DataRanges
            KSPIN_DATAFLOW_IN,                          // DataFlow
            KSPIN_COMMUNICATION_NONE,                   // Communication
            (GUID *)&KSCATEGORY_AUDIO,                  // Category
            NULL,                                       // Name
            0                                           // Reserved
        }
    }
};

PCNODE_DESCRIPTOR CMiniportDM2::Nodes[] =
{
    {
        0,                      // Flags
        NULL,                   // AutomationTable
        &KSNODETYPE_SYNTHESIZER,// Type
        NULL                    // Name TODO: fill in with correct GUID
    }
};

PCCONNECTION_DESCRIPTOR CMiniportDM2::Connections[] = {
    {PCFILTER_NODE, 1, 0, 1},
    {0, 0, PCFILTER_NODE, 0}
};

GUID CMiniportDM2::MiniportCategories[] = {
    STATICGUIDOF(KSCATEGORY_AUDIO),
    STATICGUIDOF(KSCATEGORY_RENDER),
    STATICGUIDOF(KSCATEGORY_CAPTURE)
};

PCFILTER_DESCRIPTOR CMiniportDM2::FilterDescriptor = {
    0,
    NULL,
    sizeof(PCPIN_DESCRIPTOR),
    SIZEOF_ARRAY(PinDescriptors),
    PinDescriptors,
    sizeof(PCNODE_DESCRIPTOR),
    SIZEOF_ARRAY(Nodes),
    Nodes,
    SIZEOF_ARRAY(Connections),
    Connections,
    SIZEOF_ARRAY(MiniportCategories),
    MiniportCategories
};

_When_((PoolType&NonPagedPoolMustSucceed) != 0,
    __drv_reportError("Must succeed pool allocations are forbidden. "
        "Allocation failures cause a system crash"))
    NTSTATUS CreateMiniportDM2(
        PUNKNOWN    *Unknown,
        REFCLSID    ClassID,
        PUNKNOWN    UnknownOuter,
        POOL_TYPE   PoolType,
        WDFDEVICE   WdfDevice
    )
{
    NTSTATUS ntStatus;
    CMiniportDM2 *p = new(PoolType, DM2_POOL_TAG) CMiniportDM2(UnknownOuter, WdfDevice);
    if (p) {
        *Unknown = PUNKNOWN((PMINIPORTMIDI)(p));
        (*Unknown)->AddRef();
        ntStatus = STATUS_SUCCESS;
    }
    else {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    return ntStatus;
}

NTSTATUS
MixmanDM2AddDevice(
    PDRIVER_OBJECT  DriverObject,
    PDEVICE_OBJECT  PhysicalDeviceObject
)
{
    return PcAddAdapterDevice(DriverObject, PhysicalDeviceObject, MixmanDM2StartDevice, 1, 0);
}

NTSTATUS
MixmanDM2StartDevice(
    PDEVICE_OBJECT  DeviceObject,
    PIRP  Irp,
    PRESOURCELIST  ResourceList
)
/*++

Routine Description:

    Worker routine called to create a device and its software resources.

Arguments:

    DeviceInit - Pointer to an opaque init structure. Memory for this
                    structure will be freed by the framework when the WdfDeviceCreate
                    succeeds. So don't access the structure after that point.

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status;
    PDEVICE_OBJECT pdo;
    PDEVICE_OBJECT parentDevice = NULL;
    WDF_OBJECT_ATTRIBUTES   deviceAttributes;
    WDFDEVICE wdfDevice = NULL;
    PPORT port = NULL;
    PUNKNOWN miniport = NULL;

    PAGED_CODE();

    status = PcGetPhysicalDeviceObject(DeviceObject, &pdo);
    if (!NT_SUCCESS(status)) {
        goto Cleanup;
    }

    parentDevice = IoGetLowerDeviceObject(DeviceObject);
    if (parentDevice == NULL) {
        status = STATUS_INVALID_DEVICE_STATE;
        goto Cleanup;
    }

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&deviceAttributes, DEVICE_CONTEXT);

    status = WdfDeviceMiniportCreate(
        WdfGetDriver(),
        &deviceAttributes,
        DeviceObject,
        parentDevice,
        pdo,
        &wdfDevice);

    if (!NT_SUCCESS(status)) {
        goto Cleanup;
    }

    status = PcNewPort(&port, CLSID_PortMidi);
    if (!NT_SUCCESS(status)) {
        goto Cleanup;
    }

    status = CreateMiniportDM2(&miniport, CLSID_NULL, NULL, NonPagedPoolNx, wdfDevice);
    if (!NT_SUCCESS(status)) {
        goto Cleanup;
    }

    status = port->Init(DeviceObject, Irp, miniport, NULL, ResourceList);
    if (NT_SUCCESS(status)) {
        status = PcRegisterSubdevice(DeviceObject, L"mixmandm2", port);
    }

    miniport->Release();
    miniport = NULL;

Cleanup:
    if (!NT_SUCCESS(status)) {
        if (wdfDevice != NULL) {
            WdfObjectDelete(wdfDevice);
        }
    }

    if (port) {
        port->Release();
    }

    if (parentDevice) {
        ObDereferenceObject(parentDevice);
    }

    return status;
}

NTSTATUS
MixmanDM2EvtDeviceD0Entry(
    WDFDEVICE Device,
    WDF_POWER_DEVICE_STATE PreviousState
)
{
    NTSTATUS status;
    PDEVICE_CONTEXT deviceContext;
    WDFREQUEST request;
    WDF_MEMORY_DESCRIPTOR  memoryDescriptor;
    UINT8  buf[4] = { 0x00, 0x00, 0xff, 0xff };
    ULONG bytesWritten;

    UNREFERENCED_PARAMETER(PreviousState);

    deviceContext = DeviceGetContext(Device);

    WdfRequestCreate(WDF_NO_OBJECT_ATTRIBUTES, NULL, &request);
    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&memoryDescriptor,
        (PVOID)buf,
        sizeof(buf));

    status = WdfUsbTargetPipeWriteSynchronously(deviceContext->OutPipe, request, NULL, &memoryDescriptor, &bytesWritten);

    if (NT_SUCCESS(status)) {
        status = WdfRequestGetStatus(request);
    }

    return STATUS_SUCCESS;
}

CMiniportDM2::~CMiniportDM2()
{
    if (m_Port) {
        m_Port->Release();
    }
}

STDMETHODIMP_(NTSTATUS)
CMiniportDM2::Init(
    PUNKNOWN UnknownAdapter,
    PRESOURCELIST ResourceList,
    PPORTMIDI Port,
    PSERVICEGROUP * ServiceGroup
)
/*++

Routine Description:

    In this callback, the driver does whatever is necessary to make the
    hardware ready to use.  In the case of a USB device, this involves
    reading and selecting descriptors.

Arguments:

    Device - handle to a device

Return Value:

    NT status value

--*/
{
    NTSTATUS status;
    WDF_USB_DEVICE_CREATE_CONFIG createParams;
    WDF_USB_DEVICE_SELECT_CONFIG_PARAMS configParams;
    USHORT len;
    PUCHAR validationFailure;
    PUSB_CONFIGURATION_DESCRIPTOR configDescriptor = NULL;
    PUSB_INTERFACE_DESCRIPTOR interfaceDescriptor;
    PURB urb = NULL;
    USBD_INTERFACE_LIST_ENTRY interfaces[2] = { {0} };
    PUSB_ENDPOINT_DESCRIPTOR brokenEndpoint;

    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Entry");

    status = STATUS_SUCCESS;

    //
    // Create a USB device handle so that we can communicate with the
    // underlying USB stack. The WDFUSBDEVICE handle is used to query,
    // configure, and manage all aspects of the USB device.
    // These aspects include device properties, bus properties,
    // and I/O creation and synchronization. We only create the device the first time
    // PrepareHardware is called. If the device is restarted by pnp manager
    // for resource rebalance, we will use the same device handle but then select
    // the interfaces again because the USB stack could reconfigure the device on
    // restart.
    //
    if (m_UsbDevice == NULL) {
        //
        // Specifying a client contract version of 602 enables us to query for
        // and use the new capabilities of the USB driver stack for Windows 8.
        // It also implies that we conform to rules mentioned in MSDN
        // documentation for WdfUsbTargetDeviceCreateWithParameters.
        //
        WDF_USB_DEVICE_CREATE_CONFIG_INIT(&createParams,
            USBD_CLIENT_CONTRACT_VERSION_602
        );

        status = WdfUsbTargetDeviceCreateWithParameters(m_WdfDevice,
            &createParams,
            WDF_NO_OBJECT_ATTRIBUTES,
            &m_UsbDevice
        );

        if (!NT_SUCCESS(status)) {
            TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE,
                "WdfUsbTargetDeviceCreateWithParameters failed 0x%x", status);

            goto Cleanup;
        }
    }

    //
    // Select the first configuration of the device, using the first alternate
    // setting of each interface
    //

    status = WdfUsbTargetDeviceRetrieveConfigDescriptor(m_UsbDevice, NULL, &len);
    if (status != STATUS_BUFFER_TOO_SMALL) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE,
            "WdfUsbTargetDeviceRetrieveConfigDescriptor failed 0x%x", status);

        goto Cleanup;
    }

    configDescriptor = (PUSB_CONFIGURATION_DESCRIPTOR)ExAllocatePoolWithTag(PagedPool, len, DM2_POOL_TAG);
    if (configDescriptor == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    status = WdfUsbTargetDeviceRetrieveConfigDescriptor(m_UsbDevice, configDescriptor, &len);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE,
            "WdfUsbTargetDeviceRetrieveConfigDescriptor failed 0x%x", status);

        goto Cleanup;
    }

    if (USBD_ValidateConfigurationDescriptor(
        configDescriptor,
        len,
        3,
        &validationFailure,
        DM2_POOL_TAG) != USBD_STATUS_SUCCESS) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE,
            "USBD_ValidateConfigurationDescriptor failed at offest %x",
            (UINT32)(validationFailure - (PUCHAR)configDescriptor));

        status = STATUS_DEVICE_CONFIGURATION_ERROR;
        goto Cleanup;
    }

    interfaceDescriptor = USBD_ParseConfigurationDescriptorEx(
        configDescriptor,
        configDescriptor,
        0,      // Interface #0
        -1,     // Alterate modes don't matter
        0xff,   // Vendor-defined
        0xff,   // Vendor-defined
        0xff);  // Vendor-defined

    if (interfaceDescriptor == NULL ||
        interfaceDescriptor->bNumEndpoints != 2) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE,
            "USBD_ParseConfigurationDescriptorEx found no compatible interfaces");

        status = STATUS_DEVICE_CONFIGURATION_ERROR;
        goto Cleanup;
    }

    RtlZeroMemory(interfaces, sizeof(interfaces));
    interfaces[0].InterfaceDescriptor = interfaceDescriptor;

    // The config descriptor doesn't specify a power draw. Arbitrarily set to 98mA.
    configDescriptor->MaxPower = 49;

    // The second endpoint describes a bulk device, which is forbidden
    // in low speed devices. Here, we force the second endpoint (OUT)
    // to use interrupt mode.
    brokenEndpoint = (PUSB_ENDPOINT_DESCRIPTOR)(interfaceDescriptor + 1) + 1;
    if ((UINT_PTR)brokenEndpoint + sizeof(USB_ENDPOINT_DESCRIPTOR) <= (UINT_PTR)configDescriptor + len &&
        brokenEndpoint->bEndpointAddress == 0x02 &&     // OUT
        brokenEndpoint->bmAttributes == 0x02) {         // Bulk
        brokenEndpoint->bmAttributes = 0x03;            //interrupt
        brokenEndpoint->bInterval = 0x0a;               //interval between frames (10ms)
    }

    urb = USBD_CreateConfigurationRequestEx(configDescriptor, interfaces);
    if (urb == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    WDF_USB_DEVICE_SELECT_CONFIG_PARAMS_INIT_URB(&configParams, urb);
    status = WdfUsbTargetDeviceSelectConfig(m_UsbDevice,
        WDF_NO_OBJECT_ATTRIBUTES,
        &configParams
    );

    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE,
            "WdfUsbTargetDeviceSelectConfig failed 0x%x", status);

        goto Cleanup;
    }

    m_UsbInterface = WdfUsbTargetDeviceGetInterface(m_UsbDevice, 0);
    status = CreateMixmanDM2Reader(&m_Reader, CLSID_NULL, NULL, NonPagedPoolNx, this);
    reinterpret_cast<MixmanDM2Reader*>(m_Reader)->Init(WdfUsbInterfaceGetConfiguredPipe(m_UsbInterface, 0, NULL));

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Exit");

Cleanup:
    if (configDescriptor != NULL) {
        ExFreePoolWithTag(configDescriptor, DM2_POOL_TAG);
    }
    if (urb != NULL) {
        ExFreePoolWithTag(urb, DM2_POOL_TAG);
    }

    if (NT_SUCCESS(status)) {
        m_Port = Port;
        m_Port->AddRef();
    }

    return status;
}

STDMETHODIMP_(void)
CMiniportDM2::Service(void)
{
}

STDMETHODIMP_(NTSTATUS)
CMiniportDM2::NewStream(
    PMINIPORTMIDISTREAM * Stream,
    PUNKNOWN OuterUnknown,
    POOL_TYPE PoolType,
    ULONG Pin,
    BOOLEAN Capture,
    PKSDATAFORMAT DataFormat,
    PSERVICEGROUP * ServiceGroup
)
{
    if (Pin != 0 || !Capture) {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    return m_Reader->QueryInterface(IID_IMiniportMidiStream, (PVOID*)Stream);
}

STDMETHODIMP_(void)
CMiniportDM2::PowerChangeNotify(
    POWER_STATE PowerState
)
{
}

STDMETHODIMP_(NTSTATUS)
CMiniportDM2::GetDescription(
    PPCFILTER_DESCRIPTOR *Description
)
{
    *Description = &FilterDescriptor;
    return STATUS_SUCCESS;
}

STDMETHODIMP_(NTSTATUS)
CMiniportDM2::DataRangeIntersection(
    ULONG PinId,
    PKSDATARANGE DataRange,
    PKSDATARANGE MatchingDataRange,
    ULONG OutputBufferLength,
    PVOID ResultantFormat,
    PULONG ResultantFormatLength
)
{
    return STATUS_NOT_IMPLEMENTED;
}

STDMETHODIMP_(NTSTATUS)
CMiniportDM2::NonDelegatingQueryInterface
(
    _In_ REFIID  Interface,
    _COM_Outptr_   PVOID * Object
)
{
    PAGED_CODE();

    ASSERT(Object);

    if (IsEqualGUIDAligned(Interface, IID_IUnknown))
    {
        *Object = PVOID(PUNKNOWN(PMINIPORT(this)));
    }
    else if (IsEqualGUIDAligned(Interface, IID_IMiniport))
    {
        *Object = PVOID(PMINIPORT(this));
    }
    else if (IsEqualGUIDAligned(Interface, IID_IMiniportMidi))
    {
        *Object = PVOID(PMINIPORTMIDI(this));
    }
    else if (IsEqualGUIDAligned(Interface, IID_IPowerNotify))
    {
        *Object = PVOID(PPOWERNOTIFY(this));
    }
    else
    {
        *Object = NULL;
    }

    if (*Object)
    {
        //
        // We reference the interface for the caller.
        //
        PUNKNOWN((PMINIPORT)*Object)->AddRef();
        return STATUS_SUCCESS;
    }

    return STATUS_INVALID_PARAMETER;
}
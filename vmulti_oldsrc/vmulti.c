#include "vmulti.h"

//
// Globals
//



NTSTATUS
DriverEntry (
    __in PDRIVER_OBJECT  DriverObject,
    __in PUNICODE_STRING RegistryPath
    )
{
    NTSTATUS               status = STATUS_SUCCESS;
    WDF_DRIVER_CONFIG      config;
    WDF_OBJECT_ATTRIBUTES  attributes;


    WDF_DRIVER_CONFIG_INIT(&config, VMultiEvtDeviceAdd);

    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);

    //
    // Create a framework driver object to represent our driver.
    //

    status = WdfDriverCreate(DriverObject,
                             RegistryPath,
                             &attributes,      
                             &config,         
                             WDF_NO_HANDLE
                             );

    if (!NT_SUCCESS(status)) 
    {
    }

    return status;
}

NTSTATUS
VMultiEvtDeviceAdd(
    IN WDFDRIVER       Driver,
    IN PWDFDEVICE_INIT DeviceInit
    )
{
    NTSTATUS                      status = STATUS_SUCCESS;
    WDF_IO_QUEUE_CONFIG           queueConfig;
    WDF_OBJECT_ATTRIBUTES         attributes;
    WDFDEVICE                     device;
    WDFQUEUE                      queue;
    UCHAR                         minorFunction;
    PVMULTI_CONTEXT               devContext;

    UNREFERENCED_PARAMETER(Driver);

    PAGED_CODE();


    //
    // Tell framework this is a filter driver. Filter drivers by default are  
    // not power policy owners. This works well for this driver because
    // HIDclass driver is the power policy owner for HID minidrivers.
    //
    
    WdfFdoInitSetFilter(DeviceInit);

    //
    // Because we are a virtual device the root enumerator would just put null values 
    // in response to IRP_MN_QUERY_ID. Lets override that.
    //
    
    minorFunction = IRP_MN_QUERY_ID;

    status = WdfDeviceInitAssignWdmIrpPreprocessCallback(
                                             DeviceInit,
                                             VMultiEvtWdmPreprocessMnQueryId,
                                             IRP_MJ_PNP,
                                             &minorFunction, 
                                             1 
                                             ); 
    if (!NT_SUCCESS(status)) 
    {

        return status;
    }
    
    //
    // Setup the device context
    //


    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, VMULTI_CONTEXT);

    //
    // Create a framework device object.This call will in turn create
    // a WDM device object, attach to the lower stack, and set the
    // appropriate flags and attributes.
    //
    
    status = WdfDeviceCreate(&DeviceInit, &attributes, &device);

    if (!NT_SUCCESS(status)) 
    {

        return status;
    }

	WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS IdleSettings;

	WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS_INIT(&IdleSettings, IdleCannotWakeFromS0);
	IdleSettings.IdleTimeoutType = DriverManagedIdleTimeout;
	IdleSettings.IdleTimeout = 1000;
	IdleSettings.UserControlOfIdleSettings = IdleDoNotAllowUserControl;
	IdleSettings.ExcludeD3Cold = WdfTrue;
	IdleSettings.PowerUpIdleDeviceOnSystemWake = WdfTrue;
	IdleSettings.Enabled = WdfFalse;
	IdleSettings.DxState = PowerDeviceD0;

	WdfDeviceAssignS0IdleSettings(device, &IdleSettings);

    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&queueConfig, WdfIoQueueDispatchParallel);

    queueConfig.EvtIoInternalDeviceControl = VMultiEvtInternalDeviceControl;

    status = WdfIoQueueCreate(device,
                              &queueConfig,
                              WDF_NO_OBJECT_ATTRIBUTES,
                              &queue
                              );

    if (!NT_SUCCESS (status)) 
    {

        return status;
    }

    //
    // Create manual I/O queue to take care of hid report read requests
    //

    devContext = VMultiGetDeviceContext(device);

    WDF_IO_QUEUE_CONFIG_INIT(&queueConfig, WdfIoQueueDispatchManual);

    queueConfig.PowerManaged = WdfFalse;

    status = WdfIoQueueCreate(device,
            &queueConfig,
            WDF_NO_OBJECT_ATTRIBUTES,
            &devContext->ReportQueue
            );

    if (!NT_SUCCESS(status)) 
    {

        return status;
    }

    devContext->DeviceMode = DEVICE_MODE_MOUSE;

    return status;
}

NTSTATUS
VMultiEvtWdmPreprocessMnQueryId(
    WDFDEVICE Device,
    PIRP Irp
    )
{
    NTSTATUS            status;
    PIO_STACK_LOCATION  IrpStack, previousSp;
    PDEVICE_OBJECT      DeviceObject;
    PWCHAR              buffer;

    PAGED_CODE();

    //
    // Get a pointer to the current location in the Irp
    //

    IrpStack = IoGetCurrentIrpStackLocation (Irp);

    //
    // Get the device object
    //
    DeviceObject = WdfDeviceWdmGetDeviceObject(Device); 


    //
    // This check is required to filter out QUERY_IDs forwarded
    // by the HIDCLASS for the parent FDO. These IDs are sent
    // by PNP manager for the parent FDO if you root-enumerate this driver.
    //
    previousSp = ((PIO_STACK_LOCATION) ((UCHAR *) (IrpStack) +
                sizeof(IO_STACK_LOCATION)));

    if (previousSp->DeviceObject == DeviceObject) 
    {
        //
        // Filtering out this basically prevents the Found New Hardware
        // popup for the root-enumerated VMulti on reboot.
        //
        status = Irp->IoStatus.Status;
    }
    else
    {
        switch (IrpStack->Parameters.QueryId.IdType)
        {
            case BusQueryDeviceID:
            case BusQueryHardwareIDs:
                //
                // HIDClass is asking for child deviceid & hardwareids.
                // Let us just make up some id for our child device.
                //
                buffer = (PWCHAR)ExAllocatePoolWithTag(
                        NonPagedPool,
                        VMULTI_HARDWARE_IDS_LENGTH,
                        VMULTI_POOL_TAG
                        );

                if (buffer) 
                {
                    //
                    // Do the copy, store the buffer in the Irp
                    //
                    RtlCopyMemory(buffer,
                            VMULTI_HARDWARE_IDS,
                            VMULTI_HARDWARE_IDS_LENGTH
                            );

                    Irp->IoStatus.Information = (ULONG_PTR)buffer;
                    status = STATUS_SUCCESS;
                }
                else 
                {
                    //
                    //  No memory
                    //
                    status = STATUS_INSUFFICIENT_RESOURCES;
                }

                Irp->IoStatus.Status = status;
                //
                // We don't need to forward this to our bus. This query
                // is for our child so we should complete it right here.
                // fallthru.
                //
                IoCompleteRequest (Irp, IO_NO_INCREMENT);

                break;

            default:
                status = Irp->IoStatus.Status;
                IoCompleteRequest (Irp, IO_NO_INCREMENT);
                break;
        }
    }


    return status;
}

VOID
VMultiEvtInternalDeviceControl(
    IN WDFQUEUE     Queue,
    IN WDFREQUEST   Request,
    IN size_t       OutputBufferLength,
    IN size_t       InputBufferLength,
    IN ULONG        IoControlCode
    )
{
    NTSTATUS            status = STATUS_SUCCESS;
    WDFDEVICE           device;
    PVMULTI_CONTEXT     devContext;
    BOOLEAN             completeRequest = TRUE;

    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(InputBufferLength);

    device = WdfIoQueueGetDevice(Queue);
    devContext = VMultiGetDeviceContext(device);


    //
    // Please note that HIDCLASS provides the buffer in the Irp->UserBuffer
    // field irrespective of the ioctl buffer type. However, framework is very
    // strict about type checking. You cannot get Irp->UserBuffer by using
    // WdfRequestRetrieveOutputMemory if the ioctl is not a METHOD_NEITHER
    // internal ioctl. So depending on the ioctl code, we will either
    // use retreive function or escape to WDM to get the UserBuffer.
    //

    switch(IoControlCode) 
    {

    case IOCTL_HID_GET_DEVICE_DESCRIPTOR:
        //
        // Retrieves the device's HID descriptor.
        //
        status = VMultiGetHidDescriptor(device, Request);
        break;

    case IOCTL_HID_GET_DEVICE_ATTRIBUTES:
        //
        //Retrieves a device's attributes in a HID_DEVICE_ATTRIBUTES structure.
        //
        status = VMultiGetDeviceAttributes(Request);
        break;

    case IOCTL_HID_GET_REPORT_DESCRIPTOR:
        //
        //Obtains the report descriptor for the HID device.
        //
        status = VMultiGetReportDescriptor(device, Request);
        break;

    

    case IOCTL_HID_WRITE_REPORT:
    case IOCTL_HID_SET_OUTPUT_REPORT:
        //
        //Transmits a class driver-supplied report to the device.
        //
        status = VMultiWriteReport(devContext, Request);
        break;
 
    case IOCTL_HID_READ_REPORT:
    case IOCTL_HID_GET_INPUT_REPORT:
        //
        // Returns a report from the device into a class driver-supplied buffer.
        // 
        status = VMultiReadReport(devContext, Request, &completeRequest);
        break;

    case IOCTL_HID_SET_FEATURE:
        //
        // This sends a HID class feature report to a top-level collection of
        // a HID class device.
        //
 //       status = VMultiSetFeature(devContext, Request, &completeRequest);
 //       break;

    case IOCTL_HID_GET_FEATURE:
        //
        // returns a feature report associated with a top-level collection
        //
 //       status = VMultiGetFeature(devContext, Request, &completeRequest);
  //      break;

    case IOCTL_HID_ACTIVATE_DEVICE:
        //
        // Makes the device ready for I/O operations.
        //
    case IOCTL_HID_DEACTIVATE_DEVICE:
        //
        // Causes the device to cease operations and terminate all outstanding
        // I/O requests.
        //
    default:
        status = STATUS_NOT_SUPPORTED;
        break;
    }

    if (completeRequest)
    {
        WdfRequestComplete(Request, status);

    }
    else
    {

    }

    return;
}


NTSTATUS
VMultiGetHidDescriptor(
    IN WDFDEVICE Device,
    IN WDFREQUEST Request
    )
{
    NTSTATUS            status = STATUS_SUCCESS;
    size_t              bytesToCopy = 0;
    WDFMEMORY           memory;

    UNREFERENCED_PARAMETER(Device);


    //
    // This IOCTL is METHOD_NEITHER so WdfRequestRetrieveOutputMemory
    // will correctly retrieve buffer from Irp->UserBuffer. 
    // Remember that HIDCLASS provides the buffer in the Irp->UserBuffer
    // field irrespective of the ioctl buffer type. However, framework is very
    // strict about type checking. You cannot get Irp->UserBuffer by using
    // WdfRequestRetrieveOutputMemory if the ioctl is not a METHOD_NEITHER
    // internal ioctl.
    //
    status = WdfRequestRetrieveOutputMemory(Request, &memory);

    if (!NT_SUCCESS(status)) 
    {

        return status;
    }

    //
    // Use hardcoded "HID Descriptor" 
    //
    bytesToCopy = DefaultHidDescriptor.bLength;

    if (bytesToCopy == 0) 
    {
        status = STATUS_INVALID_DEVICE_STATE;


        return status;        
    }
    
    status = WdfMemoryCopyFromBuffer(memory,
                            0, // Offset
                            (PVOID) &DefaultHidDescriptor,
                            bytesToCopy);

    if (!NT_SUCCESS(status)) 
    {

        return status;
    }

    //
    // Report how many bytes were copied
    //
    WdfRequestSetInformation(Request, bytesToCopy);


    return status;
}

NTSTATUS
VMultiGetReportDescriptor(
    IN WDFDEVICE Device,
    IN WDFREQUEST Request
    )
{
    NTSTATUS            status = STATUS_SUCCESS;
    ULONG_PTR           bytesToCopy;
    WDFMEMORY           memory;

    UNREFERENCED_PARAMETER(Device);


    //
    // This IOCTL is METHOD_NEITHER so WdfRequestRetrieveOutputMemory
    // will correctly retrieve buffer from Irp->UserBuffer. 
    // Remember that HIDCLASS provides the buffer in the Irp->UserBuffer
    // field irrespective of the ioctl buffer type. However, framework is very
    // strict about type checking. You cannot get Irp->UserBuffer by using
    // WdfRequestRetrieveOutputMemory if the ioctl is not a METHOD_NEITHER
    // internal ioctl.
    //
    status = WdfRequestRetrieveOutputMemory(Request, &memory);
    if (!NT_SUCCESS(status)) 
    {

        return status;
    }

    //
    // Use hardcoded Report descriptor
    //
    bytesToCopy = DefaultHidDescriptor.DescriptorList[0].wReportLength;

    if (bytesToCopy == 0) 
    {
        status = STATUS_INVALID_DEVICE_STATE;


        return status;        
    }
    
    status = WdfMemoryCopyFromBuffer(memory,
                            0,
                            (PVOID) DefaultReportDescriptor,
                            bytesToCopy);
    if (!NT_SUCCESS(status)) 
    {

        return status;
    }

    //
    // Report how many bytes were copied
    //
    WdfRequestSetInformation(Request, bytesToCopy);


    return status;
}


NTSTATUS
VMultiGetDeviceAttributes(
    IN WDFREQUEST Request
    )
{
    NTSTATUS                 status = STATUS_SUCCESS;
    PHID_DEVICE_ATTRIBUTES   deviceAttributes = NULL;


    //
    // This IOCTL is METHOD_NEITHER so WdfRequestRetrieveOutputMemory
    // will correctly retrieve buffer from Irp->UserBuffer. 
    // Remember that HIDCLASS provides the buffer in the Irp->UserBuffer
    // field irrespective of the ioctl buffer type. However, framework is very
    // strict about type checking. You cannot get Irp->UserBuffer by using
    // WdfRequestRetrieveOutputMemory if the ioctl is not a METHOD_NEITHER
    // internal ioctl.
    //
    status = WdfRequestRetrieveOutputBuffer(Request,
                                            sizeof (HID_DEVICE_ATTRIBUTES),
                                            &deviceAttributes,
                                            NULL);
    if (!NT_SUCCESS(status)) 
    {

        return status;
    }

    //
    // Set USB device descriptor
    //

    deviceAttributes->Size = sizeof (HID_DEVICE_ATTRIBUTES);
    deviceAttributes->VendorID = VMULTI_VID;
    deviceAttributes->ProductID = VMULTI_PID;
    deviceAttributes->VersionNumber = VMULTI_VERSION;

    //
    // Report how many bytes were copied
    //
    WdfRequestSetInformation(Request, sizeof (HID_DEVICE_ATTRIBUTES));


    return status;
}

NTSTATUS
VMultiGetString(
    IN WDFREQUEST Request
    )
{
 
    NTSTATUS status = STATUS_SUCCESS;
    PWSTR pwstrID;
    size_t lenID;
    WDF_REQUEST_PARAMETERS params;
    void *pStringBuffer = NULL;

 
    WDF_REQUEST_PARAMETERS_INIT(&params);
    WdfRequestGetParameters(Request, &params);

    switch ((ULONG_PTR)params.Parameters.DeviceIoControl.Type3InputBuffer & 0xFFFF)
    {
        case HID_STRING_ID_IMANUFACTURER:
            pwstrID = L"DJP Inc.\0";
            break;

        case HID_STRING_ID_IPRODUCT:
            pwstrID = L"Virtual Multitouch Device\0";
            break;

        case HID_STRING_ID_ISERIALNUMBER:
            pwstrID = L"123123123\0";
            break;

        default:
            pwstrID = NULL;
            break;
    }

    lenID = pwstrID ? wcslen(pwstrID)*sizeof(WCHAR) + sizeof(UNICODE_NULL): 0;

    if(pwstrID == NULL)
    {


        status = STATUS_INVALID_PARAMETER;

        return status;
    }

    status = WdfRequestRetrieveOutputBuffer(Request,
                                            lenID,
                                            &pStringBuffer,
                                            &lenID);

    if(!NT_SUCCESS( status)) 
    {

        return status;
    }

    RtlCopyMemory(pStringBuffer, pwstrID, lenID);

    WdfRequestSetInformation(Request, lenID);

 
    return status;
}

NTSTATUS
VMultiWriteReport(
	IN PVMULTI_CONTEXT DevContext,
	IN WDFREQUEST Request

)

{
	NTSTATUS status = STATUS_SUCCESS;
	WDF_REQUEST_PARAMETERS params;
	PHID_XFER_PACKET transferPacket = NULL;
	size_t bytesWritten = 0;


	WDF_REQUEST_PARAMETERS_INIT(&params);
	WdfRequestGetParameters(Request, &params);
	transferPacket = (PHID_XFER_PACKET)WdfRequestWdmGetIrp(Request)->UserBuffer;

	WDFREQUEST reqRead;
	PVOID pReadReport = NULL;



	status = WdfIoQueueRetrieveNextRequest(DevContext->ReportQueue,
		&reqRead);


	if (NT_SUCCESS(status))

	{
		status = WdfRequestRetrieveOutputBuffer(reqRead,
			0x06,
			&pReadReport,
			NULL);
		RtlCopyMemory(pReadReport,
			transferPacket->reportBuffer + 0x02,
			0x06);
		WdfRequestCompleteWithInformation(reqRead,
			status,
			0x06);
		// Return the number of bytes written for the write request completion
		//
	}
	if (NT_SUCCESS(status))

	{
		WdfRequestSetInformation(Request, bytesWritten);
	}
	return status;

}


NTSTATUS
VMultiReadReport(
	IN PVMULTI_CONTEXT DevContext,
	IN WDFREQUEST Request,
	OUT BOOLEAN* CompleteRequest
)
{
	NTSTATUS status;

	// Forward the request to the manual queue
	status = WdfRequestForwardToIoQueue(Request, DevContext->ReportQueue);

	// If status is success (0), CompleteRequest becomes FALSE (0).
	// If status is an error (negative), CompleteRequest remains untouched or 
	// we can set it based on the failure. 
	// To strictly replicate your logic branchlessly:
	*CompleteRequest = !NT_SUCCESS(status);

	return status;
}

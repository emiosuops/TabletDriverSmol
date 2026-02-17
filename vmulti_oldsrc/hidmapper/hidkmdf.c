void __security_init_cookie();
void __security_init_cookie() {
};

#include <wdm.h>
#pragma warning(disable:4201)  // suppress nameless struct/union warning
#pragma warning(disable:4214)  // suppress bit field types other than int warning
#include <hidport.h>
#pragma warning(default:4201)  // suppress nameless struct/union warning
#pragma warning(default:4214)  // suppress bit field types other than int warning

#define GET_NEXT_DEVICE_OBJECT(DO) \
    (((PHID_DEVICE_EXTENSION)(DO)->DeviceExtension)->NextDeviceObject)

DRIVER_INITIALIZE   DriverEntry;
DRIVER_ADD_DEVICE   HidKmdfAddDevice;
__drv_dispatchType_other
DRIVER_DISPATCH     HidKmdfPassThrough;
__drv_dispatchType(IRP_MJ_POWER)
DRIVER_DISPATCH     HidKmdfPowerPassThrough;
DRIVER_UNLOAD       HidKmdfUnload;


NTSTATUS
DriverEntry (
    __in PDRIVER_OBJECT  DriverObject,
    __in PUNICODE_STRING RegistryPath
    )
{
    HID_MINIDRIVER_REGISTRATION hidMinidriverRegistration;
    NTSTATUS status;
    ULONG i;

    for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++) {
        DriverObject->MajorFunction[i] = HidKmdfPassThrough;
    }
 
    DriverObject->MajorFunction[IRP_MJ_POWER] = HidKmdfPowerPassThrough;

    DriverObject->DriverExtension->AddDevice = HidKmdfAddDevice;
    DriverObject->DriverUnload = HidKmdfUnload;

    RtlZeroMemory(&hidMinidriverRegistration,
                  sizeof(hidMinidriverRegistration));

    hidMinidriverRegistration.Revision            = HID_REVISION;
    hidMinidriverRegistration.DriverObject        = DriverObject;
    hidMinidriverRegistration.RegistryPath        = RegistryPath;
    hidMinidriverRegistration.DeviceExtensionSize = 0;

    hidMinidriverRegistration.DevicesArePolled = FALSE;

    status = HidRegisterMinidriver(&hidMinidriverRegistration);

    return status;
}


NTSTATUS
HidKmdfAddDevice(
    __in PDRIVER_OBJECT DriverObject,
    __in PDEVICE_OBJECT FunctionalDeviceObject
    )
{
    UNREFERENCED_PARAMETER(DriverObject);
    FunctionalDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
    return STATUS_SUCCESS;
}

NTSTATUS
HidKmdfPassThrough(
    __in PDEVICE_OBJECT DeviceObject,
    __in PIRP Irp
    )

{
    IoCopyCurrentIrpStackLocationToNext(Irp);
    return IoCallDriver(GET_NEXT_DEVICE_OBJECT(DeviceObject), Irp);
}


NTSTATUS
HidKmdfPowerPassThrough(
    __in PDEVICE_OBJECT DeviceObject,
    __in PIRP Irp
    )
{
    PoStartNextPowerIrp(Irp);

    IoCopyCurrentIrpStackLocationToNext(Irp);
    return PoCallDriver(GET_NEXT_DEVICE_OBJECT(DeviceObject), Irp);
}


VOID
HidKmdfUnload(
    __in PDRIVER_OBJECT DriverObject
    )
{
    UNREFERENCED_PARAMETER(DriverObject);
    return;
}


#include <windows.h>
#include <setupapi.h>
#include "hiddll.h"
#include <hidclass.h>


#ifndef BOOL
typedef int BOOL;
#endif

/////////
BOOL SetOutputReportRaw(
HANDLE device,
const void* report,
DWORD reportLength
) {
    DWORD bytesReturned;

    return DeviceIoControl(
        device,
        IOCTL_HID_SET_OUTPUT_REPORT,
        (LPVOID)report,
        reportLength,
        NULL,
        0,
        &bytesReturned,
        NULL
    );
}

BOOLEAN __stdcall
Hid_GetAttributes (
    IN  HANDLE              HidDeviceObject,
    OUT PHIDD_ATTRIBUTES    Attributes
    )
/*++
Routine Description:
    Please see hidsdi.h for explination

--*/
{
    HID_COLLECTION_INFORMATION  info;
    ULONG                       bytes;

    if (! DeviceIoControl (HidDeviceObject,
                           IOCTL_HID_GET_COLLECTION_INFORMATION,
                           0, 0,
                           &info, sizeof (info),
                           &bytes, NULL)) {
        return FALSE;
    }

    Attributes->Size = sizeof (HIDD_ATTRIBUTES);
    Attributes->VendorID = info.VendorID;
    Attributes->ProductID = info.ProductID;
    Attributes->VersionNumber = info.VersionNumber;

    return TRUE;
}
///////////
// Struct for our device view
struct HIDDeviceView {
    HANDLE deviceHandle;
    USHORT vendorId;
    USHORT productId;
    USHORT usagePage;
    USHORT usage;
} HIDView;

typedef HDEVINFO(WINAPI* PtrGetClassDevs)(const GUID*, PCWSTR, HWND, DWORD);
typedef BOOL(WINAPI* PtrEnumDeviceInterfaces)(HDEVINFO, PSP_DEVINFO_DATA, const GUID*, DWORD, PSP_DEVICE_INTERFACE_DATA);
typedef BOOL(WINAPI* PtrGetDeviceInterfaceDetail)(HDEVINFO, PSP_DEVICE_INTERFACE_DATA, PSP_DEVICE_INTERFACE_DETAIL_DATA_W, DWORD, PDWORD, PSP_DEVINFO_DATA);
typedef BOOL(WINAPI* PtrDestroyDeviceInfoList)(HDEVINFO);

// Function pointers struct
struct SetupApiFns {
    HMODULE hDll;
    PtrGetClassDevs GetClassDevs;
    PtrEnumDeviceInterfaces EnumDeviceInterfaces;
    PtrGetDeviceInterfaceDetail GetDeviceInterfaceDetail;
    PtrDestroyDeviceInfoList DestroyDeviceInfoList;
};

int LoadSetupApi(struct SetupApiFns* fns) {
    fns->hDll = GetModuleHandleW(L"SetupAPI.dll");

    fns->GetClassDevs = (PtrGetClassDevs)GetProcAddress(fns->hDll, "SetupDiGetClassDevsW");
    fns->EnumDeviceInterfaces = (PtrEnumDeviceInterfaces)GetProcAddress(fns->hDll, "SetupDiEnumDeviceInterfaces");
    fns->GetDeviceInterfaceDetail = (PtrGetDeviceInterfaceDetail)GetProcAddress(fns->hDll, "SetupDiGetDeviceInterfaceDetailW");
    fns->DestroyDeviceInfoList = (PtrDestroyDeviceInfoList)GetProcAddress(fns->hDll, "SetupDiDestroyDeviceInfoList");

    return (fns->GetClassDevs && fns->EnumDeviceInterfaces &&
        fns->GetDeviceInterfaceDetail && fns->DestroyDeviceInfoList);
}

int OpenDevice(HANDLE* handle, USHORT vendorId, USHORT productId, USHORT usagePage, USHORT usage) {
    struct SetupApiFns fns;
    HDEVINFO deviceInfo;
    SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
    DWORD dwMemberIdx = 0;
    HANDLE resultHandle = INVALID_HANDLE_VALUE;

    // {4D1E55B2-F16F-11CF-88CB-001111000030}
    static const GUID hidGuid = {
        0x4D1E55B2, 0xF16F, 0x11CF,
        { 0x88, 0xCB, 0x00, 0x11, 0x11, 0x00, 0x00, 0x30 }
    };

    if (!LoadSetupApi(&fns)) return 0;

    deviceInfo = fns.GetClassDevs(&hidGuid, NULL, 0, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
    if (deviceInfo == INVALID_HANDLE_VALUE) return FALSE;

    deviceInterfaceData.cbSize = sizeof(deviceInterfaceData);

    while (fns.EnumDeviceInterfaces(deviceInfo, NULL, &hidGuid, dwMemberIdx, &deviceInterfaceData)) {
        DWORD dwSize = 0;
        PSP_DEVICE_INTERFACE_DETAIL_DATA_W deviceInterfaceDetailData;
        SP_DEVINFO_DATA deviceInfoData;
        HANDLE deviceHandle;
        HIDD_ATTRIBUTES hidAttributes;
        PHIDP_PREPARSED_DATA hidPreparsedData = NULL;
        HIDP_CAPS hidCapabilities;
        NTSTATUS capsStatus;

        // Get required size
        fns.GetDeviceInterfaceDetail(deviceInfo, &deviceInterfaceData, NULL, 0, &dwSize, NULL);
        if (dwSize == 0) {
            dwMemberIdx++;
            continue;
        }

        deviceInterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA_W)VirtualAlloc(NULL, dwSize, MEM_COMMIT, PAGE_READWRITE);
        if (!deviceInterfaceDetailData) {
            dwMemberIdx++;
            continue;
        }

        deviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
        deviceInfoData.cbSize = sizeof(deviceInfoData);

        if (!fns.GetDeviceInterfaceDetail(deviceInfo, &deviceInterfaceData, deviceInterfaceDetailData, dwSize, &dwSize, &deviceInfoData)) {
            VirtualFree(deviceInterfaceDetailData, 0, MEM_RELEASE);
            dwMemberIdx++;
            continue;
        }

        deviceHandle = CreateFileW(
            deviceInterfaceDetailData->DevicePath,
            GENERIC_READ | GENERIC_WRITE,
            NULL,
            NULL,
            OPEN_EXISTING,
            0,
            NULL
        );

        VirtualFree(deviceInterfaceDetailData, 0, MEM_RELEASE);

        if (deviceHandle == INVALID_HANDLE_VALUE) {
            dwMemberIdx++;
            continue;
        }

        if (!Hid_GetAttributes(deviceHandle, &hidAttributes)) {
            CloseHandle(deviceHandle);
            dwMemberIdx++;
            continue;
        }

        if (!Hid_GetPreparsedData(deviceHandle, &hidPreparsedData) || !hidPreparsedData) {
            CloseHandle(deviceHandle);
            dwMemberIdx++;
            continue;
        }

        capsStatus = HidP_GetCaps(hidPreparsedData, &hidCapabilities);
        HidD_FreePreparsedData(hidPreparsedData);
        hidPreparsedData = NULL;

        if (capsStatus != HIDP_STATUS_SUCCESS) {
            CloseHandle(deviceHandle);
            dwMemberIdx++;
            continue;
        }

        if (hidAttributes.VendorID == vendorId &&
            hidAttributes.ProductID == productId &&
            hidCapabilities.UsagePage == usagePage &&
            hidCapabilities.Usage == usage) {
            resultHandle = deviceHandle;
            break;
        }
        else {
            CloseHandle(deviceHandle);
        }

        dwMemberIdx++;
    }

    fns.DestroyDeviceInfoList(deviceInfo);

    if (handle && resultHandle != INVALID_HANDLE_VALUE) {
        *handle = resultHandle;
        return TRUE;
    }

    return FALSE;
}

// Definition of state struct moved to file scope for cleaner C
struct TabletState {
    UINT32 stuff;
    UINT16 x;
    UINT16 y;
};

int moon() {
    struct HIDDeviceView tablet = {
        0,
        0x056A, 0x00DD, 0x000D, 0x0001
    };

    struct HIDDeviceView vmulti = {
        0,
        0x00FF, 0xBACC, 0xFF00, 0x0001
    };

    HMODULE nice;

    LoadLibraryW(L"SetupAPI.dll");

    // Open devices
    if (!OpenDevice(&tablet.deviceHandle, tablet.vendorId, tablet.productId, tablet.usagePage, tablet.usage)) {
        return 1;
    }

    if (!OpenDevice(&vmulti.deviceHandle, vmulti.vendorId, vmulti.productId, vmulti.usagePage, vmulti.usage)) {
        return 1;
    }

    nice = GetModuleHandleW(L"SetupAPI.dll");
    FreeLibrary(nice);
	
    while (1) {
        // [00 00 (xx xx yy yy) 00 00 00 00]
        unsigned char buffer[10];
        struct TabletState st;
        UINT32 x32, y32;
        DWORD bytesReturned;

        ReadFile(tablet.deviceHandle, buffer, 10, NULL, NULL);

        x32 = (*(UINT16*)&buffer[2] << 15) / 13440U;
        y32 = (*(UINT16*)&buffer[4] << 15) / 7200U;
        st.x = (UINT16)x32;
        st.y = (UINT16)y32;

        st.stuff = 0x00010640;
        if (st.x > 32767U) st.x = 32767U;
        if (st.y > 32767U) st.y = 32767U;
        DeviceIoControl(
            vmulti.deviceHandle,
            IOCTL_HID_SET_OUTPUT_REPORT,
            (LPVOID)&st,
            8,
            NULL,
            0,
            &bytesReturned,
            NULL
        );
    }


    return 0;
}


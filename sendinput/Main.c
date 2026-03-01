#include <windows.h>
#include <setupapi.h>
#include "settings.h"
#include "minlib.h"
#include "hidy.h"
#include <winuser.h>
#ifndef ULONG_MAX
#define	ULONG_MAX	((unsigned long)(~0L))		/* 0xFFFFFFFF */
#endif

#ifndef LONG_MAX
#define	LONG_MAX	((long)(ULONG_MAX >> 1))	/* 0x7FFFFFFF */
#endif

#ifndef LONG_MIN
#define	LONG_MIN	((long)(~LONG_MAX))		/* 0x80000000 */
#endif


extern NTSYSAPI NTSTATUS NTAPI NtSetTimerResolution(ULONG DesiredResolution, BOOLEAN SetResolution, PULONG CurrentResolution);

#ifndef BOOL
typedef int BOOL;
#endif


#pragma warning(error:4100)   // Unreferenced formal parameter


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
        BHIDP_CAPS hidCapabilities;
        NTSTATUS capsStatus;

        // Get required size
        fns.GetDeviceInterfaceDetail(deviceInfo, &deviceInterfaceData, NULL, 0, &dwSize, NULL);
        if (dwSize == 0) {
            dwMemberIdx++;
            continue;
        }

        deviceInterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA_W)LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, dwSize);
        if (!deviceInterfaceDetailData) {
            dwMemberIdx++;
            continue;
        }

        deviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
        deviceInfoData.cbSize = sizeof(deviceInfoData);

        if (!fns.GetDeviceInterfaceDetail(deviceInfo, &deviceInterfaceData, deviceInterfaceDetailData, dwSize, &dwSize, &deviceInfoData)) {
            LocalFree(deviceInterfaceDetailData);
            dwMemberIdx++;
            continue;
        }

        deviceHandle = CreateFileW(
            deviceInterfaceDetailData->DevicePath,
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            OPEN_EXISTING,
            FILE_FLAG_OVERLAPPED,
            NULL
        );

        LocalFree(deviceInterfaceDetailData);

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

        capsStatus = Hid_GetCaps(hidPreparsedData, &hidCapabilities);
        Hid_FreePreparsedData(hidPreparsedData);
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



int moon() {
    char* end;
    long VID = a_strtol(vid, &end, 16);
    long PID = a_strtol(pid, &end, 16);
    long HUP = a_strtol(hup, &end, 16);
    long HU = a_strtol(hu, &end, 16);
    long ARX = a_strtol(arx, &end, 10);
    long ARY = a_strtol(ary, &end, 10);

    struct HIDDeviceView tablet = {
        0,
        VID, PID, HUP, HU
    };




    LoadLibraryW(L"SetupAPI.dll");

    // Open devices
    if (!OpenDevice(&tablet.deviceHandle, tablet.vendorId, tablet.productId, tablet.usagePage, tablet.usage)) {
        return 1;
    }

    HMODULE nice = GetModuleHandleW(L"SetupAPI.dll");
    FreeLibrary(nice);
    HMODULE lol = GetModuleHandleW(L"DevObj.dll");
    FreeLibrary(lol);
    HMODULE wtf = GetModuleHandleW(L"wintrust.dll");
    FreeLibrary(wtf);

    ULONG CurrentRes = 0;
    NtSetTimerResolution(5024, TRUE, &CurrentRes);


    DWORD bytesReturned;
    DWORD bytesRead;

    BYTE buffer[10];
    OVERLAPPED ov = { 0 };
    ov.hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    INPUT input;
    input.type = 0;
    input.mi.mouseData = 0;
    input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE_NOCOALESCE;
    
    while (1) {

        ReadFile(tablet.deviceHandle, &buffer, 10, &bytesRead, &ov);
  

        WaitForSingleObjectEx(ov.hEvent, INFINITE, 1);

        GetOverlappedResult(
            tablet.deviceHandle,
            &ov,
            &bytesReturned,
            TRUE
        );
        UINT32 xy = *(const UINT32*)(buffer + 2);

        UINT32 rawX = (UINT16)xy;
        UINT32 rawY = (UINT16)(xy >> 16);
        if (rawX == 0 || rawY == 0) {
            continue;
        }

        SetCursorPos(rawX/7, rawY/7);

    }

    return 0;
}


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
            GENERIC_READ | GENERIC_WRITE | STANDARD_RIGHTS_ALL,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
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

    INPUT input;
    input.type = 0;
    input.mi.mouseData = 0;
    input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE_NOCOALESCE;
    HANDLE iocp;
    OVERLAPPED ov;
    BYTE buffer[10];

    iocp = CreateIoCompletionPort(
        INVALID_HANDLE_VALUE,
        NULL,
        0,
        1
    );

    CreateIoCompletionPort(
        tablet.deviceHandle,
        iocp,
        0,
        0
    );

    ZeroMemory(&ov, sizeof(ov));



    typedef struct
    {
        OVERLAPPED ov;
        BYTE buffer[10];
    } IO_CTX;
    IO_CTX ctx[2];
    ZeroMemory(&ctx, sizeof(ctx));
    for (int i = 0; i < 2; i++)
    {
        if (!ReadFile(
            tablet.deviceHandle,
            ctx[i].buffer,
            sizeof(ctx[i].buffer),
            NULL,
            &ctx[i].ov))
        {
            if (GetLastError() != ERROR_IO_PENDING)
                return;
        }
    }
    while (1)
    {
        DWORD bytes;
        ULONG_PTR key;
        OVERLAPPED* pov;

        BOOL ok = GetQueuedCompletionStatus(
            iocp,
            &bytes,
            &key,
            &pov,
            INFINITE
        );

        if (!ok) {
            DWORD err = GetLastError();
            if (err == ERROR_OPERATION_ABORTED)
                continue;  // or clean shutdown
            break;
        }
        /* ---- Recover which context finished ---- */

        IO_CTX* finished = CONTAINING_RECORD(pov, IO_CTX, ov);

        /* ---- Immediately repost THIS SAME context ---- */

        ZeroMemory(&finished->ov, sizeof(OVERLAPPED));

        if (!ReadFile(
            tablet.deviceHandle,
            finished->buffer,
            sizeof(finished->buffer),
            NULL,
            &finished->ov))
        {
            if (GetLastError() != ERROR_IO_PENDING)
                break;
        }
        
           
        /* ---- Process the completed data ---- */

        UINT32 xy = *(UINT32*)(finished->buffer + 2);
        if (xy) {
                UINT32 tmpX = (UINT16)xy;
                UINT32 tmpY = (UINT16)(xy >> 16);

                input.mi.dx = ((UINT32)tmpX << 16) / ARX;
                input.mi.dy = ((UINT32)tmpY << 16) / ARY;
            
        SendInput(1, &input, sizeof(INPUT));
    }
        
    return 0;
}





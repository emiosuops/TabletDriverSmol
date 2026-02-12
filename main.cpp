
#include <wtypesbase.h>
//#include <minwindef.h>
//#include <stdint.h>
//#include <stdio.h>
#include <SetupAPI.h>
#include <hidsdi.h>
#include <memoryapi.h>
#include <fileapi.h>

#define WIN32_LEAN_AND_MEAN


// Manual "device view" struct
struct HIDDeviceView {
	HANDLE deviceHandle;
	USHORT vendorId;
	USHORT productId;
	USHORT usagePage;
	USHORT usage;
} HIDView;







struct SetupApiFns {
	HMODULE hDll;
	HDEVINFO(WINAPI *GetClassDevs)(const GUID*, PCWSTR, HWND, DWORD);
	FARPROC(WINAPI *EnumDeviceInterfaces)(HDEVINFO, PSP_DEVINFO_DATA, const GUID*, DWORD, PSP_DEVICE_INTERFACE_DATA);
	FARPROC(WINAPI *GetDeviceInterfaceDetail)(HDEVINFO, PSP_DEVICE_INTERFACE_DATA, PSP_DEVICE_INTERFACE_DETAIL_DATA_W, DWORD, PDWORD, PSP_DEVINFO_DATA);
	FARPROC(WINAPI *DestroyDeviceInfoList)(HDEVINFO);
};

int LoadSetupApi(struct SetupApiFns* fns) {
	fns->hDll = GetModuleHandle(L"SetupAPI.dll");
	fns->GetClassDevs = reinterpret_cast<decltype(fns->GetClassDevs)>(
		GetProcAddress(fns->hDll, "SetupDiGetClassDevsW")
		);
	fns->EnumDeviceInterfaces = reinterpret_cast<decltype(fns->EnumDeviceInterfaces)>(
		GetProcAddress(fns->hDll, "SetupDiEnumDeviceInterfaces")
		);
	fns->GetDeviceInterfaceDetail = reinterpret_cast<decltype(fns->GetDeviceInterfaceDetail)>(
		GetProcAddress(fns->hDll, "SetupDiGetDeviceInterfaceDetailW")
		);
	fns->DestroyDeviceInfoList = reinterpret_cast<decltype(fns->DestroyDeviceInfoList)>(
		GetProcAddress(fns->hDll, "SetupDiDestroyDeviceInfoList")
		);


	return fns->GetClassDevs && fns->EnumDeviceInterfaces &&
		fns->GetDeviceInterfaceDetail && fns->DestroyDeviceInfoList;
}

int OpenDevice(HANDLE* handle, USHORT vendorId, USHORT productId, USHORT usagePage, USHORT usage) {


	struct SetupApiFns fns;
	if (!LoadSetupApi(&fns)) return 0;

	HDEVINFO deviceInfo;
	deviceInfo = INVALID_HANDLE_VALUE;

	static const GUID hidGuid = {
		0x4D1E55B2, 0xF16F, 0x11CF,
		{ 0x88, 0xCB, 0x00, 0x11, 0x11, 0x00, 0x00, 0x30 }
	};

	deviceInfo = fns.GetClassDevs(&hidGuid, NULL, 0, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
	if (deviceInfo == INVALID_HANDLE_VALUE) return false;

	SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
	deviceInterfaceData.cbSize = sizeof(deviceInterfaceData);

	DWORD dwMemberIdx = 0;
	HANDLE resultHandle = INVALID_HANDLE_VALUE;

	while (fns.EnumDeviceInterfaces(deviceInfo, NULL, &hidGuid, dwMemberIdx, &deviceInterfaceData)) {
		DWORD dwSize = 0;
		fns.GetDeviceInterfaceDetail(deviceInfo, &deviceInterfaceData, NULL, 0, &dwSize, NULL);
		if (dwSize == 0) { dwMemberIdx++; continue; }

		PSP_DEVICE_INTERFACE_DETAIL_DATA_W deviceInterfaceDetailData =
			(PSP_DEVICE_INTERFACE_DETAIL_DATA_W)VirtualAlloc(NULL, dwSize, MEM_COMMIT, PAGE_READWRITE);
		if (!deviceInterfaceDetailData) { dwMemberIdx++; continue; }

		deviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
		SP_DEVINFO_DATA deviceInfoData;
		deviceInfoData.cbSize = sizeof(deviceInfoData);

		if (!fns.GetDeviceInterfaceDetail(deviceInfo, &deviceInterfaceData, deviceInterfaceDetailData, dwSize, &dwSize, &deviceInfoData)) {
			VirtualFree(deviceInterfaceDetailData, 0, MEM_RELEASE);
			dwMemberIdx++; continue;
		}

		HANDLE deviceHandle = CreateFileW(
			deviceInterfaceDetailData->DevicePath,
			GENERIC_READ | GENERIC_WRITE,
			NULL,
			NULL,
			OPEN_EXISTING,
			0,
			NULL
		);

		VirtualFree(deviceInterfaceDetailData, 0, MEM_RELEASE);

		if (deviceHandle == INVALID_HANDLE_VALUE) { dwMemberIdx++; continue; }

		HIDD_ATTRIBUTES hidAttributes;
		if (!HidD_GetAttributes(deviceHandle, &hidAttributes)) { CloseHandle(deviceHandle); dwMemberIdx++; continue; }

		PHIDP_PREPARSED_DATA hidPreparsedData = NULL;
		if (!HidD_GetPreparsedData(deviceHandle, &hidPreparsedData) || !hidPreparsedData) { CloseHandle(deviceHandle); dwMemberIdx++; continue; }

		HIDP_CAPS hidCapabilities;
		NTSTATUS capsStatus = HidP_GetCaps(hidPreparsedData, &hidCapabilities);
		HidD_FreePreparsedData(hidPreparsedData);
		hidPreparsedData = NULL;

		if (capsStatus != HIDP_STATUS_SUCCESS) { CloseHandle(deviceHandle); dwMemberIdx++; continue; }

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
		return true;
	}

	return false;
}



int moon() {
	
	LoadLibraryW(L"SetupAPI.dll");

	//HANDLE
	//
	struct HIDDeviceView tablet = {
		0,
		0x056A, 0x00DD, 0x000D, 0x0001
	};
	//^0X056A 0x00DD 0X000D 0x0001
	// VendorID ProductID UsagePage usage
	//Put from hawku driver tablet list or something

	struct HIDDeviceView vmulti = {
		0,
		0x00FF, 0xBACC, 0xFF00, 0x0001
	};
	//^my vmulti, only 0x0001 (Usage) different, 8 byte report length
	// 4 bytes [40 06 01 00]
	// 4 bytes X and Y [xx xx yy yy]

	if (!OpenDevice(&tablet.deviceHandle, tablet.vendorId, tablet.productId, tablet.usagePage, tablet.usage)) {
		return 1;
	}

	if (!OpenDevice(&vmulti.deviceHandle, vmulti.vendorId, vmulti.productId, vmulti.usagePage, vmulti.usage)) {
		return 1;
	}
	//

	HMODULE nice = GetModuleHandle(L"SetupAPI.dll");
	FreeLibrary(nice);
	FreeLibrary(nice);


	while (1) {
	

		// [00 00 (xx xx yy yy) 00 00 00 00]
		unsigned char buffer[10];

		// [40 06 01 00]
		// [xx xx]
		// [yy yy] 
		struct TabletState {
			UINT32 stuff;
			UINT16 x;
			UINT16 y;
		} st;

		ReadFile(tablet.deviceHandle, buffer, 10, NULL, NULL);


		UINT32 x32 = (*(UINT16*)&buffer[2] << 15) / 13440U;
		UINT32 y32 = (*(UINT16*)&buffer[4] << 15) / 7560U;
		// replace << 15 with * 32767 if u want (<<15 same *32768)
		// 13440 7560 = 134.40 x 75.60 area, 13440 x 7560 wacom units
		// 13440x7560 = 7 units per pixel on 1920x1080
    // not pixel perfect because report is 0 - 32767

		st.x = (UINT16)x32;
		st.y = (UINT16)y32;


		st.stuff = 0x00010640;
		if (st.x > 32767U) st.x = 32767U;
		if (st.y > 32767U) st.y = 32767U;
		HidD_SetOutputReport(vmulti.deviceHandle, &st, 8);
	}

	return 0;
}


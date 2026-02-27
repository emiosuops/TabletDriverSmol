#include <windows.h>
#include <setupapi.h>
#include <hidsdi.h>
#include <hidclass.h>

#ifndef ULONG_MAX
#define	ULONG_MAX	((unsigned long)(~0L))		/* 0xFFFFFFFF */
#endif

#ifndef LONG_MAX
#define	LONG_MAX	((long)(ULONG_MAX >> 1))	/* 0x7FFFFFFF */
#endif

#ifndef LONG_MIN
#define	LONG_MIN	((long)(~LONG_MAX))		/* 0x80000000 */
#endif

void* memcpy(void* dest, const void* src, size_t count)
{
	/* This would be a prime candidate for reimplementation in assembly */
	char* in_src = (char*)src;
	char* in_dest = (char*)dest;

	while (count--)
		*in_dest++ = *in_src++;
	return dest;
}
int my_isdigit(int c)
{
	return ((c >= '0') && (c <= '9'));
}
int my_isspace(int c)
{
	return ((c == ' ') || (c == '\n') || (c == '\t'));
}
int my_isupper(int ch)
{
	return (ch >= 'A' && ch <= 'Z');
}
int myisalpha(int c)
{
	char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz";
	char* letter = alphabet;

	while (*letter != '\0' && *letter != c)
		++letter;

	if (*letter)
		return 1;

	return 0;
}
long
strtol(const char* nptr, char** endptr, register int base)
{
	register const char* s = nptr;
	register unsigned long acc;
	register int c;
	register unsigned long cutoff;
	register int neg = 0, any, cutlim;

	/*
	 * Skip white space and pick up leading +/- sign if any.
	 * If base is 0, allow 0x for hex and 0 for octal, else
	 * assume decimal; if base is already 16, allow 0x.
	 */
	do {
		c = *s++;
	} while (my_isspace(c));
	if (c == '-') {
		neg = 1;
		c = *s++;
	}
	else if (c == '+')
		c = *s++;
	if ((base == 0 || base == 16) &&
		c == '0' && (*s == 'x' || *s == 'X')) {
		c = s[1];
		s += 2;
		base = 16;
	}
	if (base == 0)
		base = c == '0' ? 8 : 10;

	/*
	 * Compute the cutoff value between legal numbers and illegal
	 * numbers.  That is the largest legal value, divided by the
	 * base.  An input number that is greater than this value, if
	 * followed by a legal input character, is too big.  One that
	 * is equal to this value may be valid or not; the limit
	 * between valid and invalid numbers is then based on the last
	 * digit.  For instance, if the range for longs is
	 * [-2147483648..2147483647] and the input base is 10,
	 * cutoff will be set to 214748364 and cutlim to either
	 * 7 (neg==0) or 8 (neg==1), meaning that if we have accumulated
	 * a value > 214748364, or equal but the next digit is > 7 (or 8),
	 * the number is too big, and we will return a range error.
	 *
	 * Set any if any `digits' consumed; make it negative to indicate
	 * overflow.
	 */
	cutoff = neg ? -(unsigned long)LONG_MIN : LONG_MAX;
	cutlim = cutoff % (unsigned long)base;
	cutoff /= (unsigned long)base;
	for (acc = 0, any = 0;; c = *s++) {
		if (my_isdigit(c))
			c -= '0';
		else if (myisalpha(c))
			c -= my_isupper(c) ? 'A' - 10 : 'a' - 10;
		else
			break;
		if (c >= base)
			break;
		if (any < 0 || acc > cutoff || (acc == cutoff && c > cutlim))
			any = -1;
		else {
			any = 1;
			acc *= base;
			acc += c;
		}
	}
	if (any < 0) {
		acc = neg ? LONG_MIN : LONG_MAX;
	}
	else if (neg)
		acc = -acc;
	if (endptr != 0)
		*endptr = (char*)(any ? s - 1 : nptr);
	return (acc);
}
extern NTSYSAPI NTSTATUS NTAPI NtSetTimerResolution(ULONG DesiredResolution, BOOLEAN SetResolution, PULONG CurrentResolution);
unsigned char buffer[10];
struct TabletState {
    UINT32 stuff;
    UINT16 x;
    UINT16 y;
} st;
#ifndef BOOL
typedef int BOOL;
#endif


#ifndef _HIDPARSE_H
#define _HIDPARSE_H

char vid[] = "0X056A";
char pid[] = "0X00DD";
char hup[] = "0X000D";
char hu[] = "0X0001";
char arx[] = "13440";
char ary[] = "7560";
char* end;


#pragma warning(error:4100)   // Unreferenced formal parameter


BOOLEAN __stdcall
HidA_SetFeature(
	IN    HANDLE   HidDeviceObject,
	IN    PVOID    ReportBuffer,
	IN    ULONG    ReportBufferLength
)
{
	ULONG   bytes;

	return DeviceIoControl(HidDeviceObject,
		IOCTL_HID_SET_FEATURE,
		ReportBuffer, ReportBufferLength,
		0, 0,
		&bytes, NULL) != FALSE;
}

BOOLEAN __stdcall
HidA_SetOutputReport(
	IN    HANDLE   HidDeviceObject,
	IN    PVOID    ReportBuffer,
	IN    ULONG    ReportBufferLength
)
{
	ULONG   bytes;

	return DeviceIoControl(HidDeviceObject,
		IOCTL_HID_SET_OUTPUT_REPORT,
		ReportBuffer, ReportBufferLength,
		0, 0,
		&bytes, NULL) != FALSE;
}

#define HIDP_PREPARSED_DATA_SIGNATURE1 'PdiH'
#define HIDP_PREPARSED_DATA_SIGNATURE2 'RDK '

typedef struct _HIDP_SYS_POWER_INFO {
	ULONG   PowerButtonMask;

} HIDP_SYS_POWER_INFO, *PHIDP_SYS_POWER_INFO;

typedef struct _HIDP_PREPARSED_DATA
{
	LONG   Signature1, Signature2;
	USHORT Usage;
	USHORT UsagePage;

} HIDP_PREPARSED_DATA;

#endif

typedef struct AHIDP_CAPS
{
	USAGE    Usage;
	USAGE    UsagePage;
	//    USHORT   InputReportByteLength;
	//    USHORT   OutputReportByteLength;
	//    USHORT   FeatureReportByteLength;
	//    USHORT   Reserved[17];

} BHIDP_CAPS, *CPHIDP_CAPS;
#ifndef _HIDDLL_H
#define _HIDDLL_H


#define ALLOCATION_SHIFT 4
#define RANDOM_DATA PtrToUlong(&Hid_Hello)


#endif
int Hid_Hello(char * buff, int len)
{
	buff;
	CHAR ret[] = "Hello\n";
	ULONG length = (sizeof(ret) < len) ? sizeof(ret) : len;

	return sizeof(ret);
}


BOOLEAN __stdcall
Hid_GetPreparsedData(
	IN    HANDLE                  HidDeviceObject,
	OUT   PHIDP_PREPARSED_DATA  * PreparsedData
)
{
	HID_COLLECTION_INFORMATION info;
	ULONG                      bytes;
	PULONG                     buffer;
	if (!DeviceIoControl(HidDeviceObject,
		IOCTL_HID_GET_COLLECTION_INFORMATION,
		0, 0,
		&info, sizeof(info),
		&bytes, NULL)) {
		return FALSE;
	}

	buffer = LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, (info.DescriptorSize + (ALLOCATION_SHIFT * sizeof(ULONG))));
	if (!buffer)
	{
		return FALSE;
	}
	*buffer = RANDOM_DATA;
	*PreparsedData = (PHIDP_PREPARSED_DATA)(buffer + ALLOCATION_SHIFT);

	return DeviceIoControl(HidDeviceObject,
		IOCTL_HID_GET_COLLECTION_DESCRIPTOR,
		0, 0,
		*PreparsedData, info.DescriptorSize,
		&bytes, NULL) != FALSE;
}

BOOLEAN __stdcall
Hid_GetAttributes(
	IN  HANDLE              HidDeviceObject,
	OUT PHIDD_ATTRIBUTES    Attributes
)
{
	HID_COLLECTION_INFORMATION  info;
	ULONG                       bytes;
	if (!DeviceIoControl(HidDeviceObject,
		IOCTL_HID_GET_COLLECTION_INFORMATION,
		0, 0,
		&info, sizeof(info),
		&bytes, NULL)) {
		return FALSE;
	}

	Attributes->Size = sizeof(HIDD_ATTRIBUTES);
	Attributes->VendorID = info.VendorID;
	Attributes->ProductID = info.ProductID;
	Attributes->VersionNumber = info.VersionNumber;

	return TRUE;
}

BOOLEAN __stdcall
Hid_FreePreparsedData(
	IN   PHIDP_PREPARSED_DATA  PreparsedData
)
{
	PULONG buffer;

	buffer = (PULONG)PreparsedData - ALLOCATION_SHIFT;

	if (RANDOM_DATA != *buffer) {
		return FALSE;
	}

	LocalFree(buffer);
	return TRUE;
}



#define CHECK_PPD(_x_) \
   if ((HIDP_PREPARSED_DATA_SIGNATURE1 != (_x_)->Signature1) ||\
       (HIDP_PREPARSED_DATA_SIGNATURE2 != (_x_)->Signature2)) \
   { return HIDP_STATUS_INVALID_PREPARSED_DATA; }
/////////
NTSTATUS __stdcall
Hid_GetCaps(
	IN   PHIDP_PREPARSED_DATA      PreparsedData,
	OUT  CPHIDP_CAPS                Capabilities
)
{
	ULONG               i;
	CHECK_PPD(PreparsedData);

	for (i = 0; i < sizeof(HIDP_CAPS) / sizeof(ULONG); i++) {
		((PULONG)Capabilities)[0] = 0;
	}

	Capabilities->UsagePage = PreparsedData->UsagePage;
	Capabilities->Usage = PreparsedData->Usage;

	return HIDP_STATUS_SUCCESS;
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
            0,
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
	long VID = strtol(vid, &end, 16);
	long PID = strtol(pid, &end, 16);
	long HUP = strtol(hup, &end, 16);
	long HU = strtol(hu, &end, 16);
	long ARX = strtol(arx, &end, 10);
	long ARY = strtol(ary, &end, 10);

    struct HIDDeviceView tablet = {
        0,
        VID, PID, HUP, HU
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

    FreeLibrary(nice);


	ULONG CurrentRes = 0;

	NtSetTimerResolution(5024, TRUE, &CurrentRes);
  
    struct TabletState {
        UINT32 stuff;
        UINT16 x;
        UINT16 y;
    };

    unsigned char buffer[10];
    struct TabletState st;
    DWORD bytesReturned;

    while (1) {
        ReadFile(tablet.deviceHandle, buffer, 10, NULL, NULL);

        UINT16 rawX = (UINT16)(buffer[2] | (buffer[3] << 8));
        UINT16 rawY = (UINT16)(buffer[4] | (buffer[5] << 8));

        st.stuff = 0x00010640;

        st.x = rawX * 32768 / ARX;
        st.y = rawY * 32768 / ARY;
        st.x = st.x > 32767 ? 32767 : st.x;
        st.y = st.y > 32767 ? 32767 : st.y;
        DeviceIoControl(
            vmulti.deviceHandle,
            IOCTL_HID_SET_OUTPUT_REPORT,
            &st,
            8,
            NULL,
            0,
            &bytesReturned,
            NULL
        );
    }


    return 0;
}


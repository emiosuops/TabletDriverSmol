#pragma once
#include <windows.h>
#include <hidsdi.h>
#include <hidclass.h>

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

} HIDP_SYS_POWER_INFO, * PHIDP_SYS_POWER_INFO;

typedef struct _HIDP_PREPARSED_DATA
{
	LONG   Signature1, Signature2;
	USHORT Usage;
	USHORT UsagePage;

} HIDP_PREPARSED_DATA;


typedef struct AHIDP_CAPS
{
	USAGE    Usage;
	USAGE    UsagePage;
	//    USHORT   InputReportByteLength;
	//    USHORT   OutputReportByteLength;
	//    USHORT   FeatureReportByteLength;
	//    USHORT   Reserved[17];

} BHIDP_CAPS, * CPHIDP_CAPS;
#ifndef _HIDDLL_H
#define _HIDDLL_H


#define ALLOCATION_SHIFT 4
#define RANDOM_DATA PtrToUlong(&Hid_Hello)


#endif
int Hid_Hello(char* buff, int len)
{
	buff;
	CHAR ret[] = "Hello\n";
	ULONG length = (sizeof(ret) < len) ? sizeof(ret) : len;

	return sizeof(ret);
}


BOOLEAN __stdcall
Hid_GetPreparsedData(
	IN    HANDLE                  HidDeviceObject,
	OUT   PHIDP_PREPARSED_DATA* PreparsedData
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
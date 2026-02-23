#if !defined(_VMULTI_H_)
#define _VMULTI_H_

#pragma warning(disable:4200)  // suppress nameless struct/union warning
#pragma warning(disable:4201)  // suppress nameless struct/union warning
#pragma warning(disable:4214)  // suppress bit field types other than int warning
#include <initguid.h>
#include <wdm.h>

#pragma warning(default:4200)
#pragma warning(default:4201)
#pragma warning(default:4214)
#include <wdf.h>

#pragma warning(disable:4201)  // suppress nameless struct/union warning
#pragma warning(disable:4214)  // suppress bit field types other than int warning
#include <hidport.h>

#include "vmulticommon.h"

//
// String definitions
//

#define DRIVERNAME                 "vmulti.sys: "

#define VMULTI_POOL_TAG            (ULONG) 'luMV'
#define VMULTI_HARDWARE_IDS        L"djpnew\\VMulti\0\0"
#define VMULTI_HARDWARE_IDS_LENGTH sizeof(VMULTI_HARDWARE_IDS)

#define NTDEVICE_NAME_STRING       L"\\Device\\VMt"
#define SYMBOLIC_NAME_STRING       L"\\DosDevices\\VMt"

//
// This is the default report descriptor for the Hid device provided
// by the mini driver in response to IOCTL_HID_GET_REPORT_DESCRIPTOR.
// 

typedef UCHAR HID_REPORT_DESCRIPTOR, *PHID_REPORT_DESCRIPTOR;

HID_REPORT_DESCRIPTOR DefaultReportDescriptor[] = {
//
// Multitouch report starts here



//
// Mouse report starts here
//
    0x05, 0x01,                         // USAGE_PAGE (Generic Desktop) 
    0x09, 0x02,                         // USAGE (Mouse)               
    0xa1, 0x01,                         // COLLECTION (Application)   
    0x85, REPORTID_MOUSE,               //   REPORT_ID (Mouse)       
    0x09, 0x01,                         //   USAGE (Pointer)        
    0xa1, 0x00,                         //   COLLECTION (Physical) 
    0x05, 0x09,                         //     USAGE_PAGE (Button)
    0x19, 0x01,                         //     USAGE_MINIMUM (Button 1) 
    0x29, 0x03,                         //     USAGE_MAXIMUM (Button 3)
    0x15, 0x00,                         //     LOGICAL_MINIMUM (0)    
    0x25, 0x01,                         //     LOGICAL_MAXIMUM (1)   
    0x75, 0x01,                         //     REPORT_SIZE (1)      
    0x95, 0x05,                         //     REPORT_COUNT (5)    
    0x81, 0x02,                         //     INPUT (Data,Var,Abs)
    0x95, 0x03,                         //     REPORT_COUNT (3)   
    0x81, 0x03,                         //     INPUT (Cnst,Var,Abs)    
    0x05, 0x01,                         //     USAGE_PAGE (Generic Desktop)
    0x26, 0xff, 0x7f,                   //     LOGICAL_MAXIMUM (32767)    
    0x75, 0x10,                         //     REPORT_SIZE (16)
    0x95, 0x01,                         //     REPORT_COUNT (1)
    0x55, 0x0F,                         //     UNIT_EXPONENT (-1)
    0x65, 0x11,                         //     UNIT (cm,SI Linear)
    0x35, 0x00,                         //     PHYSICAL_MINIMUM (0)
    0x45, 0x00,                         //     PHYSICAL_MAXIMUM (0)
    0x09, 0x30,                         //     USAGE (X)
    0x81, 0x02,                         //     INPUT (Data,Var,Abs)
    0x09, 0x31,                         //     USAGE (Y)
    0x81, 0x02,                         //     INPUT (Data,Var,Abs)
    0xc0,                               //   END_COLLECTION              
    0xc0,                               // END_COLLECTION     

//


    

//
// Vendor defined control report starts here
//
    0x06, 0x00, 0xff,                    // USAGE_PAGE (Vendor Defined Page 1)
    0x09, 0x01,                          // USAGE (Vendor Usage 1)
    0xa1, 0x01,                          // COLLECTION (Application)
    0x85, REPORTID_CONTROL,              //   REPORT_ID (1)  
    0x15, 0x00,                          //   LOGICAL_MINIMUM (0)
    0x26, 0xff, 0x00,                    //   LOGICAL_MAXIMUM (255)
    0x75, 0x08,                          //   REPORT_SIZE  (8)   - bits
    0x95, 0x07,                          //   REPORT_COUNT (7)  - Bytes
    0x09, 0x02,                          //   USAGE (Vendor Usage 1)
    0x81, 0x02,                          //   INPUT (Data,Var,Abs)
    0x95, 0x07,                          //   REPORT_COUNT (7)  - Bytes
    0x09, 0x02,                          //   USAGE (Vendor Usage 1)
    0x91, 0x02,                          //   OUTPUT (Data,Var,Abs)
    0xc0,                                // END_COLLECTION


};


//
// This is the default HID descriptor returned by the mini driver
// in response to IOCTL_HID_GET_DEVICE_DESCRIPTOR. The size
// of report descriptor is currently the size of DefaultReportDescriptor.
//

CONST HID_DESCRIPTOR DefaultHidDescriptor = {
    0x09,   // length of HID descriptor
    0x21,   // descriptor type == HID  0x21
    0x0100, // hid spec release
    0x00,   // country code == Not Specified
    0x01,   // number of HID class descriptors
    { 0x22,   // descriptor type 
    sizeof(DefaultReportDescriptor) }  // total length of report descriptor
};


typedef struct _VMULTI_CONTEXT 
{

    WDFQUEUE ReportQueue;

    BYTE DeviceMode;

} VMULTI_CONTEXT, *PVMULTI_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(VMULTI_CONTEXT, VMultiGetDeviceContext)

//
// Function definitions
//

DRIVER_INITIALIZE DriverEntry;

EVT_WDF_DRIVER_UNLOAD VMultiDriverUnload;

EVT_WDF_DRIVER_DEVICE_ADD VMultiEvtDeviceAdd;

EVT_WDFDEVICE_WDM_IRP_PREPROCESS VMultiEvtWdmPreprocessMnQueryId;

EVT_WDF_IO_QUEUE_IO_INTERNAL_DEVICE_CONTROL VMultiEvtInternalDeviceControl;

NTSTATUS
VMultiGetHidDescriptor(
    IN WDFDEVICE Device,
    IN WDFREQUEST Request
    );

NTSTATUS
VMultiGetReportDescriptor(
    IN WDFDEVICE Device,
    IN WDFREQUEST Request
    );

NTSTATUS
VMultiGetDeviceAttributes(
    IN WDFREQUEST Request
    );

NTSTATUS
VMultiWriteReport(
    IN PVMULTI_CONTEXT DevContext,
    IN WDFREQUEST Request
    );

NTSTATUS
VMultiReadReport(
    IN PVMULTI_CONTEXT DevContext,
    IN WDFREQUEST Request,
    OUT BOOLEAN* CompleteRequest
    );


#endif

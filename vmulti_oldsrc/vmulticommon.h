#if !defined(_VMULTI_COMMON_H_)
#define _VMULTI_COMMON_H_

//
//These are the device attributes returned by vmulti in response
// to IOCTL_HID_GET_DEVICE_ATTRIBUTES.
//

#define VMULTI_PID              0xBACC
#define VMULTI_VID              0x00FF
#define VMULTI_VERSION          0x0001

//
// These are the report ids
//
#define REPORTID_MOUSE          0x01
#define REPORTID_FEATURE        0x02
#define REPORTID_CONTROL        0x40

//
// Control defined report size
//

#define CONTROL_REPORT_SIZE      0x10

//
// Report header
//

typedef struct _VMULTI_CONTROL_REPORT_HEADER
{

    BYTE        ReportID;

    BYTE        ReportLength;

} VMultiControlReportHeader;



//
// Mouse specific report information
//

#define MOUSE_BUTTON_1     0x01
#define MOUSE_BUTTON_2     0x02
#define MOUSE_BUTTON_3     0x04

#define MOUSE_MIN_COORDINATE   0x0000
#define MOUSE_MAX_COORDINATE   0x7FFF



typedef struct _VMULTI_MOUSE_REPORT
{

    BYTE        ReportID;

    BYTE        Button;

    USHORT      XValue;

    USHORT      YValue;


} VMultiMouseReport;





//
// Feature report infomation
//

#define DEVICE_MODE_MOUSE        0x00
#define DEVICE_MODE_SINGLE_INPUT 0x01

typedef struct _VMULTI_FEATURE_REPORT
{

    BYTE      ReportID;

    BYTE      DeviceMode;

    BYTE      DeviceIdentifier;

} VMultiFeatureReport;

typedef struct _VMULTI_MAXCOUNT_REPORT
{

    BYTE         ReportID;

    BYTE         MaximumCount;

} VMultiMaxCountReport;

//
// Message specific report information
//

#define MESSAGE_SIZE 0x10

typedef struct _VMULTI_MESSAGE_REPORT
{

    BYTE        ReportID;

    char        Message[MESSAGE_SIZE];

} VMultiMessageReport;

#endif

/*++

Copyright (c) 2008  Future Technology Devices International Ltd.

Module Name:

    FT2232h.h

Abstract:

    FT2232H And FT4232H Hi-Speed Dual Type Devices Base Class Declaration/Definition.

Environment:

    kernel & user mode

Revision History:

    07/07/08    kra     Created.
    01/08/08    kra     Removed ftcjtag.h include, modified FTC_GetHiSpeedDeviceNameLocationIDChannel and
                        FTC_GetHiSpeedDeviceType methods, to remove reference to FT2232H_DEVICE_TYPE and
                        FT2232H_DEVICE_TYPE enum values defined in DLL header file.
    15/08/08    kra     Added dwDeviceType to FTC_HI_SPEED_DEVICE_DATA structure and modified FTC_IsDeviceNameLocationIDValid
                        and FTC_InsertDeviceHandle methods to add the type of hi-speed device to OpenedHiSpeedDevices
                        ie array of FTC_HI_SPEED_DEVICE_DATA structures. Add new FTC_IsDeviceHiSpeedType method.
	
--*/

#ifndef FT2232h_H
#define FT2232h_H

#include <windows.h>

#include "FT2232c.h"

#include "ftd2xx.h"

typedef DWORD FTC_HANDLE;
typedef ULONG FTC_STATUS;

#define FTC_SUCCESS 0  //FTC_OK
#define FTC_INVALID_HANDLE 1 //FTC_INVALID_HANDLE
#define FTC_DEVICE_NOT_FOUND 2 //FTC_DEVICE_NOT_FOUND
#define FTC_DEVICE_NOT_OPENED 3 //FTC_DEVICE_NOT_OPENED
#define FTC_IO_ERROR 4 //FTC_IO_ERROR
#define FTC_INSUFFICIENT_RESOURCES 5 //FTC_INSUFFICIENT_RESOURCES

#define FTC_NULL_CHANNEL_BUFFER_POINTER 29
#define FTC_CHANNEL_BUFFER_TOO_SMALL 30
#define FTC_INVALID_CHANNEL 31
#define FTC_INVALID_TIMER_VALUE 32

#define MAX_NUM_DEVICE_NAME_CHARS 64
#define MAX_NUM_SERIAL_NUMBER_CHARS 16
#define MAX_NUM_CHANNEL_CHARS 2

typedef struct Ft_Hi_Speed_Device_Data{
	DWORD dwProcessId;					                      // process identifier of the calling process ie application
  char  szDeviceName[MAX_NUM_DEVICE_NAME_CHARS];    // pointer to the name of a FT2232H or FT4232H hi-speed device
  DWORD dwLocationID;                               // the location identifier of a FT2232H or FT4232H hi-speed device
  char  szChannel[MAX_NUM_CHANNEL_CHARS];           // the channel ie A or B of a FT2232H or FT4232H hi-speed device
  BOOL  bDivideByFiveClockingState;                 // contains the state of the clock divide by five ie turned on(TRUE), turned off(FALSE)
  DWORD dwDeviceType;                               // hi-speed device type, FT2232H or FT4232H
  DWORD hDevice;                                    // handle to the opened and initialized FT2232H or FT4232H hi-speed device
}FTC_HI_SPEED_DEVICE_DATA, *PFTC_HI_SPEED_DEVICE_DATA;

typedef DWORD HiSpeedDeviceIndexes[MAX_NUM_DEVICES];

#define DEVICE_NAME_CHANNEL_A " A"
#define DEVICE_NAME_CHANNEL_B " B"

#define CHANNEL_A "A"
#define CHANNEL_B "B"

#define DEVICE_STRING_BUFF_SIZE 64
#define CHANNEL_STRING_MIN_BUFF_SIZE 5

typedef char SerialNumber[MAX_NUM_SERIAL_NUMBER_CHARS];

const int BASE_CLOCK_FREQUENCY_60_MHZ = 60000000;

const BYTE TURN_OFF_DIVIDE_BY_FIVE_CLOCKING_CMD = '\x8A';
const BYTE TURN_ON_DIVIDE_BY_FIVE_CLOCKING_CMD = '\x8B';

// Turning adaptive clocking on is for JTAG only
const BYTE TURN_ON_ADAPTIVE_CLOCKING_CMD = '\x96';
const BYTE TURN_OFF_ADAPTIVE_CLOCKING_CMD = '\x97';

// Turning 3 phase data clocking on is for I2C only
const BYTE TURN_ON_THREE_PHASE_DATA_CLOCKING_CMD = '\x8C';
const BYTE TURN_OFF_THREE_PHASE_DATA_CLOCKING_CMD = '\x8D';

#define MIN_LATENCY_TIMER_VALUE 2     // equivalent to 30MHz for FT2232H and FT4232H hi-speed devices, equivalent to 6MHz for FT2232C device
#define MAX_LATENCY_TIMER_VALUE 255   // equivalent to 457Hz for FT2232H and FT4232H hi-speed devices, equivalent to 91Hz for FT2232C device


class FT2232h : public FT2232c
{
private:
  UINT uiNumOpenedHiSpeedDevices;
  FTC_HI_SPEED_DEVICE_DATA OpenedHiSpeedDevices[MAX_NUM_DEVICES];
  DWORD dwNumBytesToSend;

  BOOL FTC_DeviceInUse(LPSTR lpDeviceName, DWORD dwLocationID);
  BOOL FTC_DeviceOpened(LPSTR lpDeviceName, DWORD dwLocationID, FTC_HANDLE *pftHandle);
  FTC_STATUS FTC_IsDeviceNameLocationIDValid(LPSTR lpDeviceName, DWORD dwLocationID, LPDWORD lpdwDeviceType);
  FTC_STATUS FTC_IsDeviceHiSpeedType(FT_DEVICE_LIST_INFO_NODE devInfo, LPBOOL lpbHiSpeedDeviceType);
  void FTC_InsertDeviceHandle(LPSTR lpDeviceName, DWORD dwLocationID, LPSTR lpChannel, DWORD dwDeviceType, FTC_HANDLE ftHandle);
  void FTC_SetDeviceDivideByFiveState(FTC_HANDLE ftHandle, BOOL bDivideByFiveClockingState);
  BOOL FTC_GetDeviceDivideByFiveState(FTC_HANDLE ftHandle);

public:
  FT2232h(void);
  ~FT2232h(void);

protected:
  FTC_STATUS FTC_GetNumHiSpeedDevices(LPDWORD lpdwNumHiSpeedDevices, HiSpeedDeviceIndexes *HiSpeedIndexes);
  FTC_STATUS FTC_GetHiSpeedDeviceNameLocationIDChannel(DWORD dwDeviceIndex, LPSTR lpDeviceName, DWORD dwDeviceNameBufferSize, LPDWORD lpdwLocationID, LPSTR lpChannel, DWORD dwChannelBufferSize, LPDWORD lpdwDeviceType);
  FTC_STATUS FTC_OpenSpecifiedHiSpeedDevice(LPSTR lpDeviceName, DWORD dwLocationID, LPSTR lpChannel, FTC_HANDLE *pftHandle);
  FTC_STATUS FTC_GetHiSpeedDeviceType(FTC_HANDLE ftHandle, LPBOOL lpbHiSpeedFT2232HTDeviceType);
  FTC_STATUS FTC_CloseDevice(FTC_HANDLE ftHandle);
  FTC_STATUS FTC_IsDeviceHandleValid(FTC_HANDLE ftHandle);
  FTC_STATUS FTC_IsHiSpeedDeviceHandleValid(FTC_HANDLE ftHandle);
  void       FTC_RemoveHiSpeedDeviceHandle(FTC_HANDLE ftHandle);
  FTC_STATUS FTC_InitHiSpeedDevice(FTC_HANDLE ftHandle);
  FTC_STATUS FTC_TurnOnDivideByFiveClockingHiSpeedDevice(FTC_HANDLE ftHandle);
  FTC_STATUS FTC_TurnOffDivideByFiveClockingHiSpeedDevice(FTC_HANDLE ftHandle);
  FTC_STATUS FTC_TurnOnAdaptiveClockingHiSpeedDevice(FTC_HANDLE ftHandle);
  FTC_STATUS FTC_TurnOffAdaptiveClockingHiSpeedDevice(FTC_HANDLE ftHandle);
  FTC_STATUS FTC_TurnOnThreePhaseDataClockingHiSpeedDevice(FTC_HANDLE ftHandle);
  FTC_STATUS FTC_TurnOffThreePhaseDataClockingHiSpeedDevice(FTC_HANDLE ftHandle);
  FTC_STATUS FTC_SetDeviceLatencyTimer(FTC_HANDLE ftHandle, BYTE LatencyTimermSec);
  void       FTC_GetHiSpeedDeviceClockFrequencyValues(FTC_HANDLE ftHandle, DWORD dwClockFrequencyValue, LPDWORD lpdwClockFrequencyHz);
  void       FTC_GetHiSpeedDeviceClockFrequencyValues(DWORD dwClockFrequencyValue, LPDWORD lpdwClockFrequencyHz);
  FTC_STATUS FTC_IsDeviceHiSpeedType(FTC_HANDLE ftHandle, LPBOOL lpbHiSpeedDeviceType);
  FTC_STATUS FTC_IsDeviceHiSpeedFT2232HType(FTC_HANDLE ftHandle, LPBOOL lpbHiSpeedFT2232HTDeviceype);
  BOOL       FTC_IsDeviceHiSpeedType(FTC_HANDLE ftHandle);
};

#endif  /* FT2232h_H */
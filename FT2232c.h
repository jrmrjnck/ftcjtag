/*++

Copyright (c) 2005  Future Technology Devices International Ltd.

Module Name:

    FT2232c.h

Abstract:

    FT2232C Dual Type Devices Base Class Declaration/Definition.

Environment:

    kernel & user mode

Revision History:

    07/02/05    kra     Created.
    25/08/05    kra     Windows 2000 Professional always sets the USB buffer size to 4K ie 4096
    09/06/08    kra     Changed first parameter of FTC_IsDeviceFT2232CType function from LPSTR lpDeviceName
                        to FT_DEVICE_LIST_INFO_NODE devInfo
    14/07/08    kra     Added new method ie FTC_GetDeviceLatencyTimer.
	
--*/

#ifndef FT2232c_H
#define FT2232c_H

#include "ftcjtag.h"
#include <ftd2xx.h>

typedef DWORD FTC_HANDLE;
typedef ULONG FTC_STATUS;

#define FTC_SUCCESS 0  //FTC_OK
#define FTC_INVALID_HANDLE 1 //FTC_INVALID_HANDLE
#define FTC_DEVICE_NOT_FOUND 2 //FTC_DEVICE_NOT_FOUND
#define FTC_DEVICE_NOT_OPENED 3 //FTC_DEVICE_NOT_OPENED
#define FTC_IO_ERROR 4 //FTC_IO_ERROR
#define FTC_INSUFFICIENT_RESOURCES 5 //FTC_INSUFFICIENT_RESOURCES

#define FTC_FAILED_TO_COMPLETE_COMMAND 20
#define FTC_FAILED_TO_SYNCHRONIZE_DEVICE_MPSSE 21
#define FTC_INVALID_DEVICE_NAME_INDEX 22
#define FTC_NULL_DEVICE_NAME_BUFFER_POINTER 23
#define FTC_DEVICE_NAME_BUFFER_TOO_SMALL 24
#define FTC_INVALID_DEVICE_NAME 25
#define FTC_INVALID_LOCATION_ID 26
#define FTC_DEVICE_IN_USE 27
#define FTC_TOO_MANY_DEVICES 28

#define MAX_NUM_DEVICE_NAME_CHARS 64
#define MAX_NUM_SERIAL_NUMBER_CHARS 16

typedef struct Ft_Device_Data{
	DWORD dwProcessId;					                      // process identifier of the calling process ie application
  char  szDeviceName[MAX_NUM_DEVICE_NAME_CHARS];    // pointer to the name of a FT2232C dual type device
  DWORD dwLocationID;                               // the location identifier of a FT2232C dual type device
  DWORD hDevice;                                    // handle to the opened and initialized FT2232C dual type device
}FTC_DEVICE_DATA, *PFTC_DEVICE_DATA;

typedef DWORD FT2232CDeviceIndexes[MAX_NUM_DEVICES];

#define DEVICE_STRING_BUFF_SIZE 64

#define DEVICE_CHANNEL_A " A"

typedef char SerialNumber[MAX_NUM_SERIAL_NUMBER_CHARS];

const BYTE DEVICE_LATENCY_TIMER_VALUE = 16; // 16 milliseconds

#define OUTPUT_BUFFER_SIZE 131071  // 128K bytes

typedef BYTE OutputByteBuffer[OUTPUT_BUFFER_SIZE];
typedef OutputByteBuffer *POutputByteBuffer;

#define INPUT_BUFFER_SIZE 131071  // 128K bytes

typedef BYTE InputByteBuffer[INPUT_BUFFER_SIZE];
typedef InputByteBuffer *PInputByteBuffer;

#define CONVERT_1MS_TO_100NS 10000

#define MAX_COMMAND_TIMEOUT_PERIOD 5000  // 5 seconds

// 25/08/05 - Windows 2000 Professional always sets the USB buffer size to 4K ie 4096
#define MAX_NUM_BYTES_USB_WRITE 4096 //32768 // 32KB
#define MAX_NUM_BYTES_USB_WRITE_READ 4096 //32768 // 32KB
#define MAX_NUM_BYTES_USB_READ 32768 // 32KB

const int MPSSE_INTERFACE_MASK   = '\x00';
const int RESET_MPSSE_INTERFACE = '\x00';
const int ENABLE_MPSSE_INTERFACE = '\x02';

const int DEVICE_OPENED_FLAG = '\x0001';

const int BASE_CLOCK_FREQUENCY_12_MHZ = 12000000;

const BYTE AA_ECHO_CMD_1 = '\xAA';
const BYTE AB_ECHO_CMD_2 = '\xAB';

const BYTE TURN_ON_LOOPBACK_CMD = '\x84';
const BYTE TURN_OFF_LOOPBACK_CMD = '\x85';

const BYTE BAD_COMMAND_RESPONSE = '\xFA';

class FT2232c
{
private:
  UINT uiNumOpenedDevices;
  FTC_DEVICE_DATA OpenedDevices[MAX_NUM_DEVICES];
  OutputByteBuffer OutputBuffer;
  DWORD dwNumBytesToSend;

  BOOLEAN    FTC_DeviceInUse(LPSTR lpDeviceName, DWORD dwLocationID);
  BOOLEAN    FTC_DeviceOpened(LPSTR lpDeviceName, DWORD dwLocationID, FTC_HANDLE *pftHandle);
  FTC_STATUS FTC_IsDeviceNameLocationIDValid(LPSTR lpDeviceName, DWORD dwLocationID);

  FTC_STATUS FTC_IsDeviceFT2232CType(FT_DEVICE_LIST_INFO_NODE devInfo, LPBOOL lpbFT2232CTypeDevice);

public:
  FT2232c(void);
  ~FT2232c(void);

protected:
  FTC_STATUS FTC_GetNumDevices(LPDWORD lpdwNumDevices, FT2232CDeviceIndexes *FT2232CIndexes);
  FTC_STATUS FTC_GetNumNotOpenedDevices(LPDWORD lpdwNumNotOpenedDevices, FT2232CDeviceIndexes *FT2232CIndexes);
  FTC_STATUS FTC_GetDeviceNameLocationID(DWORD dwDeviceIndex, LPSTR lpDeviceName, DWORD dwBufferSize, LPDWORD lpdwLocationID);
  FTC_STATUS FTC_OpenSpecifiedDevice(LPSTR lpDeviceName, DWORD dwLocationID, FTC_HANDLE *pftHandle);
  FTC_STATUS FTC_OpenDevice(FTC_HANDLE *pftHandle);
  FTC_STATUS FTC_CloseDevice(FTC_HANDLE ftHandle);
  void       FTC_GetClockFrequencyValues(DWORD dwClockFrequencyValue, LPDWORD lpdwClockFrequencyHz);
  FTC_STATUS FTC_SetDeviceLoopbackState(FTC_HANDLE ftHandle, BOOL bLoopbackState);

  void       FTC_InsertDeviceHandle(LPSTR lpDeviceName, DWORD dwLocationID, FTC_HANDLE ftHandle);
  FTC_STATUS FTC_IsDeviceHandleValid(FTC_HANDLE ftHandle);
  void       FTC_RemoveDeviceHandle(FTC_HANDLE ftHandle);

  FTC_STATUS FTC_ResetUSBDevicePurgeUSBInputBuffer(FTC_HANDLE ftHandle);
  FTC_STATUS FTC_SetDeviceUSBBufferSizes(FTC_HANDLE ftHandle, DWORD InputBufferSize, DWORD OutputBufferSize);
  FTC_STATUS FTC_SetDeviceSpecialCharacters(FTC_HANDLE ftHandle, BOOLEAN bEventEnabled, UCHAR EventCharacter,
                                            BOOLEAN bErrorEnabled, UCHAR ErrorCharacter);
  FTC_STATUS FTC_SetReadWriteDeviceTimeouts(FTC_HANDLE ftHandle, DWORD dwReadTimeoutmSec, DWORD dwWriteTimeoutmSec);
  FTC_STATUS FTC_SetDeviceLatencyTimer(FTC_HANDLE ftHandle, BYTE LatencyTimermSec);
  FTC_STATUS FTC_GetDeviceLatencyTimer(FTC_HANDLE ftHandle, LPBYTE lpLatencyTimermSec);
  FTC_STATUS FTC_ResetMPSSEInterface(FTC_HANDLE ftHandle);
  FTC_STATUS FTC_EnableMPSSEInterface(FTC_HANDLE ftHandle);
  FTC_STATUS FTC_SendReceiveCommandFromMPSSEInterface(FTC_HANDLE ftHandle, BOOLEAN bSendEchoCommandContinuouslyOnce, BYTE EchoCommand, LPBOOL lpbCommandEchod);
  FTC_STATUS FTC_SynchronizeMPSSEInterface(FTC_HANDLE ftHandle);
  BOOLEAN    FTC_Timeout(SYSTEMTIME StartSystemTime, DWORD dwTimeoutmSecs);
  FTC_STATUS FTC_GetNumberBytesFromDeviceInputBuffer(FTC_HANDLE ftHandle, LPDWORD lpdwNumBytesDeviceInputBuffer);

  void       FTC_ClearOutputBuffer(void);
  DWORD      FTC_GetNumBytesInOutputBuffer(void);
  void       FTC_AddByteToOutputBuffer(DWORD dwOutputByte, BOOL bClearOutputBuffer);
  FTC_STATUS FTC_SendBytesToDevice(FTC_HANDLE ftHandle);
  FTC_STATUS FTC_ReadBytesFromDevice(FTC_HANDLE ftHandle, PInputByteBuffer InputBuffer,
                                     DWORD dwNumBytesToRead, LPDWORD lpdwNumBytesRead);
  FTC_STATUS FTC_ReadFixedNumBytesFromDevice(FTC_HANDLE ftHandle, PInputByteBuffer InputBuffer,
                                             DWORD dwNumBytesToRead, LPDWORD lpdwNumDataBytesRead);
  FTC_STATUS FTC_SendReadBytesToFromDevice(FTC_HANDLE ftHandle, PInputByteBuffer InputBuffer,
                                           DWORD dwNumBytesToRead, LPDWORD lpdwNumBytesRead);

  FTC_STATUS FTC_SendCommandsSequenceToDevice(FTC_HANDLE ftHandle);
  FTC_STATUS FTC_ReadCommandsSequenceBytesFromDevice(FTC_HANDLE ftHandle, PInputByteBuffer InputBuffer,
                                                     DWORD dwNumBytesToRead, LPDWORD lpdwNumBytesRead);
};

#endif  /* FT2232c_H */

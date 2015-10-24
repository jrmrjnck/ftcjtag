/*++

Copyright (c) 2008  Future Technology Devices International Ltd.

Module Name:

    ft2232hmpssejtag.h

Abstract:

    FT2232H and FT4232H Hi-Speed Dual Device Device and FT2232D Dual Device Class Declaration/Definition.

Environment:

    kernel & user mode

Revision History:

    07/02/05    kra     Created.
    24/08/05    kra     Added new function JTAG_GenerateClockPulses and new error code FTC_INVALID_NUMBER_CLOCK_PULSES
    16/09/05    kra     Version 1.50 - Added break statements after DLL_THREAD_ATTACH and DLL_THREAD_DETACH for multiple threaded applications
	  08/03/06	  ana		  Version 1.8 - fix byte boundry mising bit.
    23/05/08    kra     Version 1.9 - Modified FT2232.cpp to fix two bugs, if more than one device
                        is connected with the same device name, an invalid location id error is reported and changed
                        Sleep(0) to Sleep(1) to solve intermittent problem with FTC_Timeout failure
    11/07/08    kra     Renamed FT2232hMpsseJtag.h to ft2232hmpssejtag.h for FT2232H and FT4232H hi-speed devices
    17/07/08    kra     Version 2.0 - Added new functions for FT2232H and FT4232H hi-speed devices.
    19/08/08    kra     Added new function SetTCKTDITMSPinsCloseState.
    03/09/08    kra     Added CRITICAL_SECTION object.

--*/

#ifndef FT2232hMpsseJtag_H
#define FT2232hMpsseJtag_H

#include <windows.h>

#include "ftcjtag.h"

#include "FT2232h.h"

#define DEVICE_CHANNEL_A " A"
#define DEVICE_CHANNEL_B " B"

#define DLL_VERSION_NUM "2.0"

#define USB_INPUT_BUFFER_SIZE 65536   // 64K
#define USB_OUTPUT_BUFFER_SIZE 65536  // 64K

const BYTE FT_EVENT_VALUE = 0;
const BYTE FT_ERROR_VALUE = 0;

#define DEVICE_READ_TIMEOUT_INFINITE 0
#define DEVICE_WRITE_TIMEOUT 5000 // 5 seconds

#define MIN_CLOCK_DIVISOR 0     // equivalent to 30MHz for FT2232H and FT4232H hi-speed devices, equivalent to 6MHz for FT2232C device
#define MAX_CLOCK_DIVISOR 65535 // equivalent to 457Hz for FT2232H and FT4232H hi-speed devices, equivalent to 91Hz for FT2232C device

#define MIN_NUM_BITS 2       // specifies the minimum number of bits that can be written or read to/from an external device
#define MAX_NUM_BITS 524280  // specifies the maximum number of bits that can be written or read to/from an external device
#define MIN_NUM_BYTES 1      // specifies the minimum number of bytes that can be written to an external device
#define MAX_NUM_BYTES 65535  // specifies the maximum number of bytes that can be written to an external device

#define NUMBITSINBYTE 8

#define MIN_NUM_CLOCK_PULSES 1                      // specifies the minimum number of clock pulses that a FT2232C dual device or FT2232H hi-speed device or FT4232H hi-speed device can generate
#define MAX_NUM_CLOCK_PULSES 2000000000             // specifies the maximum number of clock pulses that a FT2232C dual device can generate
#define MAX_NUM_SINGLE_CLOCK_PULSES 8               // specifies the maximum number of clock pulses that a FT2232H hi-speed device or FT4232H hi-speed device can generate
#define MAX_NUM_TIMES_EIGHT_CLOCK_PULSES 250000000  // specifies the maximum number of clock pulses that a FT2232H hi-speed device or FT4232H hi-speed device can generate

#define NUM_BYTE_CLOCK_PULSES_BLOCK_SIZE 32000 //4000

#define PIN1_HIGH_VALUE  1
#define PIN2_HIGH_VALUE  2
#define PIN3_HIGH_VALUE  4
#define PIN4_HIGH_VALUE  8
#define PIN5_HIGH_VALUE  16
#define PIN6_HIGH_VALUE  32
#define PIN7_HIGH_VALUE  64
#define PIN8_HIGH_VALUE  128

#define NUM_WRITE_COMMAND_BYTES 18
#define NUM_READ_COMMAND_BYTES 18
#define NUM_WRITE_READ_COMMAND_BYTES 19

#define MAX_ERROR_MSG_SIZE 250

const CHAR ENGLISH[3] = "EN";

const CHAR EN_Common_Errors[FTC_INSUFFICIENT_RESOURCES + 1][MAX_ERROR_MSG_SIZE] = {
    "",
    "Invalid device handle.",
    "Device not found.",
    "Device not opened.",
    "General device IO error.",
    "Insufficient resources available to execute function."};

const CHAR EN_New_Errors[(FTC_INVALID_STATUS_CODE - FTC_FAILED_TO_COMPLETE_COMMAND) + 1][MAX_ERROR_MSG_SIZE] = {
    "Failed to complete command.",
    "Failed to synchronize the device MPSSE interface.",
    "Invalid device name index.",
    "Pointer to device name buffer is null.",
    "Buffer to contain device name is too small.",
    "Invalid device name.",
    "Invalid device location identifier.",
    "Device already in use by another application.",
    "More than one device detected.",

    "Pointer to channel buffer is null.",
    "Buffer to contain channel is too small.",
    "Invalid device channel. Valid values are A and B.",
    "Invalid latency timer value. Valid range is 2 - 255 milliseconds.",

    "Invalid clock divisor. Valid range is 0 - 65535.",
    "Pointer to input output buffer is null.",
    "Invalid number of bits. Valid range 2 to 524280. 524280 bits is equivalent to 64K bytes",
    "Pointer to write data buffer is null.",
    "Invalid size of write data buffer. Valid range is 1 - 65535",
    "Buffer to contain number of bits is too small.",
    "Invalid Test Access Port(TAP) controller state.",
    "Pointer to read data buffer is null.",
    "Command sequence buffer is full. Valid range is 1 - 131070 ie 128K bytes.",
    "Pointer to read command sequence data buffer is null.",
    "No command sequence found.",
    "Invalid number of clock pulses. Valid range is 1 - 2000,000,000.",
    "Invalid number of clock pulses. Valid range is 1 - 8.",
    "Invalid number of clock pulses. Valid range is 1 - 250,000,000.",
    "Pointer to final state buffer is null.",
    "Pointer to dll version number buffer is null.",
    "Buffer to contain dll version number is too small.",
    "Pointer to language code buffer is null.",
    "Pointer to error message buffer is null.",
    "Buffer to contain error message is too small.",
    "Unsupported language code.",
    "Unknown status code = "};

const BYTE CLK_DATA_BYTES_OUT_ON_NEG_CLK_LSB_FIRST_CMD = '\x19';
const BYTE CLK_DATA_BITS_OUT_ON_NEG_CLK_LSB_FIRST_CMD = '\x1B';
const BYTE CLK_DATA_BYTES_IN_ON_POS_CLK_LSB_FIRST_CMD = '\x28';
const BYTE CLK_DATA_BITS_IN_ON_POS_CLK_LSB_FIRST_CMD = '\x2A';
const BYTE CLK_DATA_BYTES_OUT_ON_NEG_CLK_IN_ON_POS_CLK_LSB_FIRST_CMD = '\x39';
const BYTE CLK_DATA_BITS_OUT_ON_NEG_CLK_IN_ON_POS_CLK_LSB_FIRST_CMD = '\x3B';

const BYTE CLK_DATA_TMS_NO_READ_CMD = '\x4B';
const BYTE CLK_DATA_TMS_READ_CMD = '\x6B';

const BYTE SET_LOW_BYTE_DATA_BITS_CMD = '\x80';
const BYTE GET_LOW_BYTE_DATA_BITS_CMD = '\x81';
const BYTE SET_HIGH_BYTE_DATA_BITS_CMD = '\x82';
const BYTE GET_HIGH_BYTE_DATA_BITS_CMD = '\x83';
const BYTE SET_CLOCK_FREQUENCY_CMD = '\x86';
const BYTE SEND_ANSWER_BACK_IMMEDIATELY_CMD = '\x87';

const BYTE CLK_FOR_NUM_CLOCKS_NO_DATA_BYTES_CMD = '\x8E';
const BYTE CLK_FOR_TIMES_EIGHT_CLOCKS_NO_DATA_BYTES_CMD = '\x8F';
const BYTE CLK_FOR_TIMES_EIGHT_CLOCKS_GPIOL2_HIGH_NO_DATA_BYTES_CMD = '\x98';
const BYTE CLK_FOR_TIMES_EIGHT_CLOCKS_GPIOL2_LOW_NO_DATA_BYTES_CMD = '\x99';


enum JtagStates {TestLogicReset, RunTestIdle, PauseDataRegister, PauseInstructionRegister, ShiftDataRegister, ShiftInstructionRegister, Undefined};

#define NUM_JTAG_TMS_STATES 6

// go from current JTAG state to new JTAG state ->                    tlr     rti     pdr     pir     sdr     sir
const BYTE TestLogicResetToNewJTAGState[NUM_JTAG_TMS_STATES]      = {'\x01', '\x00', '\x0A', '\x16', '\x02', '\x06'};
const BYTE RunTestIdleToNewJTAGState[NUM_JTAG_TMS_STATES]         = {'\x07', '\x00', '\x05', '\x0B', '\x01', '\x03'};
const BYTE PauseDataRegToNewJTAGState[NUM_JTAG_TMS_STATES]        = {'\x1F', '\x03', '\x17', '\x2F', '\x01', '\x0F'};
const BYTE PauseInstructionRegToNewJTAGState[NUM_JTAG_TMS_STATES] = {'\x1F', '\x03', '\x17', '\x2F', '\x07', '\x01'};
const BYTE ShiftDataRegToNewJTAGState[NUM_JTAG_TMS_STATES]        = {'\x1F', '\x03', '\x01', '\x2F', '\x00', '\x00'};
const BYTE ShiftInstructionRegToNewJTAGState[NUM_JTAG_TMS_STATES] = {'\x1F', '\x03', '\x17', '\x01', '\x00', '\x00'};

// number of TMS clocks to go from current JTAG state to new JTAG state ->       tlr rti pdr pir sdr sir
const BYTE TestLogicResetToNewJTAGStateNumTMSClocks[NUM_JTAG_TMS_STATES]      = {1,  1,  5,  6,  4,  5};
const BYTE RunTestIdleToNewJTAGStateNumTMSClocks[NUM_JTAG_TMS_STATES]         = {3,  5,  4,  5,  3,  4};
const BYTE PauseDataRegToNewJTAGStateNumTMSClocks[NUM_JTAG_TMS_STATES]        = {5,  3,  6,  7,  2,  6};
const BYTE PauseInstructionRegToNewJTAGStateNumTMSClocks[NUM_JTAG_TMS_STATES] = {5,  3,  6,  7,  5,  2};
const BYTE ShiftDataRegToNewJTAGStateNumTMSClocks[NUM_JTAG_TMS_STATES]        = {5,  3,  2,  7,  0,  0};
const BYTE ShiftInstructionRegToNewJTAGStateNumTMSClocks[NUM_JTAG_TMS_STATES] = {5,  4,  6,  2,  0,  0};

#define NO_LAST_DATA_BIT 0

#define NUM_COMMAND_SEQUENCE_READ_DATA_BYTES 2
#define INIT_COMMAND_SEQUENCE_READ_DATA_BUFFER_SIZE 100
#define COMMAND_SEQUENCE_READ_DATA_BUFFER_SIZE_INCREMENT 10

typedef DWORD ReadCommandSequenceData[NUM_COMMAND_SEQUENCE_READ_DATA_BYTES];
typedef ReadCommandSequenceData *PReadCommandSequenceData;

typedef PReadCommandSequenceData ReadCommandsSequenceData[1];
typedef ReadCommandsSequenceData *PReadCommandsSequenceData;

typedef struct Ft_Device_Cmd_Sequence_Data{
  DWORD hDevice;                                    // handle to the opened and initialized FT2232C dual type device
  DWORD dwNumBytesToSend;
  POutputByteBuffer pCommandsSequenceDataOutPutBuffer;
  DWORD dwSizeReadCommandsSequenceDataBuffer;
  PReadCommandsSequenceData pReadCommandsSequenceDataBuffer;
  DWORD dwNumReadCommandSequences;
}FTC_DEVICE_CMD_SEQUENCE_DATA, *PFTC_DEVICE_CMD_SEQUENCE_DATA;


//----------------------------------------------------------------------------
class FT2232hMpsseJtag : private FT2232h
{
private:
  // This object is used to restricted access to one thread, when a process/application has multiple 
  // threads running. The critical section object will ensure that only one public method in the DLL 
  // will be executed at a time.
  CRITICAL_SECTION threadAccess;

  DWORD dwSavedLowPinsDirection;
  DWORD dwSavedLowPinsValue;
  JtagStates CurrentJtagState;
  DWORD dwNumOpenedDevices;
  FTC_DEVICE_CMD_SEQUENCE_DATA OpenedDevicesCommandsSequenceData[MAX_NUM_DEVICES];
  INT iCommandsSequenceDataDeviceIndex;

  FTC_STATUS CheckWriteDataToExternalDeviceBitsBytesParameters(DWORD dwNumBitsToWrite, DWORD dwNumBytesToWrite);

  void       AddByteToOutputBuffer(DWORD dwOutputByte, BOOL bClearOutputBuffer);

  FTC_STATUS SetTCKTDITMSPinsCloseState(FTC_HANDLE ftHandle, PFTC_CLOSE_FINAL_STATE_PINS pCloseFinalStatePinsData);
  FTC_STATUS InitDevice(FTC_HANDLE ftHandle, DWORD dwClockDivisor);
  FTC_STATUS SetDataInOutClockFrequency(FTC_HANDLE ftHandle, DWORD dwClockDivisor);
  FTC_STATUS InitDataInOutClockFrequency(FTC_HANDLE ftHandle, DWORD dwClockDivisor);
  void       SetJTAGToNewState(DWORD dwNewJtagState, DWORD dwNumTmsClocks, BOOL bDoReadOperation);
  DWORD      MoveJTAGFromOneStateToAnother(JtagStates NewJtagState, DWORD dwLastDataBit, BOOL bDoReadOperation);
  FTC_STATUS ResetTAPContollerExternalDeviceSetToTestIdleMode(FTC_HANDLE ftHandle);
  FTC_STATUS SetGeneralPurposeLowerInputOutputPins(FTC_HANDLE ftHandle, PFTC_INPUT_OUTPUT_PINS pLowInputOutputPinsData);
  FTC_STATUS SetGeneralPurposeInputOutputPins(FTC_HANDLE ftHandle, BOOL bControlLowInputOutputPins,
                                              PFTC_INPUT_OUTPUT_PINS pLowInputOutputPinsData,
                                              BOOL bControlHighInputOutputPins,
                                              PFTC_INPUT_OUTPUT_PINS pHighInputOutputPinsData);
  FTC_STATUS SetHiSpeedDeviceGeneralPurposeInputOutputPins(FTC_HANDLE ftHandle, BOOL bControlLowInputOutputPins,
                                                           PFTC_INPUT_OUTPUT_PINS pLowInputOutputPinsData,
                                                           BOOL bControlHighInputOutputPins,
                                                           PFTH_INPUT_OUTPUT_PINS pHighInputOutputPinsData);
  void       GetGeneralPurposeInputOutputPinsInputStates(DWORD dwInputStatesReturnedValue, PFTC_LOW_HIGH_PINS pPinsInputData);
  FTC_STATUS GetGeneralPurposeLowerInputOutputPins(FTC_HANDLE ftHandle, PFTC_LOW_HIGH_PINS pLowPinsInputData);
  FTC_STATUS GetGeneralPurposeInputOutputPins(FTC_HANDLE ftHandle, BOOL bControlLowInputOutputPins,
                                              PFTC_LOW_HIGH_PINS pLowPinsInputData,
                                              BOOL bControlHighInputOutputPins,
                                              PFTC_LOW_HIGH_PINS pHighPinsInputData);
  void       GetHiSpeedDeviceGeneralPurposeInputOutputPinsInputStates(DWORD dwInputStatesReturnedValue, PFTH_LOW_HIGH_PINS pPinsInputData);
  FTC_STATUS GetHiSpeedDeviceGeneralPurposeInputOutputPins(FTC_HANDLE ftHandle, BOOL bControlLowInputOutputPins,
                                                           PFTC_LOW_HIGH_PINS pLowPinsInputData,
                                                           BOOL bControlHighInputOutputPins,
                                                           PFTH_LOW_HIGH_PINS pHighPinsInputData);
  void       AddWriteCommandDataToOutPutBuffer(BOOL bInstructionTestData, DWORD dwNumBitsToWrite,
                                               PWriteDataByteBuffer pWriteDataBuffer, DWORD dwNumBytesToWrite,
                                               DWORD dwTapControllerState);
  FTC_STATUS WriteDataToExternalDevice(FTC_HANDLE ftHandle, BOOL bInstructionData, DWORD dwNumBitsToWrite,
                                       PWriteDataByteBuffer pWriteDataBuffer, DWORD dwNumBytesToWrite,
                                       DWORD dwTapControllerState);
  void       GetNumDataBytesToRead(DWORD dwNumBitsToRead, LPDWORD lpdwNumDataBytesToRead, LPDWORD lpdwNumRemainingDataBits);
  FTC_STATUS GetDataFromExternalDevice(FTC_HANDLE ftHandle, DWORD dwNumBitsToRead, DWORD dwNumTmsClocks,
                                       PReadDataByteBuffer pReadDataBuffer, LPDWORD lpdwNumBytesReturned);
  DWORD      AddReadCommandToOutputBuffer(BOOL bInstructionTestData, DWORD dwNumBitsToRead, DWORD dwTapControllerState);
  FTC_STATUS ReadDataFromExternalDevice(FTC_HANDLE ftHandle, BOOL bInstructionTestData, DWORD dwNumBitsToRead,
                                        PReadDataByteBuffer pReadDataBuffer, LPDWORD lpdwNumBytesReturned,
                                        DWORD dwTapControllerState);
  DWORD      AddWriteReadCommandDataToOutPutBuffer(BOOL bInstructionTestData, DWORD dwNumBitsToWriteRead,
                                                   PWriteDataByteBuffer pWriteDataBuffer,
                                                   DWORD dwNumBytesToWrite, DWORD dwTapControllerState);
  FTC_STATUS WriteReadDataToFromExternalDevice(FTC_HANDLE ftHandle, BOOL bInstructionTestData, DWORD dwNumBitsToWriteRead,
                                               PWriteDataByteBuffer pWriteDataBuffer, DWORD dwNumBytesToWrite,
                                               PReadDataByteBuffer pReadDataBuffer, LPDWORD lpdwNumBytesReturned,
                                               DWORD dwTapControllerState);
  FTC_STATUS GenerateTCKClockPulses(FTC_HANDLE ftHandle, DWORD dwNumClockPulses);
  FTC_STATUS GenerateClockPulsesHiSpeedDevice(FTC_HANDLE ftHandle, BOOL bPulseClockTimesEightFactor, DWORD dwNumClockPulses, BOOL bControlLowInputOutputPin, BOOL bStopClockPulsesState);

  void       ProcessReadCommandsSequenceBytes(PInputByteBuffer pInputBuffer, DWORD dwNumBytesRead, PReadCmdSequenceDataByteBuffer pReadCmdSequenceDataBuffer,
                                              LPDWORD lpdwNumBytesReturned);
  DWORD      GetTotalNumCommandsSequenceDataBytesToRead (void);
  void       CopyReadCommandsSequenceDataBuffer(PReadCommandsSequenceData pDestinationBuffer, PReadCommandsSequenceData pSourceBuffer, DWORD dwSizeReadCommandsSequenceDataBuffer);
  FTC_STATUS AddReadCommandSequenceData(DWORD dwNumBitsToRead, DWORD dwNumTmsClocks);
  void       CreateReadCommandsSequenceDataBuffer(void);
  PReadCommandsSequenceData CreateReadCommandsSequenceDataBuffer(DWORD dwSizeReadCmdsSequenceDataBuffer);
  void       DeleteReadCommandsSequenceDataBuffer(PReadCommandsSequenceData pReadCmdsSequenceDataBuffer, DWORD dwSizeReadCommandsSequenceDataBuffer);

  FTC_STATUS CreateDeviceCommandsSequenceDataBuffers(FTC_HANDLE ftHandle);
  void       ClearDeviceCommandSequenceData(FTC_HANDLE ftHandle);
  DWORD      GetNumBytesInCommandsSequenceDataBuffer(void);
  DWORD      GetCommandsSequenceDataDeviceIndex(FTC_HANDLE ftHandle);
  void       DeleteDeviceCommandsSequenceDataBuffers(FTC_HANDLE ftHandle);

  FTC_STATUS AddDeviceWriteCommand(FTC_HANDLE ftHandle, BOOL bInstructionTestData, DWORD dwNumBitsToWrite,
                                   PWriteDataByteBuffer pWriteDataBuffer, DWORD dwNumBytesToWrite,
                                   DWORD dwTapControllerState);
  FTC_STATUS AddDeviceReadCommand(FTC_HANDLE ftHandle, BOOL bInstructionTestData, DWORD dwNumBitsToRead, DWORD dwTapControllerState);
  FTC_STATUS AddDeviceWriteReadCommand(FTC_HANDLE ftHandle, BOOL bInstructionTestData, DWORD dwNumBitsToWriteRead,
                                       PWriteDataByteBuffer pWriteDataBuffer, DWORD dwNumBytesToWrite,
                                       DWORD dwTapControllerState);

public:
  FT2232hMpsseJtag(void);
  ~FT2232hMpsseJtag(void);

  FTC_STATUS WINAPI JTAG_GetNumDevices(LPDWORD lpdwNumDevices);
  FTC_STATUS WINAPI JTAG_GetNumHiSpeedDevices(LPDWORD lpdwNumHiSpeedDevices);
  FTC_STATUS WINAPI JTAG_GetDeviceNameLocationID(DWORD dwDeviceNameIndex, LPSTR lpDeviceNameBuffer, DWORD dwBufferSize, LPDWORD lpdwLocationID);
  FTC_STATUS WINAPI JTAG_GetHiSpeedDeviceNameLocationIDChannel(DWORD dwDeviceNameIndex, LPSTR lpDeviceNameBuffer, DWORD dwBufferSize, LPDWORD lpdwLocationID, LPSTR lpChannel, DWORD dwChannelBufferSize, LPDWORD lpdwHiSpeedDeviceType);
  FTC_STATUS WINAPI JTAG_OpenDevice(FTC_HANDLE *pftHandle);
  FTC_STATUS WINAPI JTAG_OpenSpecifiedDevice(LPSTR lpDeviceName, DWORD dwLocationID, FTC_HANDLE *pftHandle);
  FTC_STATUS WINAPI JTAG_OpenSpecifiedHiSpeedDevice(LPSTR lpDeviceName, DWORD dwLocationID, LPSTR lpChannel, FTC_HANDLE *pftHandle);
  FTC_STATUS WINAPI JTAG_GetHiSpeedDeviceType(FTC_HANDLE ftHandle, LPDWORD lpdwHiSpeedDeviceType);
  FTC_STATUS WINAPI JTAG_CloseDevice(FTC_HANDLE ftHandle);
  FTC_STATUS WINAPI JTAG_CloseDevice(FTC_HANDLE ftHandle, PFTC_CLOSE_FINAL_STATE_PINS pCloseFinalStatePinsData);
  FTC_STATUS WINAPI JTAG_InitDevice(FTC_HANDLE ftHandle, DWORD dwClockDivisor);
  FTC_STATUS WINAPI JTAG_TurnOnDivideByFiveClockingHiSpeedDevice(FTC_HANDLE ftHandle);
  FTC_STATUS WINAPI JTAG_TurnOffDivideByFiveClockingHiSpeedDevice(FTC_HANDLE ftHandle);
  FTC_STATUS WINAPI JTAG_TurnOnAdaptiveClockingHiSpeedDevice(FTC_HANDLE ftHandle);
  FTC_STATUS WINAPI JTAG_TurnOffAdaptiveClockingHiSpeedDevice(FTC_HANDLE ftHandle);
  FTC_STATUS WINAPI JTAG_SetDeviceLatencyTimer(FTC_HANDLE ftHandle, BYTE LatencyTimermSec);
  FTC_STATUS WINAPI JTAG_GetDeviceLatencyTimer(FTC_HANDLE ftHandle, LPBYTE lpLatencyTimermSec);
  FTC_STATUS WINAPI JTAG_GetClock(DWORD dwClockDivisor, LPDWORD lpdwClockFrequencyHz);
  FTC_STATUS WINAPI JTAG_GetHiSpeedDeviceClock(DWORD dwClockDivisor, LPDWORD lpdwClockFrequencyHz);
  FTC_STATUS WINAPI JTAG_SetClock(FTC_HANDLE ftHandle, DWORD dwClockDivisor, LPDWORD lpdwClockFrequencyHz);
  FTC_STATUS WINAPI JTAG_SetDeviceLoopbackState(FTC_HANDLE ftHandle, BOOL bLoopbackState);
  FTC_STATUS WINAPI JTAG_SetGeneralPurposeInputOutputPins(FTC_HANDLE ftHandle, BOOL bControlLowInputOutputPins,
                                                         PFTC_INPUT_OUTPUT_PINS pLowInputOutputPinsData,
                                                         BOOL bControlHighInputOutputPins,
                                                         PFTC_INPUT_OUTPUT_PINS pHighInputOutputPinsData);
  FTC_STATUS WINAPI JTAG_SetHiSpeedDeviceGeneralPurposeInputOutputPins(FTC_HANDLE ftHandle, BOOL bControlLowInputOutputPins,
                                                                       PFTC_INPUT_OUTPUT_PINS pLowInputOutputPinsData,
                                                                       BOOL bControlHighInputOutputPins,
                                                                       PFTH_INPUT_OUTPUT_PINS pHighInputOutputPinsData);
  FTC_STATUS WINAPI JTAG_GetGeneralPurposeInputOutputPins(FTC_HANDLE ftHandle, BOOL bControlLowInputOutputPins,
                                                          PFTC_LOW_HIGH_PINS pLowPinsInputData,
                                                          BOOL bControlHighInputOutputPins,
                                                          PFTC_LOW_HIGH_PINS pHighPinsInputData);
  FTC_STATUS WINAPI JTAG_GetHiSpeedDeviceGeneralPurposeInputOutputPins(FTC_HANDLE ftHandle, BOOL bControlLowInputOutputPins,
                                                                       PFTC_LOW_HIGH_PINS pLowPinsInputData,
                                                                       BOOL bControlHighInputOutputPins,
                                                                       PFTH_LOW_HIGH_PINS pHighPinsInputData);
  FTC_STATUS WINAPI JTAG_WriteDataToExternalDevice(FTC_HANDLE ftHandle, BOOL bInstructionTestData, DWORD dwNumBitsToWrite,
                                                   PWriteDataByteBuffer pWriteDataBuffer, DWORD dwNumBytesToWrite,
                                                   DWORD dwTapControllerState);
  FTC_STATUS WINAPI JTAG_ReadDataFromExternalDevice(FTC_HANDLE ftHandle, BOOL bInstructionTestData, DWORD dwNumBitsToRead,
                                                    PReadDataByteBuffer pReadDataBuffer, LPDWORD lpdwNumBytesReturned,
                                                    DWORD dwTapControllerState);
  FTC_STATUS WINAPI JTAG_WriteReadDataToFromExternalDevice(FTC_HANDLE ftHandle, BOOL bInstructionTestData, DWORD dwNumBitsToWriteRead,
                                                           PWriteDataByteBuffer pWriteDataBuffer, DWORD dwNumBytesToWrite,
                                                           PReadDataByteBuffer pReadDataBuffer, LPDWORD lpdwNumBytesReturned,
                                                           DWORD dwTapControllerState);
  FTC_STATUS WINAPI JTAG_GenerateTCKClockPulses(FTC_HANDLE ftHandle, DWORD dwNumClockPulses);
  FTC_STATUS WINAPI JTAG_GenerateClockPulsesHiSpeedDevice(FTC_HANDLE ftHandle, BOOL bPulseClockTimesEightFactor, DWORD dwNumClockPulses, BOOL bControlLowInputOutputPin, BOOL bStopClockPulsesState);
  FTC_STATUS WINAPI JTAG_ClearCommandSequence(void);
  FTC_STATUS WINAPI JTAG_AddWriteCommand(BOOL bInstructionTestData, DWORD dwNumBitsToWrite,
                                         PWriteDataByteBuffer pWriteDataBuffer, DWORD dwNumBytesToWrite,
                                         DWORD dwTapControllerState);
  FTC_STATUS WINAPI JTAG_AddReadCommand(BOOL bInstructionTestData, DWORD dwNumBitsToRead, DWORD dwTapControllerState);
  FTC_STATUS WINAPI JTAG_AddWriteReadCommand(BOOL bInstructionTestData, DWORD dwNumBitsToWriteRead,
                                             PWriteDataByteBuffer pWriteDataBuffer, DWORD dwNumBytesToWrite,
                                             DWORD dwTapControllerState);
  FTC_STATUS WINAPI JTAG_ClearDeviceCommandSequence(FTC_HANDLE ftHandle);
  FTC_STATUS WINAPI JTAG_AddDeviceWriteCommand(FTC_HANDLE ftHandle, BOOL bInstructionTestData, DWORD dwNumBitsToWrite,
                                               PWriteDataByteBuffer pWriteDataBuffer, DWORD dwNumBytesToWrite,
                                               DWORD dwTapControllerState);
  FTC_STATUS WINAPI JTAG_AddDeviceReadCommand(FTC_HANDLE ftHandle, BOOL bInstructionTestData, DWORD dwNumBitsToRead, DWORD dwTapControllerState);
  FTC_STATUS WINAPI JTAG_AddDeviceWriteReadCommand(FTC_HANDLE ftHandle, BOOL bInstructionTestData, DWORD dwNumBitsToWriteRead,
                                                   PWriteDataByteBuffer pWriteDataBuffer, DWORD dwNumBytesToWrite,
                                                   DWORD dwTapControllerState);
  FTC_STATUS WINAPI JTAG_ExecuteCommandSequence(FTC_HANDLE ftHandle, PReadCmdSequenceDataByteBuffer pReadCmdSequenceDataBuffer,
                                                LPDWORD lpdwNumBytesReturned);
  FTC_STATUS WINAPI JTAG_GetDllVersion(LPSTR lpDllVersionBuffer, DWORD dwBufferSize);
  FTC_STATUS WINAPI JTAG_GetErrorCodeString(LPSTR lpLanguage, FTC_STATUS StatusCode,
                                            LPSTR lpErrorMessageBuffer, DWORD dwBufferSize);
};

#endif  /* FT2232hMpsseJtag_H */

/*++

Copyright (c) 2008  Future Technology Devices International Ltd.

Module Name:

    FT2232hMpsseJtag.cpp

Abstract:

    FT2232H and FT4232H Hi-Speed Dual Device Device and FT2232D Dual Device Class Implementation.

Environment:

    kernel & user mode

Revision History:

    07/02/05    kra     Created.
    24/08/05    kra     Added new function JTAG_GenerateClockPulses and new error code FTC_INVALID_NUMBER_CLOCK_PULSES
    11/07/08    kra     Renamed FT2232hMpsseJtag.cpp to ft2232hmpssejtag.cpp for FT2232H and FT4232H hi-speed devices
    17/07/08    kra     Added new functions for FT2232H and FT4232H hi-speed devices.
    01/08/08    kra     Modified JTAG_GetHiSpeedDeviceNameLocationIDChannel and JTAG_GetHiSpeedDeviceType methods,
                        to determine hi-speed device type using FT2232H_DEVICE_TYPE and FT2232H_DEVICE_TYPE enum
                        values defined in ftcjtag.h.
    19/08/08    kra     Added new function SetTCKTDITMSPinsCloseState.
    03/09/08    kra     Added critical sections to every public method, to ensure that only one public method in the
                        DLL will be executed at a time, when a process/application has multiple threads running.

--*/

#define WIO_DEFINED

#include "FT2232hMpsseJtag.h"
#include "FtcJtagInternal.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

FTC_STATUS FT2232hMpsseJtag::CheckWriteDataToExternalDeviceBitsBytesParameters(DWORD dwNumBitsToWrite, DWORD dwNumBytesToWrite)
{
  FTC_STATUS Status = FTC_SUCCESS;
  DWORD dwNumBytesForNumBits = 0;

  if (((dwNumBitsToWrite >= MIN_NUM_BITS) && (dwNumBitsToWrite <= MAX_NUM_BITS)) &&
      ((dwNumBytesToWrite >= MIN_NUM_BYTES) && (dwNumBytesToWrite <= MAX_NUM_BYTES)))
  {
    dwNumBytesForNumBits = (dwNumBitsToWrite / NUMBITSINBYTE);

    if ((dwNumBitsToWrite % NUMBITSINBYTE) > 0)
      dwNumBytesForNumBits = dwNumBytesForNumBits + 1;

    if (dwNumBytesForNumBits > dwNumBytesToWrite)
      Status = FTC_NUMBER_BYTES_TOO_SMALL;
  }
  else
  {
    if ((dwNumBitsToWrite < MIN_NUM_BITS) || (dwNumBitsToWrite > MAX_NUM_BITS))
      Status = FTC_INVALID_NUMBER_BITS;
    else
      Status = FTC_INVALID_NUMBER_BYTES;
  }

  return Status;
}

void FT2232hMpsseJtag::AddByteToOutputBuffer(DWORD dwOutputByte, BOOL bClearOutputBuffer)
{
  DWORD dwNumBytesToSend = 0;

  if (iCommandsSequenceDataDeviceIndex == -1)
    FTC_AddByteToOutputBuffer(dwOutputByte, bClearOutputBuffer);
  else
  {
    // This is used when you are building up a sequence of commands ie write, read and write/read
    dwNumBytesToSend = OpenedDevicesCommandsSequenceData[iCommandsSequenceDataDeviceIndex].dwNumBytesToSend;

    (*OpenedDevicesCommandsSequenceData[iCommandsSequenceDataDeviceIndex].pCommandsSequenceDataOutPutBuffer)[dwNumBytesToSend] = (dwOutputByte & '\xFF');

    dwNumBytesToSend = dwNumBytesToSend + 1;

    OpenedDevicesCommandsSequenceData[iCommandsSequenceDataDeviceIndex].dwNumBytesToSend = dwNumBytesToSend;
  }
}

FTC_STATUS FT2232hMpsseJtag::SetTCKTDITMSPinsCloseState(FTC_HANDLE ftHandle, PFTC_CLOSE_FINAL_STATE_PINS pCloseFinalStatePinsData)
{
  FTC_STATUS Status = FTC_SUCCESS;

  if ((pCloseFinalStatePinsData->bTCKPinState != FALSE) ||  
      (pCloseFinalStatePinsData->bTDIPinState != FALSE) ||
      (pCloseFinalStatePinsData->bTMSPinState != FALSE)) {
    AddByteToOutputBuffer(SET_LOW_BYTE_DATA_BITS_CMD, true);

    if (pCloseFinalStatePinsData->bTCKPinState != FALSE) {
      if (pCloseFinalStatePinsData->bTCKPinActiveState != FALSE)
        dwSavedLowPinsValue = (dwSavedLowPinsValue | '\x01'); // Set TCK pin high
      else
        dwSavedLowPinsValue = (dwSavedLowPinsValue & '\xFE'); // Set TCK pin low

      dwSavedLowPinsDirection = (dwSavedLowPinsDirection | '\x01'); // Ensure TCK pin is set to output
    }

    if (pCloseFinalStatePinsData->bTDIPinState != FALSE) {
      if (pCloseFinalStatePinsData->bTDIPinActiveState != FALSE)
        dwSavedLowPinsValue = (dwSavedLowPinsValue | '\x02'); // Set TDI pin high
      else
        dwSavedLowPinsValue = (dwSavedLowPinsValue & '\xFD'); // Set TDI pin low

      dwSavedLowPinsDirection = (dwSavedLowPinsDirection | '\x02'); // Ensure TDI pin is set to output
    }

    if (pCloseFinalStatePinsData->bTMSPinState != FALSE) {
      if (pCloseFinalStatePinsData->bTMSPinActiveState != FALSE)
        dwSavedLowPinsValue = (dwSavedLowPinsValue | '\x08'); // Set TMS pin high
      else
        dwSavedLowPinsValue = (dwSavedLowPinsValue & '\xF7'); // Set TMS pin low

      dwSavedLowPinsDirection = (dwSavedLowPinsDirection | '\x08'); // Ensure TMS pin is set to output
    }
    
    AddByteToOutputBuffer(dwSavedLowPinsValue, false);
    AddByteToOutputBuffer(dwSavedLowPinsDirection, false);

    Status = FTC_SendBytesToDevice(ftHandle);
  }

  return Status;
}


FTC_STATUS FT2232hMpsseJtag::InitDevice(FTC_HANDLE ftHandle, DWORD dwClockDivisor)
{
  FTC_STATUS Status = FTC_SUCCESS;

  if ((dwClockDivisor >= MIN_CLOCK_DIVISOR) && (dwClockDivisor <= MAX_CLOCK_DIVISOR))
  {
    Status = FTC_ResetUSBDevicePurgeUSBInputBuffer(ftHandle);

    if (Status == FTC_SUCCESS)
      Status = FTC_SetDeviceUSBBufferSizes(ftHandle, USB_INPUT_BUFFER_SIZE, USB_OUTPUT_BUFFER_SIZE);

    if (Status == FTC_SUCCESS)
      Status = FTC_SetDeviceSpecialCharacters(ftHandle, false, FT_EVENT_VALUE, false, FT_ERROR_VALUE);

    if (Status == FTC_SUCCESS)
      Status = FTC_SetReadWriteDeviceTimeouts(ftHandle, DEVICE_READ_TIMEOUT_INFINITE, DEVICE_WRITE_TIMEOUT);

    if (Status == FTC_SUCCESS)
      Status = FTC_SetDeviceLatencyTimer(ftHandle, DEVICE_LATENCY_TIMER_VALUE);

    if (Status == FTC_SUCCESS)
      Status = FTC_ResetMPSSEInterface(ftHandle);

    if (Status == FTC_SUCCESS)
      Status = FTC_EnableMPSSEInterface(ftHandle);

    if (Status == FTC_SUCCESS)
      Status = FTC_SynchronizeMPSSEInterface(ftHandle);

    if (Status == FTC_SUCCESS)
      Status = FTC_ResetUSBDevicePurgeUSBInputBuffer(ftHandle);

    if (Status == FTC_SUCCESS)
      Sleep(20); // wait for all the USB stuff to complete

    if (Status == FTC_SUCCESS)
      Status = InitDataInOutClockFrequency(ftHandle, dwClockDivisor);

    if (Status == FTC_SUCCESS)
      Status = FTC_SetDeviceLoopbackState(ftHandle, false);

    if (Status == FTC_SUCCESS)
      Sleep(20); // wait for all the USB stuff to complete

    if (Status == FTC_SUCCESS)
      Status = FTC_ResetUSBDevicePurgeUSBInputBuffer(ftHandle);

    if (Status == FTC_SUCCESS)
      Sleep(20); // wait for all the USB stuff to complete

    if (Status == FTC_SUCCESS)
      Status = ResetTAPContollerExternalDeviceSetToTestIdleMode(ftHandle);
  }
  else
    Status = FTC_INVALID_CLOCK_DIVISOR;

  return Status;
}

FTC_STATUS FT2232hMpsseJtag::SetDataInOutClockFrequency(FTC_HANDLE ftHandle, DWORD dwClockDivisor)
{
  FTC_STATUS Status = FTC_SUCCESS;
  
  AddByteToOutputBuffer(SET_CLOCK_FREQUENCY_CMD, true);
  AddByteToOutputBuffer(dwClockDivisor, false);
  AddByteToOutputBuffer((dwClockDivisor >> 8), false);

  Status = FTC_SendBytesToDevice(ftHandle);

  return Status;
}

FTC_STATUS FT2232hMpsseJtag::InitDataInOutClockFrequency(FTC_HANDLE ftHandle, DWORD dwClockDivisor)
{
  FTC_STATUS Status = FTC_SUCCESS;

  // set general purpose I/O low pins 1-4 all to input except TDO
  AddByteToOutputBuffer(SET_LOW_BYTE_DATA_BITS_CMD, true);

  dwSavedLowPinsValue = (dwSavedLowPinsValue & '\xF0');
  dwSavedLowPinsValue = (dwSavedLowPinsValue | '\x08'); // TDI,TCK start low
  AddByteToOutputBuffer(dwSavedLowPinsValue, false);

  dwSavedLowPinsDirection = (dwSavedLowPinsDirection & '\xF0');
  dwSavedLowPinsDirection = (dwSavedLowPinsDirection | '\x0B');
  AddByteToOutputBuffer(dwSavedLowPinsDirection, false);

  // set general purpose I/O high pins 1-4 all to input
  AddByteToOutputBuffer(SET_HIGH_BYTE_DATA_BITS_CMD, false);
  AddByteToOutputBuffer(0, false);
  AddByteToOutputBuffer(0, false);

  Status = FTC_SendBytesToDevice(ftHandle);
  
  if (Status == FTC_SUCCESS)
    SetDataInOutClockFrequency(ftHandle, dwClockDivisor);

  return Status;
}

// This procedure sets the JTAG to a new state
void FT2232hMpsseJtag::SetJTAGToNewState(DWORD dwNewJtagState, DWORD dwNumTmsClocks, BOOL bDoReadOperation)
{
  if ((dwNumTmsClocks >= 1) && (dwNumTmsClocks <= 7))
  {
    if (bDoReadOperation == TRUE)
      AddByteToOutputBuffer(CLK_DATA_TMS_READ_CMD, false);
    else
      AddByteToOutputBuffer(CLK_DATA_TMS_NO_READ_CMD, false);

    AddByteToOutputBuffer(((dwNumTmsClocks - 1) & '\xFF'), false);
    AddByteToOutputBuffer((dwNewJtagState & '\xFF'), false);
  }
}

// This function returns the number of TMS clocks to work out the last bit of TDO
DWORD FT2232hMpsseJtag::MoveJTAGFromOneStateToAnother(JtagStates NewJtagState, DWORD dwLastDataBit, BOOL bDoReadOperation)
{
  DWORD dwNumTmsClocks = 0;

  if (CurrentJtagState == Undefined)
  {
    SetJTAGToNewState('\x7F', 7, false);
    CurrentJtagState = TestLogicReset;
  }

  switch (CurrentJtagState)
  {
    case TestLogicReset:
      dwNumTmsClocks = TestLogicResetToNewJTAGStateNumTMSClocks[NewJtagState];
      SetJTAGToNewState((TestLogicResetToNewJTAGState[NewJtagState] | (dwLastDataBit << 7)), dwNumTmsClocks, bDoReadOperation);
    break;
    case RunTestIdle:
      dwNumTmsClocks = RunTestIdleToNewJTAGStateNumTMSClocks[NewJtagState];
      SetJTAGToNewState((RunTestIdleToNewJTAGState[NewJtagState] | (dwLastDataBit << 7)), dwNumTmsClocks, bDoReadOperation);
    break;
    case PauseDataRegister:
      dwNumTmsClocks = PauseDataRegToNewJTAGStateNumTMSClocks[NewJtagState];
      SetJTAGToNewState((PauseDataRegToNewJTAGState[NewJtagState] | (dwLastDataBit << 7)), dwNumTmsClocks, bDoReadOperation);
    break;
    case PauseInstructionRegister:
      dwNumTmsClocks = PauseInstructionRegToNewJTAGStateNumTMSClocks[NewJtagState];
      SetJTAGToNewState((PauseInstructionRegToNewJTAGState[NewJtagState] | (dwLastDataBit << 7)), dwNumTmsClocks, bDoReadOperation);
    break;
    case ShiftDataRegister:
      dwNumTmsClocks = ShiftDataRegToNewJTAGStateNumTMSClocks[NewJtagState];
      SetJTAGToNewState((ShiftDataRegToNewJTAGState[NewJtagState] | (dwLastDataBit << 7)), dwNumTmsClocks, bDoReadOperation);
    break;
    case ShiftInstructionRegister:
      dwNumTmsClocks = ShiftInstructionRegToNewJTAGStateNumTMSClocks[NewJtagState];
      SetJTAGToNewState((ShiftInstructionRegToNewJTAGState[NewJtagState] | (dwLastDataBit << 7)), dwNumTmsClocks, bDoReadOperation);
    break;
  }

  CurrentJtagState = NewJtagState;

  return dwNumTmsClocks;
}

FTC_STATUS FT2232hMpsseJtag::ResetTAPContollerExternalDeviceSetToTestIdleMode(FTC_HANDLE ftHandle)
{
  FTC_STATUS Status = FTC_SUCCESS;

  // set I/O low bits all out except TDO
  AddByteToOutputBuffer(SET_LOW_BYTE_DATA_BITS_CMD, true);

  dwSavedLowPinsValue = (dwSavedLowPinsValue & '\xFF');
  // TDI,TCK start low
  dwSavedLowPinsValue = (dwSavedLowPinsValue | '\x08');
  AddByteToOutputBuffer(dwSavedLowPinsValue, false);

  dwSavedLowPinsDirection = (dwSavedLowPinsDirection & '\xFF');
  dwSavedLowPinsDirection = (dwSavedLowPinsDirection | '\x0B');
  AddByteToOutputBuffer(dwSavedLowPinsDirection, false);

  //MoveJTAGFromOneStateToAnother(TestLogicReset, 1, false);JtagStates
  MoveJTAGFromOneStateToAnother(Undefined, 1, false);

  MoveJTAGFromOneStateToAnother(RunTestIdle, NO_LAST_DATA_BIT, FALSE);

  Status = FTC_SendBytesToDevice(ftHandle);
  
  return Status;
}

FTC_STATUS FT2232hMpsseJtag::SetGeneralPurposeLowerInputOutputPins(FTC_HANDLE ftHandle, PFTC_INPUT_OUTPUT_PINS pLowInputOutputPinsData)
{
  DWORD dwLowPinsDirection = 0;
  DWORD dwLowPinsValue = 0;

  if (pLowInputOutputPinsData->bPin1InputOutputState != FALSE)
    dwLowPinsDirection = (dwLowPinsDirection | '\x01');
  if (pLowInputOutputPinsData->bPin2InputOutputState != FALSE)
    dwLowPinsDirection = (dwLowPinsDirection | '\x02');
  if (pLowInputOutputPinsData->bPin3InputOutputState != FALSE)
    dwLowPinsDirection = (dwLowPinsDirection | '\x04');
  if (pLowInputOutputPinsData->bPin4InputOutputState != FALSE)
    dwLowPinsDirection = (dwLowPinsDirection | '\x08');

  if (pLowInputOutputPinsData->bPin1LowHighState != FALSE)
    dwLowPinsValue = (dwLowPinsValue | '\x01');
  if (pLowInputOutputPinsData->bPin2LowHighState != FALSE)
    dwLowPinsValue = (dwLowPinsValue | '\x02');
  if (pLowInputOutputPinsData->bPin3LowHighState != FALSE)
    dwLowPinsValue = (dwLowPinsValue | '\x04');
  if (pLowInputOutputPinsData->bPin4LowHighState != FALSE)
    dwLowPinsValue = (dwLowPinsValue | '\x08');

  // output on the general purpose I/O low pins 1-4
  AddByteToOutputBuffer(SET_LOW_BYTE_DATA_BITS_CMD, TRUE);

  // shift left by 4 bits ie move general purpose I/O low pins 1-4 from bits 0-3 to bits 4-7
  dwLowPinsValue = ((dwLowPinsValue & '\x0F') << 4);

  dwSavedLowPinsValue = (dwSavedLowPinsValue & '\x0F');
  dwSavedLowPinsValue = (dwSavedLowPinsValue | dwLowPinsValue);
  AddByteToOutputBuffer(dwSavedLowPinsValue, FALSE);

  // shift left by 4 bits ie move general purpose I/O low pins 1-4 from bits 0-3 to bits 4-7
  dwLowPinsDirection = ((dwLowPinsDirection & '\x0F') << 4);

  dwSavedLowPinsDirection = (dwSavedLowPinsDirection & '\x0F');
  dwSavedLowPinsDirection = (dwSavedLowPinsDirection | dwLowPinsDirection); 
  AddByteToOutputBuffer(dwSavedLowPinsDirection, FALSE);

  return FTC_SendBytesToDevice(ftHandle);
}

FTC_STATUS FT2232hMpsseJtag::SetGeneralPurposeInputOutputPins(FTC_HANDLE ftHandle, BOOL bControlLowInputOutputPins,
                                                              PFTC_INPUT_OUTPUT_PINS pLowInputOutputPinsData,
                                                              BOOL bControlHighInputOutputPins,
                                                              PFTC_INPUT_OUTPUT_PINS pHighInputOutputPinsData)
{
  FTC_STATUS Status = FTC_SUCCESS;
  BOOL bHiSpeedTypeDevice = FALSE;
  BOOL bHiSpeedFT2232HTDeviceype = FALSE;
  DWORD dwHighPinsDirection = 0;
  DWORD dwHighPinsValue = 0;

  if (bControlLowInputOutputPins != FALSE)
    Status = SetGeneralPurposeLowerInputOutputPins(ftHandle, pLowInputOutputPinsData);

  if (Status == FTC_SUCCESS)
  {
    if (bControlHighInputOutputPins != FALSE)
    {
      if (((Status = FTC_IsDeviceHiSpeedType(ftHandle, &bHiSpeedTypeDevice)) == FTC_SUCCESS) &&
          ((Status = FTC_IsDeviceHiSpeedFT2232HType(ftHandle, &bHiSpeedFT2232HTDeviceype)) == FTC_SUCCESS))
      {
        // If the device is not a hi-speed device or is a FT2232H hi-speed device
        if ((bHiSpeedTypeDevice == FALSE) || ((bHiSpeedTypeDevice == TRUE) && (bHiSpeedFT2232HTDeviceype == TRUE)))
        {
          if (pHighInputOutputPinsData->bPin1InputOutputState != FALSE)
            dwHighPinsDirection = (dwHighPinsDirection | '\x01');
          if (pHighInputOutputPinsData->bPin2InputOutputState != FALSE)
            dwHighPinsDirection = (dwHighPinsDirection | '\x02');
          if (pHighInputOutputPinsData->bPin3InputOutputState != FALSE)
            dwHighPinsDirection = (dwHighPinsDirection | '\x04');
          if (pHighInputOutputPinsData->bPin4InputOutputState != FALSE)
            dwHighPinsDirection = (dwHighPinsDirection | '\x08');

          if (pHighInputOutputPinsData->bPin1LowHighState != FALSE)
            dwHighPinsValue = (dwHighPinsValue | '\x01');
          if (pHighInputOutputPinsData->bPin2LowHighState != FALSE)
            dwHighPinsValue = (dwHighPinsValue | '\x02');
          if (pHighInputOutputPinsData->bPin3LowHighState != FALSE)
            dwHighPinsValue = (dwHighPinsValue | '\x04');
          if (pHighInputOutputPinsData->bPin4LowHighState != FALSE)
            dwHighPinsValue = (dwHighPinsValue | '\x08');

          // output on the general purpose I/O high pins 1-4
          AddByteToOutputBuffer(SET_HIGH_BYTE_DATA_BITS_CMD, TRUE);

          dwHighPinsValue = (dwHighPinsValue & '\x0F');
          AddByteToOutputBuffer(dwHighPinsValue, FALSE);

          dwHighPinsDirection = (dwHighPinsDirection & '\x0F');
          AddByteToOutputBuffer(dwHighPinsDirection, FALSE);

          Status = FTC_SendBytesToDevice(ftHandle);
        }
      }
    }
  }

  return Status;
}

FTC_STATUS FT2232hMpsseJtag::SetHiSpeedDeviceGeneralPurposeInputOutputPins(FTC_HANDLE ftHandle, BOOL bControlLowInputOutputPins,
                                                                           PFTC_INPUT_OUTPUT_PINS pLowInputOutputPinsData,
                                                                           BOOL bControlHighInputOutputPins,
                                                                           PFTH_INPUT_OUTPUT_PINS pHighInputOutputPinsData)
{
  FTC_STATUS Status = FTC_SUCCESS;
  BOOL bHiSpeedFT2232HTDeviceype = FALSE;
  DWORD dwHighPinsDirection = 0;
  DWORD dwHighPinsValue = 0;

  if (bControlLowInputOutputPins != FALSE)
    Status = SetGeneralPurposeLowerInputOutputPins(ftHandle, pLowInputOutputPinsData);

  if (Status == FTC_SUCCESS)
  {
    if (bControlHighInputOutputPins != FALSE)
    {
      if ((Status = FTC_IsDeviceHiSpeedFT2232HType(ftHandle, &bHiSpeedFT2232HTDeviceype)) == FTC_SUCCESS)
      {
        // If the device is a FT2232H hi-speed device
        if (bHiSpeedFT2232HTDeviceype == TRUE)
        {
          if (pHighInputOutputPinsData->bPin1InputOutputState != FALSE)
            dwHighPinsDirection = (dwHighPinsDirection | '\x01');
          if (pHighInputOutputPinsData->bPin2InputOutputState != FALSE)
            dwHighPinsDirection = (dwHighPinsDirection | '\x02');
          if (pHighInputOutputPinsData->bPin3InputOutputState != FALSE)
            dwHighPinsDirection = (dwHighPinsDirection | '\x04');
          if (pHighInputOutputPinsData->bPin4InputOutputState != FALSE)
            dwHighPinsDirection = (dwHighPinsDirection | '\x08');
          if (pHighInputOutputPinsData->bPin5InputOutputState != FALSE)
            dwHighPinsDirection = (dwHighPinsDirection | '\x10');
          if (pHighInputOutputPinsData->bPin6InputOutputState != FALSE)
            dwHighPinsDirection = (dwHighPinsDirection | '\x20');
          if (pHighInputOutputPinsData->bPin7InputOutputState != FALSE)
            dwHighPinsDirection = (dwHighPinsDirection | '\x40');
          if (pHighInputOutputPinsData->bPin8InputOutputState != FALSE)
            dwHighPinsDirection = (dwHighPinsDirection | '\x80');

          if (pHighInputOutputPinsData->bPin1LowHighState != FALSE)
            dwHighPinsValue = (dwHighPinsValue | '\x01');
          if (pHighInputOutputPinsData->bPin2LowHighState != FALSE)
            dwHighPinsValue = (dwHighPinsValue | '\x02');
          if (pHighInputOutputPinsData->bPin3LowHighState != FALSE)
            dwHighPinsValue = (dwHighPinsValue | '\x04');
          if (pHighInputOutputPinsData->bPin4LowHighState != FALSE)
            dwHighPinsValue = (dwHighPinsValue | '\x08');
          if (pHighInputOutputPinsData->bPin5LowHighState != FALSE)
            dwHighPinsValue = (dwHighPinsValue | '\x10');
          if (pHighInputOutputPinsData->bPin6LowHighState != FALSE)
            dwHighPinsValue = (dwHighPinsValue | '\x20');
          if (pHighInputOutputPinsData->bPin7LowHighState != FALSE)
            dwHighPinsValue = (dwHighPinsValue | '\x40');
          if (pHighInputOutputPinsData->bPin8LowHighState != FALSE)
            dwHighPinsValue = (dwHighPinsValue | '\x80');

          // output on the general purpose I/O high pins 1-4
          AddByteToOutputBuffer(SET_HIGH_BYTE_DATA_BITS_CMD, TRUE);

          dwHighPinsValue = (dwHighPinsValue & '\xFF');
          AddByteToOutputBuffer(dwHighPinsValue, FALSE);

          dwHighPinsDirection = (dwHighPinsDirection & '\xFF');
          AddByteToOutputBuffer(dwHighPinsDirection, FALSE);

          Status = FTC_SendBytesToDevice(ftHandle);
        }
      }
    }
  }

  return Status;
}

void FT2232hMpsseJtag::GetGeneralPurposeInputOutputPinsInputStates(DWORD dwInputStatesReturnedValue, PFTC_LOW_HIGH_PINS pPinsInputData)
{
  if ((dwInputStatesReturnedValue & PIN1_HIGH_VALUE) == PIN1_HIGH_VALUE)
    pPinsInputData->bPin1LowHighState = TRUE;

  if ((dwInputStatesReturnedValue & PIN2_HIGH_VALUE) == PIN2_HIGH_VALUE)
    pPinsInputData->bPin2LowHighState = TRUE;

  if ((dwInputStatesReturnedValue & PIN3_HIGH_VALUE) == PIN3_HIGH_VALUE)
    pPinsInputData->bPin3LowHighState = TRUE;

  if ((dwInputStatesReturnedValue & PIN4_HIGH_VALUE) == PIN4_HIGH_VALUE)
    pPinsInputData->bPin4LowHighState = TRUE;
}

FTC_STATUS FT2232hMpsseJtag::GetGeneralPurposeLowerInputOutputPins(FTC_HANDLE ftHandle, PFTC_LOW_HIGH_PINS pLowPinsInputData)
{
  FTC_STATUS Status = FTC_SUCCESS;
  InputByteBuffer InputBuffer;
  DWORD dwNumBytesRead = 0;
  DWORD dwNumBytesDeviceInputBuffer;

  pLowPinsInputData->bPin1LowHighState = FALSE;
  pLowPinsInputData->bPin2LowHighState = FALSE;
  pLowPinsInputData->bPin3LowHighState = FALSE;
  pLowPinsInputData->bPin4LowHighState = FALSE;

  // Get the number of bytes in the device input buffer
  if ((Status = FT_GetQueueStatus((FT_HANDLE)ftHandle, &dwNumBytesDeviceInputBuffer)) == FTC_SUCCESS)
  {
    if (dwNumBytesDeviceInputBuffer > 0)
      Status = FTC_ReadBytesFromDevice(ftHandle, &InputBuffer, dwNumBytesDeviceInputBuffer, &dwNumBytesRead);

    if (Status == FTC_SUCCESS)
    {
      // get the states of the general purpose I/O low pins 1-4
      AddByteToOutputBuffer(GET_LOW_BYTE_DATA_BITS_CMD, TRUE);
      AddByteToOutputBuffer(SEND_ANSWER_BACK_IMMEDIATELY_CMD, FALSE);
      Status = FTC_SendBytesToDevice(ftHandle);

      if (Status == FTC_SUCCESS)
      {
        if ((Status = FTC_GetNumberBytesFromDeviceInputBuffer(ftHandle, &dwNumBytesDeviceInputBuffer)) == FTC_SUCCESS)
        {
          if ((Status = FTC_ReadBytesFromDevice(ftHandle, &InputBuffer, dwNumBytesDeviceInputBuffer, &dwNumBytesRead)) == FTC_SUCCESS)
            // shift right by 4 bits ie move general purpose I/O low pins 1-4 from bits 4-7 to bits 0-3
            GetGeneralPurposeInputOutputPinsInputStates((InputBuffer[0] >> 4), pLowPinsInputData);
        }
      }
    }
  }

  return Status;
}

FTC_STATUS FT2232hMpsseJtag::GetGeneralPurposeInputOutputPins(FTC_HANDLE ftHandle, BOOL bControlLowInputOutputPins,
                                                              PFTC_LOW_HIGH_PINS pLowPinsInputData,
                                                              BOOL bControlHighInputOutputPins,
                                                              PFTC_LOW_HIGH_PINS pHighPinsInputData)
{
  FTC_STATUS Status = FTC_SUCCESS;
  InputByteBuffer InputBuffer;
  DWORD dwNumBytesRead = 0;
  DWORD dwNumBytesDeviceInputBuffer;
  BOOL bHiSpeedTypeDevice = FALSE;
  BOOL bHiSpeedFT2232HTDeviceype = FALSE;

  pHighPinsInputData->bPin1LowHighState = FALSE;
  pHighPinsInputData->bPin2LowHighState = FALSE;
  pHighPinsInputData->bPin3LowHighState = FALSE;
  pHighPinsInputData->bPin4LowHighState = FALSE;

  // Put in this small delay incase the application programmer does a get GPIOs immediately after a set GPIOs
  Sleep(5);

  if (bControlLowInputOutputPins != FALSE)
    Status = GetGeneralPurposeLowerInputOutputPins(ftHandle, pLowPinsInputData);

  if (Status == FTC_SUCCESS)
  {
    if (bControlHighInputOutputPins != FALSE)
    {
      if (((Status = FTC_IsDeviceHiSpeedType(ftHandle, &bHiSpeedTypeDevice)) == FTC_SUCCESS) &&
          ((Status = FTC_IsDeviceHiSpeedFT2232HType(ftHandle, &bHiSpeedFT2232HTDeviceype)) == FTC_SUCCESS))
      {
        // If the device is not a hi-speed device or is a FT2232H hi-speed device
        if ((bHiSpeedTypeDevice == FALSE) || ((bHiSpeedTypeDevice == TRUE) && (bHiSpeedFT2232HTDeviceype == TRUE)))
        {
          // Get the number of bytes in the device input buffer
          if ((Status = FT_GetQueueStatus((FT_HANDLE)ftHandle, &dwNumBytesDeviceInputBuffer)) == FTC_SUCCESS)
          {
            if (dwNumBytesDeviceInputBuffer > 0)
              Status = FTC_ReadBytesFromDevice(ftHandle, &InputBuffer, dwNumBytesDeviceInputBuffer, &dwNumBytesRead);

            if (Status == FTC_SUCCESS)
            {
              // get the states of the general purpose I/O high pins 1-4
              AddByteToOutputBuffer(GET_HIGH_BYTE_DATA_BITS_CMD, TRUE);
              AddByteToOutputBuffer(SEND_ANSWER_BACK_IMMEDIATELY_CMD, FALSE);
              Status = FTC_SendBytesToDevice(ftHandle);

              if (Status == FTC_SUCCESS)
              {
                if ((Status = FTC_GetNumberBytesFromDeviceInputBuffer(ftHandle, &dwNumBytesDeviceInputBuffer)) == FTC_SUCCESS)
                {
                  if ((Status = FTC_ReadBytesFromDevice(ftHandle, &InputBuffer, dwNumBytesDeviceInputBuffer, &dwNumBytesRead)) == FTC_SUCCESS)
                    GetGeneralPurposeInputOutputPinsInputStates(InputBuffer[0], pHighPinsInputData);
                }
              }
            }
          }
        }
      }
    }
  }

  return Status;
}

void  FT2232hMpsseJtag::GetHiSpeedDeviceGeneralPurposeInputOutputPinsInputStates(DWORD dwInputStatesReturnedValue, PFTH_LOW_HIGH_PINS pPinsInputData)
{
  if ((dwInputStatesReturnedValue & PIN1_HIGH_VALUE) == PIN1_HIGH_VALUE)
    pPinsInputData->bPin1LowHighState = TRUE;

  if ((dwInputStatesReturnedValue & PIN2_HIGH_VALUE) == PIN2_HIGH_VALUE)
    pPinsInputData->bPin2LowHighState = TRUE;

  if ((dwInputStatesReturnedValue & PIN3_HIGH_VALUE) == PIN3_HIGH_VALUE)
    pPinsInputData->bPin3LowHighState = TRUE;

  if ((dwInputStatesReturnedValue & PIN4_HIGH_VALUE) == PIN4_HIGH_VALUE)
    pPinsInputData->bPin4LowHighState = TRUE;

  if ((dwInputStatesReturnedValue & PIN5_HIGH_VALUE) == PIN5_HIGH_VALUE)
    pPinsInputData->bPin5LowHighState = TRUE;

  if ((dwInputStatesReturnedValue & PIN6_HIGH_VALUE) == PIN6_HIGH_VALUE)
    pPinsInputData->bPin6LowHighState = TRUE;

  if ((dwInputStatesReturnedValue & PIN7_HIGH_VALUE) == PIN7_HIGH_VALUE)
    pPinsInputData->bPin7LowHighState = TRUE;

  if ((dwInputStatesReturnedValue & PIN8_HIGH_VALUE) == PIN8_HIGH_VALUE)
    pPinsInputData->bPin8LowHighState = TRUE;
}

FTC_STATUS FT2232hMpsseJtag::GetHiSpeedDeviceGeneralPurposeInputOutputPins(FTC_HANDLE ftHandle, BOOL bControlLowInputOutputPins,
                                                                           PFTC_LOW_HIGH_PINS pLowPinsInputData,
                                                                           BOOL bControlHighInputOutputPins,
                                                                           PFTH_LOW_HIGH_PINS pHighPinsInputData)
{
  FTC_STATUS Status = FTC_SUCCESS;
  InputByteBuffer InputBuffer;
  DWORD dwNumBytesRead = 0;
  DWORD dwNumBytesDeviceInputBuffer;
  BOOL bHiSpeedFT2232HTDeviceype = FALSE;

  pHighPinsInputData->bPin1LowHighState = FALSE;
  pHighPinsInputData->bPin2LowHighState = FALSE;
  pHighPinsInputData->bPin3LowHighState = FALSE;
  pHighPinsInputData->bPin4LowHighState = FALSE;
  pHighPinsInputData->bPin5LowHighState = FALSE;
  pHighPinsInputData->bPin6LowHighState = FALSE;
  pHighPinsInputData->bPin7LowHighState = FALSE;
  pHighPinsInputData->bPin8LowHighState = FALSE;

  // Put in this small delay incase the application programmer does a get GPIOs immediately after a set GPIOs
  Sleep(5);

  if (bControlLowInputOutputPins != FALSE)
    Status = GetGeneralPurposeLowerInputOutputPins(ftHandle, pLowPinsInputData);

  if (Status == FTC_SUCCESS)
  {
    if (bControlHighInputOutputPins != FALSE)
    {
      if ((Status = FTC_IsDeviceHiSpeedFT2232HType(ftHandle, &bHiSpeedFT2232HTDeviceype)) == FTC_SUCCESS)
      {
        // If the device is a FT2232H hi-speed device
        if (bHiSpeedFT2232HTDeviceype == TRUE)
        {
          // Get the number of bytes in the device input buffer
          if ((Status = FT_GetQueueStatus((FT_HANDLE)ftHandle, &dwNumBytesDeviceInputBuffer)) == FTC_SUCCESS)
          {
            if (dwNumBytesDeviceInputBuffer > 0)
              Status = FTC_ReadBytesFromDevice(ftHandle, &InputBuffer, dwNumBytesDeviceInputBuffer, &dwNumBytesRead);

            if (Status == FTC_SUCCESS)
            {
              // get the states of the general purpose I/O high pins 1-4
              AddByteToOutputBuffer(GET_HIGH_BYTE_DATA_BITS_CMD, TRUE);
              AddByteToOutputBuffer(SEND_ANSWER_BACK_IMMEDIATELY_CMD, FALSE);
              Status = FTC_SendBytesToDevice(ftHandle);

              if (Status == FTC_SUCCESS)
              {
                if ((Status = FTC_GetNumberBytesFromDeviceInputBuffer(ftHandle, &dwNumBytesDeviceInputBuffer)) == FTC_SUCCESS)
                {
                  if ((Status = FTC_ReadBytesFromDevice(ftHandle, &InputBuffer, dwNumBytesDeviceInputBuffer, &dwNumBytesRead)) == FTC_SUCCESS)
                    GetHiSpeedDeviceGeneralPurposeInputOutputPinsInputStates(InputBuffer[0], pHighPinsInputData);
                }
              }
            }
          }
        }
      }
    }
  }

  return Status;
}

void FT2232hMpsseJtag::AddWriteCommandDataToOutPutBuffer(BOOL bInstructionTestData, DWORD dwNumBitsToWrite,
                                                         PWriteDataByteBuffer pWriteDataBuffer, DWORD dwNumBytesToWrite,
                                                         DWORD dwTapControllerState)
{
  DWORD dwModNumBitsToWrite = 0;
  DWORD dwDataBufferIndex = 0;
  DWORD dwNumDataBytes = 0;
  DWORD dwNumRemainingDataBits = 0;
  DWORD dwLastDataBit = 0;
  DWORD dwDataBitIndex = 0;

  // adjust for bit count of 1 less than no of bits
  dwModNumBitsToWrite = (dwNumBitsToWrite - 1);

  if (bInstructionTestData == FALSE)
    MoveJTAGFromOneStateToAnother(ShiftDataRegister, NO_LAST_DATA_BIT, false);
  else
    MoveJTAGFromOneStateToAnother(ShiftInstructionRegister, NO_LAST_DATA_BIT, false);

  dwNumDataBytes = (dwModNumBitsToWrite / 8);

  if (dwNumDataBytes > 0)
  {
    // Number of whole bytes
    dwNumDataBytes = (dwNumDataBytes - 1);

    // clk data bytes out on -ve clk LSB
    AddByteToOutputBuffer(CLK_DATA_BYTES_OUT_ON_NEG_CLK_LSB_FIRST_CMD, false);
    AddByteToOutputBuffer((dwNumDataBytes & '\xFF'), false);
    AddByteToOutputBuffer(((dwNumDataBytes / 256) & '\xFF'), false);

    // now add the data bytes to go out
    do
    {
      AddByteToOutputBuffer((*pWriteDataBuffer)[dwDataBufferIndex], false);
      dwDataBufferIndex = (dwDataBufferIndex + 1);
    }
    while (dwDataBufferIndex < (dwNumDataBytes + 1));
  }

  dwNumRemainingDataBits = (dwModNumBitsToWrite % 8);

  if (dwNumRemainingDataBits > 0)
  {
    dwNumRemainingDataBits = (dwNumRemainingDataBits - 1);

    //clk data bits out on -ve clk LSB
    AddByteToOutputBuffer(CLK_DATA_BITS_OUT_ON_NEG_CLK_LSB_FIRST_CMD, false);
    AddByteToOutputBuffer((dwNumRemainingDataBits & '\xFF'), false);
    AddByteToOutputBuffer((*pWriteDataBuffer)[dwDataBufferIndex], false);
  }

  // get last bit
  dwLastDataBit = (*pWriteDataBuffer)[dwDataBufferIndex];
  dwDataBitIndex = (dwNumBitsToWrite % 8);

  if (dwDataBitIndex == 0)
    dwLastDataBit = (dwLastDataBit >> ((8 - dwDataBitIndex) - 1));
  else
    dwLastDataBit = (dwLastDataBit >> (dwDataBitIndex - 1));

  // end it in state passed in, take 1 off the dwTapControllerState variable to correspond with JtagStates enumerated types
  MoveJTAGFromOneStateToAnother(JtagStates((dwTapControllerState - 1)), dwLastDataBit, false);
}

FTC_STATUS FT2232hMpsseJtag::WriteDataToExternalDevice(FTC_HANDLE ftHandle, BOOL bInstructionTestData, DWORD dwNumBitsToWrite,
                                                       PWriteDataByteBuffer pWriteDataBuffer, DWORD dwNumBytesToWrite,
                                                       DWORD dwTapControllerState)
{
  FTC_STATUS Status = FTC_SUCCESS;

  FTC_ClearOutputBuffer();

  AddWriteCommandDataToOutPutBuffer(bInstructionTestData, dwNumBitsToWrite, pWriteDataBuffer,
                                    dwNumBytesToWrite, dwTapControllerState);

  Status = FTC_SendBytesToDevice(ftHandle);

  return Status;
}

void FT2232hMpsseJtag::GetNumDataBytesToRead(DWORD dwNumBitsToRead, LPDWORD lpdwNumDataBytesToRead, LPDWORD lpdwNumRemainingDataBits)
{
  DWORD dwModNumBitsToRead = 0;
  DWORD dwNumDataBytesToRead = 0;
  DWORD dwNumRemainingDataBits = 0;

  // adjust for bit count of 1 less than no of bits
  dwModNumBitsToRead = (dwNumBitsToRead - 1);

  // Number of whole bytes to read
  dwNumDataBytesToRead = (dwModNumBitsToRead / 8);

  // number of remaining bits
  dwNumRemainingDataBits = (dwModNumBitsToRead % 8);

  // increase the number of whole bytes if bits left over
  if (dwNumRemainingDataBits > 0)
    dwNumDataBytesToRead = (dwNumDataBytesToRead + 1);

  // adjust for SHR of incoming byte
  dwNumRemainingDataBits = (8 - dwNumRemainingDataBits);

  // add 1 for TMS read byte
  dwNumDataBytesToRead = (dwNumDataBytesToRead + 1);

  *lpdwNumDataBytesToRead = dwNumDataBytesToRead;
  *lpdwNumRemainingDataBits = dwNumRemainingDataBits;
}

// This will work out the number of whole bytes to read and adjust for the TMS read
FTC_STATUS FT2232hMpsseJtag::GetDataFromExternalDevice(FTC_HANDLE ftHandle, DWORD dwNumBitsToRead, DWORD dwNumTmsClocks,
                                                       PReadDataByteBuffer pReadDataBuffer, LPDWORD lpdwNumBytesReturned)
{
  FTC_STATUS Status = FTC_SUCCESS;
  DWORD dwNumReadDataBytes = 0;
  DWORD dwNumRemainingDataBits = 0;
  DWORD dwNumDataBytesRead = 0;
  DWORD dwNumBytesDeviceInputBuffer = 0;
  InputByteBuffer InputBuffer;
  DWORD dwBytesReadIndex = 0;
  BYTE LastDataBit = 0;

  GetNumDataBytesToRead(dwNumBitsToRead, &dwNumReadDataBytes, &dwNumRemainingDataBits);

  Status = FTC_ReadFixedNumBytesFromDevice(ftHandle, &InputBuffer, dwNumReadDataBytes, &dwNumDataBytesRead);

  if (Status == FTC_SUCCESS)
  {
    // adjust last 2 bytes
    if (dwNumRemainingDataBits < 8)
    {
      InputBuffer[dwNumReadDataBytes - 2] = (InputBuffer[dwNumReadDataBytes - 2] >> dwNumRemainingDataBits);
      LastDataBit = (InputBuffer[dwNumReadDataBytes - 1] << (dwNumTmsClocks - 1));
      LastDataBit = (LastDataBit & '\x80'); // strip the rest
      InputBuffer[dwNumReadDataBytes - 2] = (InputBuffer[dwNumReadDataBytes - 2] | (LastDataBit >> (dwNumRemainingDataBits - 1)));

      dwNumReadDataBytes = (dwNumReadDataBytes - 1);

      for (dwBytesReadIndex = 0 ; dwBytesReadIndex < dwNumReadDataBytes; dwBytesReadIndex++)
        (*pReadDataBuffer)[dwBytesReadIndex] = InputBuffer[dwBytesReadIndex];
    }
    else // case for 0 bit shift in data + TMS read bit
    {
      LastDataBit = (InputBuffer[dwNumReadDataBytes - 1] << (dwNumTmsClocks - 1));
      LastDataBit = (LastDataBit >> 7); // strip the rest
      InputBuffer[dwNumReadDataBytes - 1] = LastDataBit;

      for (dwBytesReadIndex = 0 ; dwBytesReadIndex < dwNumReadDataBytes; dwBytesReadIndex++)
        (*pReadDataBuffer)[dwBytesReadIndex] = InputBuffer[dwBytesReadIndex];
    }

    *lpdwNumBytesReturned = dwNumReadDataBytes;
  }

  return Status;
}

DWORD FT2232hMpsseJtag::AddReadCommandToOutputBuffer(BOOL bInstructionTestData, DWORD dwNumBitsToRead, DWORD dwTapControllerState)
{
  DWORD dwModNumBitsToRead = 0;
  DWORD dwNumDataBytes = 0;
  DWORD dwNumRemainingDataBits = 0;
  DWORD dwNumTmsClocks = 0;

  // adjust for bit count of 1 less than no of bits
  dwModNumBitsToRead = (dwNumBitsToRead - 1);

  if (bInstructionTestData == FALSE)
    MoveJTAGFromOneStateToAnother(ShiftDataRegister, NO_LAST_DATA_BIT, false);
  else
    MoveJTAGFromOneStateToAnother(ShiftInstructionRegister, NO_LAST_DATA_BIT, false);

  dwNumDataBytes = (dwModNumBitsToRead / 8);

  if (dwNumDataBytes > 0)
  {
    // Number of whole bytes
    dwNumDataBytes = (dwNumDataBytes - 1);

    // clk data bytes out on -ve clk LSB
    AddByteToOutputBuffer(CLK_DATA_BYTES_IN_ON_POS_CLK_LSB_FIRST_CMD, false);
    AddByteToOutputBuffer((dwNumDataBytes & '\xFF'), false);
    AddByteToOutputBuffer(((dwNumDataBytes / 256) & '\xFF'), false);
  }

  // number of remaining bits
  dwNumRemainingDataBits = (dwModNumBitsToRead % 8);

  if (dwNumRemainingDataBits > 0)
  {
    dwNumRemainingDataBits = (dwNumRemainingDataBits - 1);

    //clk data bits out on -ve clk LSB
    AddByteToOutputBuffer(CLK_DATA_BITS_IN_ON_POS_CLK_LSB_FIRST_CMD, false);
    AddByteToOutputBuffer((dwNumRemainingDataBits & '\xFF'), false);
  }

  // end it in state passed in, take 1 off the dwTapControllerState variable to correspond with JtagStates enumerated types
  dwNumTmsClocks = MoveJTAGFromOneStateToAnother(JtagStates((dwTapControllerState - 1)), NO_LAST_DATA_BIT, true);

  return dwNumTmsClocks;
}

FTC_STATUS FT2232hMpsseJtag::ReadDataFromExternalDevice(FTC_HANDLE ftHandle, BOOL bInstructionTestData, DWORD dwNumBitsToRead,
                                                        PReadDataByteBuffer pReadDataBuffer, LPDWORD lpdwNumBytesReturned,
                                                        DWORD dwTapControllerState)
{
  FTC_STATUS Status = FTC_SUCCESS;
  DWORD dwNumTmsClocks = 0;

  FTC_ClearOutputBuffer();

  dwNumTmsClocks = AddReadCommandToOutputBuffer(bInstructionTestData, dwNumBitsToRead, dwTapControllerState);

  AddByteToOutputBuffer(SEND_ANSWER_BACK_IMMEDIATELY_CMD, false);

  Status = FTC_SendBytesToDevice(ftHandle);

  if (Status == FTC_SUCCESS)
    Status = GetDataFromExternalDevice(ftHandle, dwNumBitsToRead, dwNumTmsClocks, pReadDataBuffer, lpdwNumBytesReturned);

  return Status;
}

DWORD FT2232hMpsseJtag::AddWriteReadCommandDataToOutPutBuffer(BOOL bInstructionTestData, DWORD dwNumBitsToWriteRead,
                                                              PWriteDataByteBuffer pWriteDataBuffer,
                                                              DWORD dwNumBytesToWrite, DWORD dwTapControllerState)
{
  DWORD dwModNumBitsToWriteRead = 0;
  DWORD dwNumWriteDataBytes = 0;
  DWORD dwDataBufferIndex = 0;
  DWORD dwNumRemainingDataBits = 0;
  DWORD dwLastDataBit = 0;
  DWORD dwDataBitIndex = 0;
  DWORD dwNumTmsClocks = 0;

  // adjust for bit count of 1 less than no of bits
  dwModNumBitsToWriteRead = (dwNumBitsToWriteRead - 1);

  if (bInstructionTestData == FALSE)
    MoveJTAGFromOneStateToAnother(ShiftDataRegister, NO_LAST_DATA_BIT, false);
  else
    MoveJTAGFromOneStateToAnother(ShiftInstructionRegister, NO_LAST_DATA_BIT, false);

  dwNumWriteDataBytes = (dwModNumBitsToWriteRead / 8);

  if (dwNumWriteDataBytes > 0)
  {
    // Number of whole bytes
    dwNumWriteDataBytes = (dwNumWriteDataBytes - 1);

    // clk data bytes out on -ve in +ve clk LSB
    AddByteToOutputBuffer(CLK_DATA_BYTES_OUT_ON_NEG_CLK_IN_ON_POS_CLK_LSB_FIRST_CMD, false);
    AddByteToOutputBuffer((dwNumWriteDataBytes & '\xFF'), false);
    AddByteToOutputBuffer(((dwNumWriteDataBytes / 256) & '\xFF'), false);

    // now add the data bytes to go out
    do
    {
      AddByteToOutputBuffer((*pWriteDataBuffer)[dwDataBufferIndex], false);
      dwDataBufferIndex = (dwDataBufferIndex + 1);
    }
    while (dwDataBufferIndex < (dwNumWriteDataBytes + 1));
  }

  dwNumRemainingDataBits = (dwModNumBitsToWriteRead % 8);

  if (dwNumRemainingDataBits > 0)
  {
    dwNumRemainingDataBits = (dwNumRemainingDataBits - 1);

    // clk data bits out on -ve in +ve clk LSB
    AddByteToOutputBuffer(CLK_DATA_BITS_OUT_ON_NEG_CLK_IN_ON_POS_CLK_LSB_FIRST_CMD, false);
    AddByteToOutputBuffer((dwNumRemainingDataBits & '\xFF'), false);
    AddByteToOutputBuffer((*pWriteDataBuffer)[dwDataBufferIndex], false);
  }

  // get last bit
  dwLastDataBit = (*pWriteDataBuffer)[dwDataBufferIndex];
  dwDataBitIndex = (dwNumBitsToWriteRead % 8);

  if (dwDataBitIndex == 8)
    dwLastDataBit = (dwLastDataBit >> ((8 - dwDataBitIndex) - 1));
  else
    dwLastDataBit = (dwLastDataBit >> (dwDataBitIndex - 1));

  // end it in state passed in, take 1 off the dwTapControllerState variable to correspond with JtagStates enumerated types
  dwNumTmsClocks = MoveJTAGFromOneStateToAnother(JtagStates((dwTapControllerState - 1)), dwLastDataBit, true);

  return dwNumTmsClocks;
}

FTC_STATUS FT2232hMpsseJtag::WriteReadDataToFromExternalDevice(FTC_HANDLE ftHandle, BOOL bInstructionTestData, DWORD dwNumBitsToWriteRead,
                                                               PWriteDataByteBuffer pWriteDataBuffer, DWORD dwNumBytesToWrite,
                                                               PReadDataByteBuffer pReadDataBuffer, LPDWORD lpdwNumBytesReturned,
                                                               DWORD dwTapControllerState)
{
  FTC_STATUS Status = FTC_SUCCESS;
  DWORD dwNumRemainingDataBits = 0;
  BYTE LastDataBit = 0;
  DWORD dwNumTmsClocks = 0;
  DWORD dwNumReadDataBytes = 0;
  InputByteBuffer InputBuffer;
  DWORD dwNumDataBytesRead = 0;
  DWORD dwBytesReadIndex = 0;

  FTC_ClearOutputBuffer();

  dwNumTmsClocks = AddWriteReadCommandDataToOutPutBuffer(bInstructionTestData, dwNumBitsToWriteRead,
                                                         pWriteDataBuffer, dwNumBytesToWrite, dwTapControllerState);

  AddByteToOutputBuffer(SEND_ANSWER_BACK_IMMEDIATELY_CMD, false);

  GetNumDataBytesToRead(dwNumBitsToWriteRead, &dwNumReadDataBytes, &dwNumRemainingDataBits);

  Status = FTC_SendReadBytesToFromDevice(ftHandle, &InputBuffer, dwNumReadDataBytes, &dwNumDataBytesRead);

  if (Status == FTC_SUCCESS)
  {
    // adjust last 2 bytes
    if (dwNumRemainingDataBits < 8)
    {
      InputBuffer[dwNumReadDataBytes - 2] = (InputBuffer[dwNumReadDataBytes - 2] >> dwNumRemainingDataBits);
      LastDataBit = (InputBuffer[dwNumReadDataBytes - 1] << (dwNumTmsClocks - 1));
      LastDataBit = (LastDataBit & '\x80'); // strip the rest
      InputBuffer[dwNumReadDataBytes - 2] = (InputBuffer[dwNumReadDataBytes - 2] | (LastDataBit >> (dwNumRemainingDataBits - 1)));

      dwNumReadDataBytes = (dwNumReadDataBytes - 1);

      for (dwBytesReadIndex = 0 ; dwBytesReadIndex < dwNumReadDataBytes; dwBytesReadIndex++)
        (*pReadDataBuffer)[dwBytesReadIndex] = InputBuffer[dwBytesReadIndex];
    }
    else // case for 0 bit shift in data + TMS read bit
    {
      LastDataBit = (InputBuffer[dwNumReadDataBytes - 1] << (dwNumTmsClocks - 1));
      LastDataBit = (LastDataBit >> 7); // strip the rest
      InputBuffer[dwNumReadDataBytes - 1] = LastDataBit;

      for (dwBytesReadIndex = 0 ; dwBytesReadIndex < dwNumReadDataBytes; dwBytesReadIndex++)
        (*pReadDataBuffer)[dwBytesReadIndex] = InputBuffer[dwBytesReadIndex];
    }

    *lpdwNumBytesReturned = dwNumReadDataBytes;
  }

  return Status;
}

FTC_STATUS FT2232hMpsseJtag::GenerateTCKClockPulses(FTC_HANDLE ftHandle, DWORD dwNumClockPulses)
{
  FTC_STATUS Status = FTC_SUCCESS;
  DWORD dwTotalNumClockPulsesBytes = 0;
  DWORD dwNumClockPulsesByteBlocks = 0;
  DWORD dwNumClockPulsesByteBlockCntr = 0;
  DWORD dwNumClockPulsesBytes = 0;
  DWORD dwNumRemainingClockPulsesBits = 0;
  DWORD dwDataBufferIndex = 0;

  MoveJTAGFromOneStateToAnother(RunTestIdle, NO_LAST_DATA_BIT, FALSE);

  dwTotalNumClockPulsesBytes = (dwNumClockPulses / NUMBITSINBYTE);

  if (dwTotalNumClockPulsesBytes > 0)
  {
    dwNumClockPulsesByteBlocks = (dwTotalNumClockPulsesBytes / NUM_BYTE_CLOCK_PULSES_BLOCK_SIZE);

    do
    {
      if (dwNumClockPulsesByteBlockCntr < dwNumClockPulsesByteBlocks)
        dwNumClockPulsesBytes = NUM_BYTE_CLOCK_PULSES_BLOCK_SIZE;
      else
        dwNumClockPulsesBytes = (dwTotalNumClockPulsesBytes - (dwNumClockPulsesByteBlockCntr * NUM_BYTE_CLOCK_PULSES_BLOCK_SIZE));

      if (dwNumClockPulsesBytes > 0)
      {
        // Number of whole bytes
        dwNumClockPulsesBytes = (dwNumClockPulsesBytes - 1);

        // clk data bytes out on -ve clk LSB
        AddByteToOutputBuffer(CLK_DATA_BYTES_OUT_ON_NEG_CLK_LSB_FIRST_CMD, false);
        AddByteToOutputBuffer((dwNumClockPulsesBytes & '\xFF'), false);
        AddByteToOutputBuffer(((dwNumClockPulsesBytes / 256) & '\xFF'), false);

        DWORD dwDataBufferIndex = 0;

        // now add the data bytes ie 0 to go out with the clock pulses
        do
        {
          AddByteToOutputBuffer(0, false);
          dwDataBufferIndex = (dwDataBufferIndex + 1);
        }
        while (dwDataBufferIndex < (dwNumClockPulsesBytes + 1));
      }

      Status = FTC_SendBytesToDevice(ftHandle);

      dwNumClockPulsesByteBlockCntr = (dwNumClockPulsesByteBlockCntr + 1);
    }
    while ((dwNumClockPulsesByteBlockCntr <= dwNumClockPulsesByteBlocks) && (Status == FTC_SUCCESS));
  }

  if (Status == FTC_SUCCESS)
  {
    dwNumRemainingClockPulsesBits = (dwNumClockPulses % NUMBITSINBYTE);

    if (dwNumRemainingClockPulsesBits > 0)
    {
      dwNumRemainingClockPulsesBits = (dwNumRemainingClockPulsesBits - 1);

      //clk data bits out on -ve clk LSB
      AddByteToOutputBuffer(CLK_DATA_BITS_OUT_ON_NEG_CLK_LSB_FIRST_CMD, false);
      AddByteToOutputBuffer((dwNumRemainingClockPulsesBits & '\xFF'), false);
      AddByteToOutputBuffer('\xFF', false);

      Status = FTC_SendBytesToDevice(ftHandle);
    }
  }

  //MoveJTAGFromOneStateToAnother(RunTestIdle, NO_LAST_DATA_BIT, FALSE);

  //Status = FTC_SendBytesToDevice(ftHandle);

  return Status;
}

FTC_STATUS FT2232hMpsseJtag::GenerateClockPulsesHiSpeedDevice(FTC_HANDLE ftHandle, BOOL bPulseClockTimesEightFactor, DWORD dwNumClockPulses, BOOL bControlLowInputOutputPin, BOOL bStopClockPulsesState)
{
  FTC_STATUS Status = FTC_SUCCESS;

  if (!bControlLowInputOutputPin)
  {
    if (!bPulseClockTimesEightFactor)
    {
      // pulses the clock the specified number of times with no data transfer
      AddByteToOutputBuffer(CLK_FOR_NUM_CLOCKS_NO_DATA_BYTES_CMD, TRUE);
      AddByteToOutputBuffer((dwNumClockPulses & '\xFF'), FALSE);
    }
    else
    {
      // pulses the clock eight times the specified number of times with no data transfer
      AddByteToOutputBuffer(CLK_FOR_TIMES_EIGHT_CLOCKS_NO_DATA_BYTES_CMD, TRUE);
      AddByteToOutputBuffer((dwNumClockPulses & '\xFF'), FALSE);
      AddByteToOutputBuffer(((dwNumClockPulses / 256) & '\xFF'), FALSE);
    }
  }
  else
  {
    if (!bStopClockPulsesState)
    {
      // pulses the clock eight times the specified number of times or until GPIOL2 goes low with no data transfer
      AddByteToOutputBuffer(CLK_FOR_TIMES_EIGHT_CLOCKS_GPIOL2_LOW_NO_DATA_BYTES_CMD, TRUE);
      AddByteToOutputBuffer((dwNumClockPulses & '\xFF'), FALSE);
      AddByteToOutputBuffer(((dwNumClockPulses / 256) & '\xFF'), FALSE);
    }
    else
    {
      // pulses the clock eight times the specified number of times or until GPIOL2 goes high with no data transfer
      AddByteToOutputBuffer(CLK_FOR_TIMES_EIGHT_CLOCKS_GPIOL2_HIGH_NO_DATA_BYTES_CMD, TRUE);
      AddByteToOutputBuffer((dwNumClockPulses & '\xFF'), FALSE);
      AddByteToOutputBuffer(((dwNumClockPulses / 256) & '\xFF'), FALSE);
    }
  }

  Status = FTC_SendBytesToDevice(ftHandle);

  return Status;
}

void FT2232hMpsseJtag::ProcessReadCommandsSequenceBytes(PInputByteBuffer pInputBuffer, DWORD dwNumBytesRead, PReadCmdSequenceDataByteBuffer pReadCmdSequenceDataBuffer,
                                                        LPDWORD lpdwNumBytesReturned)
{
  DWORD CommandSequenceIndex = 0;
  PReadCommandsSequenceData pReadCommandsSequenceDataBuffer;
  DWORD dwNumReadCommandSequences;
  PReadCommandSequenceData pReadCmdSequenceData;
  DWORD dwNumBitsToRead = 0;
  DWORD dwNumTmsClocks = 0;
  DWORD dwNumReadDataBytes = 0;
  DWORD dwNumRemainingDataBits = 0;
  BYTE  LastDataBit = 0;
  DWORD dwBytesReadIndex = 0;
  DWORD dwNumReadBytesProcessed = 0;
  DWORD dwTotalNumBytesRead = 0;
  DWORD dwNumBytesReturned = 0;

  if (iCommandsSequenceDataDeviceIndex != -1)
  {
    pReadCommandsSequenceDataBuffer = OpenedDevicesCommandsSequenceData[iCommandsSequenceDataDeviceIndex].pReadCommandsSequenceDataBuffer;
    dwNumReadCommandSequences = OpenedDevicesCommandsSequenceData[iCommandsSequenceDataDeviceIndex].dwNumReadCommandSequences;

    for (CommandSequenceIndex = 0; (CommandSequenceIndex < dwNumReadCommandSequences); CommandSequenceIndex++)
    {
      pReadCmdSequenceData = (*pReadCommandsSequenceDataBuffer)[CommandSequenceIndex];
      dwNumBitsToRead = (*pReadCmdSequenceData)[0];
      dwNumTmsClocks = (*pReadCmdSequenceData)[1];

      GetNumDataBytesToRead(dwNumBitsToRead, &dwNumReadDataBytes, &dwNumRemainingDataBits);

      dwNumReadBytesProcessed = (dwNumReadBytesProcessed + dwNumReadDataBytes);

      // adjust last 2 bytes
      if (dwNumRemainingDataBits < 8)
      {
        (*pInputBuffer)[dwNumReadBytesProcessed - 2] = ((*pInputBuffer)[dwNumReadBytesProcessed - 2] >> dwNumRemainingDataBits);
        LastDataBit = ((*pInputBuffer)[dwNumReadBytesProcessed - 1] << (dwNumTmsClocks - 1));
        LastDataBit = (LastDataBit & '\x80'); // strip the rest
        (*pInputBuffer)[dwNumReadBytesProcessed - 2] = ((*pInputBuffer)[dwNumReadBytesProcessed - 2] | (LastDataBit >> (dwNumRemainingDataBits - 1)));

        dwNumReadDataBytes = (dwNumReadDataBytes - 1);

        for (dwBytesReadIndex = 0; dwBytesReadIndex < dwNumReadDataBytes; dwBytesReadIndex++)
          (*pReadCmdSequenceDataBuffer)[(dwBytesReadIndex + dwNumBytesReturned)] = (*pInputBuffer)[(dwBytesReadIndex + dwTotalNumBytesRead)];
      }
      else // case for 0 bit shift in data + TMS read bit
      {
        LastDataBit = ((*pInputBuffer)[dwNumReadBytesProcessed - 1] << (dwNumTmsClocks - 1));
        LastDataBit = (LastDataBit >> 7); // strip the rest
        (*pInputBuffer)[dwNumReadBytesProcessed - 1] = LastDataBit;

        for (dwBytesReadIndex = 0; dwBytesReadIndex < dwNumReadDataBytes; dwBytesReadIndex++)
          (*pReadCmdSequenceDataBuffer)[(dwBytesReadIndex + dwNumBytesReturned)] = (*pInputBuffer)[(dwBytesReadIndex + dwTotalNumBytesRead)];
      }

      dwTotalNumBytesRead = dwNumReadBytesProcessed;

      dwNumBytesReturned = (dwNumBytesReturned + dwNumReadDataBytes);
    }
  }

  *lpdwNumBytesReturned = dwNumBytesReturned;
}

DWORD FT2232hMpsseJtag::GetTotalNumCommandsSequenceDataBytesToRead(void)
{
  DWORD dwTotalNumBytesToBeRead = 0;
  DWORD CommandSequenceIndex = 0;
  PReadCommandsSequenceData pReadCommandsSequenceDataBuffer;
  DWORD dwNumReadCommandSequences = 0;
  PReadCommandSequenceData pReadCmdSequenceData;
  DWORD dwNumBitsToRead = 0;
  DWORD dwNumDataBytesToRead = 0;
  DWORD dwNumRemainingDataBits = 0;

  if (iCommandsSequenceDataDeviceIndex != -1)
  {
    pReadCommandsSequenceDataBuffer = OpenedDevicesCommandsSequenceData[iCommandsSequenceDataDeviceIndex].pReadCommandsSequenceDataBuffer;
    dwNumReadCommandSequences = OpenedDevicesCommandsSequenceData[iCommandsSequenceDataDeviceIndex].dwNumReadCommandSequences;

    for (CommandSequenceIndex = 0; (CommandSequenceIndex < dwNumReadCommandSequences); CommandSequenceIndex++)
    {
      pReadCmdSequenceData = (*pReadCommandsSequenceDataBuffer)[CommandSequenceIndex];
      dwNumBitsToRead = (*pReadCmdSequenceData)[0];

      GetNumDataBytesToRead(dwNumBitsToRead, &dwNumDataBytesToRead, &dwNumRemainingDataBits);

      dwTotalNumBytesToBeRead = (dwTotalNumBytesToBeRead + dwNumDataBytesToRead);
    }
  }

  return dwTotalNumBytesToBeRead;
}

void FT2232hMpsseJtag::CopyReadCommandsSequenceDataBuffer(PReadCommandsSequenceData pDestinationBuffer, PReadCommandsSequenceData pSourceBuffer, DWORD dwSizeReadCommandsSequenceDataBuffer)
{
  DWORD CommandSequenceIndex = 0;
  PReadCommandSequenceData pReadCmdSequenceData;
  DWORD dwNumBitsToRead = 0;
  DWORD dwNumTmsClocks = 0;

  for (CommandSequenceIndex = 0; (CommandSequenceIndex < dwSizeReadCommandsSequenceDataBuffer); CommandSequenceIndex++)
  {
    pReadCmdSequenceData = (*pSourceBuffer)[CommandSequenceIndex];
    dwNumBitsToRead = (*pReadCmdSequenceData)[0];
    dwNumTmsClocks = (*pReadCmdSequenceData)[1];

    pReadCmdSequenceData = (*pDestinationBuffer)[CommandSequenceIndex];
    (*pReadCmdSequenceData)[0] = dwNumBitsToRead;
    (*pReadCmdSequenceData)[1] = dwNumTmsClocks;
  }
}

FTC_STATUS FT2232hMpsseJtag::AddReadCommandSequenceData(DWORD dwNumBitsToRead, DWORD dwNumTmsClocks)
{
  FTC_STATUS Status = FTC_SUCCESS;
  DWORD dwSizeReadCommandsSequenceDataBuffer;
  PReadCommandsSequenceData pReadCommandsSequenceDataBuffer;
  DWORD dwNumReadCommandSequences;
  PReadCommandsSequenceData pTmpReadCmdsSequenceDataBuffer;
  PReadCommandSequenceData pReadCmdSequenceData;

  if (iCommandsSequenceDataDeviceIndex != -1)
  {
    dwSizeReadCommandsSequenceDataBuffer = OpenedDevicesCommandsSequenceData[iCommandsSequenceDataDeviceIndex].dwSizeReadCommandsSequenceDataBuffer;
    pReadCommandsSequenceDataBuffer = OpenedDevicesCommandsSequenceData[iCommandsSequenceDataDeviceIndex].pReadCommandsSequenceDataBuffer;
    dwNumReadCommandSequences = OpenedDevicesCommandsSequenceData[iCommandsSequenceDataDeviceIndex].dwNumReadCommandSequences;

    if (dwNumReadCommandSequences > (dwSizeReadCommandsSequenceDataBuffer - 1))
    {
      pTmpReadCmdsSequenceDataBuffer = CreateReadCommandsSequenceDataBuffer(dwSizeReadCommandsSequenceDataBuffer);

      if (pTmpReadCmdsSequenceDataBuffer != NULL)
      {
        // Temporary save the contents of the read commands sequence data buffer
        CopyReadCommandsSequenceDataBuffer(pTmpReadCmdsSequenceDataBuffer, pReadCommandsSequenceDataBuffer, dwSizeReadCommandsSequenceDataBuffer);

        DeleteReadCommandsSequenceDataBuffer(pReadCommandsSequenceDataBuffer, dwSizeReadCommandsSequenceDataBuffer);

        // Increase the size of the read commands sequence data buffer
        pReadCommandsSequenceDataBuffer = CreateReadCommandsSequenceDataBuffer((dwSizeReadCommandsSequenceDataBuffer + COMMAND_SEQUENCE_READ_DATA_BUFFER_SIZE_INCREMENT));
  
        if (pReadCommandsSequenceDataBuffer != NULL)
        {
          CopyReadCommandsSequenceDataBuffer(pReadCommandsSequenceDataBuffer, pTmpReadCmdsSequenceDataBuffer, dwSizeReadCommandsSequenceDataBuffer);

          DeleteReadCommandsSequenceDataBuffer(pTmpReadCmdsSequenceDataBuffer, dwSizeReadCommandsSequenceDataBuffer);

          dwSizeReadCommandsSequenceDataBuffer = (dwSizeReadCommandsSequenceDataBuffer + COMMAND_SEQUENCE_READ_DATA_BUFFER_SIZE_INCREMENT);

          OpenedDevicesCommandsSequenceData[iCommandsSequenceDataDeviceIndex].dwSizeReadCommandsSequenceDataBuffer = dwSizeReadCommandsSequenceDataBuffer;
          OpenedDevicesCommandsSequenceData[iCommandsSequenceDataDeviceIndex].pReadCommandsSequenceDataBuffer = pReadCommandsSequenceDataBuffer;
        }
        else
          Status = FTC_INSUFFICIENT_RESOURCES;
      }
      else
        Status = FTC_INSUFFICIENT_RESOURCES;
    }

    if (Status == FTC_SUCCESS)
    {
      if (dwNumReadCommandSequences > 0)
        pReadCmdSequenceData = (*pReadCommandsSequenceDataBuffer)[(dwNumReadCommandSequences - 1)];

      pReadCmdSequenceData = (*pReadCommandsSequenceDataBuffer)[dwNumReadCommandSequences];

      (*pReadCmdSequenceData)[0] = dwNumBitsToRead;
      (*pReadCmdSequenceData)[1] = dwNumTmsClocks;

      dwNumReadCommandSequences = (dwNumReadCommandSequences + 1);

      OpenedDevicesCommandsSequenceData[iCommandsSequenceDataDeviceIndex].dwNumReadCommandSequences = dwNumReadCommandSequences;
    }
  }

  return Status;
}

PReadCommandsSequenceData FT2232hMpsseJtag::CreateReadCommandsSequenceDataBuffer(DWORD dwSizeReadCmdsSequenceDataBuffer)
{
  PReadCommandsSequenceData pReadCmdsSequenceDataBuffer;
  DWORD CommandSequenceIndex = 0;
  PReadCommandSequenceData pReadCmdSequenceData;

  pReadCmdsSequenceDataBuffer = PReadCommandsSequenceData(new ReadCommandsSequenceData[dwSizeReadCmdsSequenceDataBuffer]);

  if (pReadCmdsSequenceDataBuffer != NULL)
  {
    for (CommandSequenceIndex = 0; (CommandSequenceIndex < dwSizeReadCmdsSequenceDataBuffer); CommandSequenceIndex++)
    {
      pReadCmdSequenceData = PReadCommandSequenceData(new ReadCommandSequenceData);

      (*pReadCmdsSequenceDataBuffer)[CommandSequenceIndex] = pReadCmdSequenceData;
    }
  }

  return pReadCmdsSequenceDataBuffer;
}

void FT2232hMpsseJtag::DeleteReadCommandsSequenceDataBuffer(PReadCommandsSequenceData pReadCmdsSequenceDataBuffer, DWORD dwSizeReadCommandsSequenceDataBuffer)
{
  DWORD CommandSequenceIndex = 0;
  PReadCommandSequenceData pReadCmdSequenceData;

  for (CommandSequenceIndex = 0; (CommandSequenceIndex < dwSizeReadCommandsSequenceDataBuffer); CommandSequenceIndex++)
  {
    pReadCmdSequenceData = (*pReadCmdsSequenceDataBuffer)[CommandSequenceIndex];

    delete [] pReadCmdSequenceData;
  }

  delete [] pReadCmdsSequenceDataBuffer;
}

FTC_STATUS FT2232hMpsseJtag::CreateDeviceCommandsSequenceDataBuffers(FTC_HANDLE ftHandle)
{
  FTC_STATUS Status = FTC_SUCCESS;
  DWORD dwDeviceIndex = 0;
  bool bDeviceDataBuffersCreated = false;

  for (dwDeviceIndex = 0; ((dwDeviceIndex < MAX_NUM_DEVICES) && !bDeviceDataBuffersCreated); dwDeviceIndex++)
  {
    if (OpenedDevicesCommandsSequenceData[dwDeviceIndex].hDevice == 0)
    {
      bDeviceDataBuffersCreated = true;

      OpenedDevicesCommandsSequenceData[dwDeviceIndex].pCommandsSequenceDataOutPutBuffer = POutputByteBuffer(new OutputByteBuffer);

      if (OpenedDevicesCommandsSequenceData[dwDeviceIndex].pCommandsSequenceDataOutPutBuffer != NULL)
      {
        OpenedDevicesCommandsSequenceData[dwDeviceIndex].pReadCommandsSequenceDataBuffer = CreateReadCommandsSequenceDataBuffer(INIT_COMMAND_SEQUENCE_READ_DATA_BUFFER_SIZE);

        if (OpenedDevicesCommandsSequenceData[dwDeviceIndex].pReadCommandsSequenceDataBuffer != NULL)
        {
          OpenedDevicesCommandsSequenceData[dwDeviceIndex].hDevice = ftHandle;
          OpenedDevicesCommandsSequenceData[dwDeviceIndex].dwNumBytesToSend = 0;
          OpenedDevicesCommandsSequenceData[dwDeviceIndex].dwSizeReadCommandsSequenceDataBuffer = INIT_COMMAND_SEQUENCE_READ_DATA_BUFFER_SIZE;
          OpenedDevicesCommandsSequenceData[dwDeviceIndex].dwNumReadCommandSequences = 0;
        }
        else
        {
          delete [] OpenedDevicesCommandsSequenceData[dwDeviceIndex].pCommandsSequenceDataOutPutBuffer;

          Status = FTC_INSUFFICIENT_RESOURCES;
        }
      }
      else
        Status = FTC_INSUFFICIENT_RESOURCES;
    }
  }

  if ((Status == FTC_SUCCESS) && (bDeviceDataBuffersCreated == true))
    dwNumOpenedDevices = dwNumOpenedDevices + 1;

  return Status;
}

void FT2232hMpsseJtag::ClearDeviceCommandSequenceData(FTC_HANDLE ftHandle)
{
  DWORD dwDeviceIndex = 0;
  BOOLEAN bDeviceHandleFound = false;

  if (ftHandle != 0)
  {
    for (dwDeviceIndex = 0; ((dwDeviceIndex < MAX_NUM_DEVICES) && !bDeviceHandleFound); dwDeviceIndex++)
    {
      if (OpenedDevicesCommandsSequenceData[dwDeviceIndex].hDevice == ftHandle)
      {
        bDeviceHandleFound = true;

        OpenedDevicesCommandsSequenceData[dwDeviceIndex].dwNumBytesToSend = 0;
        OpenedDevicesCommandsSequenceData[dwDeviceIndex].dwNumReadCommandSequences = 0;
      }
    }
  }
  else
  {
    // This code is executed if there is only one device connected to the system, this code is here just in case
    // that a device was unplugged from the system, while the system was still running
    for (dwDeviceIndex = 0; ((dwDeviceIndex < MAX_NUM_DEVICES) && !bDeviceHandleFound); dwDeviceIndex++)
    {
      if (OpenedDevicesCommandsSequenceData[dwDeviceIndex].hDevice != 0)
      {
        bDeviceHandleFound = true;

        OpenedDevicesCommandsSequenceData[dwDeviceIndex].dwNumBytesToSend = 0;
        OpenedDevicesCommandsSequenceData[dwDeviceIndex].dwNumReadCommandSequences = 0;
      }
    }
  }
}

DWORD FT2232hMpsseJtag::GetNumBytesInCommandsSequenceDataBuffer(void)
{
  DWORD dwNumBytesToSend = 0;

  if (iCommandsSequenceDataDeviceIndex != -1)
    // Get the number commands to be executed in sequence ie write, read and write/read
    dwNumBytesToSend = OpenedDevicesCommandsSequenceData[iCommandsSequenceDataDeviceIndex].dwNumBytesToSend;

  return dwNumBytesToSend;
}

DWORD FT2232hMpsseJtag::GetCommandsSequenceDataDeviceIndex(FTC_HANDLE ftHandle)
{
  DWORD dwDeviceIndex = 0;
  BOOLEAN bDeviceHandleFound = false;
  INT iCmdsSequenceDataDeviceIndex = 0;

  if (ftHandle != 0)
  {
    for (dwDeviceIndex = 0; ((dwDeviceIndex < MAX_NUM_DEVICES) && !bDeviceHandleFound); dwDeviceIndex++)
    {
      if (OpenedDevicesCommandsSequenceData[dwDeviceIndex].hDevice == ftHandle)
      {
        bDeviceHandleFound = true;

        iCmdsSequenceDataDeviceIndex = dwDeviceIndex;
      }
    }
  }
  else
  {
    // This code is executed if there is only one device connected to the system, this code is here just in case
    // that a device was unplugged from the system, while the system was still running
    for (dwDeviceIndex = 0; ((dwDeviceIndex < MAX_NUM_DEVICES) && !bDeviceHandleFound); dwDeviceIndex++)
    {
      if (OpenedDevicesCommandsSequenceData[dwDeviceIndex].hDevice != 0)
      {
        bDeviceHandleFound = true;

        iCmdsSequenceDataDeviceIndex = dwDeviceIndex;
      }
    }
  }

  return iCmdsSequenceDataDeviceIndex;
}

void FT2232hMpsseJtag::DeleteDeviceCommandsSequenceDataBuffers(FTC_HANDLE ftHandle)
{
  DWORD dwDeviceIndex = 0;
  BOOLEAN bDeviceHandleFound = false;
  POutputByteBuffer pCmdsSequenceDataOutPutBuffer;

  for (dwDeviceIndex = 0; ((dwDeviceIndex < MAX_NUM_DEVICES) && !bDeviceHandleFound); dwDeviceIndex++)
  {
    if (OpenedDevicesCommandsSequenceData[dwDeviceIndex].hDevice == ftHandle)
    {
      bDeviceHandleFound = true;

      OpenedDevicesCommandsSequenceData[dwDeviceIndex].hDevice = 0;
      OpenedDevicesCommandsSequenceData[dwDeviceIndex].dwNumBytesToSend = 0;
      pCmdsSequenceDataOutPutBuffer = OpenedDevicesCommandsSequenceData[dwDeviceIndex].pCommandsSequenceDataOutPutBuffer;
      delete [] pCmdsSequenceDataOutPutBuffer;
      OpenedDevicesCommandsSequenceData[dwDeviceIndex].pCommandsSequenceDataOutPutBuffer = NULL;
      DeleteReadCommandsSequenceDataBuffer(OpenedDevicesCommandsSequenceData[dwDeviceIndex].pReadCommandsSequenceDataBuffer,
                                           OpenedDevicesCommandsSequenceData[dwDeviceIndex].dwSizeReadCommandsSequenceDataBuffer);
      OpenedDevicesCommandsSequenceData[dwDeviceIndex].pReadCommandsSequenceDataBuffer = NULL;
    }
  }

  if ((dwNumOpenedDevices > 0) && bDeviceHandleFound)
    dwNumOpenedDevices = dwNumOpenedDevices - 1;
}

FTC_STATUS FT2232hMpsseJtag::AddDeviceWriteCommand(FTC_HANDLE ftHandle, BOOL bInstructionTestData, DWORD dwNumBitsToWrite,
                                                   PWriteDataByteBuffer pWriteDataBuffer, DWORD dwNumBytesToWrite,
                                                   DWORD dwTapControllerState)
{
  FTC_STATUS Status = FTC_SUCCESS;
  DWORD dwNumCommandDataBytes = 0;

  if (pWriteDataBuffer != NULL)
  {
    Status = CheckWriteDataToExternalDeviceBitsBytesParameters(dwNumBitsToWrite, dwNumBytesToWrite);

    if (Status == FTC_SUCCESS)
    {
      if ((dwTapControllerState >= TEST_LOGIC_STATE) && (dwTapControllerState <= SHIFT_INSTRUCTION_REGISTER_STATE))
      {
        dwNumCommandDataBytes = (NUM_WRITE_COMMAND_BYTES + dwNumBytesToWrite);

        iCommandsSequenceDataDeviceIndex = GetCommandsSequenceDataDeviceIndex(ftHandle);

        if ((GetNumBytesInCommandsSequenceDataBuffer() + dwNumCommandDataBytes) < OUTPUT_BUFFER_SIZE)
          AddWriteCommandDataToOutPutBuffer(bInstructionTestData, dwNumBitsToWrite, pWriteDataBuffer,
                                            dwNumBytesToWrite, dwTapControllerState);
        else
          Status = FTC_COMMAND_SEQUENCE_BUFFER_FULL;

        // Reset to indicate that you are not building up a sequence of commands
        iCommandsSequenceDataDeviceIndex = -1;
      }
      else
        Status = FTC_INVALID_TAP_CONTROLLER_STATE;
    }
  }
  else
    Status = FTC_NULL_WRITE_DATA_BUFFER_POINTER;

  return Status;
}

FTC_STATUS FT2232hMpsseJtag::AddDeviceReadCommand(FTC_HANDLE ftHandle, BOOL bInstructionTestData, DWORD dwNumBitsToRead, DWORD dwTapControllerState)
{
  FTC_STATUS Status = FTC_SUCCESS;
  DWORD dwNumTmsClocks = 0;

  if ((dwNumBitsToRead >= MIN_NUM_BITS) && (dwNumBitsToRead <= MAX_NUM_BITS))
  {
    if ((dwTapControllerState >= TEST_LOGIC_STATE) && (dwTapControllerState <= SHIFT_INSTRUCTION_REGISTER_STATE))
    {
      iCommandsSequenceDataDeviceIndex = GetCommandsSequenceDataDeviceIndex(ftHandle);

      if ((GetNumBytesInCommandsSequenceDataBuffer() + NUM_READ_COMMAND_BYTES) < OUTPUT_BUFFER_SIZE)
      {
        dwNumTmsClocks = AddReadCommandToOutputBuffer(bInstructionTestData, dwNumBitsToRead, dwTapControllerState);

        Status = AddReadCommandSequenceData(dwNumBitsToRead, dwNumTmsClocks);
      }
      else
        Status = FTC_COMMAND_SEQUENCE_BUFFER_FULL;

      // Reset to indicate that you are not building up a sequence of commands
      iCommandsSequenceDataDeviceIndex = -1;
    }
    else
      Status = FTC_INVALID_TAP_CONTROLLER_STATE;
  }
  else
    Status = FTC_INVALID_NUMBER_BITS;

  return Status;
}

FTC_STATUS FT2232hMpsseJtag::AddDeviceWriteReadCommand(FTC_HANDLE ftHandle, BOOL bInstructionTestData, DWORD dwNumBitsToWriteRead,
                                                       PWriteDataByteBuffer pWriteDataBuffer, DWORD dwNumBytesToWrite,
                                                       DWORD dwTapControllerState)
{
  FTC_STATUS Status = FTC_SUCCESS;
  DWORD dwNumCommandDataBytes = 0;
  DWORD dwNumTmsClocks = 0;

  if (pWriteDataBuffer != NULL)
  {
    Status = CheckWriteDataToExternalDeviceBitsBytesParameters(dwNumBitsToWriteRead, dwNumBytesToWrite);

    if (Status == FTC_SUCCESS)
    {
      if ((dwTapControllerState >= TEST_LOGIC_STATE) && (dwTapControllerState <= SHIFT_INSTRUCTION_REGISTER_STATE))
      {
        dwNumCommandDataBytes = (NUM_WRITE_READ_COMMAND_BYTES + dwNumBytesToWrite);

        iCommandsSequenceDataDeviceIndex = GetCommandsSequenceDataDeviceIndex(ftHandle);

        if ((GetNumBytesInCommandsSequenceDataBuffer() + dwNumCommandDataBytes) < OUTPUT_BUFFER_SIZE)
        {
          dwNumTmsClocks = AddWriteReadCommandDataToOutPutBuffer(bInstructionTestData, dwNumBitsToWriteRead, pWriteDataBuffer,
                                                                   dwNumBytesToWrite, dwTapControllerState);
 
          Status = AddReadCommandSequenceData(dwNumBitsToWriteRead, dwNumTmsClocks);
        }
        else
          Status = FTC_COMMAND_SEQUENCE_BUFFER_FULL;

        // Reset to indicate that you are not building up a sequence of commands
        iCommandsSequenceDataDeviceIndex = -1;
      }
      else
        Status = FTC_INVALID_TAP_CONTROLLER_STATE;
    }
  }
  else
    Status = FTC_NULL_WRITE_DATA_BUFFER_POINTER;

  return Status;
}

FT2232hMpsseJtag::FT2232hMpsseJtag(void)
{
  DWORD dwDeviceIndex = 0;

  CurrentJtagState = Undefined;

  dwSavedLowPinsDirection = 0;
  dwSavedLowPinsValue = 0;

  dwNumOpenedDevices = 0;

  for (dwDeviceIndex = 0; (dwDeviceIndex < MAX_NUM_DEVICES); dwDeviceIndex++)
    OpenedDevicesCommandsSequenceData[dwDeviceIndex].hDevice = 0;

  iCommandsSequenceDataDeviceIndex = -1;

  InitializeCriticalSection(&threadAccess);
}

FT2232hMpsseJtag::~FT2232hMpsseJtag(void)
{
  DWORD dwDeviceIndex = 0;
  POutputByteBuffer pCmdsSequenceDataOutPutBuffer;

  if (dwNumOpenedDevices > 0)
  {
    for (dwDeviceIndex = 0; (dwDeviceIndex < MAX_NUM_DEVICES); dwDeviceIndex++)
    {
      if (OpenedDevicesCommandsSequenceData[dwDeviceIndex].hDevice != 0)
      {
        OpenedDevicesCommandsSequenceData[dwDeviceIndex].hDevice = 0;

        pCmdsSequenceDataOutPutBuffer = OpenedDevicesCommandsSequenceData[dwDeviceIndex].pCommandsSequenceDataOutPutBuffer;

        if (pCmdsSequenceDataOutPutBuffer != NULL)
          delete [] pCmdsSequenceDataOutPutBuffer;

        OpenedDevicesCommandsSequenceData[dwDeviceIndex].pCommandsSequenceDataOutPutBuffer = NULL;

        if (OpenedDevicesCommandsSequenceData[dwDeviceIndex].pReadCommandsSequenceDataBuffer != NULL)
          DeleteReadCommandsSequenceDataBuffer(OpenedDevicesCommandsSequenceData[dwDeviceIndex].pReadCommandsSequenceDataBuffer,
                                               OpenedDevicesCommandsSequenceData[dwDeviceIndex].dwSizeReadCommandsSequenceDataBuffer);

        OpenedDevicesCommandsSequenceData[dwDeviceIndex].pReadCommandsSequenceDataBuffer = NULL;
      }
    }
  }

  DeleteCriticalSection(&threadAccess);
}

FTC_STATUS FT2232hMpsseJtag::JTAG_GetNumDevices(LPDWORD lpdwNumDevices)
{
  FTC_STATUS Status = FTC_SUCCESS;
  FT2232CDeviceIndexes FT2232CIndexes;

  EnterCriticalSection(&threadAccess);

  *lpdwNumDevices = 0;

  Status = FTC_GetNumDevices(lpdwNumDevices, &FT2232CIndexes);

  LeaveCriticalSection(&threadAccess);

  return Status;
}

FTC_STATUS FT2232hMpsseJtag::JTAG_GetNumHiSpeedDevices(LPDWORD lpdwNumHiSpeedDevices)
{
  FTC_STATUS Status = FTC_SUCCESS;
  HiSpeedDeviceIndexes HiSpeedIndexes;

  EnterCriticalSection(&threadAccess);

  *lpdwNumHiSpeedDevices = 0;

  Status = FTC_GetNumHiSpeedDevices(lpdwNumHiSpeedDevices, &HiSpeedIndexes);

  LeaveCriticalSection(&threadAccess);

  return Status;
}

FTC_STATUS FT2232hMpsseJtag::JTAG_GetDeviceNameLocationID(DWORD dwDeviceNameIndex, LPSTR lpDeviceNameBuffer, DWORD dwBufferSize, LPDWORD lpdwLocationID)
{
  FTC_STATUS Status = FTC_SUCCESS;

  EnterCriticalSection(&threadAccess);

  Status = FTC_GetDeviceNameLocationID(dwDeviceNameIndex, lpDeviceNameBuffer, dwBufferSize, lpdwLocationID);

  LeaveCriticalSection(&threadAccess);

  return Status;
}

FTC_STATUS FT2232hMpsseJtag::JTAG_GetHiSpeedDeviceNameLocationIDChannel(DWORD dwDeviceNameIndex, LPSTR lpDeviceNameBuffer, DWORD dwBufferSize, LPDWORD lpdwLocationID, LPSTR lpChannel, DWORD dwChannelBufferSize, LPDWORD lpdwHiSpeedDeviceType)
{
  FTC_STATUS Status = FTC_SUCCESS;
  DWORD dwDeviceType = 0;

  EnterCriticalSection(&threadAccess);

  *lpdwHiSpeedDeviceType = 0;

  Status = FTC_GetHiSpeedDeviceNameLocationIDChannel(dwDeviceNameIndex, lpDeviceNameBuffer, dwBufferSize, lpdwLocationID, lpChannel, dwChannelBufferSize, &dwDeviceType);

  if (Status == FTC_SUCCESS)
  {
    if ((dwDeviceType == FT_DEVICE_2232H) || (dwDeviceType == FT_DEVICE_4232H))
    {
      if (dwDeviceType == FT_DEVICE_2232H)
        *lpdwHiSpeedDeviceType = FT2232H_DEVICE_TYPE;
      else
        *lpdwHiSpeedDeviceType = FT4232H_DEVICE_TYPE;
    }
    else
      Status = FTC_DEVICE_NOT_FOUND;
  }

  LeaveCriticalSection(&threadAccess);

  return Status;
}

FTC_STATUS FT2232hMpsseJtag::JTAG_OpenDevice(FTC_HANDLE *pftHandle)
{
  FTC_STATUS Status = FTC_SUCCESS;

  EnterCriticalSection(&threadAccess);

  Status = FTC_OpenDevice(pftHandle);

  if (Status == FTC_SUCCESS)
  {
    Status = CreateDeviceCommandsSequenceDataBuffers(*pftHandle);

    if (Status != FTC_SUCCESS)
    {
      FTC_CloseDevice(*pftHandle);

      *pftHandle = 0;
    }
  }

  LeaveCriticalSection(&threadAccess);

  return Status;
}

FTC_STATUS FT2232hMpsseJtag::JTAG_OpenSpecifiedDevice(LPSTR lpDeviceName, DWORD dwLocationID, FTC_HANDLE *pftHandle)
{
  FTC_STATUS Status = FTC_SUCCESS;

  EnterCriticalSection(&threadAccess);

  Status = FTC_OpenSpecifiedDevice(lpDeviceName, dwLocationID, pftHandle);

  if (Status == FTC_SUCCESS)
  {
    Status = CreateDeviceCommandsSequenceDataBuffers(*pftHandle);

    if (Status != FTC_SUCCESS)
    {
      FTC_CloseDevice(*pftHandle);

      *pftHandle = 0;
    }
  }

  LeaveCriticalSection(&threadAccess);

  return Status;
}

FTC_STATUS FT2232hMpsseJtag::JTAG_OpenSpecifiedHiSpeedDevice(LPSTR lpDeviceName, DWORD dwLocationID, LPSTR lpChannel, FTC_HANDLE *pftHandle)
{
  FTC_STATUS Status = FTC_SUCCESS;

  EnterCriticalSection(&threadAccess);

  Status = FTC_OpenSpecifiedHiSpeedDevice(lpDeviceName, dwLocationID, lpChannel, pftHandle);

  if (Status == FTC_SUCCESS)
  {
    Status = CreateDeviceCommandsSequenceDataBuffers(*pftHandle);

    if (Status != FTC_SUCCESS)
    {
      FTC_CloseDevice(*pftHandle);

      *pftHandle = 0;
    }
  }

  LeaveCriticalSection(&threadAccess);

  return Status;
}

FTC_STATUS FT2232hMpsseJtag::JTAG_GetHiSpeedDeviceType(FTC_HANDLE ftHandle, LPDWORD lpdwHiSpeedDeviceType)
{
  FTC_STATUS Status = FTC_SUCCESS;
  BOOL bHiSpeedFT2232HTDeviceType = FALSE;

  EnterCriticalSection(&threadAccess);

  *lpdwHiSpeedDeviceType = 0;

  Status = FTC_GetHiSpeedDeviceType(ftHandle, &bHiSpeedFT2232HTDeviceType);

  if (Status == FTC_SUCCESS)
  {
    // Is the device a FT2232H hi-speed device
    if (bHiSpeedFT2232HTDeviceType == TRUE)
      *lpdwHiSpeedDeviceType = FT2232H_DEVICE_TYPE;
    else
      *lpdwHiSpeedDeviceType = FT4232H_DEVICE_TYPE;
  }

  LeaveCriticalSection(&threadAccess);

  return Status;
}


FTC_STATUS FT2232hMpsseJtag::JTAG_CloseDevice(FTC_HANDLE ftHandle)
{
  FTC_STATUS Status = FTC_SUCCESS;

  EnterCriticalSection(&threadAccess);

  if ((Status = FTC_CloseDevice(ftHandle)) == FTC_SUCCESS)
    DeleteDeviceCommandsSequenceDataBuffers(ftHandle);

  LeaveCriticalSection(&threadAccess);

  return Status;
}

FTC_STATUS WINAPI FT2232hMpsseJtag::JTAG_CloseDevice(FTC_HANDLE ftHandle, PFTC_CLOSE_FINAL_STATE_PINS pCloseFinalStatePinsData)
{
  FTC_STATUS Status = FTC_SUCCESS;

  EnterCriticalSection(&threadAccess);

  if ((Status = FTC_IsDeviceHandleValid(ftHandle)) == FTC_SUCCESS)
  {
    if (pCloseFinalStatePinsData != NULL)
    {
      if ((Status = SetTCKTDITMSPinsCloseState(ftHandle, pCloseFinalStatePinsData)) == FTC_SUCCESS)
        Status = JTAG_CloseDevice(ftHandle);
    }
    else
      Status = FTC_NULL_CLOSE_FINAL_STATE_BUFFER_POINTER;
  }

  LeaveCriticalSection(&threadAccess);

  return Status;
}

FTC_STATUS FT2232hMpsseJtag::JTAG_InitDevice(FTC_HANDLE ftHandle, DWORD dwClockDivisor)
{
  FTC_STATUS Status = FTC_SUCCESS;

  EnterCriticalSection(&threadAccess);

  if ((Status = FTC_IsHiSpeedDeviceHandleValid(ftHandle)) == FTC_SUCCESS)
  {
    if ((Status = FTC_InitHiSpeedDevice(ftHandle)) == FTC_SUCCESS)
    {
      Status = InitDevice(ftHandle, dwClockDivisor);
    }
  }

  if (Status == FTC_INVALID_HANDLE)
  {
    if ((Status = FT2232c::FTC_IsDeviceHandleValid(ftHandle)) == FTC_SUCCESS)
    {
      Status = InitDevice(ftHandle, dwClockDivisor);
    }
  }

  LeaveCriticalSection(&threadAccess);

  return Status;
}

FTC_STATUS FT2232hMpsseJtag::JTAG_TurnOnDivideByFiveClockingHiSpeedDevice(FTC_HANDLE ftHandle)
{
  FTC_STATUS Status = FTC_SUCCESS;

  EnterCriticalSection(&threadAccess);

  Status = FTC_TurnOnDivideByFiveClockingHiSpeedDevice(ftHandle);

  LeaveCriticalSection(&threadAccess);

  return Status;
}

FTC_STATUS FT2232hMpsseJtag::JTAG_TurnOffDivideByFiveClockingHiSpeedDevice(FTC_HANDLE ftHandle)
{
  FTC_STATUS Status = FTC_SUCCESS;

  EnterCriticalSection(&threadAccess);

  Status = FTC_TurnOffDivideByFiveClockingHiSpeedDevice(ftHandle);

  LeaveCriticalSection(&threadAccess);

  return Status;
}

FTC_STATUS FT2232hMpsseJtag::JTAG_TurnOnAdaptiveClockingHiSpeedDevice(FTC_HANDLE ftHandle)
{
  FTC_STATUS Status = FTC_SUCCESS;

  EnterCriticalSection(&threadAccess);

  Status = FTC_TurnOnAdaptiveClockingHiSpeedDevice(ftHandle);

  LeaveCriticalSection(&threadAccess);

  return Status;
}

FTC_STATUS FT2232hMpsseJtag::JTAG_TurnOffAdaptiveClockingHiSpeedDevice(FTC_HANDLE ftHandle)
{
  FTC_STATUS Status = FTC_SUCCESS;

  EnterCriticalSection(&threadAccess);

  Status = FTC_TurnOffAdaptiveClockingHiSpeedDevice(ftHandle);

  LeaveCriticalSection(&threadAccess);

  return Status;
}

FTC_STATUS FT2232hMpsseJtag::JTAG_SetDeviceLatencyTimer(FTC_HANDLE ftHandle, BYTE LatencyTimermSec)
{
  FTC_STATUS Status = FTC_SUCCESS;

  EnterCriticalSection(&threadAccess);

  Status = FTC_SetDeviceLatencyTimer(ftHandle, LatencyTimermSec);

  LeaveCriticalSection(&threadAccess);

  return Status;
}

FTC_STATUS FT2232hMpsseJtag::JTAG_GetDeviceLatencyTimer(FTC_HANDLE ftHandle, LPBYTE lpLatencyTimermSec)
{
  FTC_STATUS Status = FTC_SUCCESS;

  EnterCriticalSection(&threadAccess);

  Status = FTC_GetDeviceLatencyTimer(ftHandle, lpLatencyTimermSec);

  LeaveCriticalSection(&threadAccess);

  return Status;
}

FTC_STATUS FT2232hMpsseJtag::JTAG_GetClock(DWORD dwClockDivisor, LPDWORD lpdwClockFrequencyHz)
{
  FTC_STATUS Status = FTC_SUCCESS;

  EnterCriticalSection(&threadAccess);

  if ((dwClockDivisor >= MIN_CLOCK_DIVISOR) && (dwClockDivisor <= MAX_CLOCK_DIVISOR))
    FTC_GetClockFrequencyValues(dwClockDivisor, lpdwClockFrequencyHz);
  else
    Status = FTC_INVALID_CLOCK_DIVISOR;

  LeaveCriticalSection(&threadAccess);

  return Status;
}

FTC_STATUS FT2232hMpsseJtag::JTAG_GetHiSpeedDeviceClock(DWORD dwClockDivisor, LPDWORD lpdwClockFrequencyHz)
{
  FTC_STATUS Status = FTC_SUCCESS;

  EnterCriticalSection(&threadAccess);

  if ((dwClockDivisor >= MIN_CLOCK_DIVISOR) && (dwClockDivisor <= MAX_CLOCK_DIVISOR))
    FTC_GetHiSpeedDeviceClockFrequencyValues(dwClockDivisor, lpdwClockFrequencyHz);
  else
    Status = FTC_INVALID_CLOCK_DIVISOR;

  LeaveCriticalSection(&threadAccess);

  return Status;
}

FTC_STATUS FT2232hMpsseJtag::JTAG_SetClock(FTC_HANDLE ftHandle, DWORD dwClockDivisor, LPDWORD lpdwClockFrequencyHz)
{
  FTC_STATUS Status = FTC_SUCCESS;

  EnterCriticalSection(&threadAccess);

  if ((dwClockDivisor >= MIN_CLOCK_DIVISOR) && (dwClockDivisor <= MAX_CLOCK_DIVISOR))
  {
    if ((Status = FTC_IsHiSpeedDeviceHandleValid(ftHandle)) == FTC_SUCCESS)
    {
      FTC_GetHiSpeedDeviceClockFrequencyValues(ftHandle, dwClockDivisor, lpdwClockFrequencyHz);
    }

    if (Status == FTC_INVALID_HANDLE)
    {
      if ((Status = FT2232c::FTC_IsDeviceHandleValid(ftHandle)) == FTC_SUCCESS)
      {
        FTC_GetClockFrequencyValues(dwClockDivisor, lpdwClockFrequencyHz);
      }
    }

    if (Status == FTC_SUCCESS)
    {
      Status = SetDataInOutClockFrequency(ftHandle, dwClockDivisor);
    }
  }
  else
    Status = FTC_INVALID_CLOCK_DIVISOR;

  LeaveCriticalSection(&threadAccess);

  return Status;
}

FTC_STATUS FT2232hMpsseJtag::JTAG_SetDeviceLoopbackState(FTC_HANDLE ftHandle, BOOL bLoopbackState)
{
  FTC_STATUS Status = FTC_SUCCESS;

  EnterCriticalSection(&threadAccess);

  if ((Status = FTC_IsDeviceHandleValid(ftHandle)) == FTC_SUCCESS) {
    Status = FTC_SetDeviceLoopbackState(ftHandle, bLoopbackState);
  }

  LeaveCriticalSection(&threadAccess);

  return Status;
}

FTC_STATUS FT2232hMpsseJtag::JTAG_SetGeneralPurposeInputOutputPins(FTC_HANDLE ftHandle, BOOL bControlLowInputOutputPins,
                                                                   PFTC_INPUT_OUTPUT_PINS pLowInputOutputPinsData,
                                                                   BOOL bControlHighInputOutputPins,
                                                                   PFTC_INPUT_OUTPUT_PINS pHighInputOutputPinsData)
{
  FTC_STATUS Status = FTC_SUCCESS;

  EnterCriticalSection(&threadAccess);

  Status = FTC_IsDeviceHandleValid(ftHandle);

  if (Status == FTC_SUCCESS)
  {
    if ((pLowInputOutputPinsData != NULL) && (pHighInputOutputPinsData != NULL))
      Status = SetGeneralPurposeInputOutputPins(ftHandle, bControlLowInputOutputPins,
                                                pLowInputOutputPinsData,
                                                bControlHighInputOutputPins,
                                                pHighInputOutputPinsData);
    else
      Status = FTC_NULL_INPUT_OUTPUT_BUFFER_POINTER;
  }

  LeaveCriticalSection(&threadAccess);

  return Status;
}

FTC_STATUS FT2232hMpsseJtag::JTAG_SetHiSpeedDeviceGeneralPurposeInputOutputPins(FTC_HANDLE ftHandle, BOOL bControlLowInputOutputPins,
                                                                                PFTC_INPUT_OUTPUT_PINS pLowInputOutputPinsData,
                                                                                BOOL bControlHighInputOutputPins,
                                                                                PFTH_INPUT_OUTPUT_PINS pHighInputOutputPinsData)
{
  FTC_STATUS Status = FTC_SUCCESS;

  EnterCriticalSection(&threadAccess);

  if ((Status = FTC_IsHiSpeedDeviceHandleValid(ftHandle)) == FTC_SUCCESS)
  {
    if ((pLowInputOutputPinsData != NULL) && (pHighInputOutputPinsData != NULL))
      Status = SetHiSpeedDeviceGeneralPurposeInputOutputPins(ftHandle, bControlLowInputOutputPins,
                                                             pLowInputOutputPinsData,
                                                             bControlHighInputOutputPins,
                                                             pHighInputOutputPinsData);
    else
      Status = FTC_NULL_INPUT_OUTPUT_BUFFER_POINTER;
  }

  LeaveCriticalSection(&threadAccess);

  return Status;
}

FTC_STATUS FT2232hMpsseJtag::JTAG_GetGeneralPurposeInputOutputPins(FTC_HANDLE ftHandle, BOOL bControlLowInputOutputPins,
                                                                   PFTC_LOW_HIGH_PINS pLowPinsInputData,
                                                                   BOOL bControlHighInputOutputPins,
                                                                   PFTC_LOW_HIGH_PINS pHighPinsInputData)
{
  FTC_STATUS Status = FTC_SUCCESS;

  EnterCriticalSection(&threadAccess);

  Status = FTC_IsDeviceHandleValid(ftHandle);

  if (Status == FTC_SUCCESS)
  {
    if ((pLowPinsInputData != NULL) && (pHighPinsInputData != NULL))
      Status = GetGeneralPurposeInputOutputPins(ftHandle, bControlLowInputOutputPins, pLowPinsInputData,
                                                bControlHighInputOutputPins, pHighPinsInputData);
    else
      Status = FTC_NULL_INPUT_OUTPUT_BUFFER_POINTER;
  }

  LeaveCriticalSection(&threadAccess);

  return Status;
}

FTC_STATUS FT2232hMpsseJtag::JTAG_GetHiSpeedDeviceGeneralPurposeInputOutputPins(FTC_HANDLE ftHandle, BOOL bControlLowInputOutputPins,
                                                                                PFTC_LOW_HIGH_PINS pLowPinsInputData,
                                                                                BOOL bControlHighInputOutputPins,
                                                                                PFTH_LOW_HIGH_PINS pHighPinsInputData)
{
  FTC_STATUS Status = FTC_SUCCESS;

  EnterCriticalSection(&threadAccess);

  if ((Status = FTC_IsHiSpeedDeviceHandleValid(ftHandle)) == FTC_SUCCESS)
  {
    if ((pLowPinsInputData != NULL) && (pHighPinsInputData != NULL))
      Status = GetHiSpeedDeviceGeneralPurposeInputOutputPins(ftHandle, bControlLowInputOutputPins, pLowPinsInputData,
                                                             bControlHighInputOutputPins, pHighPinsInputData);
    else
      Status = FTC_NULL_INPUT_OUTPUT_BUFFER_POINTER;
  }

  LeaveCriticalSection(&threadAccess);

  return Status;
}

FTC_STATUS FT2232hMpsseJtag::JTAG_WriteDataToExternalDevice(FTC_HANDLE ftHandle, BOOL bInstructionTestData, DWORD dwNumBitsToWrite,
                                                            PWriteDataByteBuffer pWriteDataBuffer, DWORD dwNumBytesToWrite,
                                                            DWORD dwTapControllerState)
{
  FTC_STATUS Status = FTC_SUCCESS;

  EnterCriticalSection(&threadAccess);

  Status = FTC_IsDeviceHandleValid(ftHandle);

  if (Status == FTC_SUCCESS)
  {
    if (pWriteDataBuffer != NULL)
    {
      Status = CheckWriteDataToExternalDeviceBitsBytesParameters(dwNumBitsToWrite, dwNumBytesToWrite);

      if (Status == FTC_SUCCESS)
      {
        if ((dwTapControllerState >= TEST_LOGIC_STATE) && (dwTapControllerState <= SHIFT_INSTRUCTION_REGISTER_STATE))
          Status = WriteDataToExternalDevice(ftHandle, bInstructionTestData, dwNumBitsToWrite, pWriteDataBuffer,
                                             dwNumBytesToWrite, dwTapControllerState);
        else
          Status = FTC_INVALID_TAP_CONTROLLER_STATE;
      }
    }
    else
      Status = FTC_NULL_WRITE_DATA_BUFFER_POINTER;
  }

  LeaveCriticalSection(&threadAccess);

  return Status;
}

FTC_STATUS FT2232hMpsseJtag::JTAG_ReadDataFromExternalDevice(FTC_HANDLE ftHandle, BOOL bInstructionTestData, DWORD dwNumBitsToRead,
                                                             PReadDataByteBuffer pReadDataBuffer, LPDWORD lpdwNumBytesReturned,
                                                             DWORD dwTapControllerState)
{
  FTC_STATUS Status = FTC_SUCCESS;

  EnterCriticalSection(&threadAccess);

  Status = FTC_IsDeviceHandleValid(ftHandle);

  if (Status == FTC_SUCCESS)
  {
    if (pReadDataBuffer != NULL)
    {
      if ((dwNumBitsToRead >= MIN_NUM_BITS) && (dwNumBitsToRead <= MAX_NUM_BITS))
      {
        if ((dwTapControllerState >= TEST_LOGIC_STATE) && (dwTapControllerState <= SHIFT_INSTRUCTION_REGISTER_STATE))
          Status = ReadDataFromExternalDevice(ftHandle, bInstructionTestData, dwNumBitsToRead, pReadDataBuffer,
                                              lpdwNumBytesReturned, dwTapControllerState);
        else
          Status = FTC_INVALID_TAP_CONTROLLER_STATE;
      }
      else
        Status = FTC_INVALID_NUMBER_BITS;
    }
    else
      Status = FTC_NULL_READ_DATA_BUFFER_POINTER;
  }

  LeaveCriticalSection(&threadAccess);

  return Status;
}

FTC_STATUS FT2232hMpsseJtag::JTAG_WriteReadDataToFromExternalDevice(FTC_HANDLE ftHandle, BOOL bInstructionTestData, DWORD dwNumBitsToWriteRead,
                                                                    PWriteDataByteBuffer pWriteDataBuffer, DWORD dwNumBytesToWrite,
                                                                    PReadDataByteBuffer pReadDataBuffer, LPDWORD lpdwNumBytesReturned,
                                                                    DWORD dwTapControllerState)
{
  FTC_STATUS Status = FTC_SUCCESS;

  EnterCriticalSection(&threadAccess);

  Status = FTC_IsDeviceHandleValid(ftHandle);

  if (Status == FTC_SUCCESS)
  {
    if ((pWriteDataBuffer != NULL) && (pReadDataBuffer != NULL))
    {
      Status = CheckWriteDataToExternalDeviceBitsBytesParameters(dwNumBitsToWriteRead, dwNumBytesToWrite);

      if (Status == FTC_SUCCESS)
      {
        if ((dwTapControllerState >= TEST_LOGIC_STATE) && (dwTapControllerState <= SHIFT_INSTRUCTION_REGISTER_STATE))
          Status = WriteReadDataToFromExternalDevice(ftHandle, bInstructionTestData, dwNumBitsToWriteRead,
                                                     pWriteDataBuffer, dwNumBytesToWrite, pReadDataBuffer,
                                                     lpdwNumBytesReturned, dwTapControllerState);
        else
          Status = FTC_INVALID_TAP_CONTROLLER_STATE;
      }
    }
    else
    {
      if (pWriteDataBuffer == NULL)
        Status = FTC_NULL_WRITE_DATA_BUFFER_POINTER;
      else
        Status = FTC_NULL_READ_DATA_BUFFER_POINTER;
    }
  }

  LeaveCriticalSection(&threadAccess);

  return Status;
}

FTC_STATUS FT2232hMpsseJtag::JTAG_GenerateTCKClockPulses(FTC_HANDLE ftHandle, DWORD dwNumClockPulses)
{
  FTC_STATUS Status = FTC_SUCCESS;

  EnterCriticalSection(&threadAccess);

  if ((Status = FTC_IsDeviceHandleValid(ftHandle)) == FTC_SUCCESS)
  {
    if ((dwNumClockPulses >= MIN_NUM_CLOCK_PULSES) && (dwNumClockPulses <= MAX_NUM_CLOCK_PULSES))
      Status = GenerateTCKClockPulses(ftHandle, dwNumClockPulses);
    else
      Status = FTC_INVALID_NUMBER_CLOCK_PULSES;
  }

  LeaveCriticalSection(&threadAccess);

  return Status;
}

FTC_STATUS FT2232hMpsseJtag::JTAG_GenerateClockPulsesHiSpeedDevice(FTC_HANDLE ftHandle, BOOL bPulseClockTimesEightFactor, DWORD dwNumClockPulses, BOOL bControlLowInputOutputPin, BOOL bStopClockPulsesState)
{
  FTC_STATUS Status = FTC_SUCCESS;

  EnterCriticalSection(&threadAccess);

  if ((Status = FTC_IsHiSpeedDeviceHandleValid(ftHandle)) == FTC_SUCCESS)
  {
    if (bPulseClockTimesEightFactor)
    {
      if ((dwNumClockPulses < MIN_NUM_CLOCK_PULSES) || (dwNumClockPulses > MAX_NUM_TIMES_EIGHT_CLOCK_PULSES))
        Status = FTC_INVALID_NUMBER_TIMES_EIGHT_CLOCK_PULSES;
    }
    else
    {
      if ((dwNumClockPulses < MIN_NUM_CLOCK_PULSES) || (dwNumClockPulses > MAX_NUM_SINGLE_CLOCK_PULSES))
        Status = FTC_INVALID_NUMBER_SINGLE_CLOCK_PULSES;
    }
  }

  if (Status == FTC_SUCCESS)
  {
    Status = GenerateClockPulsesHiSpeedDevice(ftHandle, bPulseClockTimesEightFactor, dwNumClockPulses, bControlLowInputOutputPin, bStopClockPulsesState);
  }

  LeaveCriticalSection(&threadAccess);

  return Status;
}


FTC_STATUS FT2232hMpsseJtag::JTAG_ClearCommandSequence(void)
{
  FTC_STATUS Status = FTC_SUCCESS;
  DWORD dwNumDevices;
  FT2232CDeviceIndexes FT2232CIndexes;

  EnterCriticalSection(&threadAccess);

  Status = FTC_GetNumDevices(&dwNumDevices, &FT2232CIndexes);

  if (Status == FTC_SUCCESS)
  {
    if (dwNumDevices == 1)
      ClearDeviceCommandSequenceData(0);
    else
      Status = FTC_TOO_MANY_DEVICES;
  }

  LeaveCriticalSection(&threadAccess);

  return Status;
}

FTC_STATUS FT2232hMpsseJtag::JTAG_AddWriteCommand(BOOL bInstructionTestData, DWORD dwNumBitsToWrite,
                                                  PWriteDataByteBuffer pWriteDataBuffer, DWORD dwNumBytesToWrite,
                                                  DWORD dwTapControllerState)
{
  FTC_STATUS Status = FTC_SUCCESS;
  DWORD dwNumCommandDataBytes = 0;
  DWORD dwNumDevices;
  FT2232CDeviceIndexes FT2232CIndexes;

  EnterCriticalSection(&threadAccess);

  Status = FTC_GetNumDevices(&dwNumDevices, &FT2232CIndexes);

  if (Status == FTC_SUCCESS)
  {
    if (dwNumDevices == 1)
      // ftHandle parameter set to 0 to indicate only one device present in the system
      Status = AddDeviceWriteCommand(0, bInstructionTestData, dwNumBitsToWrite, pWriteDataBuffer, dwNumBytesToWrite, dwTapControllerState);
    else
      Status = FTC_TOO_MANY_DEVICES;
  }

  LeaveCriticalSection(&threadAccess);

  return Status;
}

FTC_STATUS FT2232hMpsseJtag::JTAG_AddReadCommand(BOOL bInstructionTestData, DWORD dwNumBitsToRead, DWORD dwTapControllerState)
{
  FTC_STATUS Status = FTC_SUCCESS;
  DWORD dwNumTmsClocks = 0;
  DWORD dwNumDevices;
  FT2232CDeviceIndexes FT2232CIndexes;

  EnterCriticalSection(&threadAccess);

  Status = FTC_GetNumDevices(&dwNumDevices, &FT2232CIndexes);

  if (Status == FTC_SUCCESS)
  {
    if (dwNumDevices == 1)
      // ftHandle parameter set to 0 to indicate only one device present in the system
      Status = AddDeviceReadCommand(0, bInstructionTestData, dwNumBitsToRead, dwTapControllerState);
    else
      Status = FTC_TOO_MANY_DEVICES;
  }

  LeaveCriticalSection(&threadAccess);

  return Status;
}

FTC_STATUS FT2232hMpsseJtag::JTAG_AddWriteReadCommand(BOOL bInstructionTestData, DWORD dwNumBitsToWriteRead,
                                                      PWriteDataByteBuffer pWriteDataBuffer, DWORD dwNumBytesToWrite,
                                                      DWORD dwTapControllerState)
{
  FTC_STATUS Status = FTC_SUCCESS;
  DWORD dwNumCommandDataBytes = 0;
  DWORD dwNumTmsClocks = 0;
  DWORD dwNumDevices;
  FT2232CDeviceIndexes FT2232CIndexes;

  EnterCriticalSection(&threadAccess);

  Status = FTC_GetNumDevices(&dwNumDevices, &FT2232CIndexes);

  if (Status == FTC_SUCCESS)
  {
    if (dwNumDevices == 1)
      // ftHandle parameter set to 0 to indicate only one device present in the system
      Status = AddDeviceWriteReadCommand(0, bInstructionTestData, dwNumBitsToWriteRead, pWriteDataBuffer, dwNumBytesToWrite, dwTapControllerState);
    else
      Status = FTC_TOO_MANY_DEVICES;
  }

  LeaveCriticalSection(&threadAccess);

  return Status;
}

FTC_STATUS FT2232hMpsseJtag::JTAG_ClearDeviceCommandSequence(FTC_HANDLE ftHandle)
{
  FTC_STATUS Status = FTC_SUCCESS;

  EnterCriticalSection(&threadAccess);

  Status = FTC_IsDeviceHandleValid(ftHandle);

  if (Status == FTC_SUCCESS)
    ClearDeviceCommandSequenceData(ftHandle);

  LeaveCriticalSection(&threadAccess);

  return Status;
}

FTC_STATUS FT2232hMpsseJtag::JTAG_AddDeviceWriteCommand(FTC_HANDLE ftHandle, BOOL bInstructionTestData, DWORD dwNumBitsToWrite,
                                                        PWriteDataByteBuffer pWriteDataBuffer, DWORD dwNumBytesToWrite,
                                                        DWORD dwTapControllerState)
{
  FTC_STATUS Status = FTC_SUCCESS;

  EnterCriticalSection(&threadAccess);

  Status = FTC_IsDeviceHandleValid(ftHandle);

  if (Status == FTC_SUCCESS)
    Status = AddDeviceWriteCommand(ftHandle, bInstructionTestData, dwNumBitsToWrite, pWriteDataBuffer, dwNumBytesToWrite, dwTapControllerState);

  LeaveCriticalSection(&threadAccess);

  return Status;
}

FTC_STATUS FT2232hMpsseJtag::JTAG_AddDeviceReadCommand(FTC_HANDLE ftHandle, BOOL bInstructionTestData, DWORD dwNumBitsToRead, DWORD dwTapControllerState)
{
  FTC_STATUS Status = FTC_SUCCESS;

  EnterCriticalSection(&threadAccess);

  Status = FTC_IsDeviceHandleValid(ftHandle);

  if (Status == FTC_SUCCESS)
    Status = AddDeviceReadCommand(ftHandle, bInstructionTestData, dwNumBitsToRead, dwTapControllerState);

  LeaveCriticalSection(&threadAccess);

  return Status;
}

FTC_STATUS FT2232hMpsseJtag::JTAG_AddDeviceWriteReadCommand(FTC_HANDLE ftHandle, BOOL bInstructionTestData, DWORD dwNumBitsToWriteRead,
                                                            PWriteDataByteBuffer pWriteDataBuffer, DWORD dwNumBytesToWrite,
                                                            DWORD dwTapControllerState)
{
  FTC_STATUS Status = FTC_SUCCESS;

  EnterCriticalSection(&threadAccess);

  Status = FTC_IsDeviceHandleValid(ftHandle);

  if (Status == FTC_SUCCESS)
    Status = AddDeviceWriteReadCommand(ftHandle, bInstructionTestData, dwNumBitsToWriteRead, pWriteDataBuffer, dwNumBytesToWrite, dwTapControllerState);

  LeaveCriticalSection(&threadAccess);

  return Status;
}

FTC_STATUS FT2232hMpsseJtag::JTAG_ExecuteCommandSequence(FTC_HANDLE ftHandle, PReadCmdSequenceDataByteBuffer pReadCmdSequenceDataBuffer,
                                                         LPDWORD lpdwNumBytesReturned)
{
  FTC_STATUS Status = FTC_SUCCESS;
  DWORD dwNumCmdSequenceBytes = 0;
  DWORD dwCmdSequenceByteIndex = 0;
  InputByteBuffer InputBuffer;
  DWORD dwTotalNumBytesToBeRead = 0;
  DWORD dwNumBytesRead = 0;

  EnterCriticalSection(&threadAccess);

  Status = FTC_IsDeviceHandleValid(ftHandle);

  if (Status == FTC_SUCCESS)
  {
    if (pReadCmdSequenceDataBuffer != NULL)
    {
      iCommandsSequenceDataDeviceIndex = GetCommandsSequenceDataDeviceIndex(ftHandle);

      dwNumCmdSequenceBytes = GetNumBytesInCommandsSequenceDataBuffer();

      if (dwNumCmdSequenceBytes > 0)
      {
        AddByteToOutputBuffer(SEND_ANSWER_BACK_IMMEDIATELY_CMD, false);

        FTC_ClearOutputBuffer();

        dwNumCmdSequenceBytes = GetNumBytesInCommandsSequenceDataBuffer();
          
        // Transfer sequence of commands for specified device to output buffer for transmission to device
        for (dwCmdSequenceByteIndex = 0; (dwCmdSequenceByteIndex < dwNumCmdSequenceBytes); dwCmdSequenceByteIndex++)
          FTC_AddByteToOutputBuffer((*OpenedDevicesCommandsSequenceData[iCommandsSequenceDataDeviceIndex].pCommandsSequenceDataOutPutBuffer)[dwCmdSequenceByteIndex], false);

        OpenedDevicesCommandsSequenceData[iCommandsSequenceDataDeviceIndex].dwNumBytesToSend = 0;

        Status = FTC_SendCommandsSequenceToDevice(ftHandle);

        if (Status == FTC_SUCCESS)
        {
          if (OpenedDevicesCommandsSequenceData[iCommandsSequenceDataDeviceIndex].dwNumReadCommandSequences > 0)
          {
            // Calculate the total number of bytes to be read, as a result of a command sequence
            dwTotalNumBytesToBeRead = GetTotalNumCommandsSequenceDataBytesToRead();

            Status = FTC_ReadCommandsSequenceBytesFromDevice(ftHandle, &InputBuffer, dwTotalNumBytesToBeRead, &dwNumBytesRead);
        
            if (Status == FTC_SUCCESS)
            {
              // Process all bytes received and return them in the read data buffer
              ProcessReadCommandsSequenceBytes(&InputBuffer, dwNumBytesRead, pReadCmdSequenceDataBuffer, lpdwNumBytesReturned);
            }
          }
        }

        OpenedDevicesCommandsSequenceData[iCommandsSequenceDataDeviceIndex].dwNumReadCommandSequences = 0;
      }
      else
        Status = FTC_NO_COMMAND_SEQUENCE;

      iCommandsSequenceDataDeviceIndex = -1;
    }
    else
      Status = FTC_NULL_READ_CMDS_DATA_BUFFER_POINTER;
  }

  LeaveCriticalSection(&threadAccess);

  return Status;
}

FTC_STATUS FT2232hMpsseJtag::JTAG_GetDllVersion(LPSTR lpDllVersionBuffer, DWORD dwBufferSize)
{
  FTC_STATUS Status = FTC_SUCCESS;

  EnterCriticalSection(&threadAccess);

  if (lpDllVersionBuffer != NULL)
  {
    if (dwBufferSize > strlen(DLL_VERSION_NUM))
      strcpy(lpDllVersionBuffer, DLL_VERSION_NUM);
    else
      Status = FTC_DLL_VERSION_BUFFER_TOO_SMALL;
  }
  else
    Status = FTC_NULL_DLL_VERSION_BUFFER_POINTER;

  LeaveCriticalSection(&threadAccess);

  return Status;
}

FTC_STATUS FT2232hMpsseJtag::JTAG_GetErrorCodeString(LPSTR lpLanguage, FTC_STATUS StatusCode,
                                                     LPSTR lpErrorMessageBuffer, DWORD dwBufferSize)
{
  FTC_STATUS Status = FTC_SUCCESS;
  char szErrorMsg[MAX_ERROR_MSG_SIZE];
  INT iCharCntr = 0;

  EnterCriticalSection(&threadAccess);

  if ((lpLanguage != NULL) && (lpErrorMessageBuffer != NULL))
  {
    for (iCharCntr = 0; (iCharCntr < MAX_ERROR_MSG_SIZE); iCharCntr++)
      szErrorMsg[iCharCntr] = '\0';

    if (((StatusCode >= FTC_SUCCESS) && (StatusCode <= FTC_INSUFFICIENT_RESOURCES)) ||
        ((StatusCode >= FTC_FAILED_TO_COMPLETE_COMMAND) && (StatusCode <= FTC_INVALID_STATUS_CODE)))
    {
      if (strcmp(lpLanguage, ENGLISH) == 0)
      {
        if ((StatusCode >= FTC_SUCCESS) && (StatusCode <= FTC_INSUFFICIENT_RESOURCES))
          strcpy(szErrorMsg, EN_Common_Errors[StatusCode]);
        else
          strcpy(szErrorMsg, EN_New_Errors[(StatusCode - FTC_FAILED_TO_COMPLETE_COMMAND)]);
      }
      else
      {
        strcpy(szErrorMsg, EN_New_Errors[FTC_INVALID_LANGUAGE_CODE - FTC_FAILED_TO_COMPLETE_COMMAND]);

        Status = FTC_INVALID_LANGUAGE_CODE;
      }
    }
    else
    {
      sprintf(szErrorMsg, "%s%d", EN_New_Errors[FTC_INVALID_STATUS_CODE - FTC_FAILED_TO_COMPLETE_COMMAND], StatusCode);

      Status = FTC_INVALID_STATUS_CODE;
    }

    if (dwBufferSize > strlen(szErrorMsg))
      strcpy(lpErrorMessageBuffer, szErrorMsg);
    else
      Status = FTC_ERROR_MESSAGE_BUFFER_TOO_SMALL;
  }
  else
  {
    if (lpLanguage == NULL)
      Status = FTC_NULL_LANGUAGE_CODE_BUFFER_POINTER;
    else
      Status = FTC_NULL_ERROR_MESSAGE_BUFFER_POINTER;
  }

  LeaveCriticalSection(&threadAccess);

  return Status;
}

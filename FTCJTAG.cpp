/*++

Copyright (c) 2005  Future Technology Devices International Ltd.

Module Name:

    ftcjtag.cpp

Abstract:

    API DLL for FT2232H and FT4232H Hi-Speed Dual Device and FT2232D Dual Device setup to simulate the
    Joint Test Action Group(JTAG) synchronous serial protocol.
    Defines the entry point for the DLL application.

Environment:

    kernel & user mode

Revision History:

    07/02/05    kra     Created.
    24/08/05    kra     Added new function JTAG_GenerateClockPulses and new error code FTC_INVALID_NUMBER_CLOCK_PULSES
    16/09/05    kra     Added break statements after DLL_THREAD_ATTACH and DLL_THREAD_DETACH for multiple threaded applications
    07/07/08    kra     Added new functions for FT2232H and FT4232H hi-speed devices.
    19/08/08    kra     Added new function JTAG_CloseDevice.
	
--*/

#include "ftcjtag.h"
#include "FtcJtagInternal.h"
#include "FT2232hMpsseJtag.h"

#ifdef _WIN32
static FT2232hMpsseJtag *pFT2232hMpsseJtag = NULL;
BOOL WINAPI DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    switch (ul_reason_for_call)
  	{
		case DLL_PROCESS_ATTACH:
      if (pFT2232hMpsseJtag == NULL)
        pFT2232hMpsseJtag = new FT2232hMpsseJtag();
      break;
		case DLL_THREAD_ATTACH:
      break;
		case DLL_THREAD_DETACH:
      break;
		case DLL_PROCESS_DETACH:
      pFT2232hMpsseJtag->~FT2232hMpsseJtag();
		  break;
    }

    return TRUE;
}
#else
static FT2232hMpsseJtag ftcjtag_object;
static FT2232hMpsseJtag *pFT2232hMpsseJtag = &ftcjtag_object;
#endif

//---------------------------------------------------------------------------

extern "C" FTCJTAG_API
FTC_STATUS WINAPI JTAG_GetNumDevices(LPDWORD lpdwNumDevices)
{
  return pFT2232hMpsseJtag->JTAG_GetNumDevices(lpdwNumDevices);
}

extern "C" FTCJTAG_API
FTC_STATUS WINAPI JTAG_GetNumHiSpeedDevices(LPDWORD lpdwNumHiSpeedDevices)
{
  return pFT2232hMpsseJtag->JTAG_GetNumHiSpeedDevices(lpdwNumHiSpeedDevices);
}

extern "C" FTCJTAG_API
FTC_STATUS WINAPI JTAG_GetDeviceNameLocID(DWORD dwDeviceNameIndex, LPSTR lpDeviceNameBuffer, DWORD dwBufferSize, LPDWORD lpdwLocationID)
{
  return pFT2232hMpsseJtag->JTAG_GetDeviceNameLocationID(dwDeviceNameIndex, lpDeviceNameBuffer, dwBufferSize, lpdwLocationID);
}

extern "C" FTCJTAG_API
FTC_STATUS WINAPI JTAG_GetHiSpeedDeviceNameLocIDChannel(DWORD dwDeviceNameIndex, LPSTR lpDeviceNameBuffer, DWORD dwBufferSize, LPDWORD lpdwLocationID, LPSTR lpChannel, DWORD dwChannelBufferSize, LPDWORD lpdwHiSpeedDeviceType)
{
  return pFT2232hMpsseJtag->JTAG_GetHiSpeedDeviceNameLocationIDChannel(dwDeviceNameIndex, lpDeviceNameBuffer, dwBufferSize, lpdwLocationID, lpChannel, dwChannelBufferSize, lpdwHiSpeedDeviceType);
}

extern "C" FTCJTAG_API
FTC_STATUS WINAPI JTAG_Open(FTC_HANDLE *pftHandle)
{
  return pFT2232hMpsseJtag->JTAG_OpenDevice(pftHandle);
}

extern "C" FTCJTAG_API
FTC_STATUS WINAPI JTAG_OpenEx(LPSTR lpDeviceName, DWORD dwLocationID, FTC_HANDLE *pftHandle)
{
  return pFT2232hMpsseJtag->JTAG_OpenSpecifiedDevice(lpDeviceName, dwLocationID, pftHandle);
}

extern "C" FTCJTAG_API
FTC_STATUS WINAPI JTAG_OpenHiSpeedDevice(LPSTR lpDeviceName, DWORD dwLocationID, LPSTR lpChannel, FTC_HANDLE *pftHandle)
{
  return pFT2232hMpsseJtag->JTAG_OpenSpecifiedHiSpeedDevice(lpDeviceName, dwLocationID, lpChannel, pftHandle);
}

extern "C" FTCJTAG_API
FTC_STATUS WINAPI JTAG_GetHiSpeedDeviceType(FTC_HANDLE ftHandle, LPDWORD lpdwHiSpeedDeviceType)
{
  return pFT2232hMpsseJtag->JTAG_GetHiSpeedDeviceType(ftHandle, lpdwHiSpeedDeviceType);
}

extern "C" FTCJTAG_API
FTC_STATUS WINAPI JTAG_Close(FTC_HANDLE ftHandle)
{
  return pFT2232hMpsseJtag->JTAG_CloseDevice(ftHandle);
}

extern "C" FTCJTAG_API
FTC_STATUS WINAPI JTAG_CloseDevice(FTC_HANDLE ftHandle, PFTC_CLOSE_FINAL_STATE_PINS pCloseFinalStatePinsData)
{
  return pFT2232hMpsseJtag->JTAG_CloseDevice(ftHandle, pCloseFinalStatePinsData);
}

extern "C" FTCJTAG_API
FTC_STATUS WINAPI JTAG_InitDevice(FTC_HANDLE ftHandle, DWORD dwClockDivisor)
{
  return pFT2232hMpsseJtag->JTAG_InitDevice(ftHandle, dwClockDivisor);
}

extern "C" FTCJTAG_API
FTC_STATUS WINAPI JTAG_TurnOnDivideByFiveClockingHiSpeedDevice(FTC_HANDLE ftHandle)
{
  return pFT2232hMpsseJtag->JTAG_TurnOnDivideByFiveClockingHiSpeedDevice(ftHandle);
}

extern "C" FTCJTAG_API
FTC_STATUS WINAPI JTAG_TurnOffDivideByFiveClockingHiSpeedDevice(FTC_HANDLE ftHandle)
{
  return pFT2232hMpsseJtag->JTAG_TurnOffDivideByFiveClockingHiSpeedDevice(ftHandle);
}

extern "C" FTCJTAG_API
FTC_STATUS WINAPI JTAG_TurnOnAdaptiveClockingHiSpeedDevice(FTC_HANDLE ftHandle)
{
  return pFT2232hMpsseJtag->JTAG_TurnOnAdaptiveClockingHiSpeedDevice(ftHandle);
}

extern "C" FTCJTAG_API
FTC_STATUS WINAPI JTAG_TurnOffAdaptiveClockingHiSpeedDevice(FTC_HANDLE ftHandle)
{
  return pFT2232hMpsseJtag->JTAG_TurnOffAdaptiveClockingHiSpeedDevice(ftHandle);
}

extern "C" FTCJTAG_API
FTC_STATUS WINAPI JTAG_SetDeviceLatencyTimer(FTC_HANDLE ftHandle, BYTE timerValue)
{
  return pFT2232hMpsseJtag->JTAG_SetDeviceLatencyTimer(ftHandle, timerValue);
}

extern "C" FTCJTAG_API
FTC_STATUS WINAPI JTAG_GetDeviceLatencyTimer(FTC_HANDLE ftHandle, LPBYTE lpTimerValue)
{
  return pFT2232hMpsseJtag->JTAG_GetDeviceLatencyTimer(ftHandle, lpTimerValue);
}

extern "C" FTCJTAG_API
FTC_STATUS WINAPI JTAG_GetClock(DWORD dwClockDivisor, LPDWORD lpdwClockFrequencyHz)
{
  return pFT2232hMpsseJtag->JTAG_GetClock(dwClockDivisor, lpdwClockFrequencyHz);
}

extern "C" FTCJTAG_API
FTC_STATUS WINAPI JTAG_GetHiSpeedDeviceClock(DWORD dwClockDivisor, LPDWORD lpdwClockFrequencyHz)
{
  return pFT2232hMpsseJtag->JTAG_GetHiSpeedDeviceClock(dwClockDivisor, lpdwClockFrequencyHz);
}

extern "C" FTCJTAG_API
FTC_STATUS WINAPI JTAG_SetClock(FTC_HANDLE ftHandle, DWORD dwClockDivisor, LPDWORD lpdwClockFrequencyHz)
{
  return pFT2232hMpsseJtag->JTAG_SetClock(ftHandle, dwClockDivisor, lpdwClockFrequencyHz);
}

extern "C" FTCJTAG_API
FTC_STATUS WINAPI JTAG_SetLoopback(FTC_HANDLE ftHandle, BOOL bLoopbackState)
{
  return pFT2232hMpsseJtag->JTAG_SetDeviceLoopbackState(ftHandle, bLoopbackState);
}

extern "C" FTCJTAG_API
FTC_STATUS WINAPI JTAG_SetGPIOs(FTC_HANDLE ftHandle, BOOL bControlLowInputOutputPins,
                                PFTC_INPUT_OUTPUT_PINS pLowInputOutputPinsData,
                                BOOL bControlHighInputOutputPins,
                                PFTC_INPUT_OUTPUT_PINS pHighInputOutputPinsData)
{
  return pFT2232hMpsseJtag->JTAG_SetGeneralPurposeInputOutputPins(ftHandle, bControlLowInputOutputPins,
                                                                  pLowInputOutputPinsData, bControlHighInputOutputPins,
                                                                  pHighInputOutputPinsData);
}

extern "C" FTCJTAG_API
FTC_STATUS WINAPI JTAG_SetHiSpeedDeviceGPIOs(FTC_HANDLE ftHandle, BOOL bControlLowInputOutputPins,
                                             PFTC_INPUT_OUTPUT_PINS pLowInputOutputPinsData,
                                             BOOL bControlHighInputOutputPins,
                                             PFTH_INPUT_OUTPUT_PINS pHighInputOutputPinsData)
{
  return pFT2232hMpsseJtag->JTAG_SetHiSpeedDeviceGeneralPurposeInputOutputPins(ftHandle, bControlLowInputOutputPins,
                                                                               pLowInputOutputPinsData, bControlHighInputOutputPins,
                                                                               pHighInputOutputPinsData);
}

extern "C" FTCJTAG_API
FTC_STATUS WINAPI JTAG_GetGPIOs(FTC_HANDLE ftHandle, BOOL bControlLowInputOutputPins,
                                PFTC_LOW_HIGH_PINS pLowPinsInputData,
                                BOOL bControlHighInputOutputPins,
                                PFTC_LOW_HIGH_PINS pHighPinsInputData)
{
  return pFT2232hMpsseJtag->JTAG_GetGeneralPurposeInputOutputPins(ftHandle, bControlLowInputOutputPins,
                                                                  pLowPinsInputData, bControlHighInputOutputPins,
                                                                  pHighPinsInputData);
}

extern "C" FTCJTAG_API
FTC_STATUS WINAPI JTAG_GetHiSpeedDeviceGPIOs(FTC_HANDLE ftHandle, BOOL bControlLowInputOutputPins,
                                             PFTC_LOW_HIGH_PINS pLowPinsInputData,
                                             BOOL bControlHighInputOutputPins,
                                             PFTH_LOW_HIGH_PINS pHighPinsInputData)
{
  return pFT2232hMpsseJtag->JTAG_GetHiSpeedDeviceGeneralPurposeInputOutputPins(ftHandle, bControlLowInputOutputPins,
                                                                               pLowPinsInputData, bControlHighInputOutputPins,
                                                                               pHighPinsInputData);
}

extern "C" FTCJTAG_API
FTC_STATUS WINAPI JTAG_Write(FTC_HANDLE ftHandle, BOOL bInstructionTestData, DWORD dwNumBitsToWrite,
                             PWriteDataByteBuffer pWriteDataBuffer, DWORD dwNumBytesToWrite,
                             DWORD dwTapControllerState)
{
  return pFT2232hMpsseJtag->JTAG_WriteDataToExternalDevice(ftHandle, bInstructionTestData, dwNumBitsToWrite,
                                                           pWriteDataBuffer, dwNumBytesToWrite, dwTapControllerState);
}

extern "C" FTCJTAG_API
FTC_STATUS WINAPI JTAG_Read(FTC_HANDLE ftHandle, BOOL bInstructionTestData, DWORD dwNumBitsToRead,
                            PReadDataByteBuffer pReadDataBuffer, LPDWORD lpdwNumBytesReturned,
                            DWORD dwTapControllerState)
{
  return pFT2232hMpsseJtag->JTAG_ReadDataFromExternalDevice(ftHandle, bInstructionTestData, dwNumBitsToRead,
                                                            pReadDataBuffer, lpdwNumBytesReturned, dwTapControllerState);
}

extern "C" FTCJTAG_API
FTC_STATUS WINAPI JTAG_WriteRead(FTC_HANDLE ftHandle, BOOL bInstructionData, DWORD dwNumBitsToWriteRead,
                                 PWriteDataByteBuffer pWriteDataBuffer, DWORD dwNumBytesToWrite,
                                 PReadDataByteBuffer pReadDataBuffer, LPDWORD lpdwNumBytesReturned,
                                 DWORD dwTapControllerState)
{
  return pFT2232hMpsseJtag->JTAG_WriteReadDataToFromExternalDevice(ftHandle, bInstructionData, dwNumBitsToWriteRead,
                                                                   pWriteDataBuffer,  dwNumBytesToWrite,
                                                                   pReadDataBuffer, lpdwNumBytesReturned, dwTapControllerState);
}

extern "C" FTCJTAG_API
FTC_STATUS WINAPI JTAG_GenerateClockPulses(FTC_HANDLE ftHandle, DWORD dwNumClockPulses)
{
  return pFT2232hMpsseJtag->JTAG_GenerateTCKClockPulses(ftHandle, dwNumClockPulses);
}

extern "C" FTCJTAG_API
FTC_STATUS WINAPI JTAG_GenerateClockPulsesHiSpeedDevice(FTC_HANDLE ftHandle, BOOL bPulseClockTimesEightFactor, DWORD dwNumClockPulses, BOOL bControlLowInputOutputPin, BOOL bStopClockPulsesState)
{
  return pFT2232hMpsseJtag->JTAG_GenerateClockPulsesHiSpeedDevice(ftHandle, bPulseClockTimesEightFactor, dwNumClockPulses, bControlLowInputOutputPin, bStopClockPulsesState);
}

extern "C" FTCJTAG_API
FTC_STATUS WINAPI JTAG_ClearCmdSequence(void)
{
  return pFT2232hMpsseJtag->JTAG_ClearCommandSequence();
}

extern "C" FTCJTAG_API
FTC_STATUS WINAPI JTAG_AddWriteCmd(BOOL bInstructionTestData, DWORD dwNumBitsToWrite,
                                   PWriteDataByteBuffer pWriteDataBuffer, DWORD dwNumBytesToWrite,
                                   DWORD dwTapControllerState)
{
  return pFT2232hMpsseJtag->JTAG_AddWriteCommand(bInstructionTestData, dwNumBitsToWrite, pWriteDataBuffer,
                                                 dwNumBytesToWrite, dwTapControllerState);
}

extern "C" FTCJTAG_API
FTC_STATUS WINAPI JTAG_AddReadCmd(BOOL bInstructionTestData, DWORD dwNumBitsToRead, DWORD dwTapControllerState)
{
  return pFT2232hMpsseJtag->JTAG_AddReadCommand(bInstructionTestData, dwNumBitsToRead, dwTapControllerState);
}

extern "C" FTCJTAG_API
FTC_STATUS WINAPI JTAG_AddWriteReadCmd(BOOL bInstructionTestData, DWORD dwNumBitsToWriteRead,
                                       PWriteDataByteBuffer pWriteDataBuffer, DWORD dwNumBytesToWrite,
                                       DWORD dwTapControllerState)
{
  return pFT2232hMpsseJtag->JTAG_AddWriteReadCommand(bInstructionTestData, dwNumBitsToWriteRead, pWriteDataBuffer,
                                                     dwNumBytesToWrite, dwTapControllerState);
}

extern "C" FTCJTAG_API
FTC_STATUS WINAPI JTAG_ClearDeviceCmdSequence(FTC_HANDLE ftHandle)
{
  return pFT2232hMpsseJtag->JTAG_ClearDeviceCommandSequence(ftHandle);
}

extern "C" FTCJTAG_API
FTC_STATUS WINAPI JTAG_AddDeviceWriteCmd(FTC_HANDLE ftHandle, BOOL bInstructionTestData, DWORD dwNumBitsToWrite,
                                         PWriteDataByteBuffer pWriteDataBuffer, DWORD dwNumBytesToWrite,
                                         DWORD dwTapControllerState)
{
  return pFT2232hMpsseJtag->JTAG_AddDeviceWriteCommand(ftHandle, bInstructionTestData, dwNumBitsToWrite,
                                                       pWriteDataBuffer, dwNumBytesToWrite, dwTapControllerState);
}

extern "C" FTCJTAG_API
FTC_STATUS WINAPI JTAG_AddDeviceReadCmd(FTC_HANDLE ftHandle, BOOL bInstructionTestData, DWORD dwNumBitsToRead, DWORD dwTapControllerState)
{
  return pFT2232hMpsseJtag->JTAG_AddDeviceReadCommand(ftHandle, bInstructionTestData, dwNumBitsToRead, dwTapControllerState);
}

extern "C" FTCJTAG_API
FTC_STATUS WINAPI JTAG_AddDeviceWriteReadCmd(FTC_HANDLE ftHandle, BOOL bInstructionTestData, DWORD dwNumBitsToWriteRead,
                                             PWriteDataByteBuffer pWriteDataBuffer, DWORD dwNumBytesToWrite,
                                             DWORD dwTapControllerState)
{
  return pFT2232hMpsseJtag->JTAG_AddDeviceWriteReadCommand(ftHandle, bInstructionTestData, dwNumBitsToWriteRead, pWriteDataBuffer,
                                                           dwNumBytesToWrite, dwTapControllerState);
}

extern "C" FTCJTAG_API
FTC_STATUS WINAPI JTAG_ExecuteCmdSequence(FTC_HANDLE ftHandle, PReadCmdSequenceDataByteBuffer pReadCmdSequenceDataBuffer,
                                          LPDWORD lpdwNumBytesReturned)
{
  return pFT2232hMpsseJtag->JTAG_ExecuteCommandSequence(ftHandle, pReadCmdSequenceDataBuffer, lpdwNumBytesReturned);
}

extern "C" FTCJTAG_API
FTC_STATUS WINAPI JTAG_GetDllVersion(LPSTR lpDllVersionBuffer, DWORD dwBufferSize)
{
  return pFT2232hMpsseJtag->JTAG_GetDllVersion(lpDllVersionBuffer, dwBufferSize);
}

extern "C" FTCJTAG_API
FTC_STATUS WINAPI JTAG_GetErrorCodeString(LPSTR lpLanguage, FTC_STATUS StatusCode, LPSTR lpErrorMessageBuffer, DWORD dwBufferSize)
{
  return pFT2232hMpsseJtag->JTAG_GetErrorCodeString(lpLanguage, StatusCode, lpErrorMessageBuffer, dwBufferSize);
}
//---------------------------------------------------------------------------


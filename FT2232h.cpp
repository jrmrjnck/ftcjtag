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
    15/08/08    kra     Modified FTC_IsDeviceNameLocationIDValid and FTC_InsertDeviceHandle methods to add the type
                        of hi-speed device to OpenedHiSpeedDevices ie array of FTC_HI_SPEED_DEVICE_DATA structures.
                        Add new FTC_IsDeviceHiSpeedType method.
	
--*/

#define WIO_DEFINED

#include "stdafx.h"

#include "FT2232h.h"

BOOL FT2232h::FTC_DeviceInUse(LPSTR lpDeviceName, DWORD dwLocationID)
{
  BOOL bDeviceInUse = FALSE;
  DWORD dwProcessId = 0;
  bool bLocationIDFound = false;
  INT iDeviceCntr = 0;

  if (uiNumOpenedHiSpeedDevices > 0)
  {
    dwProcessId = GetCurrentProcessId();

    for (iDeviceCntr = 0; ((iDeviceCntr < MAX_NUM_DEVICES) && !bLocationIDFound); iDeviceCntr++)
    {
      // Only check device name and location id not the current application
      if (OpenedHiSpeedDevices[iDeviceCntr].dwProcessId != dwProcessId)
      {
        if (strcmp(OpenedHiSpeedDevices[iDeviceCntr].szDeviceName, lpDeviceName) == 0)
        {
          if (OpenedHiSpeedDevices[iDeviceCntr].dwLocationID == dwLocationID)
            bLocationIDFound = true;
        }
      }
    }

    if (bLocationIDFound)
      bDeviceInUse = TRUE;
  }

  return bDeviceInUse;
}

BOOL FT2232h::FTC_DeviceOpened(LPSTR lpDeviceName, DWORD dwLocationID, FTC_HANDLE *pftHandle)
{
  BOOL bDeviceOpen = FALSE;
  DWORD dwProcessId = 0;
  bool bLocationIDFound = false;
  INT iDeviceCntr = 0;

  if (uiNumOpenedHiSpeedDevices > 0)
  {
    dwProcessId = GetCurrentProcessId();

    for (iDeviceCntr = 0; ((iDeviceCntr < MAX_NUM_DEVICES) && !bLocationIDFound); iDeviceCntr++)
    {
      if (OpenedHiSpeedDevices[iDeviceCntr].dwProcessId == dwProcessId)
      {
        if (strcmp(OpenedHiSpeedDevices[iDeviceCntr].szDeviceName, lpDeviceName) == 0)
        {
          if (OpenedHiSpeedDevices[iDeviceCntr].dwLocationID == dwLocationID)
          {
            // Device has already been opened by this application, so just return the handle to the device
            *pftHandle = OpenedHiSpeedDevices[iDeviceCntr].hDevice;
            bLocationIDFound = true;
          }
        }
      }
    }

    if (bLocationIDFound)
      bDeviceOpen = TRUE;
  }

  return bDeviceOpen;
}

FTC_STATUS FT2232h::FTC_IsDeviceNameLocationIDValid(LPSTR lpDeviceName, DWORD dwLocationID, LPDWORD lpdwDeviceType)
{
  FTC_STATUS Status = FTC_SUCCESS;
  DWORD dwNumHiSpeedDevices = 0;
  HiSpeedDeviceIndexes HiSpeedIndexes;
  DWORD dwFlags = 0;
  DWORD dwProductVendorID = 0;
  DWORD dwLocID = 0;
  SerialNumber szSerialNumber;
  char szDeviceNameBuffer[DEVICE_STRING_BUFF_SIZE + 1];
  FT_HANDLE ftHandle;
  bool bDeviceNameFound = false;
  bool bLocationIDFound = false;
  DWORD dwDeviceIndex = 0;

  // Get the number of hi-speed devices connected to the system
  if ((Status = FTC_GetNumHiSpeedDevices(&dwNumHiSpeedDevices, &HiSpeedIndexes)) == FTC_SUCCESS)
  {
    if (dwNumHiSpeedDevices > 0)
    {
      do
      {
        bDeviceNameFound = false;

        // If the FT_GetDeviceInfoDetail function returns a location id of 0 then this device is already open
        Status = FT_GetDeviceInfoDetail(HiSpeedIndexes[dwDeviceIndex], &dwFlags, lpdwDeviceType, &dwProductVendorID,
                                        &dwLocID, szSerialNumber, szDeviceNameBuffer, &ftHandle);

        if (Status == FTC_SUCCESS)
        {
          if (strcmp(szDeviceNameBuffer, lpDeviceName) == 0)
          {
            bDeviceNameFound = true;

            if (dwLocID == dwLocationID)
              bLocationIDFound = true;
          }
        }
        dwDeviceIndex++;
      }
      while ((dwDeviceIndex < dwNumHiSpeedDevices) && (Status == FTC_SUCCESS) && (bDeviceNameFound == false));

      if (bDeviceNameFound == true)
      {
        if (dwLocID == 0) {
          Status = FTC_DEVICE_IN_USE;
        }
        else 
        {
          if (bLocationIDFound == false)
            Status = FTC_INVALID_LOCATION_ID;
        }
      }
      else
        Status = FTC_INVALID_DEVICE_NAME;
    }
    else
      Status = FTC_DEVICE_NOT_FOUND;
  }

  return Status;
}

FTC_STATUS FT2232h::FTC_IsDeviceHiSpeedType(FT_DEVICE_LIST_INFO_NODE devInfo, LPBOOL lpbHiSpeedDeviceType)
{
  FTC_STATUS Status = FTC_SUCCESS;
  LPSTR pszStringSearch;

  *lpbHiSpeedDeviceType = FALSE;

  // The MPSSE hi-speed controller on both the FT2232H and FT4232H hi-speed dual devices, is available
  // through channel A and channel B
  if ((devInfo.Type == FT_DEVICE_2232H) || (devInfo.Type == FT_DEVICE_4232H))
  {
    // Search for the first occurrence of the channel string ie ' A' or ' B'. The Description field contains the device name
    if (((pszStringSearch = strstr(strupr(devInfo.Description), DEVICE_NAME_CHANNEL_A)) != NULL) ||
        ((pszStringSearch = strstr(strupr(devInfo.Description), DEVICE_NAME_CHANNEL_B)) != NULL))
    {
      // Ensure the last two characters of the device name is ' A' ie channel A or ' B' ie channel B
      if (strlen(pszStringSearch) == 2)
        *lpbHiSpeedDeviceType = TRUE; 
    }
  }

  return Status;
}

void FT2232h::FTC_InsertDeviceHandle(LPSTR lpDeviceName, DWORD dwLocationID, LPSTR lpChannel, DWORD dwDeviceType, FTC_HANDLE ftHandle)
{
  DWORD dwProcessId = 0;
  INT iDeviceCntr = 0;
  bool bDeviceftHandleInserted = false;

  if (uiNumOpenedHiSpeedDevices < MAX_NUM_DEVICES)
  {
    dwProcessId = GetCurrentProcessId();

    for (iDeviceCntr = 0; ((iDeviceCntr < MAX_NUM_DEVICES) && !bDeviceftHandleInserted); iDeviceCntr++)
    {
      if (OpenedHiSpeedDevices[iDeviceCntr].dwProcessId == 0)
      {
        OpenedHiSpeedDevices[iDeviceCntr].dwProcessId = dwProcessId;
        strcpy_s(OpenedHiSpeedDevices[iDeviceCntr].szDeviceName, lpDeviceName);
        OpenedHiSpeedDevices[iDeviceCntr].dwLocationID = dwLocationID;
        strcpy_s(OpenedHiSpeedDevices[iDeviceCntr].szChannel, lpChannel);
        OpenedHiSpeedDevices[iDeviceCntr].bDivideByFiveClockingState = TRUE;
        OpenedHiSpeedDevices[iDeviceCntr].dwDeviceType = dwDeviceType;
        OpenedHiSpeedDevices[iDeviceCntr].hDevice = ftHandle;

        uiNumOpenedHiSpeedDevices = uiNumOpenedHiSpeedDevices + 1;

        bDeviceftHandleInserted = true;
      }
    }
  }
}

void FT2232h::FTC_SetDeviceDivideByFiveState(FTC_HANDLE ftHandle, BOOL bDivideByFiveClockingState)
{
  DWORD dwProcessId = 0;
  INT iDeviceCntr = 0;
  BOOL bDevicempHandleFound = FALSE;

  if (uiNumOpenedHiSpeedDevices > 0)
  {
    dwProcessId = GetCurrentProcessId();

    for (iDeviceCntr = 0; ((iDeviceCntr < MAX_NUM_DEVICES) && !bDevicempHandleFound); iDeviceCntr++)
    {
      if (OpenedHiSpeedDevices[iDeviceCntr].dwProcessId == dwProcessId)
      {
        if (OpenedHiSpeedDevices[iDeviceCntr].hDevice == ftHandle)
        {
          bDevicempHandleFound = TRUE;

          OpenedHiSpeedDevices[iDeviceCntr].bDivideByFiveClockingState = bDivideByFiveClockingState;
        }
      }
    }
  }
}

BOOL FT2232h::FTC_GetDeviceDivideByFiveState(FTC_HANDLE ftHandle)
{
  BOOL bDivideByFiveClockingState = FALSE;
  DWORD dwProcessId = 0;
  INT iDeviceCntr = 0;
  BOOL bDevicempHandleFound = FALSE;

  if (uiNumOpenedHiSpeedDevices > 0)
  {
    dwProcessId = GetCurrentProcessId();

    for (iDeviceCntr = 0; ((iDeviceCntr < MAX_NUM_DEVICES) && !bDevicempHandleFound); iDeviceCntr++)
    {
      if (OpenedHiSpeedDevices[iDeviceCntr].dwProcessId == dwProcessId)
      {
        if (OpenedHiSpeedDevices[iDeviceCntr].hDevice == ftHandle)
        {
          bDevicempHandleFound = TRUE;

          bDivideByFiveClockingState = OpenedHiSpeedDevices[iDeviceCntr].bDivideByFiveClockingState;
        }
      }
    }
  }

  return bDivideByFiveClockingState;
}

FT2232h::FT2232h(void)
{
  INT iDeviceCntr = 0;

  uiNumOpenedHiSpeedDevices = 0;

  for (iDeviceCntr = 0; (iDeviceCntr < MAX_NUM_DEVICES); iDeviceCntr++)
    OpenedHiSpeedDevices[iDeviceCntr].dwProcessId = 0;

  dwNumBytesToSend = 0;
}

FT2232h::~FT2232h(void)
{

}

FTC_STATUS FT2232h::FTC_GetNumHiSpeedDevices(LPDWORD lpdwNumHiSpeedDevices, HiSpeedDeviceIndexes *HiSpeedIndexes)
{
  FTC_STATUS Status = FTC_SUCCESS;

  DWORD dwNumOfDevices = 0;
  FT_DEVICE_LIST_INFO_NODE *pDevInfoList;
  FT_DEVICE_LIST_INFO_NODE devInfo;
  DWORD dwDeviceIndex = 0;
  BOOL bHiSpeedTypeDevice = FALSE;

  *lpdwNumHiSpeedDevices = 0;

  // Get the number of high speed devices(FT2232H and FT4232H) connected to the system
  if ((Status = FT_CreateDeviceInfoList(&dwNumOfDevices)) == FTC_SUCCESS)
  {
    if (dwNumOfDevices > 0)
    {
      // allocate storage for the device list based on dwNumOfDevices
      if ((pDevInfoList = new FT_DEVICE_LIST_INFO_NODE[dwNumOfDevices]) != NULL )
      {
        if ((Status = FT_GetDeviceInfoList(pDevInfoList, &dwNumOfDevices)) == FTC_SUCCESS)
        {
          do
          {
            devInfo = pDevInfoList[dwDeviceIndex];

            if ((Status = FTC_IsDeviceHiSpeedType(devInfo, &bHiSpeedTypeDevice)) == FTC_SUCCESS)
            {
              if (bHiSpeedTypeDevice == TRUE)
              {
                // The number of devices returned is, not opened devices ie channel A and channel B plus devices opened
                // by the calling application. Devices previously opened by another application are not included in this
                // number.
                (*HiSpeedIndexes)[*lpdwNumHiSpeedDevices] = dwDeviceIndex;

                *lpdwNumHiSpeedDevices = *lpdwNumHiSpeedDevices + 1;
              }
            }

            dwDeviceIndex++;
          }
          while ((dwDeviceIndex < dwNumOfDevices) && (Status == FTC_SUCCESS));
        }

        delete pDevInfoList;
      }
      else
        Status = FTC_INSUFFICIENT_RESOURCES;
    }
  }

  return Status;
}

FTC_STATUS FT2232h::FTC_GetHiSpeedDeviceNameLocationIDChannel(DWORD dwDeviceIndex, LPSTR lpDeviceName, DWORD dwDeviceNameBufferSize, LPDWORD lpdwLocationID, LPSTR lpChannel, DWORD dwChannelBufferSize, LPDWORD lpdwDeviceType)
{
  FTC_STATUS Status = FTC_SUCCESS;
  DWORD dwNumHiSpeedDevices = 0;
  HiSpeedDeviceIndexes HiSpeedIndexes;
  DWORD dwFlags = 0;
  DWORD dwProductVendorID = 0;
  SerialNumber szSerialNumber;
  char szDeviceNameBuffer[DEVICE_STRING_BUFF_SIZE + 1];
  FT_HANDLE ftHandle;
  LPSTR pszStringSearch;

  *lpdwDeviceType = 0;

  if ((lpDeviceName != NULL) && (lpChannel != NULL))
  {
    if ((Status = FTC_GetNumHiSpeedDevices(&dwNumHiSpeedDevices, &HiSpeedIndexes)) == FTC_SUCCESS)
    {
      if (dwNumHiSpeedDevices > 0)
      {
        if (dwDeviceIndex < dwNumHiSpeedDevices)
        {
          Status = FT_GetDeviceInfoDetail(HiSpeedIndexes[dwDeviceIndex], &dwFlags, lpdwDeviceType, &dwProductVendorID,
                                         lpdwLocationID, szSerialNumber, szDeviceNameBuffer, &ftHandle);

          if (Status == FTC_SUCCESS)
          {
            if (strlen(szDeviceNameBuffer) <= dwDeviceNameBufferSize)
            {
              strcpy(lpDeviceName, szDeviceNameBuffer);

              // Check for hi-speed device channel A or channel B
              if (((pszStringSearch = strstr(strupr(szDeviceNameBuffer), DEVICE_NAME_CHANNEL_A)) != NULL) ||
                  ((pszStringSearch = strstr(strupr(szDeviceNameBuffer), DEVICE_NAME_CHANNEL_B)) != NULL))
              {
                if (dwChannelBufferSize >= CHANNEL_STRING_MIN_BUFF_SIZE)
                {
                  if ((pszStringSearch = strstr(strupr(szDeviceNameBuffer), DEVICE_NAME_CHANNEL_A)) != NULL)
                    strcpy(lpChannel, CHANNEL_A);
                  else
                    strcpy(lpChannel, CHANNEL_B);
                }
                else
                  Status = FTC_CHANNEL_BUFFER_TOO_SMALL;
              }
              else
                Status = FTC_DEVICE_NOT_FOUND;
            }
            else
              Status = FTC_DEVICE_NAME_BUFFER_TOO_SMALL;
          }
        }
        else
          Status = FTC_INVALID_DEVICE_NAME_INDEX;
      }
      else
        Status = FTC_DEVICE_NOT_FOUND;
    }
  }
  else
  {
    if (lpDeviceName == NULL)
      Status = FTC_NULL_DEVICE_NAME_BUFFER_POINTER;
    else
      Status = FTC_NULL_CHANNEL_BUFFER_POINTER;
  }

  return Status;
}

FTC_STATUS FT2232h::FTC_OpenSpecifiedHiSpeedDevice(LPSTR lpDeviceName, DWORD dwLocationID, LPSTR lpChannel, FTC_HANDLE *pftHandle)
{
  FTC_STATUS Status = FTC_SUCCESS;
  DWORD dwDeviceType = 0;
  FT_HANDLE ftHandle;

  if ((lpDeviceName != NULL) && (lpChannel != NULL))
  {
    if ((strcmp(strupr(lpChannel), CHANNEL_A) == 0) || (strcmp(strupr(lpChannel), CHANNEL_B) == 0))
    {
      if ((Status = FTC_IsDeviceNameLocationIDValid(lpDeviceName, dwLocationID, &dwDeviceType)) == FTC_SUCCESS)
      {
        if (!FTC_DeviceInUse(lpDeviceName, dwLocationID))
        {
          if (!FTC_DeviceOpened(lpDeviceName, dwLocationID, pftHandle))
          {
            if ((Status = FT_OpenEx((PVOID)dwLocationID, FT_OPEN_BY_LOCATION, &ftHandle)) == FTC_SUCCESS)
            {
              *pftHandle = (DWORD)ftHandle;

              FTC_InsertDeviceHandle(lpDeviceName, dwLocationID, lpChannel, dwDeviceType, *pftHandle);
            }
          }
        }
        else
          Status = FTC_DEVICE_IN_USE;
      }
    }
    else
      Status = FTC_INVALID_CHANNEL;
  }
  else
  {
    if (lpDeviceName == NULL)
      Status = FTC_NULL_DEVICE_NAME_BUFFER_POINTER;
    else
      Status = FTC_NULL_CHANNEL_BUFFER_POINTER;
  }

  return Status;
}

FTC_STATUS FT2232h::FTC_GetHiSpeedDeviceType(FTC_HANDLE ftHandle, LPBOOL lpbHiSpeedFT2232HTDeviceType)
{
  FTC_STATUS Status = FTC_SUCCESS;
  BOOL bHiSpeedTypeDevice = FALSE;

  if ((Status = FTC_IsHiSpeedDeviceHandleValid( ftHandle)) == FTC_SUCCESS)
  {
    if ((Status = FTC_IsDeviceHiSpeedType(ftHandle, &bHiSpeedTypeDevice)) == FTC_SUCCESS)
    {
      // Is the device a hi-speed device ie a FT2232H or FT4232H hi-speed device
      if (bHiSpeedTypeDevice == TRUE)
        Status = FTC_IsDeviceHiSpeedFT2232HType(ftHandle, lpbHiSpeedFT2232HTDeviceType);
      else
        Status = FTC_INVALID_HANDLE;
    }
  }
 
  return Status;
}

FTC_STATUS FT2232h::FTC_CloseDevice(FTC_HANDLE ftHandle)
{
  FTC_STATUS Status = FTC_SUCCESS;

  if ((Status = FTC_IsHiSpeedDeviceHandleValid(ftHandle)) == FTC_SUCCESS)
  {
    Status = FT_Close((FT_HANDLE)ftHandle);

    FTC_RemoveHiSpeedDeviceHandle(ftHandle);
  }

  if (Status == FTC_INVALID_HANDLE) {
    Status = FT2232c::FTC_CloseDevice(ftHandle);
  }

  return Status;
}

FTC_STATUS FT2232h::FTC_IsDeviceHandleValid(FTC_HANDLE ftHandle)
{
  FTC_STATUS Status = FTC_SUCCESS;

  if ((Status = FTC_IsHiSpeedDeviceHandleValid(ftHandle)) == FTC_INVALID_HANDLE)
  {
    Status = FT2232c::FTC_IsDeviceHandleValid(ftHandle);
  }

  return Status;
}

FTC_STATUS FT2232h::FTC_IsHiSpeedDeviceHandleValid(FTC_HANDLE ftHandle)
{
  FTC_STATUS Status = FTC_SUCCESS;
  DWORD dwProcessId = 0;
  INT iDeviceCntr = 0;
  bool bDevicempHandleFound = false;

  if ((uiNumOpenedHiSpeedDevices > 0) && (ftHandle > 0))
  {
    dwProcessId = GetCurrentProcessId();

    for (iDeviceCntr = 0; ((iDeviceCntr < MAX_NUM_DEVICES) && !bDevicempHandleFound); iDeviceCntr++)
    {
      if (OpenedHiSpeedDevices[iDeviceCntr].dwProcessId == dwProcessId)
      {
        if (OpenedHiSpeedDevices[iDeviceCntr].hDevice == ftHandle)
          bDevicempHandleFound = true;
      }
    }

    if (!bDevicempHandleFound)
      Status = FTC_INVALID_HANDLE;
  }
  else
    Status = FTC_INVALID_HANDLE;

  return Status;
}

void FT2232h::FTC_RemoveHiSpeedDeviceHandle(FTC_HANDLE ftHandle)
{
  DWORD dwProcessId = 0;
  INT iDeviceCntr = 0;
  bool bDevicempHandleFound = false;

  if (uiNumOpenedHiSpeedDevices > 0)
  {
    dwProcessId = GetCurrentProcessId();

    for (iDeviceCntr = 0; ((iDeviceCntr < MAX_NUM_DEVICES) && !bDevicempHandleFound); iDeviceCntr++)
    {
      if (OpenedHiSpeedDevices[iDeviceCntr].dwProcessId == dwProcessId)
      {
        if (OpenedHiSpeedDevices[iDeviceCntr].hDevice == ftHandle)
        {
          OpenedHiSpeedDevices[iDeviceCntr].dwProcessId = 0;
          strcpy(OpenedHiSpeedDevices[iDeviceCntr].szDeviceName, "");
          OpenedHiSpeedDevices[iDeviceCntr].dwLocationID = 0;
          OpenedHiSpeedDevices[iDeviceCntr].hDevice = 0;

          uiNumOpenedHiSpeedDevices = uiNumOpenedHiSpeedDevices - 1;

          bDevicempHandleFound = true;
        }
      }
    }
  }
}

FTC_STATUS FT2232h::FTC_InitHiSpeedDevice(FTC_HANDLE ftHandle)
{
  FTC_STATUS Status = FTC_SUCCESS;

  if ((Status = FTC_TurnOffDivideByFiveClockingHiSpeedDevice(ftHandle)) == FTC_SUCCESS)
  {
    if ((Status = FTC_TurnOffAdaptiveClockingHiSpeedDevice(ftHandle)) == FTC_SUCCESS)
      Status = FTC_TurnOffThreePhaseDataClockingHiSpeedDevice(ftHandle);
  }

  return Status;
}

FTC_STATUS FT2232h::FTC_TurnOnDivideByFiveClockingHiSpeedDevice(FTC_HANDLE ftHandle)
{
  FTC_STATUS Status = FTC_SUCCESS;

  if ((Status = FTC_IsHiSpeedDeviceHandleValid( ftHandle)) == FTC_SUCCESS)
  {
    FTC_SetDeviceDivideByFiveState(ftHandle, TRUE);

    FTC_AddByteToOutputBuffer(TURN_ON_DIVIDE_BY_FIVE_CLOCKING_CMD, TRUE);
    Status = FTC_SendBytesToDevice(ftHandle);
  }

  return Status;
}

FTC_STATUS FT2232h::FTC_TurnOffDivideByFiveClockingHiSpeedDevice(FTC_HANDLE ftHandle)
{
  FTC_STATUS Status = FTC_SUCCESS;

  if ((Status = FTC_IsHiSpeedDeviceHandleValid( ftHandle)) == FTC_SUCCESS)
  {
    FTC_SetDeviceDivideByFiveState(ftHandle, FALSE);

    FTC_AddByteToOutputBuffer(TURN_OFF_DIVIDE_BY_FIVE_CLOCKING_CMD, TRUE);
    Status = FTC_SendBytesToDevice(ftHandle);
  }

  return Status;
}

FTC_STATUS FT2232h::FTC_TurnOnAdaptiveClockingHiSpeedDevice(FTC_HANDLE ftHandle)
{
  FTC_AddByteToOutputBuffer(TURN_ON_ADAPTIVE_CLOCKING_CMD, TRUE);

  return FTC_SendBytesToDevice(ftHandle);
}

FTC_STATUS FT2232h::FTC_TurnOffAdaptiveClockingHiSpeedDevice(FTC_HANDLE ftHandle)
{
  FTC_AddByteToOutputBuffer(TURN_OFF_ADAPTIVE_CLOCKING_CMD, TRUE);

  return FTC_SendBytesToDevice(ftHandle);
}

FTC_STATUS FT2232h::FTC_TurnOnThreePhaseDataClockingHiSpeedDevice(FTC_HANDLE ftHandle)
{
  FTC_AddByteToOutputBuffer(TURN_ON_THREE_PHASE_DATA_CLOCKING_CMD, TRUE);

  return FTC_SendBytesToDevice(ftHandle);
}

FTC_STATUS FT2232h::FTC_TurnOffThreePhaseDataClockingHiSpeedDevice(FTC_HANDLE ftHandle)
{
  FTC_AddByteToOutputBuffer(TURN_OFF_THREE_PHASE_DATA_CLOCKING_CMD, TRUE);

  return FTC_SendBytesToDevice(ftHandle);
}

FTC_STATUS FT2232h::FTC_SetDeviceLatencyTimer(FTC_HANDLE ftHandle, BYTE LatencyTimermSec)
{
  FTC_STATUS Status = FTC_SUCCESS;

  if ((LatencyTimermSec >= MIN_LATENCY_TIMER_VALUE) && (LatencyTimermSec <= MAX_LATENCY_TIMER_VALUE))
    FT2232c::FTC_SetDeviceLatencyTimer(ftHandle, LatencyTimermSec);
  else
    Status = FTC_INVALID_TIMER_VALUE;

  return Status;
}

void FT2232h::FTC_GetHiSpeedDeviceClockFrequencyValues(FTC_HANDLE ftHandle, DWORD dwClockFrequencyValue, LPDWORD lpdwClockFrequencyHz)
{
  if (FTC_GetDeviceDivideByFiveState(ftHandle) == FALSE)
    // the state of the clock divide by five is turned off(FALSE)
    *lpdwClockFrequencyHz = (BASE_CLOCK_FREQUENCY_60_MHZ / ((1 + dwClockFrequencyValue) * 2));
  else
    // the state of the clock divide by five is turned on(TRUE)
    *lpdwClockFrequencyHz = (BASE_CLOCK_FREQUENCY_12_MHZ / ((1 + dwClockFrequencyValue) * 2));
}

void FT2232h::FTC_GetHiSpeedDeviceClockFrequencyValues(DWORD dwClockFrequencyValue, LPDWORD lpdwClockFrequencyHz)
{
  *lpdwClockFrequencyHz = (BASE_CLOCK_FREQUENCY_60_MHZ / ((1 + dwClockFrequencyValue) * 2));
}

FTC_STATUS FT2232h::FTC_IsDeviceHiSpeedType(FTC_HANDLE ftHandle, LPBOOL lpbHiSpeedDeviceType)
{
  FTC_STATUS Status = FTC_SUCCESS;
  DWORD dwDeviceType = 0;
  DWORD dwDeviceID = 0;
  SerialNumber szSerialNumber;
  char szDeviceNameBuffer[DEVICE_STRING_BUFF_SIZE + 1];
  PVOID pvDummy = NULL;

  *lpbHiSpeedDeviceType = FALSE;

  if ((Status = FT_GetDeviceInfo((FT_HANDLE)ftHandle, &dwDeviceType, &dwDeviceID, szSerialNumber, szDeviceNameBuffer, pvDummy)) == FTC_SUCCESS)
  {
    if ((dwDeviceType == FT_DEVICE_2232H) || (dwDeviceType == FT_DEVICE_4232H))
    {
      *lpbHiSpeedDeviceType = TRUE;
    }
  }

  return Status;
}

FTC_STATUS FT2232h::FTC_IsDeviceHiSpeedFT2232HType(FTC_HANDLE ftHandle, LPBOOL lpbHiSpeedFT2232HTDeviceype)
{
  FTC_STATUS Status = FTC_SUCCESS;
  DWORD dwDeviceType = 0;
  DWORD dwDeviceID = 0;
  SerialNumber szSerialNumber;
  char szDeviceNameBuffer[DEVICE_STRING_BUFF_SIZE + 1];
  PVOID pvDummy = NULL;

  *lpbHiSpeedFT2232HTDeviceype = FALSE;

  if ((Status = FT_GetDeviceInfo((FT_HANDLE)ftHandle, &dwDeviceType, &dwDeviceID, szSerialNumber, szDeviceNameBuffer, pvDummy)) == FTC_SUCCESS)
  {
    if (dwDeviceType == FT_DEVICE_2232H)
    {
      *lpbHiSpeedFT2232HTDeviceype = TRUE;
    }
  }

  return Status;
}

BOOL FT2232h::FTC_IsDeviceHiSpeedType(FTC_HANDLE ftHandle)
{
  BOOL bHiSpeedDeviceType = FALSE;
  DWORD dwDeviceType = 0;
  DWORD dwProcessId = 0;
  INT iDeviceCntr = 0;
  BOOL bDevicempHandleFound = FALSE;

  if (uiNumOpenedHiSpeedDevices > 0)
  {
    dwProcessId = GetCurrentProcessId();

    for (iDeviceCntr = 0; ((iDeviceCntr < MAX_NUM_DEVICES) && !bDevicempHandleFound); iDeviceCntr++)
    {
      if (OpenedHiSpeedDevices[iDeviceCntr].dwProcessId == dwProcessId)
      {
        if (OpenedHiSpeedDevices[iDeviceCntr].hDevice == ftHandle)
        {
          bDevicempHandleFound = TRUE;

          dwDeviceType = OpenedHiSpeedDevices[iDeviceCntr].dwDeviceType;

          if ((dwDeviceType == FT_DEVICE_2232H) || (dwDeviceType == FT_DEVICE_4232H))
          {
            bHiSpeedDeviceType = TRUE;
          }
        }
      }
    }
  }

  return bHiSpeedDeviceType;
}

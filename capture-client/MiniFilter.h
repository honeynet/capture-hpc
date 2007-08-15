/*
 *	PROJECT: Capture
 *	FILE: MiniFilter.h
 *	AUTHORS: Ramon Steenson (rsteenson@gmail.com) & Christian Seifert (christian.seifert@gmail.com)
 *
 *	Developed by Victoria University of Wellington and the New Zealand Honeynet Alliance
 *
 *	This file is part of Capture.
 *
 *	Capture is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Capture is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Capture; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

typedef struct _INSTANCE_FULL_INFORMATION {
  ULONG  NextEntryOffset;
  USHORT  InstanceNameLength;
  USHORT  InstanceNameBufferOffset;
  USHORT  AltitudeLength;
  USHORT  AltitudeBufferOffset;
  USHORT  VolumeNameLength;
  USHORT  VolumeNameBufferOffset;
  USHORT  FilterNameLength;
  USHORT  FilterNameBufferOffset;
} INSTANCE_FULL_INFORMATION, *PINSTANCE_FULL_INFORMATION;

typedef struct _INSTANCE_BASIC_INFORMATION {
    ULONG  NextEntryOffset;
    USHORT  InstanceNameLength;
    USHORT  InstanceNameBufferOffset;
} INSTANCE_BASIC_INFORMATION, *PINSTANCE_BASIC_INFORMATION;

typedef struct _FILTER_VOLUME_BASIC_INFORMATION {
  USHORT  FilterVolumeNameLength;
  WCHAR  FilterVolumeName[1];
} FILTER_VOLUME_BASIC_INFORMATION, *PFILTER_VOLUME_BASIC_INFORMATION;

typedef enum _FILTER_VOLUME_INFORMATION_CLASS {

    FilterVolumeBasicInformation,
    FilterVolumeStandardInformation     //Longhorn and later

} FILTER_VOLUME_INFORMATION_CLASS, *PFILTER_VOLUME_INFORMATION_CLASS;
extern "C" {
HRESULT
WINAPI
  FilterConnectCommunicationPort(
    IN LPCWSTR  lpPortName,
    IN DWORD  dwOptions,
    IN LPVOID  lpContext OPTIONAL,
    IN DWORD  dwSizeOfContext,
    IN LPSECURITY_ATTRIBUTES  lpSecurityAttributes OPTIONAL,
    OUT HANDLE  *hPort
    ); 

HRESULT
WINAPI
  FilterSendMessage(
    IN HANDLE  hPort,
    IN LPVOID  lpInBuffer OPTIONAL,
    IN DWORD  dwInBufferSize,
    IN OUT LPVOID  lpOutBuffer OPTIONAL,
    IN DWORD  dwOutBufferSize,
    OUT LPDWORD  lpBytesReturned
    ); 

HRESULT
WINAPI
  FilterLoad(
    IN LPCWSTR  lpFilterName
    ); 
HRESULT
WINAPI
  FilterUnload(
    IN LPCWSTR  lpFilterName
    ); 

HRESULT
WINAPI
  FilterGetDosName(
    IN LPCWSTR  lpVolumeName,
    IN OUT LPWSTR  lpDosName,
    IN DWORD  dwDosNameBufferSize
    ); 

HRESULT
WINAPI
  FilterVolumeFindFirst(
    IN FILTER_VOLUME_INFORMATION_CLASS  dwInformationClass,
    IN LPVOID  lpBuffer,
    IN DWORD  dwBufferSize,
    OUT LPDWORD  lpBytesReturned,
    OUT PHANDLE  lpFilterFind
    ); 

HRESULT
WINAPI
  FilterVolumeFindNext(
    IN HANDLE  hFilterFind,
    IN FILTER_VOLUME_INFORMATION_CLASS  dwInformationClass,
    IN LPVOID  lpBuffer,
    IN DWORD  dwBufferSize,
    OUT LPDWORD  lpBytesReturned
    ); 
HRESULT
WINAPI
  FilterVolumeFindClose(
    IN HANDLE  hVolumeFind
    ); 
}
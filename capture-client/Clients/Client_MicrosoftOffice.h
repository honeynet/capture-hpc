/*
 *	PROJECT: Capture
 *	FILE: Client_MicrosoftOffice.h
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
#pragma once
#define _WIN32_WINNT 0x0501
#include "..\Client.h"
#include "ComHelper.h"
#include <Exdisp.h>
#include <comutil.h>
#include <exdispid.h>
#include <objbase.h>
#include <hash_map>

using namespace std;

class Client_MicrosoftOffice : IClient
{
public:
	Client_MicrosoftOffice(stdext::hash_map<wstring, Client*>* visitorClientsMap);
	~Client_MicrosoftOffice(void);
	DWORD Open(std::wstring client);
	DWORD Visit(std::wstring url, int visitTime, DWORD* minorErrorCode);
	DWORD Close();
private:
	IDispatch* pOfficeApplication;
	IDispatch* pOfficeApplicationDocument;
	wstring OLEobjectName;
};

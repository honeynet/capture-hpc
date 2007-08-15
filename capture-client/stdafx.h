/*
 *	PROJECT: Capture
 *	FILE: stdafx.h
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
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

#include <stdio.h>
#include <tchar.h>
#include "Common.h"
//#define UNITTEST

//enum { VISIT_START = 20, VISIT_FINISH = 21, VISIT_TIMEOUT_ERROR = 22, VISIT_NETWORK_ERROR = 23, VISIT_PROCESS_ERROR = 24, VISIT_PRESTART = 25, VISIT_POSTFINISH  = 26};
//enum { COM_INVOKE_ERROR = 10, COM_GETIDOFNAME_ERROR = 11, COM_CREATEINSTANCE_ERROR = 12, COM_FINDPROGID_ERROR = 13};
// TODO: reference additional headers your program requires here

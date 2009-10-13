/*
 *	PROJECT: Capture
 *	FILE: CaptureGlobal.h
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
#define WIN32_LEAN_AND_MEAN		
// Exclude rarely-used stuff from Windows headers
//#define LOCALTEST			// Output some local statistics for debugging

/*
	This header is included by all objects in the client. This allows all object
	to have access to the various singleton objects in the project
*/
#include "ErrorCodes.h"
#include <windows.h>
#include <string>



class OptionsManager;
class Logger;
class EventController;
class ProcessManager;



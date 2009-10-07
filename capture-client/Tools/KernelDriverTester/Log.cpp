/*
*	PROJECT: Capture 3.0
*	FILE: Log.cpp
*	AUTHORS: Ramon Steenson (rsteenson@gmail.com)
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

#include "Precompiled.h"

#include <stdio.h>

void
Log::LogPrintF(int level, const char* file, int line, const char *format, ...)
{
	char buffer[4096];
	bool error = false;


	
	//itoa(GetTickCount(), buffer, 10);
	
	switch(level)
	{
	case INFO:
		strcpy_s(buffer, 4096, " [INFO] ");
		break;
	case WARNING:
		strcpy_s(buffer, 4096, " [WARNING] ");
		break;
	case ERR:
		strcpy_s(buffer, 4096, " [ERROR] ");
		error = true;
		break;
	default:
		strcpy_s(buffer, 4096, " [INFO] ");
		break;
	}

	int length = strlen(buffer);

	va_list vargs;
	va_start(vargs, format);
	int n_written = vsprintf_s(buffer+length, 4096-length, format, vargs);
	va_end(vargs);

	strcat_s(buffer, 4096, "\n");

	

	//if(error)
	//{
		// TODO What happens if we are in client<->server mode?
	//	MessageBoxA(NULL, buffer, "Capture Error", MB_ICONERROR);
	//}
	OutputDebugStringA(buffer);	
	//_ASSERT( !error );
}
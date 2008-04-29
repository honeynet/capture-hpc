/*
 *	PROJECT: Capture
 *	FILE: Logger.h
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
#include "CaptureGlobal.h"
#include <string>
/*
	Class: Logger

	A singleton which is accessible to all objects. Allows any object to write data
	to the log file if required. The "-l file" option must be specified for data
	to be logged
*/
class Logger
{
public:
	~Logger(void);
	static Logger* getInstance();

	void openLogFile(const std::wstring& file);

	void writeToLog(const std::wstring& message);
	void writeSystemEventToLog(const std::wstring& type, const std::wstring& time, const std::wstring& processId, const std::wstring& process, const std::wstring& action, const std::wstring& object1, const std::wstring& object2);

	void closeLogFile();

	inline bool isFileOpen() { return fileOpen; }

	inline const std::wstring& getLogFileName() { return logFileName; }
	inline const std::wstring& getLogFullPath() { return logFullPath; }

private:
	static bool instanceCreated;
    static Logger *logger;
	Logger(void);

	std::string convertToMultiByteString(const std::wstring& message, size_t& charsConverted);

	bool fileOpen;
	std::wstring logFullPath;
	std::wstring logFileName;
	HANDLE hLog;
};
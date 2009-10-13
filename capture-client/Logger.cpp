#include "Precompiled.h"

#include <algorithm>

#include <strsafe.h>

bool Logger::debug_output_enabled = true;
bool Logger::console_output = false;

Logger::Logger(void)
{
	fileOpen = false;
	hLog = INVALID_HANDLE_VALUE;
}

Logger::~Logger(void)
{
	instanceCreated = false;
}

Logger*
Logger::getInstance()
{
	if(!instanceCreated)
	{
		logger = new Logger();
		instanceCreated = true;	
	}
	return logger;
}

std::string
Logger::convertToMultiByteString(const std::wstring& message, size_t& charsConverted)
{
	std::string converted_message;

	charsConverted = 0;

	int length = static_cast<int>(message.length());
	int mbsize = WideCharToMultiByte(CP_UTF8, 0, message.c_str(), length, NULL, 0, NULL, NULL);
	if (mbsize > 0)
	{	
		char* buffer = (char*)malloc(mbsize);
		int bytes = WideCharToMultiByte(CP_UTF8, 0, message.c_str(), length, 
			buffer, mbsize, NULL, NULL);
		charsConverted = bytes;
		converted_message = buffer;
		free(buffer);
	}
	
	if(converted_message.empty())
	{
		LOG(ERR, "Logger: Multi-byte conversion error %08x allocated: %i", GetLastError(), mbsize);
	}

	return converted_message;
}

void
Logger::writeSystemEventToLog(const std::wstring& type, const std::wstring& time, const std::wstring& processId, 
							  const std::wstring& process, const std::wstring& action, const std::wstring& object1, 
							  const std::wstring& object2)
{
	if(isFileOpen())
	{
		std::wstring message = L"\"";
		message += type;
		message += L"\",\"";
		message += time;
		message += L"\",\"";
		message += processId;
		message += L"\",\"";
		message += process;
		message += L"\",\"";
		message += action;
		message += L"\",\"";
		message += object1;
		message += L"\",\"";
		message += object2;
		message += L"\"\r\n";
		writeToLog(message);
	}
}

void 
Logger::writeToLog(const std::wstring& message)
{
	if(isFileOpen())
	{
		size_t charsConverted;
		std::string szMessage = convertToMultiByteString(message, charsConverted);
		if(szMessage.length() > 0)
		{
			DWORD bytesWritten;
			WriteFile(
				hLog,
				szMessage.c_str(),
				static_cast<unsigned long>(charsConverted), // Ignore null charater
				&bytesWritten,
				NULL
				);
		}
	}
}

void
Logger::openLogFile(const std::wstring& file)
{
	if(!isFileOpen())
	{
		wchar_t* szFullPath = new wchar_t[4096];
		wchar_t* pFileName;
		GetFullPathName(
			file.c_str(),
			4096,
			szFullPath,
			&pFileName
		);
		logFullPath = szFullPath;
		logFileName = pFileName;
		std::transform(logFullPath.begin(),logFullPath.end(),logFullPath.begin(), towlower);
		hLog = CreateFile(
					szFullPath,
					GENERIC_WRITE,
					FILE_SHARE_READ,
					NULL,
					CREATE_ALWAYS,
					FILE_ATTRIBUTE_NORMAL,
					NULL
					);
		
		if(hLog != INVALID_HANDLE_VALUE)
		{
			LOG(INFO, "Logger: Log file opened - %ls", szFullPath);
			fileOpen = true;
		} else {
			LOG(ERR, "Logger: Log file not opened - %08x - %ls", GetLastError(), szFullPath);
		}
		delete [] pFileName;
		delete [] szFullPath;
	}
}

void
Logger::closeLogFile()
{
	if(isFileOpen())
	{
		CloseHandle(hLog);
		fileOpen = false;
	}
}

static const char* OutputTypeStrings[] = 
{
	"INFO: ",
	"WARNING: ",
	"ERROR: "
};

void Logger::DebugOutput(OutputType type, const char* format, ... )
{
	if(!Logger::debug_output_enabled)
		return;

	std::string output = OutputTypeStrings[type];

	char buffer[256];

	va_list arg_list;
	va_start(arg_list, format);
	StringCchVPrintfA(buffer, 256, format, arg_list);
	va_end(arg_list);

	output += buffer;
	output += "\n";

	if(Logger::console_output)
	{
		LOG(INFO, output.c_str());
	}
	else
	{
		OutputDebugStringA(output.c_str());
	}
}
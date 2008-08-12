#include "CaptureGlobal.h"
#include <strsafe.h>
#include <boost/lexical_cast.hpp>

#ifdef CAPTURE_DEBUG
#define FUNCTION_TRACE
#endif

#define XX 100

const char Base64::b64_list[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
const int Base64::b64_index[256] = {
		XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
		XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
		XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,62, XX,XX,XX,63,
		52,53,54,55, 56,57,58,59, 60,61,XX,XX, XX,XX,XX,XX,
		XX, 0, 1, 2,  3, 4, 5, 6,  7, 8, 9,10, 11,12,13,14,
		15,16,17,18, 19,20,21,22, 23,24,25,XX, XX,XX,XX,XX,
		XX,26,27,28, 29,30,31,32, 33,34,35,36, 37,38,39,40,
		41,42,43,44, 45,46,47,48, 49,50,51,XX, XX,XX,XX,XX,
		XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
		XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
		XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
		XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
		XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
		XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
		XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
		XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
	};



void DebugPrint(LPCTSTR pszFormat, ... )
{
	#ifdef CAPTURE_DEBUG
		wchar_t szOutput[MAX_PATH * 2];
		va_list argList;
		va_start(argList, pszFormat);
		StringCchVPrintf(szOutput, MAX_PATH*2, pszFormat, argList);
		va_end(argList);
		printf("%ls\n", szOutput);
		//OutputDebugString(szOutput);
	
	#endif
}

void DebugPrintTrace(LPCTSTR pszFormat, ... )
{
	#ifdef FUNCTION_TRACE
		wchar_t szOutput[MAX_PATH * 2];
		va_list argList;
		va_start(argList, pszFormat);
		StringCchVPrintf(szOutput, MAX_PATH*2, pszFormat, argList);
		va_end(argList);
		printf("%ls\n", szOutput);
		//OutputDebugString(szOutput);
	
	#endif
}

void Warn(LPCTSTR pszFormat, ... )
{
	#ifdef WARN
		wchar_t szOutput[MAX_PATH * 2];
		va_list argList;
		va_start(argList, pszFormat);
		StringCchVPrintf(szOutput, MAX_PATH*2, pszFormat, argList);
		va_end(argList);
		printf("%ls\n", szOutput);
		//OutputDebugString(szOutput);
	
	#endif
}

char*
Base64::decode(const char* encodedBuffer)
{
	size_t position = 0;
	char encoded[4];
	size_t len = strlen(encodedBuffer);
	char* buffer = (char*)malloc(len);
	for(size_t i = 0; i < len+1; i++)
	{
		if((i > 0) && ((i % 4) == 0))
		{
			buffer[position++] = (unsigned char) (b64_index[encoded[0]] << 2 | b64_index[encoded[1]] >> 4);
			buffer[position++] = (unsigned char) (b64_index[encoded[1]] << 4 | b64_index[encoded[2]] >> 2);	
			buffer[position++] = (unsigned char) (b64_index[encoded[2]] << 6 | b64_index[encoded[3]]);
		}
		encoded[i%4] = encodedBuffer[i];
	}
	/* Should alway succeed as base64 encoded string length > decoded string length */
	if(position < len)
	{
		buffer[position] = '\0';
	}
	return buffer;
}

char* 
Base64::encode(char* cleartextBuffer, unsigned int length, size_t* encodedLength)
{
	/* Fairly ineffecient base64 encoding method ... could be a lot better but it works
	   at the moment. If performance is slow when transferring large files this could be
	   the problem */
	unsigned int len = length;	
	int nBlocks = len/3;
	unsigned int remainder = len % 3;
	int position = 0;
	if(remainder != 0)
	{
		nBlocks++;
	}
	char* encodedBuffer = (char*)malloc((nBlocks*4)+2);
	//*encodedLength = (nBlocks*4)+2;
	int k = 0;
	for(unsigned int i = 0; i < len; i+=3)
	{
		unsigned int block = 0;

		block |= ((unsigned char)cleartextBuffer[i] << 16);
		if(i+1 < len)
			block |= ((unsigned char)cleartextBuffer[i+1] << 8);
		if(i+2 < len)
			block |= ((unsigned char)cleartextBuffer[i+2]);

		encodedBuffer[k++] = b64_list[(block >> 18) & 0x3F];
		encodedBuffer[k++] = b64_list[(block >> 12) & 0x3F];
		encodedBuffer[k++] = b64_list[(block >> 6) & 0x3F];
		encodedBuffer[k++] = b64_list[(block & 0x3F)];
	}

	if(remainder > 0)
	{
		for(unsigned int i = remainder; i < 3; i++)
		{
			encodedBuffer[k-(3-i)] = '=';
		}
	}
	*encodedLength = k;
	return encodedBuffer;
}

std::wstring
Time::timefieldToString(const TIME_FIELDS& time)
{
	std::wstring stime;
	stime += boost::lexical_cast<std::wstring>(time.wDay);
	stime += L"/";
	stime += boost::lexical_cast<std::wstring>(time.wMonth);
	stime += L"/";
	stime += boost::lexical_cast<std::wstring>(time.wYear);
	stime += L" ";
	stime += boost::lexical_cast<std::wstring>(time.wHour);
	stime += L":";
	stime += boost::lexical_cast<std::wstring>(time.wMinute);
	stime += L":";
	stime += boost::lexical_cast<std::wstring>(time.wSecond);
	stime += L".";
	stime += boost::lexical_cast<std::wstring>(time.wMilliseconds);
	return stime;
}

std::wstring
Time::systemtimeToString(const SYSTEMTIME& time)
{
	std::wstring stime;
	stime += boost::lexical_cast<std::wstring>(time.wDay);
	stime += L"/";
	stime += boost::lexical_cast<std::wstring>(time.wMonth);
	stime += L"/";
	stime += boost::lexical_cast<std::wstring>(time.wYear);
	stime += L" ";
	stime += boost::lexical_cast<std::wstring>(time.wHour);
	stime += L":";
	stime += boost::lexical_cast<std::wstring>(time.wMinute);
	stime += L":";
	stime += boost::lexical_cast<std::wstring>(time.wSecond);
	stime += L".";
	stime += boost::lexical_cast<std::wstring>(time.wMilliseconds);
	return stime;
}

std::wstring
Time::getCurrentTime()
{
	SYSTEMTIME st;
	GetLocalTime(&st);
	return systemtimeToString(st);
}
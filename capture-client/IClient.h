#pragma once
#include <windows.h>
#include <string>

using namespace std;

class IClient
{
public:
	virtual DWORD Open(std::wstring client) = 0;
	virtual DWORD Visit(std::wstring url, int visitTime, DWORD* minorErrorCode) = 0;
	virtual DWORD Close() = 0;
};

enum { VISIT_START = 20, VISIT_FINISH = 21, VISIT_TIMEOUT_ERROR = 22, VISIT_NETWORK_ERROR = 23, VISIT_PROCESS_ERROR = 24, VISIT_PRESTART = 25, VISIT_POSTFINISH  = 26};
enum { COM_INVOKE_ERROR = 10, COM_GETIDOFNAME_ERROR = 11, COM_CREATEINSTANCE_ERROR = 12, COM_FINDPROGID_ERROR = 13};
// TODO: reference additional headers your program requires here
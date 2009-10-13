#include "Precompiled.h"

#include "StringHelper.h"

std::wstring 
StringHelper::multiByteStringToWideString(const char* string, size_t length)
{
	unsigned int converted_characters = 0;

	wchar_t* wide_string = (wchar_t*)malloc(length*sizeof(wchar_t));

	mbstowcs_s(&converted_characters, wide_string, length, string, _TRUNCATE);

	std::wstring converted_wide_string = wide_string;

	free(wide_string);

	return converted_wide_string;
}

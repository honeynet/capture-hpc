#include "Url.h"
#include <boost/lexical_cast.hpp>

const wchar_t * Url::hexenc[] = {
	L"%00", L"%01", L"%02", L"%03", L"%04", L"%05", L"%06", L"%07",
	L"%08", L"%09", L"%0a", L"%0b", L"%0c", L"%0d", L"%0e", L"%0f",
	L"%10", L"%11", L"%12", L"%13", L"%14", L"%15", L"%16", L"%17",
	L"%18", L"%19", L"%1a", L"%1b", L"%1c", L"%1d", L"%1e", L"%1f",
	L"%20", L"%21", L"%22", L"%23", L"%24", L"%25", L"%26", L"%27",
	L"%28", L"%29", L"%2a", L"%2b", L"%2c", L"%2d", L"%2e", L"%2f",
	L"%30", L"%31", L"%32", L"%33", L"%34", L"%35", L"%36", L"%37",
	L"%38", L"%39", L"%3a", L"%3b", L"%3c", L"%3d", L"%3e", L"%3f",
	L"%40", L"%41", L"%42", L"%43", L"%44", L"%45", L"%46", L"%47",
	L"%48", L"%49", L"%4a", L"%4b", L"%4c", L"%4d", L"%4e", L"%4f",
	L"%50", L"%51", L"%52", L"%53", L"%54", L"%55", L"%56", L"%57",
	L"%58", L"%59", L"%5a", L"%5b", L"%5c", L"%5d", L"%5e", L"%5f",
	L"%60", L"%61", L"%62", L"%63", L"%64", L"%65", L"%66", L"%67",
	L"%68", L"%69", L"%6a", L"%6b", L"%6c", L"%6d", L"%6e", L"%6f",
	L"%70", L"%71", L"%72", L"%73", L"%74", L"%75", L"%76", L"%77",
	L"%78", L"%79", L"%7a", L"%7b", L"%7c", L"%7d", L"%7e", L"%7f",
	L"%80", L"%81", L"%82", L"%83", L"%84", L"%85", L"%86", L"%87",
	L"%88", L"%89", L"%8a", L"%8b", L"%8c", L"%8d", L"%8e", L"%8f",
	L"%90", L"%91", L"%92", L"%93", L"%94", L"%95", L"%96", L"%97",
	L"%98", L"%99", L"%9a", L"%9b", L"%9c", L"%9d", L"%9e", L"%9f",
	L"%a0", L"%a1", L"%a2", L"%a3", L"%a4", L"%a5", L"%a6", L"%a7",
	L"%a8", L"%a9", L"%aa", L"%ab", L"%ac", L"%ad", L"%ae", L"%af",
	L"%b0", L"%b1", L"%b2", L"%b3", L"%b4", L"%b5", L"%b6", L"%b7",
	L"%b8", L"%b9", L"%ba", L"%bb", L"%bc", L"%bd", L"%be", L"%bf",
	L"%c0", L"%c1", L"%c2", L"%c3", L"%c4", L"%c5", L"%c6", L"%c7",
	L"%c8", L"%c9", L"%ca", L"%cb", L"%cc", L"%cd", L"%ce", L"%cf",
	L"%d0", L"%d1", L"%d2", L"%d3", L"%d4", L"%d5", L"%d6", L"%d7",
	L"%d8", L"%d9", L"%da", L"%db", L"%dc", L"%dd", L"%de", L"%df",
	L"%e0", L"%e1", L"%e2", L"%e3", L"%e4", L"%e5", L"%e6", L"%e7",
	L"%e8", L"%e9", L"%ea", L"%eb", L"%ec", L"%ed", L"%ee", L"%ef",
	L"%f0", L"%f1", L"%f2", L"%f3", L"%f4", L"%f5", L"%f6", L"%f7",
	L"%f8", L"%f9", L"%fa", L"%fb", L"%fc", L"%fd", L"%fe", L"%ff"
};

Url::Url(std::wstring u, std::wstring app, int vTime)
{
	url = u;
	visitTime = vTime;
	program = app;
	visited = false;
	majorErrorCode = 0;
	minorErrorCode = 0;
	malicious = false;
}

Url::~Url(void)
{
}

Element
Url::toElement()
{
	Element element;
	element.setName(L"item");
	element.addAttribute(L"url", Url::encode(url));
	element.addAttribute(L"program", program);
	element.addAttribute(L"major-error-code", boost::lexical_cast<std::wstring>(majorErrorCode));
	element.addAttribute(L"minor-error-code", boost::lexical_cast<std::wstring>(minorErrorCode));
	element.addAttribute(L"visited", boost::lexical_cast<std::wstring>(visited));
	return element;
}

std::wstring
Url::encode(const std::wstring& text)
{
	size_t len = text.length();
	std::wstring encoded = L"";
	for(size_t i = 0; i < len; i++)
	{
		wchar_t wch = text.at(i);
		if ('A' <= wch && wch <= 'Z') {
			encoded += wch;
		} else if ('a' <= wch && wch <= 'z') {
			encoded += wch;
		} else if ('0' <= wch && wch <= '9') {
			encoded += wch;
		} else if (wch == ' ') {
			encoded += L"+";
		} else if (wch == '-' || wch == '_'
			|| wch == '.' || wch == '!'
			|| wch == '~' || wch == '*'
			|| wch == '\'' || wch == '('
			|| wch == ')') {
			encoded += wch;
		} else if (wch <= 0x007f) {		// other ASCII
			encoded += Url::hexenc[wch];
		} else if (wch <= 0x07FF) {		// non-ASCII <= 0x7FF
			encoded += Url::hexenc[0xc0 | (wch >> 6)];
			encoded += Url::hexenc[0x80 | (wch & 0x3F)];
		} else {					// 0x7FF < ch <= 0xFFFF
			encoded += Url::hexenc[0xe0 | (wch >> 12)];
			encoded += Url::hexenc[0x80 | ((wch >> 6) & 0x3F)];
			encoded += Url::hexenc[0x80 | (wch & 0x3F)];
		}
	}
	return encoded;
}

std::wstring
Url::decode(const std::wstring& text)
{
	std::wstring decoded = L"";
	wchar_t temp[] = L"0x00";
	size_t len = text.length();
	int sequence = 0;
	wchar_t conwch = 0;
	for(size_t i = 0; i < len; i++)
	{	
		wchar_t wch = text.at(i++);
		if((wch == '%') && (i+1 < len))
		{			
			temp[2] = text.at(i++);
			temp[3] = text.at(i);
			long tconwch = wcstol(temp, NULL, 16);
			if(tconwch <= 0x7F) {
				decoded += tconwch; // normal ascii char
			} else if(tconwch >= 0x80 && tconwch <= 0xBF) { // partial byte
				tconwch = tconwch & 0x3F;
				if(sequence-- == 2)
					tconwch = tconwch << 6;
				conwch |= tconwch;
				if(sequence == 0)
					decoded += conwch;
			} else if(tconwch >= 0xC0 && tconwch <= 0xDF) {
				conwch = (tconwch & 0x1F) << 6; // make space for partial bytes
				sequence = 1; // 1 more partial bytes follow
			} else if(tconwch >= 0xE0 && tconwch <= 0xEF) {
				conwch = (tconwch & 0xF) << 12; // make space for partial bytes
				sequence = 2; // 2 more partial bytes follow
			} // TODO add case fore 3 partial bytes ... very rare
		} else {
			decoded += text.at(--i);
		}
	}
	return decoded;
}
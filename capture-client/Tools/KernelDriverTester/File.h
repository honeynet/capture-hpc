/*
*	PROJECT: Capture 3.0
*	FILE: File.h
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

#pragma once

class File
{
public:
	File( const std::wstring& file_name, unsigned int mode );
	virtual ~File(void);

	bool open();
	void close();
	void flush();

	bool readLine( std::wstring& buffer );
	unsigned int read( wchar_t* buffer, unsigned int bytes );
	
	void write( const std::wstring& buffer );

public:
	static unsigned int append;
	static unsigned int end;
	static unsigned int binary;
	static unsigned int in;
	static unsigned int out;
	static unsigned int truncate;

public:
	static bool createDirectory(const std::wstring& directory);
	static bool deleteDirectory(const std::wstring& directory);
	static void getFullPathName( std::wstring& file );

private:
	bool opened;
	unsigned int mode;
	std::wstring file_name;

	std::wfstream* file_stream;
};

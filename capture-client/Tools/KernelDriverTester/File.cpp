/*
*	PROJECT: Capture 3.0
*	FILE: File.cpp
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

#include "File.h"
#include <fstream>
#include <shellAPI.h>

#define INTERNAL_BUFFER_SIZE 256;

unsigned int File::append = std::fstream::app;
unsigned int File::end = std::fstream::ate;
unsigned int File::binary = std::fstream::binary;
unsigned int File::in = std::fstream::in;
unsigned int File::out = std::fstream::out;
unsigned int File::truncate = std::fstream::trunc;

File::File( const std::wstring& file_name, unsigned int mode )
{
	this->file_name = file_name;
	this->mode = mode;
	file_stream = new std::wfstream();
}

File::~File(void)
{
	delete file_stream;
}

bool
File::open()
{
	file_stream->open( file_name.c_str(), mode );
	return file_stream->is_open();
}

void
File::close()
{
	file_stream->close();
}

bool
File::readLine( std::wstring& buffer )
{
	std::getline( *file_stream, buffer );
	return !file_stream->eof();
}

unsigned int
File::read( wchar_t* buffer, unsigned int characters )
{
	file_stream->read( buffer, characters );
	return file_stream->gcount();
}
	
void
File::write( const std::wstring& buffer )
{
	size_t length = buffer.length() + 1;
	file_stream->write( buffer.c_str(), length );
}

bool
File::createDirectory(const std::wstring& directory)
{
	return CreateDirectory(directory.c_str(), NULL);
}

void
File::getFullPathName( std::wstring& file )
{
	wchar_t* full_file_path = NULL;
	DWORD size = GetFullPathName( file.c_str(), 0, NULL, NULL );

	full_file_path = new wchar_t[size];

	GetFullPathName( file.c_str(), size, full_file_path, NULL );

	file = full_file_path;

	delete [] full_file_path;
}

bool
File::deleteDirectory(const std::wstring& directory)
{
	wchar_t directory_path[1024];
	SHFILEOPSTRUCT delete_directory;
	
	// Get the full path of the directory specified
	GetFullPathName( directory.c_str(), 1024, directory_path, NULL );

	delete_directory.hwnd = NULL;
	delete_directory.pTo = NULL;
	delete_directory.lpszProgressTitle = NULL;
	delete_directory.wFunc = FO_DELETE;
	delete_directory.pFrom = directory_path;
	delete_directory.fFlags = FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI;
	
	return SHFileOperation( &delete_directory );
}
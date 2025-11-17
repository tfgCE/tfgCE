#include "file.h"

#include "dir.h"

#include "..\casts.h"

#include "..\performance\performanceUtils.h"
#include "..\types\colour.h"

#include <iostream>

#ifdef AN_WINDOWS
	#include <io.h>
	#include <share.h>
#endif
#ifdef AN_LINUX
	#include <ios>
#endif
#ifdef AN_ANDROID
	#include <unistd.h>
#endif

#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace IO;

//

File::File()
: Interface(&read_data_from_file, &write_data_to_file, &go_to_in_file, &get_loc_in_file)
, file(noFile)
, isWriting(false)
, bufLen(0)
{
}

File::File(String const & _fileName)
: Interface(&read_data_from_file, &write_data_to_file, &go_to_in_file, &get_loc_in_file)
, file(noFile)
, isWriting(false)
, bufLen(0)
{
	open(_fileName);
}

File::~File()
{
	close();
}

bool File::open(String const & _fileName)
{
	close();

	#ifdef AN_WINDOWS
	int errNo = tsopen_s(&file, _fileName.to_char(), _O_BINARY, _SH_DENYNO, _S_IREAD | _S_IWRITE);
	#else
	#ifdef AN_LINUX
	file = ::open(_fileName.to_char(), std::_S_bin | std::_S_in);
	#else
	#ifdef AN_ANDROID
	file = ::open(_fileName.to_char8_array().get_data(), O_RDWR);
	#else
	#error TODO implement
	#endif
	#endif
	#endif

	if (file < 0)
	{
		file = noFile;
	}

	isWriting = false;
	bufLen = 0;
	bufPtr = buf;

	return is_open();
}

bool File::open_to_write(String const & _fileName)
{
	close();

	#ifdef AN_WINDOWS
	int errNo = tsopen_s(&file, _fileName.to_char(), _O_BINARY | _O_RDWR, _SH_DENYWR, _S_IREAD | _S_IWRITE);
	#else
	#ifdef AN_LINUX
	file = open(_fileName.to_char(), std::_S_bin | std::_S_in);
	#else
	#ifdef AN_ANDROID
	file = ::open(_fileName.to_char8_array().get_data(), O_RDWR);
	#else
	#error TODO implement
	#endif
	#endif
	#endif

	if (file < 0)
	{
		file = noFile;
    }

	isWriting = true;
	bufLen = 0;
	bufPtr = buf;

	return is_open();
}

bool File::append_to(String const & _fileName)
{
	close();

	if (!does_exist(_fileName))
	{
		create(_fileName);
		close();
	}

#ifdef AN_WINDOWS
	int errNo = tsopen_s(&file, _fileName.to_char(), _O_APPEND | _O_RDWR | _O_BINARY, _SH_DENYWR, _S_IREAD | _S_IWRITE);
#else
#ifdef AN_LINUX
	file = open(_fileName.to_char(), std::_S_ate | std::_S_append | std::_S_bin | std::_S_in | std::_S_out);
#else
#ifdef AN_ANDROID
	file = ::open(_fileName.to_char8_array().get_data(), O_APPEND | O_RDWR);
#else
#error TODO implement
#endif
#endif
#endif


	if (file < 0)
	{
		file = noFile;
	}

	isWriting = true;
	bufLen = 0;
	bufPtr = buf;

	return is_open();
}

bool File::create(String const & _fileName)
{
	// auto create path to the file
	int lastDirChar = _fileName.find_last_of('\\');
	lastDirChar = max(lastDirChar, _fileName.find_last_of('/'));
	if (lastDirChar != NONE && lastDirChar > 0)
	{
		IO::Dir::create(_fileName.get_left(lastDirChar));
	}

	close();

	#ifdef AN_WINDOWS
	int errNo = tsopen_s(&file, _fileName.to_char(), _O_CREAT | _O_TRUNC | _O_RDWR | _O_BINARY, _SH_DENYWR, _S_IREAD | _S_IWRITE);
	#else
	#ifdef AN_LINUX
	file = open(_fileName.to_char(), std::_S_ate | std::_S_trunc | std::_S_bin | std::_S_in | std::_S_out);
	#else
	#ifdef AN_ANDROID
	file = ::open(_fileName.to_char8_array().get_data(), O_CREAT | O_TRUNC | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
	#else
	#error TODO implement
	#endif
	#endif
	#endif

	if (file < 0)
	{
		file = noFile;
    }

	isWriting = true;
	bufLen = 0;
	bufPtr = buf;

	return is_open();
}

uint File::read_buffer()
{
	#ifdef AN_WINDOWS
	bufLen = _read(file, buf, bufSize);
	#else
	#ifdef AN_LINUX_OR_ANDROID
	bufLen = ::read(file, buf, bufSize);
	#else
	#error TODO implement
	#endif
	#endif

	if (bufLen <= 0)
	{
		bufLen = 0;
	}

	bufPtr = buf;

	return bufLen;
}

uint File::write_buffer()
{
	uint written = bufLen;
	if (bufLen > 0)
	{
		PERFORMANCE_MARKER(Colour::magenta);
		#ifdef AN_WINDOWS
		written = _write(file, buf, bufLen);
		#else
		#ifdef AN_LINUX_OR_ANDROID
		written = ::write(file, buf, bufLen);
		#else
		#error TODO implement
		#endif
		#endif
		bufLen = 0;
		PERFORMANCE_MARKER(Colour::c64Brown);
	}

	bufPtr = buf;

	return written;
}

void File::close()
{
	if (! is_open())
	{
		return;
	}

	if (isWriting)
	{
		write_buffer();
	}

	#ifdef AN_WINDOWS
	_close(file);
	#else
	#ifdef AN_LINUX_OR_ANDROID
	::close(file);
	#else
	#error TODO implement
	#endif
	#endif

	file = noFile;
}

uint File::read_data_from_file(Interface * io, void * const _data, uint const _numBytes)
{
	File* file = (File*)io;

	if (! file->is_open())
	{
		return 0;
	}

	byte * ptr;
	uint read = 0;

	ptr = plain_cast<byte>(_data);
	uint toRead = _numBytes;
	while (toRead)
	{
		if (file->bufLen <= 0)
		{
			if (! file->read_buffer())
			{
				return read;
			}
		}
		// TODO could speed that up to read blocks instead of doing one by one
		while (file->bufLen && toRead)
		{
			*ptr = *file->bufPtr;
			++ file->bufPtr;
			++ ptr;
			-- file->bufLen;
			-- toRead;
			++ read;
		}
	}

	return read;
}

uint File::write_data_to_file(Interface * io, void const * const _data, uint const _numBytes)
{
	File* file = (File*)io;

	if (! file->is_open())
	{
		return 0;
	}

	byte const * ptr;
	uint written = 0;

	ptr = plain_cast<byte>(_data);
	uint toWrite = _numBytes;
	while (toWrite)
	{
		if (file->bufLen == bufSize)
		{
			if (! file->write_buffer())
			{
				return written;
			}
		}
		// TODO could speed that up to read blocks instead of doing one by one
		while (file->bufLen != bufSize && toWrite)
		{
			*file->bufPtr = *ptr;
			++ file->bufPtr;
			++ ptr;
			++ file->bufLen;
			-- toWrite;
			++ written;
		}
    }

	return written;
}

uint File::go_to_in_file(Interface * io, uint const _newLoc)
{
	File* file = (File*)io;

	if (! file->is_open())
	{
		return 0;
	}

	if (file->isWriting)
	{
		file->write_buffer();
	}

	#ifdef AN_WINDOWS
	uint loc = _lseek(file->file, _newLoc, SEEK_SET);
	#else
	#ifdef AN_LINUX_OR_ANDROID
	uint loc = ::lseek(file->file, _newLoc, SEEK_SET);
	#else
	#error TODO implement
	#endif
	#endif

	if (! file->isWriting)
	{
		file->read_buffer();
	}
	else
	{
		file->bufLen = 0;
		file->bufPtr = file->buf;
	}

	return loc;
}

uint File::get_loc_in_file(Interface * io)
{
	File* file = (File*)io;

	if (!file->is_open())
	{
		return 0;
	}

	if (file->isWriting)
	{
		// write buffer to get proper loc in file
		file->write_buffer();
	}

#ifdef AN_WINDOWS
	uint loc = _lseek(file->file, 0, SEEK_CUR);
#else
#ifdef AN_LINUX_OR_ANDROID
	uint loc = lseek(file->file, 0, SEEK_CUR);
#else
#error TODO implement
#endif
#endif

	if (!file->isWriting)
	{
		// where we actually are when reading, move back by current bufLen (this is how much do we have left in our buffer
		loc = (uint)((int)loc - (int)file->bufLen);
	}

	return loc;
}

bool File::is_open() const
{
	return file != noFile;
}

bool File::does_exist(String const & _fileName)
{
	todo_future(TXT("awkward for checking if file exists"));

	int file;
#ifdef AN_WINDOWS
	int errNo = tsopen_s(&file, _fileName.to_char(), _O_BINARY, _SH_DENYNO, _S_IREAD);
#else
#ifdef AN_LINUX
	file = open(_fileName.to_char(), std::_S_bin | std::_S_in);
#else
#ifdef AN_ANDROID
	file = ::open(_fileName.to_char8_array().get_data(), O_RDWR);
#else
#error TODO implement
#endif
#endif
#endif

	if (file < 0)
	{
		file = noFile;
	}

	if (file != noFile)
	{
#ifdef AN_WINDOWS
		_close(file);
#else
#ifdef AN_LINUX_OR_ANDROID
		::close(file);
#else
#error TODO implement
#endif
#endif
	}

	return file != noFile;
}

uint File::get_size_of(String const & _fileName)
{
	File *file = new File();
	uint result = 0;
	if (file->open(_fileName))
	{
#ifdef AN_WINDOWS
		uint loc = _lseek(file->file, 0, SEEK_END);
#else
#ifdef AN_LINUX_OR_ANDROID
		uint loc = ::lseek(file->file, 0, SEEK_END);
#else
#error TODO implement
#endif
#endif
		result = loc;
	}
	delete file;
	return result;
}

void File::copy_to_file(String const& _fileName)
{
	int writeToFile;
#ifdef AN_WINDOWS
	int errNo = tsopen_s(&writeToFile, _fileName.to_char(), _O_CREAT | _O_TRUNC | _O_RDWR | _O_BINARY, _SH_DENYWR, _S_IREAD | _S_IWRITE);
#else
#ifdef AN_LINUX
	writeToFile = open(_fileName.to_char(), std::_S_ate | std::_S_trunc | std::_S_bin | std::_S_in | std::_S_out);
#else
#ifdef AN_ANDROID
	writeToFile = ::open(_fileName.to_char8_array().get_data(), O_CREAT | O_TRUNC | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
#else
#error TODO implement
#endif
#endif
#endif

	int loc = get_loc();
	go_to(0);
	while (read_buffer())
	{
		if (bufLen > 0)
		{
#ifdef AN_WINDOWS
			_write(writeToFile, buf, bufLen);
#else
#ifdef AN_LINUX_OR_ANDROID
			::write(writeToFile, buf, bufLen);
#else
#error TODO implement
#endif
#endif
		}
	}
	go_to(loc);

#ifdef AN_WINDOWS
	_close(writeToFile);
#else
#ifdef AN_LINUX_OR_ANDROID
	::close(writeToFile);
#else
#error TODO implement
#endif
#endif
}

void File::rename_file(String const& _from, String const& _to)
{
	auto from = _from.to_char8_array();
	auto to = _to.to_char8_array();
	std::rename(from.get_data(), to.get_data());
}

void File::delete_file(String const& _file)
{
	auto file = _file.to_char8_array();
	std::remove(file.get_data());
}

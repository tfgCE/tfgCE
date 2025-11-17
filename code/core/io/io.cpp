#include "io.h"

#include "dir.h"
#include "file.h"

#include "..\concurrency\scopedSpinLock.h"

#include "..\types\optional.h"
#include "..\types\unicode.h"

#include "..\other\parserUtils.h"

#include "..\system\android\androidApp.h"

#include "..\buildInformation.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace IO;

//

struct IOStatic
{
	String mainDirectory;

	IOStatic()
	{
#ifdef AN_WINDOWS
		TCHAR currentDirBuf[16384];
		GetCurrentDirectory(16384, currentDirBuf); // we want to be in the main directory - always

		mainDirectory = currentDirBuf;
#else
		// no need for time being?
#endif
	}
};
IOStatic* ioStatic = nullptr;

void IO::initialise_static()
{
	an_assert(!ioStatic);
	ioStatic = new IOStatic();
}

void IO::close_static()
{
	an_assert(ioStatic);
	delete_and_clear(ioStatic);
}

String const & IO::get_main_directory()
{
	an_assert(ioStatic);
	return ioStatic->mainDirectory;
}

void IO::go_to_main_directory()
{
#ifdef AN_WINDOWS
	SetCurrentDirectory(get_main_directory().to_char());
#else
	// no need for time being?
#endif
}

void IO::go_to_directory(String const & _directory)
{
	// so we will always start at the relative directory
	go_to_main_directory();
#ifdef AN_WINDOWS
	SetCurrentDirectory(_directory.to_char());
#else
	// no need for time being?
#endif
}

#ifdef AN_ANDROID
AAssetManager* IO::get_asset_manager()
{
	return ::System::Android::App::get().get_activity()->assetManager;
}
#endif

String IO::get_user_game_data_directory()
{
#ifdef AN_ANDROID
	return String::from_char8(::System::Android::App::get().get_activity()->externalDataPath) + get_directory_separator();
#endif
	return String();
}

String IO::get_asset_data_directory()
{
	// will know where assets are
	return String();
}

String IO::get_auto_directory_name()
{
	return String(TXT("_auto"));
}

tchar const* IO::get_directory_separator()
{
#ifdef AN_ANDROID
	return TXT("/");
#endif
	return TXT("\\");
}

void IO::validate_auto_directory()
{
	String autoFolder = IO::get_user_game_data_directory() + IO::get_auto_directory_name();
	String buildInfoFile = autoFolder + IO::get_directory_separator() + String(TXT("buildInfo"));
	bool deleteAutoFolder = true;
	int currentBuildNumber = get_build_version_build_no();
	if (IO::File::does_exist(buildInfoFile))
	{
		IO::File f;
		if (f.open(buildInfoFile))
		{
			f.set_type(IO::InterfaceType::Text);
			int readBuildNumber;
			if (f.read_int(readBuildNumber))
			{
				deleteAutoFolder = readBuildNumber != currentBuildNumber;
			}
		}
	}
	if (deleteAutoFolder)
	{
		output(TXT("_auto: clear"));
		IO::Dir::delete_dir(autoFolder);
		output(TXT("_auto: cleared"));
	}
	else
	{
		output(TXT("_auto: keep"));
	}
	if (!IO::File::does_exist(buildInfoFile))
	{
		IO::File f;
		if (f.create(buildInfoFile))
		{
			f.set_type(IO::InterfaceType::Text);
			f.write_text(String::printf(TXT("%i"), currentBuildNumber));
		}
	}
}

String IO::sanitise_filepath(String const & _filePath)
{
#ifdef AN_ANDROID
	return _filePath.replace(String(TXT("\\")), String(TXT("/")));
#endif
	return _filePath;
}

String IO::unify_filepath(String const & _filePath)
{
	return _filePath.replace(String(TXT("\\")), String(TXT("/")));
}

String IO::safe_filename(String const & _fileName)
{
	String fileName = _fileName;
	for_every(ch, fileName.access_data())
	{
		if (*ch != 0)
		{
			if (*ch >= '0' && *ch <= '9')
			{
				continue;
			}
			if (*ch >= 'a' && *ch <= 'z')
			{
				continue;
			}
			if (*ch >= 'A' && *ch <= 'Z')
			{
				continue;
			}
			*ch = '_';
		}
	}
	return fileName;
}

Interface::Interface(ReadFunc _read_func, WriteFunc _write_func, GoToFunc _go_to_func, GetLocFunc _get_loc_func)
: read_func(_read_func)
, write_func(_write_func)
, go_to_func(_go_to_func)
, get_loc_func(_get_loc_func)
, lastReadChar(0)
, hasLastChar(false)
{
	an_assert(read_func != nullptr, TXT("read function not provided"));
	// write func may be missing if no writing possible
	an_assert(go_to_func != nullptr, TXT("go to function not provided"));
	an_assert(get_loc_func != nullptr, TXT("get location function not provided"));
}

Interface::Interface()
: read_func(nullptr)
, write_func(nullptr)
, go_to_func(nullptr)
, get_loc_func(nullptr)
, lastReadChar(0)
, hasLastChar(false)
{
}

uint Interface::read_into(Array<int8> & _array)
{
	int const blockSize = 16384;
	int8 tempBlock[blockSize];
	uint tempBlockSize = 0;
	uint totalReadSize = 0;
	while ((tempBlockSize = read(tempBlock, blockSize)))
	{
		_array.grow_size(tempBlockSize);
		memory_copy(&_array[totalReadSize], tempBlock, tempBlockSize);
		totalReadSize += tempBlockSize;
	}
	_array.prune();
	return totalReadSize;
}

uint Interface::read(void * const _data, uint const _numBytes)
{
	return read_func? read_func(this, _data, _numBytes) : 0;
}

uint Interface::write(void const * const _data, uint const _numBytes)
{
	return write_func? write_func(this, _data, _numBytes) : 0;
}

uint Interface::go_to(uint const _where)
{
	hasLastChar = false;
	return go_to_func? go_to_func(this, _where) : 0;
}

uint Interface::get_loc()
{
	return get_loc_func? get_loc_func(this) : 0;
}

uint Interface::read_into_string(OUT_ String& _string)
{
	return read_into_string(_string, NP);
}

uint Interface::read_into_string(OUT_ String & _string, Optional<int> const & _size)
{
	_string = String(TXT(""));
	if (_size.is_set())
	{
		_string.access_data().make_space_for(_size.get() + 1);
	}
	while (tchar const ch = read_char())
	{
		_string += ch;
		if (_size.is_set() && _string.get_length() >= _size.get())
		{
			break;
		}
	}
	return _string.get_length();
}

uint Interface::read_lines(List<String> & _lines)
{
	an_assert(is_type_text(), TXT("should be used only for text interfaces"));

	String line;
	tchar prevch = 0;
	uint readCount = 0;
	while (tchar ch = read_char())
	{
		if (ch != 13 && ch != 10)
		{
			line += ch;
		}
		else
		{
			if (ch == 10 && prevch == 13)
			{
				// it was already handled
			}
			else
			{
				++ readCount;
				_lines.push_back(line);
				line = String(TXT(""));
			}
			prevch = ch;
		}
	}
	++ readCount;
	_lines.push_back(line);
	return readCount;
}

bool Interface::read_token(Token & _token, bool _readEmptySymbol)
{
	an_assert(is_type_text(), TXT("should be used only for text interfaces"));

	_token.clear();
	if (! hasLastChar)
	{
		read_char();
	}
	else if (lastReadChar == ' ' ||
			 lastReadChar == '\t' ||
			 lastReadChar == '\n' ||
			 lastReadChar == 13)
	{
		// pretend that such last char did not exist and read next
		read_char();
	}

	bool readingNumber = false;
	bool readingNumberE = false;
	bool readingText = false;
	while (true)
	{
		if (! hasLastChar)
		{
			return ! _token.is_empty();
		}
		tchar ch = lastReadChar;
		if (ch != ' ' &&
			ch != '\t' &&
			ch != '\n' &&
			ch != 13)
		{
			if (_token.get_length() > 0)
			{
				if (readingNumber)
				{
					if (ch == 'E' ||
						(ch == '-' && readingNumberE))
					{
						readingNumberE = true;
					}
					else if (! ((ch >= '0' && ch <= '9') || ch == '.'))
					{
						return true;
					}
				}
				else if (readingText)
				{
					if (! ((ch >= 'a' && ch <= 'z') ||
						   (ch >= 'A' && ch <= 'Z') ||
						   (ch == '_') ||
						   (ch >= '0' && ch <= '9')))
					{
						return true;
					}
				}
			}
			else
			{
				if (ch >= '0' && ch <= '9')
				{
					readingNumber = true;
				}
				else if ((ch >= 'a' && ch <= 'z') ||
						 (ch >= 'A' && ch <= 'Z') ||
						 (ch == '_'))
				{
					readingText = true;
				}
				else
				{
					// read a single symbol
					_token += ch;
					clear_last_read_char();
					return true;
				}
			}
			_token += ch;
		}
		else
		{
			// token has ended
			if (_token.get_length() > 0)
			{
				return true;
			}
			if ((ch != ' ' &&
				 ch != '\t') ||
				_readEmptySymbol)
			{
				_token += ch; // \n or 13 or empty symbol
				clear_last_read_char();
				return true;
			}
			// otherwise don't store empty character
		}
		read_char();
	}
	return false;
}

bool Interface::read_int(int & _value)
{
	Token token;
	if (!read_token(token))
	{
		_value = 0;
		return false;
	}
	_value = 1;
	if (token == TXT("-"))
	{
		_value = -1;
		if (!read_token(token))
		{
			return false;
		}
	}
	_value = _value * ParserUtils::parse_int(token.to_char());
	return true;
}

bool Interface::read_float(float & _value)
{
	Token token;
	if (! read_token(token))
	{
		_value = 0.0f;
		return false;
	}
	_value = 1.0f;
	if (token == TXT("-"))
	{
		_value = -1.0f;
		if (! read_token(token))
		{
			return false;
		}
	}
	_value = _value * ParserUtils::parse_float(token.to_char());
	return true;
}

bool Interface::read_next_line()
{
	an_assert(is_type_text(), TXT("should be used only for text interfaces"));

	if (!hasLastChar)
	{
		read_char();
	}
	while (true)
	{
		if (! hasLastChar)
		{
			return false;
		}
		tchar ch = lastReadChar;
		read_char();
		if (ch == '\n' ||
			ch == 13)
		{
			// until we read something else than end of line
			if (lastReadChar == '\n' ||
				lastReadChar == 13)
			{
				continue;
			}
			return true;
		}
	}
}

uint Interface::read_single(void *const _data, const uint _numBytes)
{
	uint readBytes = read(_data, _numBytes);

	adjust_endian(_data, readBytes);

	return readBytes;
}

uint Interface::write_single(void *const _data, const uint _numBytes)
{
	adjust_endian(_data, _numBytes);

	uint writtenBytes = write(_data, _numBytes);

	adjust_endian(_data, _numBytes);

	return writtenBytes;
}

uint Interface::write_single(void const * const _data, uint const _numBytes)
{
	if (does_need_endian_adjust(_numBytes))
	{
		byte * data = new byte[_numBytes];
		memory_copy(data, _data, _numBytes);
		uint writtenBytes = write_single(data, _numBytes);
		delete [] data;
		return writtenBytes;
	}
	else
	{
		return write(_data, _numBytes);
	}
}

uint Interface::omit_spaces(bool _startingWithLastRead /* = false */, int * _lineNumber /* = nullptr */)
{
	an_assert(is_type_text(), TXT("should be used only for text interfaces"));

	uint spaces = 0;

	if (_startingWithLastRead)
	{
		tchar const ch = get_last_read_char();

		if (ch == ' ')
		{
			++spaces;
		}

		if (ch != ' ')
		{
			return spaces;
		}
	}

	// trim all spaces and special characters
	while (tchar const ch = read_char(_lineNumber))
	{
		if (ch == ' ')
		{
			++spaces;
		}

		if (ch != ' ')
		{
			break;
		}
	}

	return spaces;
}

uint Interface::omit_spaces_and_special_characters(bool _startingWithLastRead /* = false */, int * _lineNumber /* = nullptr */)
{
	an_assert(is_type_text(), TXT("should be used only for text interfaces"));

	uint spaces = 0;

	if (_startingWithLastRead)
	{
		tchar const ch = get_last_read_char();

		if (ch == ' ')
		{
			++ spaces;
		}

		if (ch != ' ' &&
			ch != '\n' &&
			ch != '\t' &&
			ch != 13)
		{
			return spaces;
		}
	}

	// trim all spaces and special characters
	while (tchar const ch = read_char(_lineNumber))
	{
		if (ch == ' ')
		{
			++ spaces;
		}

		if (ch != ' ' &&
			ch != '\n' &&
			ch != '\t' &&
			ch != 13)
		{
			break;
		}
	}

	return spaces;
}

tchar const Interface::read_char(int * _lineNumber)
{
	an_assert(is_type_text(), TXT("should be used only for text interfaces"));

	// based on: http://ratfactor.com/utf-8-to-unicode

	byte ch[4]; // for bytes 1-3 masks are applied at the beginning, for byte 0 when calculating final value
	int bytesRead = 0;
	if (read_single(ch, 1))
	{
		bool readOk = true;
		++bytesRead;
		if (type == IO::InterfaceType::UTF8)
		{
			if (_lineNumber && ch[0] == '\n')
			{
				if (_lineNumber)
				{
					*_lineNumber = *_lineNumber + 1;
				}
			}
			int multiByteCheck[3] = { 192, 224, 240 };
			for(int i = 0; i < 3; ++ i)
			{
				if ((ch[0] & multiByteCheck[i]) == multiByteCheck[i]) // all required bits are set
				{
					if (read_single(&ch[bytesRead], 1))
					{
						ch[bytesRead] &= 0x3F;
						++bytesRead;
					}
				}
			}
			if (readOk)
			{
				uint32 readCh = 0;
				if (bytesRead == 1)
				{
					readCh = ((uint32)ch[0] & 0x7F);
				}
				else if (bytesRead == 2)
				{
					readCh = ((uint32)ch[0] & 0x1F) * 64 + (uint32)ch[1];
				}
				else if (bytesRead == 3)
				{
					readCh = (((uint32)ch[0] & 0xF) * 64 + (uint32)ch[1]) * 64 + (uint32)ch[2];
				}
				else if (bytesRead == 4)
				{
					readCh = ((((uint32)ch[0] & 0x7) * 64 + (uint32)ch[1]) * 64 + (uint32)ch[2]) * 64 + (uint32)ch[3];
				}
				else
				{
					an_assert(false);
				}
#ifdef AN_WIDE_CHAR
				lastReadChar = Unicode::unicode_to_tchar(readCh);
#else
				if (bytesRead > 1)
				{
					lastReadChar = '?';
				}
				else
				{
					lastReadChar = Unicode::unicode_to_tchar(readCh);
				}
#endif
			}
		}
		else if (type == IO::InterfaceType::ANSI)
		{
			if (ch[0] >= 128)
			{
				ch[0] = '?';
				warn(TXT("consider using UTF8 file"));
			}
			lastReadChar = (tchar)ch[0];
		}
		if (readOk)
		{
			hasLastChar = true;
			return lastReadChar;
		}
	}

	// no char read
	hasLastChar = false;
	lastReadChar = 0;
	return lastReadChar;
}

uint Interface::write_text(String const & _string)
{
	an_assert(is_type_text(), TXT("should be used only for text interfaces"));

	return write_text(_string.get_data().get_data());
}

uint Interface::write_text(tchar const * const _text)
{
	an_assert(is_type_text(), TXT("should be used only for text interfaces"));

	// based on: http://ratfactor.com/utf-8-to-unicode
	// reversed

	uint writtenChars(0);
	uint length((uint)wcslen(_text));
	tchar const * ch = _text;
	for (uint i = 0; i < length; ++i, ++ ch)
	{
		if (type == IO::InterfaceType::UTF8)
		{
			uint uchar = Unicode::tchar_to_unicode(*ch);
			if (/*uchar >= 0 &&*/uchar < 128)
			{
				// 00000000
				// 0xxxxxxx
				// .6543210
				char8 ch8 = (byte)(uchar & 0x7F);
				writtenChars += write_single(&ch8, 1);
			}
			else if (uchar < 2048) // 2^11
			{
				// 00000000 11111111 
				// 110xxxxx 10xxxxxx
				// ...09876 ..543210
				// ...1.... ........
				byte bytes[2];
				bytes[1] = (byte)((uchar)      & 0x3F) | 0x80;
				bytes[0] = (byte)((uchar >> 6) & 0x1F) | 0xC0;
				write_single(&bytes[0], 1);
				write_single(&bytes[1], 1);
				++writtenChars;
			}
			else if (uchar < 65536) // 2 ^ 16
			{
				// 00000000 11111111 22222222
				// 1110xxxx 10xxxxxx 10xxxxxx
				// ....5432 ..109876 ..543210
				// ....1111 ..11.... ........
				byte bytes[3];
				bytes[2] = (byte)((uchar)       & 0x3F) | 0x80;
				bytes[1] = (byte)((uchar >> 6)  & 0x3F) | 0x80;
				bytes[0] = (byte)((uchar >> 12) & 0x0F) | 0xE0;
				write_single(&bytes[0], 1);
				write_single(&bytes[1], 1);
				write_single(&bytes[2], 1);
				++writtenChars;
			}
			else
			{
				// 00000000 11111111 22222222 33333333
				// 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
				// .....987 ..654321 ..109876 ..543210
				// .....111 ..111111 ..11.... ........
				byte bytes[4];
				bytes[3] = (byte)((uchar)       & 0x3F) | 0x80;
				bytes[2] = (byte)((uchar >> 6)  & 0x3F) | 0x80;
				bytes[1] = (byte)((uchar >> 12) & 0x3F) | 0x80;
				bytes[0] = (byte)((uchar >> 18) & 0x07) | 0xF0;
				write_single(&bytes[0], 1);
				write_single(&bytes[1], 1);
				write_single(&bytes[2], 1);
				write_single(&bytes[3], 1);
				++writtenChars;
			}
		}
		else if (type == IO::InterfaceType::ANSI)
		{
			char8 ch8;
			if (
#ifndef AN_CLANG
				*ch >= 0 &&
#endif
				*ch < 128)
			{
				ch8 = (byte)(*ch & 0x7F);
			}
			else
			{
				ch8 = '?';
			}
			writtenChars += write_single(&ch8, 1);
		}
		else
		{
			an_assert(false);
		}
	}

	return writtenChars;
}

uint Interface::write_text(char8 const * const _text)
{
	an_assert(is_type_text(), TXT("should be used only for text interfaces"));

	uint writtenChars(0);
	uint length((uint)(strlen(_text)));
	char8 const * ch = _text;
	for (uint i = 0; i < length; ++i, ++ ch)
	{
		writtenChars += write_single(ch, 1);
	}

	return writtenChars;
}

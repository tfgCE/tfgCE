#include "dir.h"

#include "file.h"
#include "utils.h"

#ifdef AN_LINUX_OR_ANDROID
#include <sys\stat.h>
#include <dirent.h>
#include <errno.h>
#endif

#include <iostream>

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace IO;

//

FileTime::FileTime()
: lowDateTime(0)
, highDateTime(0)
{
}

FileTime::FileTime(int _lowDateTime, int _highDateTime)
: lowDateTime(_lowDateTime)
, highDateTime(_highDateTime)
{
}

FileTime::FileTime(long int _dateTime)
: lowDateTime(_dateTime & 0xffffffff)
#ifdef AN_LINUX_OR_ANDROID
, highDateTime((_dateTime >> 32) & 0xffffffff)
#else
, highDateTime(0)
#endif
{
}

bool FileTime::operator == (FileTime const & _other) const
{
	return lowDateTime == _other.lowDateTime &&
		   highDateTime == _other.highDateTime;
}

bool FileTime::operator != (FileTime const & _other) const
{
	return lowDateTime != _other.lowDateTime ||
		   highDateTime != _other.highDateTime;
}

//

DirEntry::DirEntry()
: isDirectory(false)
{
}

bool DirEntry::operator == (DirEntry const & _other) const
{
	return path == _other.path &&
		   filename == _other.filename &&
		   isDirectory == _other.isDirectory &&
		   creationTime == _other.creationTime &&
		   lastAccessTime == _other.lastAccessTime &&
		   lastWriteTime == _other.lastWriteTime;
}

bool DirEntry::operator != (DirEntry const & _other) const
{
	return path != _other.path ||
		   filename != _other.filename ||
		   isDirectory != _other.isDirectory ||
		   creationTime != _other.creationTime ||
		   lastAccessTime != _other.lastAccessTime ||
		   lastWriteTime != _other.lastWriteTime;
}

wchar_t* convert_to_wide_char(tchar const * _text)
{
#ifdef AN_WIDE_CHAR
	wchar_t* wideCharString = new wchar_t[4096];
	tsprintf(wideCharString, 4096-1, _text);
	return wideCharString;
#else
	#error this doesn't make too much sense (although as there are no multibyte characters, it will work), we shouldn't be using it ever
	wchar_t* wideCharString = new wchar_t[4096];
	MultiByteToWideChar(CP_ACP, 0, _text, -1, wideCharString, 4096);
	return wideCharString;
#endif
}

void DirEntry::setup_for_file(String const & _path)
{
	path = _path;
	filename = IO::Utils::get_file(_path);
	isDirectory = false;

#ifdef AN_WINDOWS
	wchar_t* fileNameWideChar = convert_to_wide_char(path.to_char());
	HANDLE hFile = CreateFile(fileNameWideChar, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		creationTime = FileTime();
		lastAccessTime = FileTime();
		lastWriteTime = FileTime();
	}
	else
	{
		FILETIME ftCreation;
		FILETIME ftLastAccess;
		FILETIME ftLastWrite;
		GetFileTime(hFile, &ftCreation, &ftLastAccess, &ftLastWrite);
		CloseHandle(hFile);

		creationTime = FileTime(ftCreation.dwLowDateTime, ftCreation.dwHighDateTime);
		lastAccessTime = FileTime(ftLastAccess.dwLowDateTime, ftLastAccess.dwHighDateTime);
		lastWriteTime = FileTime(ftLastWrite.dwLowDateTime, ftLastWrite.dwHighDateTime);
	}
	delete[] fileNameWideChar;
#else
#ifdef AN_LINUX_OR_ANDROID
	Array<char8> filename8 = path.to_char8_array();
	struct stat attrib;
	stat(filename8.get_data(), &attrib);
	creationTime = FileTime(attrib.st_ctim.tv_sec);
	lastAccessTime = FileTime(attrib.st_atim.tv_sec);
	lastWriteTime = FileTime(attrib.st_mtim.tv_sec);
#else
		#error implement
#endif
#endif
}

//

Dir::Dir()
{
}

bool Dir::does_exist(String const& _dir)
{
#ifdef AN_WINDOWS
	wchar_t* dirWideChar = convert_to_wide_char(_dir.to_char());
	DWORD fattr = GetFileAttributes(dirWideChar);
	delete[] dirWideChar;
	if (fattr == INVALID_FILE_ATTRIBUTES)
	{
		// no such path?
		return false;
	}

	return (fattr & FILE_ATTRIBUTE_DIRECTORY);
#else
#ifdef AN_LINUX_OR_ANDROID
	Array<char8> dir8 = _dir.to_char8_array();
	struct stat attrib;
	stat(dir8.get_data(), &attrib);
	return S_ISDIR(attrib.st_mode);
#else
#error implement
#endif
#endif
}

#ifdef AN_LINUX_OR_ANDROID
int mkpath(const char* path, mode_t mode)
{
	char* pp;
	char* sp;
	int result;
	int pathLen = (strlen(path) + 1);
	int pathSize = pathLen * sizeof(char);
	allocate_stack_var(char, copypath, pathLen);
	memcpy(copypath, path, pathSize);

	// create dir after dir
	result = 0;
	pp = copypath;
	while (true)
	{
		char* minSlash = strchr(pp, '/');
		char* minBSlash = strchr(pp, '\\');
		if (minSlash && minBSlash)
		{
			sp = pp + min(minSlash - pp, minBSlash - pp);
		}
		else
		{
			sp = minSlash ? minSlash : minBSlash;
		}
		if (! sp)
		{
			break;
		}
		if (sp != pp)
		{
			if (sp)
			{
				*sp = 0;
			}
			result = mkdir(copypath, mode);
			if (sp)
			{
				*sp = '/';
			}
		}
		pp = sp + 1;
	}
	result = mkdir(path, mode);
	return (result);
}
#endif

void Dir::create(String const & _dir)
{
#ifdef AN_WINDOWS
	wchar_t* dirWideChar = convert_to_wide_char(_dir.to_char());
	CreateDirectory(dirWideChar, nullptr);
	delete[] dirWideChar;
#else
#ifdef AN_LINUX_OR_ANDROID
	Array<char8> filename8 = _dir.to_char8_array();
	mkpath(filename8.get_data(), 0777);
#else
#error implement
#endif
#endif
}

void Dir::clear()
{
	files.clear();
	directories.clear();
	infos.clear();
}

bool Dir::list(String const & _inDir)
{
	clear();
	infos.make_space_for(256);
#ifdef AN_WINDOWS
	String inDir = _inDir;
	if (!inDir.is_empty())
	{
		if (inDir.get_right(1) == TXT("\\") ||
			inDir.get_right(1) == TXT("/"))
		{
			inDir.get_left(inDir.get_length() - 1);
		}
	}
	if (!inDir.is_empty())
	{
		inDir += TXT("\\*"); // look inside dir
	}
	else
	{
		inDir = TXT(".\\*");
	}
	HANDLE hFind = INVALID_HANDLE_VALUE;
#ifdef AN_WIDE_CHAR
	WIN32_FIND_DATAW ffd;
	hFind = FindFirstFileW(inDir.to_char(), &ffd);
#else
	WIN32_FIND_DATAA ffd;
	hFind = FindFirstFileA(inDir.to_char(), &ffd);
#endif

	if (hFind == INVALID_HANDLE_VALUE)
	{
		return false;
	} 
   
	// List all the files in the directory with some info about them.

	do
	{
		DirEntry de;
		de.path = _inDir.is_empty() ? String(ffd.cFileName) : _inDir + TXT("/") + String(ffd.cFileName);
		de.filename = String(ffd.cFileName);
		de.isDirectory = ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY? true : false;
		de.creationTime = FileTime(ffd.ftCreationTime.dwLowDateTime, ffd.ftCreationTime.dwHighDateTime);
		de.lastAccessTime = FileTime(ffd.ftLastAccessTime.dwLowDateTime, ffd.ftLastAccessTime.dwHighDateTime);
		de.lastWriteTime = FileTime(ffd.ftLastWriteTime.dwLowDateTime, ffd.ftLastWriteTime.dwHighDateTime);
		infos.push_back(de);
		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			if (tstrcmp(ffd.cFileName, TXT(".")) != 0 &&
				tstrcmp(ffd.cFileName, TXT("..")) != 0)
			{
				directories.push_back(String(ffd.cFileName));
			}
		}
		else
		{
			files.push_back(String(ffd.cFileName));
		}
	}
#ifdef AN_WIDE_CHAR
	while (FindNextFileW(hFind, &ffd) != 0);
#else
	while (FindNextFileA(hFind, &ffd) != 0);
#endif
#else
#ifdef AN_LINUX_OR_ANDROID
	Array<char8> filename8 = _inDir.to_char8_array();
	DIR* dir = opendir(filename8.get_data());
	if (! dir)
	{
#ifdef AN_DEVELOPMENT_OR_PROFILER
		int err = errno;
		tchar const* eno = TXT("unknown");
		if (err == EACCES) eno = TXT("EACCES");
		if (err == EBADF) eno = TXT("EBADF");
		if (err == EMFILE) eno = TXT("EMFILE");
		if (err == ENFILE) eno = TXT("ENFILE");
		if (err == ENOENT) eno = TXT("ENOENT");
		if (err == ENOMEM) eno = TXT("ENOMEM");
		if (err == ENOTDIR) eno = TXT("ENOTDIR");

		output(TXT("error %i %S"), err, eno);
#endif

		return false;
	}

	while (struct dirent* dirEntry = readdir(dir))
	{
		if (dirEntry->d_type == DT_REG)
		{
			files.push_back(String::from_char8(dirEntry->d_name));
		}
		else if (dirEntry->d_type == DT_DIR)
		{
			if (strcmp(dirEntry->d_name, ".") != 0 &&
				strcmp(dirEntry->d_name, "..") != 0)
			{
				directories.push_back(String::from_char8(dirEntry->d_name));
			}
		}
		else
		{
			char8 buf[512];
			sprintf(buf, "%s%s", filename8.get_data(), dirEntry->d_name);
			struct stat s;
			stat(buf, &s);
			if (S_ISREG(s.st_mode))
			{
				files.push_back(String::from_char8(dirEntry->d_name));
			}
			else if (S_ISDIR(s.st_mode))
			{
				if (strcmp(dirEntry->d_name, ".") != 0 &&
					strcmp(dirEntry->d_name, "..") != 0)
				{
					directories.push_back(String::from_char8(dirEntry->d_name));
				}
			}
			else
			{
				error(TXT("don't know what to do"));
			}
		}
	}
	closedir(dir);
#else
	#error TODO implement listing directories
	return false;
#endif
#endif
	// we want them sorted, as it may be important later on
	sort(files);
	sort(directories);

	return true;
}

void Dir::delete_dir(String const& _dir)
{
	String dirWithSeparator = _dir;
	if (dirWithSeparator.get_right((uint)tstrlen(IO::get_directory_separator())) != IO::get_directory_separator())
	{
		dirWithSeparator += IO::get_directory_separator();
	}

	Dir d;
	d.list(_dir);

	for_every(f, d.get_files())
	{
		IO::File::delete_file(dirWithSeparator + *f);
	}
	for_every(dir, d.get_directories())
	{
		IO::Dir::delete_dir(dirWithSeparator + *dir);
	}

	auto dir8 = _dir.to_char8_array();
	std::remove(dir8.get_data());
}


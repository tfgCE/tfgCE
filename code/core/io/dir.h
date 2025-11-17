#pragma once

#include "..\types\string.h"

namespace IO
{
	struct FileTime
	{
		int lowDateTime;
		int highDateTime;

		FileTime();
		FileTime(int _lowDateTime, int _highDateTime);
		FileTime(long int _dateTime);

		bool operator == (FileTime const & _other) const;
		bool operator != (FileTime const & _other) const;
	};

	struct DirEntry
	{
		String path;
		String filename;
		bool isDirectory;
		FileTime creationTime;
		FileTime lastAccessTime;
		FileTime lastWriteTime;

		DirEntry();

		void setup_for_file(String const & _path);

		bool operator == (DirEntry const & _other) const;
		bool operator != (DirEntry const & _other) const;
	};

	class Dir
	{
	public:
		Dir();

		static bool does_exist(String const& _dir);
		static void create(String const & _dir);
		static void delete_dir(String const & _dir);

		void clear();

		bool list(String const & _inDir = String::empty());

		Array<String> const & get_files() { return files; }
		Array<String> const & get_directories() { return directories; }
		Array<DirEntry> const & get_infos() { return infos; }

	private:
		Array<String> files;
		Array<String> directories;
		Array<DirEntry> infos;
	};
};

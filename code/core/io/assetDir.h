#pragma once

#include "..\types\string.h"

namespace IO
{
	/**
	 *	Available only on android
	 *	Main directory is empty string
	 */
	class AssetDir
	{
	public:
		AssetDir();

		void clear();

		bool list(String const & _inDir);

		Array<String> const & get_files() { return files; }
		Array<String> const & get_directories() { return directories; }

	private:
		Array<String> files;
		Array<String> directories;
	};
};

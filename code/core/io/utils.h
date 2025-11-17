#pragma once

#include "..\types\string.h"

namespace IO
{
	class Utils
	{
		public:
			static String as_valid_directory(String const & _dir);
			static String get_file(String const & _dir);
			static String get_directory(String const & _dir);
			static bool is_file(String const & _path); // if not empty and no directory separator at the end
			static String make_path(String const & _dir, String const & _path);
			static String get_extension(String const & _path);
			static String make_relative(String const & _path); // to current directory
			static String make_absolute(String const & _path); // from current directory
	};
};

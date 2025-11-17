#include "utils.h"

#include "io.h"

//

using namespace IO;

//

String Utils::as_valid_directory(String const & _dir)
{
	String dir = _dir;

	if (dir.get_length() > 0)
	{
		if (dir[dir.get_length() - 1] != '/' &&
			dir[dir.get_length() - 1] != '\\')
		{
			dir += '/';
		}
	}

	return dir;
}

String Utils::get_file(String const & _dir)
{
	String file = _dir;

	int i = _dir.get_length() - 1;
	while (i >= 0)
	{
		if (_dir[i] == '/' ||
			_dir[i] == '\\')
		{
			file = _dir.get_sub(i + 1);
			break;
		}
		--i;
	}

	return file;
}

String Utils::get_directory(String const & _dir)
{
	String dir;

	int i = _dir.get_length() - 1;
    while (i >= 0)
    {
        if (_dir[i] == '/' ||
            _dir[i] == '\\')
        {
            dir = _dir.get_left(i);
			break;
        }
        -- i;
    }

	return as_valid_directory(dir);
}

bool Utils::is_file(String const& _path)
{
	if (!_path.is_empty())
	{
		tchar lastch = _path[_path.get_length() - 1];
		if (lastch != '/' &&
			lastch != '\\')
		{
			return true;
		}
	}
	return false;
}

String Utils::make_path(String const & _dir, String const & _path)
{
	if (_path[0] == '=')
	{
		return _path.get_sub(1);
	}
	return Utils::get_directory(_dir) + _path;
}

String Utils::get_extension(String const & _path)
{
	int lastDot = _path.find_last_of('.');
	if (lastDot >= 0)
	{
		return _path.get_sub(lastDot + 1);
	}
	else
	{
		return String::empty();
	}
}

String Utils::make_relative(String const& _path)
{
	String currentDirectory;
#ifdef AN_WINDOWS
	{
		TCHAR currentDirBuf[16384];
		GetCurrentDirectory(16384, currentDirBuf);

		currentDirectory = String::convert_from(currentDirBuf);
		currentDirectory = as_valid_directory(currentDirectory);
	}
#endif

	String path = IO::unify_filepath(_path);
	currentDirectory = IO::unify_filepath(currentDirectory);

	if (String::compare_icase(path.get_left(currentDirectory.get_length()), currentDirectory))
	{
		return path.get_sub(currentDirectory.get_length());
	}
	return path;
}

String Utils::make_absolute(String const& _path)
{
	String currentDirectory;
#ifdef AN_WINDOWS
	{
		TCHAR currentDirBuf[16384];
		GetCurrentDirectory(16384, currentDirBuf);

		currentDirectory = String::convert_from(currentDirBuf);
		currentDirectory = as_valid_directory(currentDirectory);
	}
#endif

	String path = IO::unify_filepath(_path);
	currentDirectory = IO::unify_filepath(currentDirectory);

	if (! String::compare_icase(path.get_left(currentDirectory.get_length()), currentDirectory))
	{
		return currentDirectory + path;
	}
	return path;
}

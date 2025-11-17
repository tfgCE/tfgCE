#include "assetDir.h"

#include "io.h"

#ifdef AN_ANDROID

#include "assetFile.h"

#include <android/asset_manager.h>
#endif

//

using namespace IO;

//

AssetDir::AssetDir()
{
}

void AssetDir::clear()
{
	files.clear();
	directories.clear();
}

/** should not end in \ or / */
bool AssetDir::list(String const & _inDir)
{
	clear();

	String inDir = _inDir;

#ifdef AN_ANDROID
	AAssetDir* dir = AAssetManager_openDir(IO::get_asset_manager(), inDir.to_char8_array().get_data());

	if (! dir)
	{
		return false;
	}

	while (char const * file = AAssetDir_getNextFileName(dir))
	{
		String fileName = String::from_char8(file);
		String fullPathToFile = (_inDir.is_empty()? _inDir : (_inDir + IO::get_directory_separator())) + fileName;
		if (AssetFile::does_exist(fullPathToFile))
		{
			files.push_back(fileName);
		}
		else
		{
			directories.push_back(fileName);
		}
	}

	AAssetDir_close(dir);

	// we want them sorted, as it may be important later on
	sort(files);
	sort(directories);

	return true;
#else
	return false;
#endif
}

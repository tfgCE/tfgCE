#include "assetFile.h"

#ifdef AN_ANDROID
#include "..\casts.h"

using namespace IO;

AssetFile::AssetFile()
: Interface(&read_data_from_asset, nullptr, &go_to_in_asset, &get_loc_in_asset)
, bufLen(0)
{
}

AssetFile::AssetFile(String const & _fileName)
: Interface(&read_data_from_asset, nullptr, &go_to_in_asset, &get_loc_in_asset)
, bufLen(0)
{
	open(_fileName);
}

AssetFile::~AssetFile()
{
	close();
}

bool AssetFile::open(String const & _fileName)
{
	close();

	asset = AAssetManager_open(IO::get_asset_manager(), IO::sanitise_filepath(_fileName).to_char8_array().get_data(), AASSET_MODE_RANDOM);

	bufLen = 0;
	bufPtr = buf;

	return is_open();
}

uint AssetFile::read_buffer()
{
	bufLen = AAsset_read(asset, buf, bufSize);

	if (bufLen <= 0)
	{
		bufLen = 0;
	}

	bufPtr = buf;

	return bufLen;
}

void AssetFile::close()
{
	if (! is_open())
	{
		return;
	}

	AAsset_close(asset);

	asset = nullptr;
}

uint AssetFile::read_data_from_asset(Interface * io, void * const _data, uint const _numBytes)
{
	AssetFile* file = (AssetFile*)io;

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

uint AssetFile::go_to_in_asset(Interface * io, uint const _newLoc)
{
	AssetFile* file = (AssetFile*)io;

	if (! file->is_open())
	{
		return 0;
	}

	uint loc = AAsset_seek(file->asset, _newLoc, SEEK_SET);

	file->read_buffer();

	return loc;
}

uint AssetFile::get_loc_in_asset(Interface * io)
{
	AssetFile* file = (AssetFile*)io;

	if (!file->is_open())
	{
		return 0;
	}

	uint loc = AAsset_seek(file->asset, 0, SEEK_CUR);

	// where we actually are when reading, move back by current bufLen (this is how much do we have left in our buffer
	loc = (uint)((int)loc - (int)file->bufLen);

	return loc;
}

bool AssetFile::is_open() const
{
	return asset != nullptr;
}

bool AssetFile::does_exist(String const & _fileName)
{
	todo_future(TXT("awkward for checking if file exists"));

	AAsset* asset = AAssetManager_open(IO::get_asset_manager(), IO::sanitise_filepath(_fileName).to_char8_array().get_data(), AASSET_MODE_RANDOM);

	if (asset)
	{
		AAsset_close(asset);
		return true;
	}
	else
	{
		return false;
	}
}

uint AssetFile::get_size_of(String const & _fileName)
{
	AssetFile file;
	if (file.open(_fileName))
	{
		return AAsset_getLength(file.asset);
	}
	else
	{
		return 0;
	}
}


#endif // AN_ANDROID

#include "il_tga.h"

#include "..\..\..\io\assetFile.h"
#include "..\..\..\io\file.h"

using namespace System;

bool ImageLoader::TGA::load(String const & _filename)
{
	IO::Interface* src = nullptr;
#ifdef AN_ANDROID
	IO::AssetFile assetFile;
	if (!src)
	{
		if (assetFile.open(IO::get_asset_data_directory() + _filename))
		{
			assetFile.set_type(IO::InterfaceType::Binary);
			src = &assetFile;
		}
	}
#endif
	IO::File file;
	if (!src)
	{
		if (file.open(IO::get_user_game_data_directory() + _filename))
		{
			file.set_type(IO::InterfaceType::Binary);
			src = &file;
		}
	}

	if (!src)
	{
		return false;
	}

	uint8 header[18] = { 0 };
	static uint8 headerRaw[12] = { 0x0, 0x0, 0x2, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 };
	static uint8 headerCompressed[12] = { 0x0, 0x0, 0xA, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 };

	src->read(header, sizeof(header));

	bool rawTGA = memory_compare(headerRaw, &header, sizeof(headerRaw));
	bool compressedTGA = memory_compare(headerCompressed, &header, sizeof(headerCompressed));
	if (rawTGA || compressedTGA)
	{
		bitsPerPixel = header[16];
		size.x = header[13] * 256 + header[12];
		size.y = header[15] * 256 + header[14];
		int rowSize = ((size.x * bitsPerPixel + 31) / 32) * 4;
		int dataSize = rowSize * size.y;

		if (bitsPerPixel != 24 && bitsPerPixel != 32)
		{
			error(TXT("only 24 and 32 "));
			return false;
		}

		if (rawTGA)
		{
			data.set_size(dataSize);
			src->read(data.get_data(), dataSize);
		}
		else if (compressedTGA)
		{
			struct Pixel
			{
				uint8 r = 0;
				uint8 g = 0;
				uint8 b = 0;
				uint8 a = 0;
			};

			Pixel pixel;
			int bytesPerPixel = bitsPerPixel / 8;
			data.set_size(size.x * size.y * bytesPerPixel);
			uint8* pixPtr = data.begin();
			int currentPixel = 0;
			uint8 chunkHeader = 0;

			while (currentPixel < (size.x * size.y))
			{
				src->read(&chunkHeader, sizeof(chunkHeader));

				if (chunkHeader < 128)
				{
					++chunkHeader;
					for (int i = 0; i < chunkHeader; ++i, ++currentPixel)
					{
						src->read(&pixel, bytesPerPixel);

						*pixPtr = pixel.b; ++pixPtr;
						*pixPtr = pixel.g; ++pixPtr;
						*pixPtr = pixel.r; ++pixPtr;
						if (bitsPerPixel == 32) { *pixPtr = pixel.a; ++pixPtr; }
					}
				}
				else
				{
					// rle
					chunkHeader -= 127;
					src->read(&pixel, bytesPerPixel);

					for (int i = 0; i < chunkHeader; ++i, ++currentPixel)
					{
						*pixPtr = pixel.b; ++pixPtr;
						*pixPtr = pixel.g; ++pixPtr;
						*pixPtr = pixel.r; ++pixPtr;
						if (bitsPerPixel == 32) { *pixPtr = pixel.a; ++pixPtr; }
					}
				}
			}
		}

		// upside down
		uint8* tempRow = (uint8*)allocate_stack(rowSize);
		for_count(int, i, size.y / 2)
		{
			uint8* rowLow = &data[rowSize * i];
			uint8* rowHi = &data[rowSize * (size.y - 1 - i)];
			memory_copy(tempRow, rowLow, rowSize);
			memory_copy(rowLow, rowHi, rowSize);
			memory_copy(rowHi, tempRow, rowSize);
		}
	}
	else
	{
		error(TXT("only 24 and 32 TGA. maybe not a valid tga file?"));
		return false;
	}

	return true;
}

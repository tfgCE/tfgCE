#include "is_tga.h"

#include "..\..\..\io\assetFile.h"
#include "..\..\..\io\file.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace System;

//

bool ImageSaver::TGA::save(String const& _filename, bool _dataIsUpsideDown)
{
	IO::Interface* dest = nullptr;
	IO::File file;
	if (!dest)
	{
		if (file.create(_filename))
		{
			file.set_type(IO::InterfaceType::Binary);
			dest = &file;
		}
		else if (file.create(IO::get_user_game_data_directory() + _filename))
		{
			file.set_type(IO::InterfaceType::Binary);
			dest = &file;
		}
	}

	if (!dest)
	{
		error(TXT("no destination file to save \"%S\""), _filename.to_char());
		return false;
	}

	return save(dest, _dataIsUpsideDown);
}

bool ImageSaver::TGA::save(IO::Interface * dest, bool _dataIsUpsideDown)
{
	if (!dest)
	{
		error(TXT("no destination to save"));
		return false;
	}

	static uint8 headerRaw[12] = { 0x0, 0x0, 0x2, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 };

	uint8 header[18] = { 0 };
	memory_copy(header, headerRaw, sizeof(header));

	header[12] = size.x % 256;
	header[13] = (size.x - (size.x % 256)) / 256;
	header[14] = size.y % 256;
	header[15] = (size.y - (size.y % 256)) / 256;
	header[16] = bitsPerPixel;
	header[17] = bitsPerPixel == 32? 8 : 0;

	dest->write(header, sizeof(header));

	{
		int rowSize = bitsPerPixel != 32? (((size.x * bitsPerPixel + 31) / 32) * 4) : (size.x * 4);
		int dataRowSize = size.x * (bitsPerPixel / 8);

		if (bitsPerPixel != 24 && bitsPerPixel != 32)
		{
			error(TXT("only 24 and 32 "));
			return false;
		}

		// upside down
		Array<uint8> tempRowData;
		tempRowData.set_size(rowSize);
		uint8* tempRow = tempRowData.begin();
		memory_zero(tempRow, rowSize);
		for_count(int, y, size.y)
		{
			memory_copy(tempRow, &data[dataRowSize * (_dataIsUpsideDown? y : size.y - 1 - y)], dataRowSize);
			// BGR!
			if (bitsPerPixel == 24)
			{
				// rgb -> bgr
				uint8* r = tempRow;
				uint8* b = tempRow + 2;
				for_count(int, x, size.x)
				{
					swap(*r, *b);
					r += 3;
					b += 3;
				}
			}
			if (bitsPerPixel == 32)
			{
				// rgba -> bgra
				uint8* r = tempRow;
				uint8* b = tempRow + 2;
				for_count(int, x, size.x)
				{
					swap(*r, *b);
					r += 4;
					b += 4;
				}
			}
			dest->write(tempRow, rowSize);
		}
	}

	char const* footer = "\0\0\0\0\0\0\0\0TRUEVISION-XFILE.\0";
	dest->write(footer, 26);

	return true;
}

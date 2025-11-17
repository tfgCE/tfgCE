#pragma once

#include "..\..\..\containers\array.h"
#include "..\..\..\math\math.h"
#include "..\..\..\types\string.h"

namespace IO
{
	class Interface;
};

namespace System
{
	namespace ImageSaver
	{
		class TGA
		{
		public:
			bool save(String const& _filename, bool _dataIsUpsideDown = false);
			bool save(IO::Interface * _to, bool _dataIsUpsideDown = false);
			Array<uint8> & access_data() { return data; } // store as RGB!
			void set_bytes_per_pixel(int _bytesPerPixel) { bitsPerPixel = _bytesPerPixel * 8; }
			void set_size(VectorInt2 const& _size) { size = _size; data.set_size((bitsPerPixel / 8) * size.x * size.y); }

		private:
			Array<uint8> data;
			VectorInt2 size = VectorInt2::zero;
			int bitsPerPixel = 0;
		};
	};
};

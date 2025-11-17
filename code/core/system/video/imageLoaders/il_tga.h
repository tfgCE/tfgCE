#pragma once

#include "..\..\..\containers\array.h"
#include "..\..\..\math\math.h"
#include "..\..\..\types\string.h"

namespace System
{
	namespace ImageLoader
	{
		class TGA
		{
		public:
			bool load(String const& _filename);
			Array<uint8> const& get_data() { return data; }
			VectorInt2 get_size() const { return size; }
			int get_bytes_per_pixel() const { return bitsPerPixel / 8; }

		private:
			Array<uint8> data;
			VectorInt2 size = VectorInt2::zero;
			int bitsPerPixel = 0;
		};
	};
};

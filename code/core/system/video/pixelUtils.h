#pragma once

#include "..\..\containers\array.h"
#include "..\..\math\math.h"

namespace System
{
	namespace PixelUtils
	{
		byte convert_linear_to_srgb(byte s);
		void convert_linear_to_srgb(VectorInt2 const& _size, Array<byte>& pixels, int _bytesPerPixel);
		byte convert_srgb_to_linear(byte s);
		void convert_srgb_to_linear(VectorInt2 const& _size, Array<byte>& pixels, int _bytesPerPixel);
		void remove_alpha(VectorInt2 const& _size, Array<byte>& pixels, REF_ int & _bytesPerPixel);
		void add_alpha(VectorInt2 const& _size, Array<byte>& pixels, REF_ int & _bytesPerPixel);
	};
};

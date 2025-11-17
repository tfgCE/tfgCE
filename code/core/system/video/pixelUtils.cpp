#include "pixelUtils.h"

//

using namespace System;

//


byte PixelUtils::convert_linear_to_srgb(byte _s)
{
	// based on: https://entropymine.com/imageworsener/srgbformula/

	float s = (float)_s / 255.0f;
	if (s <= 0.0031308f)
	{
		s = s * 12.92f;
	}
	else if (s <= 1.0f)
	{
		s = pow(s, 1.0f / 2.4f);
		s = 1.055f * s - 0.055f;
	}
	else
	{
		s = 1.0f;
	}

	return (byte)clamp(TypeConversions::Normal::f_i_closest(s * 255), 0, 255);
}

void PixelUtils::convert_linear_to_srgb(VectorInt2 const& _size, Array<byte>& pixels, int _bytesPerPixel)
{
	byte* s = pixels.get_data();
	byte* d = pixels.get_data();
	for (int i = 0; i < _size.y; i++)
	{
		for (int j = 0; j < _size.x; j++)
		{
			if (_bytesPerPixel >= 1) { *d = convert_linear_to_srgb(*s); ++s; ++d; }
			if (_bytesPerPixel >= 2) { *d = convert_linear_to_srgb(*s); ++s; ++d; }
			if (_bytesPerPixel >= 3) { *d = convert_linear_to_srgb(*s); ++s; ++d; }
			if (_bytesPerPixel >= 4) { *d = convert_linear_to_srgb(*s); ++s; ++d; }
		}
	}
}

byte PixelUtils::convert_srgb_to_linear(byte _s)
{
	// based on: https://entropymine.com/imageworsener/srgbformula/

	float s = (float)_s / 255.0f;
	if (s <= 0.04045f)
	{
		s = s / 12.92f;
	}
	else if (s <= 1.0f)
	{
		s = (s + 0.055f) / 1.055f;
		s = pow(s, 2.4f);
	}
	else
	{
		s = 1.0f;
	}

	return (byte)clamp(TypeConversions::Normal::f_i_closest(s * 255), 0, 255);
}

void PixelUtils::convert_srgb_to_linear(VectorInt2 const& _size, Array<byte>& pixels, int _bytesPerPixel)
{
	byte* s = pixels.get_data();
	byte* d = pixels.get_data();
	for (int i = 0; i < _size.y; i++)
	{
		for (int j = 0; j < _size.x; j++)
		{
			if (_bytesPerPixel >= 1) { *d = convert_srgb_to_linear(*s); ++s; ++d; }
			if (_bytesPerPixel >= 2) { *d = convert_srgb_to_linear(*s); ++s; ++d; }
			if (_bytesPerPixel >= 3) { *d = convert_srgb_to_linear(*s); ++s; ++d; }
			if (_bytesPerPixel >= 4) { *d = convert_srgb_to_linear(*s); ++s; ++d; }
		}
	}
}

void PixelUtils::remove_alpha(VectorInt2 const& _size, Array<byte>& pixels, REF_ int& _bytesPerPixel)
{
	if (_bytesPerPixel == 4)
	{
		// remove alpha channel
		byte* s = pixels.get_data();
		byte* d = pixels.get_data();
		for (int i = 0; i < _size.y; i++)
		{
			for (int j = 0; j < _size.x; j++)
			{
				*d = *s; ++s; ++d;
				*d = *s; ++s; ++d;
				*d = *s; ++s; ++d;
				++s; // skip alpha
			}
		}
		pixels.set_size((int)(d - pixels.get_data()));
		_bytesPerPixel = 3;
	}
}

void PixelUtils::add_alpha(VectorInt2 const& _size, Array<byte>& pixels, REF_ int& _bytesPerPixel)
{
	if (_bytesPerPixel == 3)
	{
		// fill alpha channel
		for (int i = 0; i < _size.y; i++)
		{
			byte* a = pixels.get_data() + _size.x * _bytesPerPixel * i + 3 /* alpha */;
			for (int j = 0; j < _size.x; j++, a += _bytesPerPixel)
			{
				*a = 255;
			}
		}

		_bytesPerPixel = 4;
	}
}

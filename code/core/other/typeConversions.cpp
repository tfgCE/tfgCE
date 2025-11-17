#include "typeConversions.h"

#include "..\math\math.h"

#define EPS 0.0001f
#define LARGE_EPS 0.01f

uint8 TypeConversions::VideoFormat::f_ui8(float _value)
{
	return ((uint8)clamp<int>((int)round((_value + EPS) * 255.0f), 0, 255));
}

float TypeConversions::VideoFormat::ui8_f(uint8 _value)
{
	return ((float)_value) / 255.0f;
}

uint16 TypeConversions::VideoFormat::f_ui10(float _value)
{
	return ((uint16)clamp<int>((int)round((_value + EPS) * 1023.0f), 0, 1023));
}

float TypeConversions::VideoFormat::ui10_f(uint16 _value)
{
	return ((float)_value) / 1023.0f;
}

uint8 TypeConversions::VideoFormat::f_ui2(float _value)
{
	return ((uint8)clamp<int>((int)round((_value + EPS) * 3.0f), 0, 3));
}

float TypeConversions::VideoFormat::ui2_f(uint8 _value)
{
	return ((float)_value) / 3.0f;
}


int16 TypeConversions::Vertex::f_i10_normal(float _value)
{
	//  0..1 -> 0..511
	// -1..0 -> 512..1023
	if (_value < 0.0f)
	{
		return clamp<int16>((int16)(round((1.0f + _value) * 511.0f)), 0, 511) | 512;
	}
	else
	{
		return clamp<int16>((int16)(round(_value * 511.0f)), 0, 511);
	}
}

float TypeConversions::Vertex::i10_f_normal(int16 _value)
{
	return ((float)(_value & 511) / 511.0f) + ((_value >> 9) ? -1.0f : 0.0f);
}

uint8 TypeConversions::Vertex::f_ui8(float _value)
{
	return ((uint8)clamp<int>((int)round(_value * 255.0f), 0, 255));
}

float TypeConversions::Vertex::ui8_f(uint8 _value)
{
	return ((float)_value) / 255.0f;
}

uint16 TypeConversions::Vertex::f_ui16(float _value)
{
	return ((uint16)clamp<int>((int)round(_value * 65535.0f), 0, 65535));
}

float TypeConversions::Vertex::ui16_f(uint16 _value)
{
	return ((float)_value) / 65535.0f;
}

uint32 TypeConversions::Vertex::f_ui32(float _value)
{
	return ((uint32)clamp<uint>((uint)round(_value * 4294967295.0f), 0, 4294967295));
}

float TypeConversions::Vertex::ui32_f(uint32 _value)
{
	return ((float)_value) / 4294967295.0f;
}

int TypeConversions::Normal::f_i_cells(float _v)
{
	if (_v >= 0.0f)
	{
		return (int)(floor(_v) + LARGE_EPS);
	}
	else
	{
		_v = -_v;
		_v += 1.0f;
		return -(int)(floor(_v) + LARGE_EPS);
	}
}

VectorInt2 TypeConversions::Normal::f_i_cells(Vector2 const& _v)
{
	return VectorInt2(f_i_cells(_v.x), f_i_cells(_v.y));
}

VectorInt3 TypeConversions::Normal::f_i_cells(Vector3 const& _v)
{
	return VectorInt3(f_i_cells(_v.x), f_i_cells(_v.y), f_i_cells(_v.z));
}

int TypeConversions::Normal::f_i_floor(float _v)
{
	if (_v >= 0.0f)
	{
		return (int)(floor(_v) + LARGE_EPS);
	}
	else
	{
		_v = -_v;
		return -(int)(floor(_v) + LARGE_EPS);
	}
}

int TypeConversions::Normal::f_i_ceil(float _v)
{
	if (_v >= 0.0f)
	{
		return (int)(ceil(_v) + LARGE_EPS);
	}
	else
	{
		_v = -_v;
		return -(int)(ceil(_v) + LARGE_EPS);
	}
}

VectorInt2 TypeConversions::Normal::f_i_floor(Vector2 const& _v)
{
	return VectorInt2(f_i_floor(_v.x), f_i_floor(_v.y));
}

VectorInt2 TypeConversions::Normal::f_i_ceil(Vector2 const& _v)
{
	return VectorInt2(f_i_ceil(_v.x), f_i_ceil(_v.y));
}

int TypeConversions::Normal::f_i_closest(float _v)
{
	_v = round(_v);
	if (_v >= 0.0f)
	{
		return (int)(_v + LARGE_EPS);
	}
	else
	{
		return (int)(_v - LARGE_EPS);
	}
}

VectorInt2 TypeConversions::Normal::f_i_closest(Vector2 const& _v)
{
	return VectorInt2(f_i_closest(_v.x), f_i_closest(_v.y));
}

VectorInt3 TypeConversions::Normal::f_i_closest(Vector3 const& _v)
{
	return VectorInt3(f_i_closest(_v.x), f_i_closest(_v.y), f_i_closest(_v.z));
}

#undef EPS
#undef LARGE_EPS

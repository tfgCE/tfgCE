#pragma once

#include "..\globalDefinitions.h"

struct Vector2;
struct Vector3;
struct VectorInt2;
struct VectorInt3;

namespace TypeConversions
{
	namespace VideoFormat
	{
		// ui <-> f   f<0,1>
		uint8 f_ui8(float _value);
		float ui8_f(uint8 _value);
		uint16 f_ui10(float _value);
		float ui10_f(uint16 _value);
		uint8 f_ui2(float _value);
		float ui2_f(uint8 _value);
	};

	namespace Vertex
	{
		// ui <-> f   f<0,1>
		uint8 f_ui8(float _value);
		float ui8_f(uint8 _value);
		uint16 f_ui16(float _value);
		float ui16_f(uint16 _value);
		uint32 f_ui32(float _value);
		float ui32_f(uint32 _value);

		//  i <-> f   f<-1,1>
		int16 f_i10_normal(float _value);
		float i10_f_normal(int16 _value);
	};

	namespace Normal
	{
		int f_i_cells(float _v); // -1 - -0.001 -> -1, 0 - 0.999 -> 0
		int f_i_floor(float _v); // -1 - -0.001 -> 0, 0 - 0.999 -> 0
		int f_i_ceil(float _v); // -1 - -0.001 -> 0, 0 - 0.999 -> 0
		int f_i_closest(float _v); // -0.5 - 0.4999 -> 0, 0.5 - 1.4999 -> 1

		VectorInt2 f_i_cells(Vector2 const & _v);
		VectorInt2 f_i_floor(Vector2 const& _v);
		VectorInt2 f_i_ceil(Vector2 const& _v);
		VectorInt2 f_i_closest(Vector2 const & _v);

		VectorInt3 f_i_cells(Vector3 const & _v);
		VectorInt3 f_i_closest(Vector3 const & _v);
	};
};

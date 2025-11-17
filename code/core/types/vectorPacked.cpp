#include "vectorPacked.h"

#include "colour.h"
#include "..\other\typeConversions.h"

Vector3 VectorPacked::get_as_vertex_normal() const
{
	uint16 x = packed & 0x3FF;
	uint16 y = (packed >> 10) & 0x3FF;
	uint16 z = (packed >> 20) & 0x3FF;
	Vector3 result;
	result.x = TypeConversions::Vertex::i10_f_normal((int)x);
	result.y = TypeConversions::Vertex::i10_f_normal((int)y);
	result.z = TypeConversions::Vertex::i10_f_normal((int)z);
	return result;// .normal();
}

Colour VectorPacked::get_as_video_format_colour() const
{
	// bbbb-bbaa gggg-bbbb rrgg-gggg rrrr-rrrr
	uint16 r = 0;
	uint16 g = 0;
	uint16 b = 0;
	uint8 a = 0;
	int8 const* at = (int8*)&packed;
	a |= (*((uint8*)at) & 0x03);
	b |= ((uint16) * ((uint8*)at)) >> 2;				at += sizeof(uint8);
	b |= (((uint16) * ((uint8*)at)) & 0x0f) << 6;
	g |= ((uint16) * ((uint8*)at)) >> 4;				at += sizeof(uint8);
	g |= (((uint16) * ((uint8*)at)) & 0x3f) << 4;
	r |= ((uint16) * ((uint8*)at)) >> 6;				at += sizeof(uint8);
	r |= ((uint16) * ((uint8*)at)) << 2;
	Colour result;
	result.r = TypeConversions::VideoFormat::ui10_f(r);
	result.g = TypeConversions::VideoFormat::ui10_f(g);
	result.b = TypeConversions::VideoFormat::ui10_f(b);
	result.a = TypeConversions::VideoFormat::ui2_f(a);
	return result;
}

void VectorPacked::set_as_vertex_normal(Vector3 const& _normal)
{
	uint32 x = (uint16)TypeConversions::Vertex::f_i10_normal(_normal.x);
	uint32 y = (uint16)TypeConversions::Vertex::f_i10_normal(_normal.y);
	uint32 z = (uint16)TypeConversions::Vertex::f_i10_normal(_normal.z);
	packed = z << 20 |
			 y << 10 |
		     x;
}

void VectorPacked::set_as_video_format_colour(Colour const& _colour)
{
	// bbbb-bbaa gggg-bbbb rrgg-gggg rrrr-rrrr
	uint16 r = TypeConversions::VideoFormat::f_ui10(_colour.r);
	uint16 g = TypeConversions::VideoFormat::f_ui10(_colour.g);
	uint16 b = TypeConversions::VideoFormat::f_ui10(_colour.b);
	uint8 a = TypeConversions::VideoFormat::f_ui2(_colour.a);
	int8* at = (int8*)&packed;
	*((uint8*)at) = (uint8)((b << 2) | a);					at += sizeof(uint8);
	*((uint8*)at) = (uint8)((g << 4) | (b >> 6));			at += sizeof(uint8);
	*((uint8*)at) = (uint8)((r << 6) | (g >> 4));			at += sizeof(uint8);
	*((uint8*)at) = (uint8)(r >> 2);						at += sizeof(uint8);
}

/*
	test code

				for (float a = -1.0f; a <= 1.01f; a += 0.025f)
				{
					int ai = TypeConversions::Vertex::f_i10_normal(a);
					float b = TypeConversions::Vertex::i10_f_normal(ai);
					output(TXT("%8.3f %5i %08X %8.3f"), a, ai, ai, b);
				}
				for_count(int, i, 50)
				{
					Vector3 a = Vector3(Random::get_float(-1.0f, 1.0f), Random::get_float(-1.0f, 1.0f), Random::get_float(-1.0f, 1.0f)).normal();
					VectorPacked ap;
					ap.set_as_vertex_normal(a);
					Vector3 b = ap.get_as_vertex_normal();
					output(TXT("%S -> %08X -> %S"), a.to_string().to_char(), ap.packed, b.to_string().to_char());
				}
 */
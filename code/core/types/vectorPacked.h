#pragma once

#include "..\globalDefinitions.h"

struct Vector3;
struct Colour;

struct VectorPacked
{
	// packed into int, four bytes, GL_INT_2_10_10_10_REV or GL_UNSIGNED_INT_2_10_10_10_REV
	uint32 packed;

	Vector3 get_as_vertex_normal() const;
	Colour get_as_video_format_colour() const;

	void set_as_vertex_normal(Vector3 const & _normal);
	void set_as_video_format_colour(Colour const & _colour);
};
#pragma once

struct Name;
struct String;
struct Rotator3;
struct Vector2;
struct VectorInt2;

#include "optional.h"

namespace DirFourClockwise
{
	enum Type
	{
		Up,
		Right,
		Down,
		Left,
		NUM
	};

	Type parse(String const & _text);
	String to_string(Type _dir);

	Type parse(Name const & _text);
	Name const & to_name(Type _dir);

	VectorInt2 dir_to_vector_int2(Type _dir);
	Rotator3 dir_to_rotator3_yaw(Type _dir);
	Optional<Type> vector_int2_to_dir(VectorInt2 const & _dir);
	inline Type opposite_dir(Type _dir) { return (Type)((_dir + 2) % NUM); }
	inline bool same_axis_dir(Type _a, Type _b) { return (_a % 2) == (_b % 2); }

	Type next_to(Type _dir);
	Type prev_to(Type _dir);

	Type local_to_world(Type _local, Type _toWorldOf);
	Type world_to_local(Type _world, Type _toLocalOf);

	Vector2 vector_to_world(Vector2 const& _loc, Type _toWorldOf);
};

template <> bool Optional<DirFourClockwise::Type>::load_from_xml(IO::XML::Node const* _node);
template <> bool Optional<DirFourClockwise::Type>::load_from_xml(IO::XML::Attribute const* _attr);


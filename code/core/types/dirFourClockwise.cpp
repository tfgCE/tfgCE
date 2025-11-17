#include "dirFourClockwise.h"

#include "name.h"
#include "string.h"

#include "..\math\math.h"

//

DEFINE_STATIC_NAME(up);
DEFINE_STATIC_NAME(right);
DEFINE_STATIC_NAME(down);
DEFINE_STATIC_NAME(left);

//

template <>
bool Optional<DirFourClockwise::Type>::load_from_xml(IO::XML::Node const* _node)
{
	return load_optional_enum_from_xml<DirFourClockwise::Type>(*this, _node, DirFourClockwise::parse);
}

template <>
bool Optional<DirFourClockwise::Type>::load_from_xml(IO::XML::Attribute const* _attr)
{
	return load_optional_enum_from_xml<DirFourClockwise::Type>(*this, _attr, DirFourClockwise::parse);
}

//

DirFourClockwise::Type DirFourClockwise::parse(String const & _text)
{
	if (_text == TXT("up"))
	{
		return DirFourClockwise::Up;
	}
	if (_text == TXT("right"))
	{
		return DirFourClockwise::Right;
	}
	if (_text == TXT("down"))
	{
		return DirFourClockwise::Down;
	}
	if (_text == TXT("left"))
	{
		return DirFourClockwise::Left;
	}
	error(TXT("not recognised dir-four-clockwise \"%S\""), _text.to_char());
	return DirFourClockwise::Left;
}

String DirFourClockwise::to_string(DirFourClockwise::Type _dir)
{
	if (_dir == Up) return NAME(up).to_string();
	if (_dir == Right) return NAME(right).to_string();
	if (_dir == Down) return NAME(down).to_string();
	if (_dir == Left) return NAME(left).to_string();
	return String::empty();
}

DirFourClockwise::Type DirFourClockwise::parse(Name const& _text)
{
	if (_text == NAME(up))
	{
		return DirFourClockwise::Up;
	}
	if (_text == NAME(right))
	{
		return DirFourClockwise::Right;
	}
	if (_text == NAME(down))
	{
		return DirFourClockwise::Down;
	}
	if (_text == NAME(left))
	{
		return DirFourClockwise::Left;
	}
	error(TXT("not recognised dir-four-clockwise \"%S\""), _text.to_char());
	return DirFourClockwise::Left;
}

Name const& DirFourClockwise::to_name(DirFourClockwise::Type _dir)
{
	if (_dir == Up) return NAME(up);
	if (_dir == Right) return NAME(right);
	if (_dir == Down) return NAME(down);
	if (_dir == Left) return NAME(left);
	return Name::invalid();
}

VectorInt2 DirFourClockwise::dir_to_vector_int2(Type _dir)
{
	if (_dir == Up) return VectorInt2(0, 1);
	if (_dir == Right) return VectorInt2(1, 0);
	if (_dir == Down) return VectorInt2(0, -1);
	if (_dir == Left) return VectorInt2(-1, 0);
	an_assert(false);
	return VectorInt2(0, 1);
}

Rotator3 DirFourClockwise::dir_to_rotator3_yaw(Type _dir)
{
	if (_dir == Up) return Rotator3(0.0f, 0.0f, 0.0f);
	if (_dir == Right) return Rotator3(0.0f, 90.0f, 0.0f);
	if (_dir == Down) return Rotator3(0.0f, 180.0f, 0.0f);
	if (_dir == Left) return Rotator3(0.0f, -90.0f, 0.0f);
	an_assert(false);
	return Rotator3(0.0f, 0.0f, 0.0f);
}

Optional<DirFourClockwise::Type> DirFourClockwise::vector_int2_to_dir(VectorInt2 const& _dir)
{
	if (_dir.x == 0) { return _dir.y > 0 ? DirFourClockwise::Up : DirFourClockwise::Down; }
	if (_dir.y == 0) { return _dir.x > 0 ? DirFourClockwise::Right : DirFourClockwise::Left; }
	return NP;
}

DirFourClockwise::Type DirFourClockwise::local_to_world(Type _local, Type _toWorldOf)
{
	return (Type)(mod<int>(_toWorldOf + _local, NUM));
}

DirFourClockwise::Type DirFourClockwise::world_to_local(Type _world, Type _toLocalOf)
{
	return (Type)(mod<int>(_world - _toLocalOf, NUM));
}

DirFourClockwise::Type DirFourClockwise::next_to(Type _dir)
{
	return (Type)(mod<int>(_dir + 1, NUM));
}

DirFourClockwise::Type DirFourClockwise::prev_to(Type _dir)
{
	return (Type)(mod<int>(_dir - 1, NUM));
}

Vector2 DirFourClockwise::vector_to_world(Vector2 const& _loc, Type _toWorldOf)
{
	if (_toWorldOf == DirFourClockwise::Right)
	{
		return _loc.rotated_right();
	}
	if (_toWorldOf == DirFourClockwise::Down)
	{
		return -_loc;
	}
	if (_toWorldOf == DirFourClockwise::Left)
	{
		return -_loc.rotated_right();
	}
	return _loc;
}

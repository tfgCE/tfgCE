#include "boneID.h"

#include "skeleton.h"

using namespace Meshes;

BoneID BoneID::s_invalid;

BoneID::BoneID()
: skeleton(nullptr)
{
	invalidate();
}

BoneID::BoneID(Name const & _name)
: name(_name)
, skeleton(nullptr)
{
	invalidate();
}

BoneID::BoneID(Name const & _name, Skeleton const * _skeleton)
: name(_name)
{
	invalidate();
	look_up(_skeleton);
}

BoneID::BoneID(Name const & _name, Skeleton const * _skeleton, int _index)
: name(_name)
, skeleton(_skeleton)
, index(_index)
, isValid(true)
{
}

BoneID::BoneID(Skeleton const * _skeleton, int _index)
: skeleton(_skeleton)
, index(_index)
{
	name = _skeleton->get_bones().is_index_valid(_index)? _skeleton->get_bones()[_index].get_name() : Name::invalid();
	isValid = name.is_valid();
}

bool BoneID::load_from_xml(IO::XML::Node const * _node, tchar const * _attrName)
{
	bool result = true;

	if (_node)
	{
		name = _node->get_name_attribute(_attrName, name);
		invalidate();
	}

	return result;
}

void BoneID::look_up(Skeleton const * _skeleton, ShouldAllowToFail _shouldAllowToFail, String const* _overrideName)
{
	skeleton = _skeleton;
	index = NONE;
	if (skeleton)
	{
		if (_overrideName)
		{
			index = skeleton->find_bone_index(*_overrideName);
			if (index != NONE)
			{
				name = skeleton->get_bones()[index].get_name();
			}

			if (index == NONE && _shouldAllowToFail == DisallowToFail)
			{
				error(TXT("couldn't find bone \"%S\""), _overrideName->to_char());
			}
		}
		else
		{
			index = skeleton->find_bone_index(name);

			if (index == NONE && name.is_valid() && _shouldAllowToFail == DisallowToFail)
			{
				error(TXT("couldn't find bone \"%S\""), name.to_char());
			}
		}
	}

	isValid = index != NONE;
}

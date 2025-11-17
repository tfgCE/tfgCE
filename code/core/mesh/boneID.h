#pragma once

#include "..\math\math.h"

#include "boneRenameFunc.h"

namespace Meshes
{
	class Skeleton;

	PLAIN_ struct BoneID
	{
	public:
		BoneID();
		BoneID(Name const & _name);
		BoneID(Name const & _name, Skeleton const * _skeleton);
		BoneID(Name const & _name, Skeleton const * _skeleton, int _index);
		BoneID(Skeleton const * _skeleton, int _index);

		static BoneID const & invalid() { return s_invalid; }

		bool load_from_xml(IO::XML::Node const * _node, tchar const * _attrName = TXT("bone"));

		void set_name(Name const & _name) { if (name != _name) { name = _name; invalidate(); } }
		void reskin(Meshes::BoneRenameFunc _reskin) { set_name(_reskin(name, NP)); }

		void look_up(Skeleton const * _skeleton, ShouldAllowToFail _allowToFail = DisallowToFail, String const * _overrideName = nullptr); // _overrideName to avoid creation of heavy names
		void invalidate() { index = NONE; isValid = false; }

		Name const & get_name() const { return name; }
		int get_index() const { an_assert(isValid); return index; }
		bool is_valid() const { return isValid; } // index valid!
		bool is_name_valid() const { return name.is_valid(); }

		Skeleton const* get_skeleton() const { return skeleton; }

	private:
		static BoneID s_invalid;

		Name name;
		Skeleton const * skeleton;
		int index;
		bool isValid;
	};
};

DECLARE_REGISTERED_TYPE(Meshes::BoneID);

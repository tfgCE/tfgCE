#pragma once

#include "..\math\math.h"

namespace Meshes
{
	class Skeleton;

	struct BoneDefinition
	{
	public:
		BoneDefinition();

		bool load_from_xml(IO::XML::Node const * _node);
		static Name load_name_from_xml(IO::XML::Node const * _node);
		static Name load_parent_name_from_xml(IO::XML::Node const * _node);

		void set_name(Name const & _name) { name = _name; }
		void set_parent_name(Name const & _name) { parentName = _name; parentIndex = NONE; }
		void set_placement_MS(Transform const & _placementMS) { placementMS = _placementMS; }

		Name const & get_name() const { return name; }
		Name const & get_parent_name() const { return parentName; }
		int32 get_parent_index() const { return parentIndex; }
		Transform const & get_placement_MS() const { return placementMS; }

		bool serialise(Serialiser & _serialiser);

	private:
		Name name;
		Name parentName;
		CACHED_ int32 parentIndex;
		Transform placementMS;
		CACHED_ Transform placementInvMS;
		CACHED_ Transform placementLS;
		CACHED_ Matrix44 placementMatMS;
		CACHED_ Matrix44 placementMatLS;
		CACHED_ Matrix44 placementInvMatMS;

		friend class Skeleton;
	};
};

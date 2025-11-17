#pragma once

#include "..\..\core\math\math.h"
#include "..\..\core\memory\refCountObject.h"
#include "..\..\core\other\simpleVariableStorage.h"
#include "..\..\core\tags\tag.h"

namespace Framework
{
	struct MeshNode;
	struct LibraryLoadingContext;
	typedef RefCountObjectPtr<MeshNode> MeshNodePtr;

	/**
	 *	And now: what is mesh node
	 *	Nodes can be used to generate stuff further.
	 *
	 *	For example, some mesh generation objects may add few "joint" nodes
	 *	and mesh generation element "with mesh nodes" may find such nodes (that are close to each other and angle difference is top 100')
	 *	and use them as params to generate further objects.
	 *	Example for this example could be two panels connected with small edges.
	 */
	struct MeshNode
	: public RefCountObject
	{
		Name name;
		Tags tags;
		Transform placement = Transform::identity;
		SimpleVariableStorage variables;
		//bool input = false; // was provided as an input

		void apply(Matrix44 const & _mat);
		void apply(Transform const & _transform);

		MeshNode* create_copy() const;

		bool is_dropped() const { return dropped; }
		void drop() { dropped = true; }

		bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);

		void debug_draw(Transform const& _upperPlacement);

		static void remove_dropped_from(REF_ Array<MeshNodePtr>& _in);
		static void add_not_dropped_to(Array<MeshNodePtr> const & _from, OUT_ Array<MeshNodePtr>& _to); // will add existing ones
		static void add_not_dropped_except_to(Array<MeshNodePtr> const& _from, Array<MeshNodePtr> const& _exceptNodes, OUT_ Array<MeshNodePtr>& _to); // will add existing ones
		static void copy_not_dropped_to(Array<MeshNodePtr> const& _from, OUT_ Array<MeshNodePtr>& _to, Optional<Transform> const& _at = NP); // will create copies
		static void log(LogInfoContext & _log, Array<MeshNodePtr> const& _nodes);

	private:
		bool dropped = false; // was dropped during mesh generation
	};
};

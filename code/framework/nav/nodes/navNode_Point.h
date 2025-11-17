#pragma once

#include "..\navNode.h"

#include "..\..\..\core\memory\softPooledObject.h"

namespace Framework
{
	namespace Nav
	{
		namespace Nodes
		{
			class Point
			: public Nav::Node
			, public SoftPooledObject<Point>
			{
				FAST_CAST_DECLARE(Point);
				FAST_CAST_BASE(Node);
				FAST_CAST_END();

				typedef Nav::Node base;
			public: // Nav::Node
				override_ void connect_as_open_node();
				override_ void unify_as_open_node();

				override_ bool find_location(Vector3 const & _at, OUT_ Vector3 & _found, REF_ float & _dist) const;

			protected: // RefCountObject
				override_ void destroy_ref_count_object() { release(); }

			protected: // SoftPooledObject
				override_ void on_release() { reset_to_new(); }
			};
		};
	};
};

TYPE_AS_CHAR(Framework::Nav::Nodes::Point);
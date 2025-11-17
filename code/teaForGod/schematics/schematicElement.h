#pragma once

#include "..\..\core\fastCast.h"
#include "..\..\core\memory\refCountObject.h"

struct Range2;

namespace IO
{
	namespace XML
	{
		class Node;
	};
};

namespace Framework
{
	struct LibraryLoadingContext;
};

namespace Meshes
{
	namespace Builders
	{
		class IPU;
	};
};

namespace TeaForGodEmperor
{
	struct SchematicElement
	: public RefCountObject
	{
		FAST_CAST_DECLARE(SchematicElement);
		FAST_CAST_END();

	public:
		SchematicElement();
		virtual ~SchematicElement();

		void set_sort_priority(int _sortPriority) { sortPriority = _sortPriority; }
		int get_sort_priority() const { return sortPriority; }

		void full_schematic_only(bool _fullSchematicOnly = true) { fullSchematicOnly = _fullSchematicOnly; }
		bool is_full_schematic_only() const { return fullSchematicOnly; }

		virtual void build(Meshes::Builders::IPU& _ipu, bool _outline) const = 0;

		static int compare_ref(void const* _a, void const* _b)
		{
			RefCountObjectPtr<SchematicElement> const& a = *(plain_cast<RefCountObjectPtr<SchematicElement>>(_a));
			RefCountObjectPtr<SchematicElement> const& b = *(plain_cast<RefCountObjectPtr<SchematicElement>>(_b));
			int aV = a->get_sort_priority();
			int bV = b->get_sort_priority();
			return aV > bV ? B_BEFORE_A :
				   aV < bV ? A_BEFORE_B :
							 A_AS_B;
		};

		virtual bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);

		virtual void grow_size(REF_ Range2& _size) const = 0;

	private:
		int sortPriority = 0;
		bool fullSchematicOnly = false; // if true, does not appear in outline

	};
};


#pragma once

#include "meshGenValueDef.h"

#include "..\library\libraryName.h"

namespace Framework
{
	class PhysicalMaterial;
	class PhysicalMaterialMap;

	namespace MeshGeneration
	{
		struct GenerationContext;
		struct ElementInstance;

		struct CreateSpaceBlockerInfo
		{
			bool create = false;
			ValueDef<bool> createCondition;
			ValueDef<bool> blocks;
			ValueDef<bool> requiresToBeEmpty;

			bool load_from_xml(IO::XML::Node const * _node, tchar const * _childAttrName = TXT("createSpaceBlocker"));

			bool get_blocks(GenerationContext const & _context) const;
			bool get_requires_to_be_empty(GenerationContext const & _context) const;
		};
	};
};

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

		struct SimplifyToBoxInfo
		{
			RangeInt forLOD = RangeInt::empty;
			ValueDef<float> u;
			ValueDef<Name> skinToBone;
			ValueDef<Vector3> scale;

			// will find "simplifyToBox"
			bool load_from_child_node_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc, tchar const* _childNodeName = TXT("simplifyToBox"));
			bool load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc);

			PhysicalMaterial * get_provided_physical_material(GenerationContext const & _context) const;

			bool process(GenerationContext& _context, ElementInstance& _instance, int _meshPartsSoFar, bool _requiresDefinedForLOD) const; // _requiresDefinedForLOD if false, accepts empty forLOD (useful for modifier, for set of elements use true)
		};
	};
};

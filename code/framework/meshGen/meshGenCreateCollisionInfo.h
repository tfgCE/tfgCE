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

		struct CreateCollisionInfo
		{
			bool create = false;
			bool makeSizeValid = false;
			ValueDef<bool> createCondition;
			ValueDef<LibraryName> physicalMaterial;
			ValueDef<LibraryName> physicalMaterialMap;
			ValueDef<float> subStep; // if makes sense (edges->boxes makes sense)
			ValueDef<Vector3> sizeCoef; // if makes sense and is implemented

			bool load_from_xml(IO::XML::Node const * _node, tchar const * _childAttrName, LibraryLoadingContext & _lc);

			PhysicalMaterial * get_provided_physical_material(GenerationContext const & _context) const;
			PhysicalMaterial * get_physical_material(GenerationContext const & _context, ElementInstance const & _instance, Optional<float> const & _uOverride = NP) const;
			PhysicalMaterial * get_physical_material(GenerationContext const & _context, int _materialIndex, Optional<float> const & _uOverride = NP) const;
			PhysicalMaterialMap * get_physical_material_map(GenerationContext const & _context, ElementInstance const & _instance) const;
			PhysicalMaterialMap * get_physical_material_map(GenerationContext const & _context, int _materialIndex) const;
			Vector3 get_size_coef(GenerationContext const& _context) const;
		};
	};
};

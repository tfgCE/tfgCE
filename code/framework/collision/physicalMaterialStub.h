#pragma once

#include "..\..\core\io\xml.h"
#include "..\..\core\collision\physicalMaterial.h"

#include "..\library\libraryStored.h"

namespace Framework
{
	/**
	 *	Stub just points at the actual physical material
	 */
	class PhysicalMaterialStub
	: public Collision::PhysicalMaterial
	{
		FAST_CAST_DECLARE(PhysicalMaterialStub);
		FAST_CAST_BASE(Collision::PhysicalMaterial);
		FAST_CAST_END();
	public:
		PhysicalMaterialStub();

		LibraryName const & get_library_name() { return libraryName; }

	public: // Collision::PhysicalMaterial
		override_ bool load_from_xml(IO::XML::Node const * _node, Collision::LoadingContext const & _loadingContext);
		override_ bool load_from_xml(IO::XML::Attribute const * _attr, Collision::LoadingContext const & _loadingContext);

	private:
		LibraryName libraryName;
	};
};

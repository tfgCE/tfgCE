#pragma once

#include "..\..\core\io\xml.h"

#include "..\library\libraryStored.h"

namespace Framework
{
	class PhysicalMaterial;

	/**
	 *	PhysicalMaterialMap maps "u" to physical material to allow automatic assignment of physical materials when no physical material is provided for collision.
	 */
	class PhysicalMaterialMap
	: public LibraryStored
	{
		FAST_CAST_DECLARE(PhysicalMaterialMap);
		FAST_CAST_BASE(LibraryStored);
		FAST_CAST_END();

		LIBRARY_STORED_DECLARE_TYPE();

		typedef LibraryStored base;
	public:
		PhysicalMaterialMap(Library * _library, LibraryName const & _name);

		PhysicalMaterial* get_for(float _u) const;

	public: // LibraryStored
		override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
		override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

	private:
		struct Entry
		{
			float u;
			UsedLibraryStored<PhysicalMaterial> physicalMaterial;
		};
		Array<Entry> map;
		float lowestDiffHalf = 0.00001f;

		UsedLibraryStored<PhysicalMaterial> defaultPhysicalMaterial;

		inline bool is_entry_for(Entry const * _entry, float _u) const { return abs(_u - _entry->u) <= lowestDiffHalf; }
	};
};

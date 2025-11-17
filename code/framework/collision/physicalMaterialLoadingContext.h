#pragma once

#include "..\..\core\io\xml.h"
#include "..\..\core\collision\physicalMaterial.h"
#include "..\..\core\collision\loadingContext.h"

#include "..\library\libraryStored.h"

namespace Framework
{
	struct PhysicalMaterialLoadingContext
	: public Collision::LoadingContext
	{
		FAST_CAST_DECLARE(PhysicalMaterialLoadingContext);
		FAST_CAST_BASE(Collision::LoadingContext);
		FAST_CAST_END();

	public:
		PhysicalMaterialLoadingContext();

		LibraryLoadingContext & get_library_loading_context() const { return *libraryLoadingContext; }
		void set_library_loading_context(LibraryLoadingContext * _lc) { libraryLoadingContext = _lc; }

	private:
		LibraryLoadingContext * libraryLoadingContext;
	};
};

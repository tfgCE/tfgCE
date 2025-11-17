#pragma once

#include "..\library\libraryName.h"

namespace Framework
{
	interface_class ILibrarySolver
	{
	public:
		virtual LibraryName const & solve_reference(Name const & _reference, Name const & _ofType = Name::invalid());
	};
};

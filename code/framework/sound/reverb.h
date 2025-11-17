#pragma once

#include "..\..\core\io\xml.h"
#include "..\..\core\sound\reverb.h"

#include "..\library\libraryStored.h"

namespace Random
{
	class Generator;
};

namespace Framework
{
	/**
	 *	Reverb as wrap for a core reverb.
	 */
	class Reverb
	: public LibraryStored
	{
		FAST_CAST_DECLARE(Reverb);
		FAST_CAST_BASE(LibraryStored);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef LibraryStored base;
	public:
		Reverb(Library * _library, LibraryName const & _name);

		::Sound::Reverb const * get_reverb() const { return &reverb; }

	public: // LibraryStored
		override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);

	protected:
		::Sound::Reverb reverb;
	};
};

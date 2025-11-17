#pragma once

#include "..\pilgrim\pilgrimSetup.h"

#include "..\..\framework\library\libraryStored.h"

namespace TeaForGodEmperor
{
	class DefaultPilgrimSetupPreset
	: public Framework::LibraryStored
	{
		FAST_CAST_DECLARE(DefaultPilgrimSetupPreset);
		FAST_CAST_BASE(Framework::LibraryStored);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef Framework::LibraryStored base;
	public:
		DefaultPilgrimSetupPreset(Framework::Library* _library, Framework::LibraryName const& _name);
		virtual ~DefaultPilgrimSetupPreset();

	public: // LibraryStored
		override_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);

	public:
		PilgrimSetupPreset const& get_pilgrim_setup_preset() const { return pilgrimSetupPreset; }

	private:
		PilgrimSetupPreset pilgrimSetupPreset;
	};
};

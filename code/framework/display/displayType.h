#pragma once

#include "displaySetup.h"
#include "displayHardwareMeshGenerator.h"

#include "..\library\libraryStored.h"
#include "..\library\usedLibraryStored.h"
#include "..\text\localisedString.h"

namespace Framework
{
	class Sample;

	class DisplayType
	: public LibraryStored
	{
		FAST_CAST_DECLARE(DisplayType);
		FAST_CAST_BASE(LibraryStored);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef LibraryStored base;
	public:
		DisplayType(Library * _library, LibraryName const & _name);
		virtual ~DisplayType();

		DisplaySetup const & get_setup() const { return setup; }
		DisplayHardwareMeshSetup const & get_hardware_mesh_setup() const { return hardwareMeshSetup; }

		Sample* get_background_sound_sample() const { return backgroundSoundSample.get(); }
		Map<Name, UsedLibraryStored<Sample>> const & get_sound_samples() const { return soundSamples; }

	public: // LibraryStored
		override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
		override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

		override_ void debug_draw() const;
		override_ void log(LogInfoContext & _context) const;

	private:
		LocalisedString shortDescription;
		DisplaySetup setup;
		DisplayHardwareMeshSetup hardwareMeshSetup;
		UsedLibraryStored<Sample> backgroundSoundSample;
		Map<Name, UsedLibraryStored<Sample>> soundSamples;
	};
};

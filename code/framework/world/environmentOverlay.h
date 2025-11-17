#pragma once

#include "environmentInfo.h"

#include "..\library\libraryStored.h"

namespace Framework
{
	namespace Render
	{
		class EnvironmentProxy;
	};

	/**
	 *	used only to change render values (to adjust colour, fog, etc)
	 */
	class EnvironmentOverlay
	: public LibraryStored
	{
		FAST_CAST_DECLARE(EnvironmentOverlay);
		FAST_CAST_BASE(LibraryStored);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef LibraryStored base;
	public:
		EnvironmentOverlay(Library * _library, LibraryName const & _name);

		EnvironmentParams const & get_params() const { return params; }

		float calculate_power(Room const* _room) const;

	public:	// LibraryStored
		override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
		override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

#ifdef AN_DEVELOPMENT
		override_ void ready_for_reload();
#endif

	private:
		bool additive = false;
		Name blendUsingRoomVar; // if not present, at full

		EnvironmentParams params;

	private: friend class Render::EnvironmentProxy;
		void apply_to(Render::EnvironmentProxy& _ep, Room const* _room) const;

	};
};

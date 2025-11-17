#pragma once

#include "roomType.h"
#include "environmentInfo.h"

namespace Framework
{
	/**
	 *	?
	 */
	class EnvironmentType
	: public RoomType
	{
		FAST_CAST_DECLARE(EnvironmentType);
		FAST_CAST_BASE(RoomType);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef RoomType base;
	public:
		EnvironmentType(Library * _library, LibraryName const & _name);

		EnvironmentInfo const & get_info() const { return info; }

		float get_time_before_usage_reset() const { return timeBeforeUsageReset; }

	public:	// LibraryStored
		override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
		override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

#ifdef AN_DEVELOPMENT
		override_ void ready_for_reload();
#endif

	private:
		EnvironmentInfo info;

		float timeBeforeUsageReset; // if negative, never (default)
	};
};

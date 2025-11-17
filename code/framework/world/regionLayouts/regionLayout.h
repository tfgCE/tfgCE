#pragma once

#include "..\..\..\core\functionParamsStruct.h"
#include "..\..\..\core\random\randomNumber.h"

#include "..\roomGeneratorInfo.h"

#include "..\..\library\libraryStored.h"
#include "..\..\library\usedLibraryStoredOrTagged.h"

namespace Framework
{
	class DoorType;
	class Region;
	class RoomLayout;

	class RegionLayout
	: public LibraryStored
	{
		FAST_CAST_DECLARE(RegionLayout);
		FAST_CAST_BASE(LibraryStored);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef LibraryStored base;
	public:
		RegionLayout(Library * _library, LibraryName const & _name);

		static void async_reconnect_doors_connect_all_rooms(Region* _inRegion, Optional<Random::Generator> const& _rg = NP);
		static void async_reconnect_doors(Region* _inRegion, Optional<int> const& _amount = NP, Optional<Random::Generator> const& _rg = NP);

	public:
		struct CreateRegionParams
		{
			ADD_FUNCTION_PARAM(CreateRegionParams, Random::Generator, randomGenerator, use_random_generator);
			ADD_FUNCTION_PARAM_PLAIN_INITIAL(CreateRegionParams, Region*, region, in_region, nullptr);
			ADD_FUNCTION_PARAM_PLAIN(CreateRegionParams, Tags, roomContentTagsRequired, with_room_content_required_tags);
		};
		Region* async_create_and_generate_region(CreateRegionParams const & _params);

	public: // LibraryStored
		override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
		override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

	protected:
		virtual ~RegionLayout();

	private:
		struct IncludeRoomLayout
		{
			UsedLibraryStoredOrTagged<RoomLayout> roomLayout;
			Random::Number<int> count = Random::Number<int>(1);

			bool load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc);
			bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);
		};

		Tags roomContentTagsRequired; // check RoomLayout

		List<IncludeRoomLayout> includeRoomLayouts;

		List<UsedLibraryStoredOrTagged<DoorType>> doorTypes;

	private:
		bool generate(Region* _region) const;
	};
};

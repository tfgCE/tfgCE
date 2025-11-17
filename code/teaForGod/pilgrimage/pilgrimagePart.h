#pragma once

#include "..\..\framework\library\libraryStored.h"
#include "..\..\framework\library\usedLibraryStored.h"

namespace Framework
{
	class DoorType;
	class EnvironmentType;
	class RegionType;
	class RoomType;
};

namespace TeaForGodEmperor
{
	class Pilgrimage;
	class PilgrimageEnvironmentMix;

	class PilgrimagePart
	: public RefCountObject
	{
	public:
		Name const & get_name() const { return name; }
		Name const & get_next_part_name() const { return nextPart; }

		Framework::RoomType* get_station_room_type() const { return stationRoomType.get(); }
		Framework::DoorType* get_station_door_type() const { return stationDoorType.get(); }
		Framework::RoomType* get_station_entrance_room_type() const { return stationEntranceRoomType.get(); }
		Framework::RoomType* get_station_exit_room_type() const { return stationExitRoomType.get(); }

		Framework::RegionType* get_container_region_type() const { return containerRegionType.get(); }
		Framework::RegionType* get_inner_region_type() const { return innerRegionType.get(); }

		Array<RefCountObjectPtr<PilgrimageEnvironmentMix>> const & get_environments() const { return environments; }

	public:
		bool load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);
		bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);

	private:
		Name name;
		Name nextPart;

		Framework::UsedLibraryStored<Framework::RoomType> stationRoomType; // this is station starting this part!
		Framework::UsedLibraryStored<Framework::DoorType> stationDoorType;
		Framework::UsedLibraryStored<Framework::RoomType> stationEntranceRoomType; // entrance to station
		Framework::UsedLibraryStored<Framework::RoomType> stationExitRoomType; // exit from station

		Framework::UsedLibraryStored<Framework::RegionType> containerRegionType; // not generated, only to get parameters, environments etc. it is just a container to hold station entrance/exit rooms
		Framework::UsedLibraryStored<Framework::RegionType> innerRegionType; // generated, holds actual rooms (may be empty for non looped, for the last one

		Array<RefCountObjectPtr<PilgrimageEnvironmentMix>> environments; // at station, knowing next station/part will know how to blend - this is to provide change of mood between stations basing on distance to end
																		 // ATM used only if not using environment cycles
																		 // todo: add own environment cycles!

		friend class Pilgrimage;
	};

};

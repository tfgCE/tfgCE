#include "regionLayout.h"

#include "..\roomLayouts\roomLayout.h"
#include "..\roomLayouts\roomLayoutNames.h"

#include "..\..\game\game.h"
#include "..\..\debug\previewGame.h"
#include "..\..\library\library.h"
#include "..\..\library\usedLibraryStored.inl"
#include "..\..\library\usedLibraryStoredOrTagged.inl"

#include "..\..\..\core\random\randomUtils.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

//

// global references
DEFINE_STATIC_NAME_STR(grRegionLayoutRegionType, TXT("region layout region type"));

//

REGISTER_FOR_FAST_CAST(RegionLayout);
LIBRARY_STORED_DEFINE_TYPE(RegionLayout, regionLayout);

RegionLayout::RegionLayout(Library * _library, LibraryName const & _name)
: base(_library, _name)
{
}

RegionLayout::~RegionLayout()
{
}

bool RegionLayout::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	// clear/reset first
	//
	includeRoomLayouts.clear();
	doorTypes.clear();

	// load
	//
	for_every(node, _node->children_named(TXT("include")))
	{
		includeRoomLayouts.push_back();
		if (includeRoomLayouts.get_last().load_from_xml(node, _lc))
		{
			// ok
		}
		else
		{
			includeRoomLayouts.pop_back();
			result = false;
		}
	}

	for_every(node, _node->children_named(TXT("doorType")))
	{
		doorTypes.push_back();
		if (doorTypes.get_last().load_from_xml(node, _lc))
		{
			// ok
		}
		else
		{
			doorTypes.pop_back();
			result = false;
		}
	}

	return result;
}

bool RegionLayout::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	for_every(irl, includeRoomLayouts)
	{
		result &= irl->prepare_for_game(_library, _pfgContext);
	}

	for_every(doorType, doorTypes)
	{
		result &= doorType->prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);
	}

	return result;
}

Region* RegionLayout::async_create_and_generate_region(CreateRegionParams const& _params)
{
	ASSERT_ASYNC_OR_LOADING_OR_PREVIEW;

	an_assert(_params.region);

	Random::Generator rg;
	if (_params.randomGenerator.is_set())
	{
		rg = _params.randomGenerator.get();
	}

	Region* region = nullptr;

	if (RegionType* regionType = ::Library::get_current()->get_global_references().get<RegionType>(NAME(grRegionLayoutRegionType)))
	{
		Game::get()->perform_sync_world_job(TXT("create region"), [&region, regionType, &rg, &_params]()
			{
				region = new Region(_params.region->get_in_sub_world(), _params.region, regionType, rg);
				region->set_world_address_id(0);
			});

		{
			generate(region);
		}
	}

	return region;
}

bool RegionLayout::generate(Region* _region) const
{
	SimpleVariableStorage variables;
	_region->collect_variables(REF_ variables);

	RoomLayoutContext context;
	{
		Tags useRoomContentTagsRequired;
		useRoomContentTagsRequired.set_tags_from(roomContentTagsRequired);
		context.roomContentTagged.setup_with_tags(useRoomContentTagsRequired);
	}

	auto rg = _region->get_individual_random_generator();
	rg.advance_seed(2083, 607823);

	_region->update_default_environment();

	struct CreateRoom
	{
		Room* room;
		RoomLayout* roomLayout;
		int requiredDoors = 0;
		int optionalDoors = 0;
		int optionalDoorsLeft = 0;
	};
	Array<CreateRoom> createRooms;

	// to keep track of how many doors do we actually have
	int requiredDoors = 0;
	int optionalDoors = 0;
	int optionalDoorsLeft = 0;

	// fill list, we need to have a full list of rooms we want to have here
	{
		int tryIdx = 0;
		while (true)
		{
			++tryIdx;
			if (tryIdx > 1000)
			{
				error(TXT("can't generate"));
#ifndef AN_DEVELOPMENT
				break;
#endif
			}

			bool isOk = true;

			requiredDoors = 0;
			optionalDoors = 0;
			optionalDoorsLeft = 0;

			createRooms.clear();
			{
				for_every(irl, includeRoomLayouts)
				{
					int amount = max(0, irl->count.get(rg));
					for_count(int, i, amount)
					{
						if (RoomLayout* rl = UsedLibraryStoredOrTagged<RoomLayout>::choose_random(irl->roomLayout, REF_ rg, context.roomContentTagged, true))
						{
							createRooms.push_back();
							auto& cr = createRooms.get_last();
							cr.roomLayout = rl;
							cr.requiredDoors = rl->get_door_count().get(rg);
							int maxTotal = rl->get_max_total_door_count();
							if (maxTotal > 0)
							{
								cr.requiredDoors = min(cr.requiredDoors, maxTotal);
							}
							maxTotal = max(maxTotal, cr.requiredDoors);
							cr.optionalDoors = 0;
							cr.optionalDoorsLeft = maxTotal - cr.optionalDoors - cr.requiredDoors;

							requiredDoors += cr.requiredDoors;
							optionalDoors += cr.optionalDoors;
							optionalDoorsLeft += cr.optionalDoorsLeft;
						}
					}
				}
			}

			// check parity of doors
			{
				int totalDoors = requiredDoors + optionalDoors;
				if ((totalDoors % 2) == 1)
				{
					if (optionalDoorsLeft > 0)
					{
						// odd number, add one optional door
						for_every(cr, createRooms)
						{
							if (cr->optionalDoorsLeft > 0)
							{
								++cr->optionalDoors;
								--cr->optionalDoorsLeft;
								++optionalDoors;
								--optionalDoorsLeft;
								break;
							}
						}
					}
					else if (optionalDoors > 0)
					{
						// odd number, remove one optional door
						for_every(cr, createRooms)
						{
							if (cr->optionalDoors > 0)
							{
								--cr->optionalDoors;
								++cr->optionalDoorsLeft;
								--optionalDoors;
								++optionalDoorsLeft;
								break;
							}
						}
					}
					else
					{
						isOk = false;
					}
				}
			}

			if (isOk)
			{
				break;
			}
		}
	}

	Array<DoorInRoom*> doorInRooms;
	doorInRooms.make_space_for(requiredDoors + optionalDoors);

	// create rooms and doors inside (and store them in a common array here)
	{
		for_every(cr, createRooms)
		{
			cr->room = cr->roomLayout->async_create_room(RoomLayout::CreateRoomParams().in_region(_region).use_random_generator(rg.spawn()));

			Game::get()->perform_sync_world_job(TXT("create door in room"), [&cr, &doorInRooms]()
				{
					for_count(int, i, cr->requiredDoors + cr->optionalDoors)
					{
						auto* dir = new DoorInRoom(cr->room);
						dir->access_tags().set_tag(RoomLayoutNames::DoorInRoomTags::reconnectable());
						doorInRooms.push_back(dir);
					}
				});
		}
	}

	an_assert((doorInRooms.get_size() % 2) == 0);

	// connect door-in-rooms at random and create actual doors on the way
	{
		SubWorld* subWorld = _region->get_in_sub_world();
		auto* useDoorType = UsedLibraryStoredOrTagged<DoorType>::choose_random(doorTypes, REF_ rg, context.roomContentTagged, true);
		if (!useDoorType)
		{
			error(TXT("no door type"));
			return false;
		}

		Door* door = nullptr;
		for_every_ptr(doorInRoom, doorInRooms)
		{
			if (!door)
			{
				Game::get()->perform_sync_world_job(TXT("doors between rooms"), [subWorld, useDoorType, &door]()
					{
						door = new Door(subWorld, useDoorType);
					});
				door->link(doorInRoom);
			}
			else
			{
				door->link(doorInRoom);
				door = nullptr;
			}
		}

		an_assert(door == nullptr, TXT("should be cleared after last pair is created"));
	}

	// init meshes for all door in rooms now (as they are linked to doors)
	{
		for_every_ptr(doorInRoom, doorInRooms)
		{
			doorInRoom->init_meshes();
		}
	}

	// randomly first
	RegionLayout::async_reconnect_doors(_region, NP, rg);
	RegionLayout::async_reconnect_doors_connect_all_rooms(_region, rg);

	// generate rooms
	{
		// do it in a loop as we may have separate rooms
		// we do not guarantee here that everything is connected
		while (true)
		{
			bool allReady = true;
			for_every(cr, createRooms)
			{
				if (!cr->room->is_generated())
				{
					cr->room->ready_for_game();
					allReady = false;
					break;
				}
			}
			if (allReady)
			{
				break;
			}
		}
	}

	return true;
}

void RegionLayout::async_reconnect_doors_connect_all_rooms(Region* _inRegion, Optional<Random::Generator> const& _rg)
{
	ASSERT_ASYNC_OR_LOADING_OR_PREVIEW;

	struct RoomInfo
	{
		Room* room;
		int doorAmount;
		ArrayStatic<int, 16> connectedToRoomIdx;
		ArrayStatic<DoorInRoom*, 16> doors;
		bool connected;
		RoomInfo() {}

		static bool connect(Array<RoomInfo>& _rooms, int _a, int _b)
		{
			if (_a == _b)
			{
				auto& ra = _rooms[_a];
				if (ra.connectedToRoomIdx.get_size() + 1 < ra.doorAmount)
				{
					ra.connectedToRoomIdx.push_back(_a);
					ra.connectedToRoomIdx.push_back(_a);
				}
				else
				{
					return false;
				}
			}
			auto& ra = _rooms[_a];
			auto& rb = _rooms[_b];
			if (ra.connectedToRoomIdx.get_size() < ra.doorAmount &&
				rb.connectedToRoomIdx.get_size() < rb.doorAmount)
			{
				ra.connectedToRoomIdx.push_back(_b);
				rb.connectedToRoomIdx.push_back(_a);
				return true;
			}
			return false;
		}

		static void disconnect(Array<RoomInfo>& _rooms, int _a, int _b)
		{
			auto& ra = _rooms[_a];
			auto& rb = _rooms[_b];
			for_every(c, ra.connectedToRoomIdx)
			{
				if (*c == _b)
				{
					ra.connectedToRoomIdx.remove_at(for_everys_index(c));
				}
			}
			for_every(c, rb.connectedToRoomIdx)
			{
				if (*c == _a)
				{
					rb.connectedToRoomIdx.remove_at(for_everys_index(c));
				}
			}
		}

		static void disconnect_last_door(Array<RoomInfo>& _rooms, int _roomIdx)
		{
			auto& ra = _rooms[_roomIdx];
			if (ra.connectedToRoomIdx.is_empty())
			{
				return;
			}
			int otherRoomIdx = ra.connectedToRoomIdx.get_last();
			for_every(ctri, _rooms[otherRoomIdx].connectedToRoomIdx)
			{
				if (*ctri == _roomIdx)
				{
					_rooms[otherRoomIdx].connectedToRoomIdx.remove_at(for_everys_index(ctri));
					break;
				}
			}
			ra.connectedToRoomIdx.pop_back();
		}

		static bool are_all_connected(Array<RoomInfo>& _rooms)
		{
			for_every(r, _rooms)
			{
				r->connected = false;
			}

			_rooms[0].connected = true;
			while (true)
			{
				bool doneAnything = false;
				for_every(r, _rooms)
				{
					if (r->connected)
					{
						for_every(ctri, r->connectedToRoomIdx)
						{
							if (!_rooms[*ctri].connected)
							{
								_rooms[*ctri].connected = true;
								doneAnything = true;
							}
						}
					}
				}
				if (!doneAnything)
				{
					break;
				}
			}
			bool anyNotConnected = false;
			for_every(r, _rooms)
			{
				if (!r->connected)
				{
					anyNotConnected = true;
					break;
				}
			}
			return ! anyNotConnected;
		}
	};
	Array<RoomInfo> rooms;
	Array<Door*> doors;
	Array<DoorInRoom*> doorInRooms;

	// we can do this as being async prevents from anything else altering the world
	_inRegion->for_every_room([&rooms, &doors, &doorInRooms](Room* _room)
		{
			rooms.push_back();
			rooms.get_last().room = _room;
			rooms.get_last().doorAmount = 0;
			FOR_EVERY_ALL_DOOR_IN_ROOM(door, _room)
			{
				if (door->get_tags().get_tag(RoomLayoutNames::DoorInRoomTags::reconnectable()))
				{
					doorInRooms.push_back(door);
					doors.push_back_unique(door->get_door());
					rooms.get_last().doors.push_back(door);
					++rooms.get_last().doorAmount;
				}
			}
		});

	struct RoomConnector
	{
		int roomIdx;
		Array<int> tryRooms;
		int connectionCount = 0;

		void fill_for(int _roomIdx, int _roomsAmount)
		{
			roomIdx = _roomIdx;
			tryRooms.clear();
			for_count(int, i, _roomsAmount)
			{
				tryRooms.push_back(i);
			}
		}
	};

	Array<RoomConnector> roomConnector;
	roomConnector.make_space_for(rooms.get_size());
	roomConnector.push_back();
	roomConnector.get_last().fill_for(0, rooms.get_size());

	struct Step
	{
		int roomAIdx;
		int roomBIdx;
		Step() {}
		Step(int a, int b) : roomAIdx(a), roomBIdx(b) {}
		bool perform(Array<RoomInfo>& _rooms)
		{
			bool result = RoomInfo::connect(_rooms, roomAIdx, roomBIdx);
			an_assert(result);
			return result;
		}
		void roll_back(Array<RoomInfo>& _rooms)
		{
			RoomInfo::disconnect(_rooms, roomAIdx, roomBIdx);
		}

		static void roll_back_one(Array<Step>& _steps, Array<RoomInfo>& _rooms)
		{
			if (_steps.is_empty())
			{
				return;
			}
			_steps.get_last().roll_back(_rooms);
			_steps.pop_back();
		}
	};

	Array<Step> steps;

	Random::Generator rg;
	if (_rg.is_set())
	{
		rg = _rg.get();
	}

	Array<int> availableRoomsToConnect;

	bool okToGo = false;
	while (!roomConnector.is_empty())
	{
		if (RoomInfo::are_all_connected(rooms))
		{
			okToGo = true;
			break;
		}

		auto& lastrc = roomConnector.get_last();
		if (rooms[lastrc.roomIdx].connectedToRoomIdx.get_size() < rooms[lastrc.roomIdx].doorAmount)
		{
			if (lastrc.tryRooms.get_size() > 0)
			{
				bool connectedSomething = false;
				// try the last connected room, try connecting to something not connected first
				for_count(int, trySingleExits, 2)
				{
					availableRoomsToConnect.clear();
					for_every(roomIdx, lastrc.tryRooms)
					{
						auto& room = rooms[*roomIdx];
						if (room.connectedToRoomIdx.get_size() < room.doorAmount)
						{
							if (!room.connected)
							{
								if (!trySingleExits && (room.doorAmount - room.connectedToRoomIdx.get_size() <= 1))
								{
									// skip this time
									continue;
								}
								availableRoomsToConnect.push_back(*roomIdx);
							}
						}
					}
					while (!availableRoomsToConnect.is_empty())
					{
						int artcIdx = rg.get_int(availableRoomsToConnect.get_size());
						int connectToRoomIdx = availableRoomsToConnect[artcIdx];
						availableRoomsToConnect.remove_fast_at(artcIdx);
						lastrc.tryRooms.remove_fast(connectToRoomIdx);

						{
							bool addNewRC = true;
							for_every(rc, roomConnector)
							{
								if (rc->roomIdx == connectToRoomIdx)
								{
									addNewRC = false;
									break;
								}
							}
							if (addNewRC)
							{
								roomConnector.push_back();
								roomConnector.get_last().fill_for(connectToRoomIdx, rooms.get_size());
							}
						}

						Step step(lastrc.roomIdx, connectToRoomIdx);
						if (step.perform(rooms))
						{
							steps.push_back(step);
							connectedSomething = true;
							++lastrc.connectionCount;
							break;
						}
					}
					if (connectedSomething)
					{
						break;
					}
				}
				if (connectedSomething)
				{
					continue;
				}
			}
		}
		if (lastrc.connectionCount > 0)
		{
			// we can't connect more and not all are connected
			// we will try something else from "tryRooms"
			an_assert(steps.get_last().roomAIdx == lastrc.roomIdx);
			Step::roll_back_one(steps, rooms);
			--lastrc.connectionCount;
			continue;
		}
		else
		{
			// not the best choice, try something else
			roomConnector.pop_back();
			continue;
		}
	}
	
	an_assert(okToGo);
	if (!okToGo)
	{
		warn(TXT("couldn't connect all"));
		return;
	}

	// all connected, now lets connect the rest, we can do this randomly
	{
		Array<int> availableRoomsToConnect;
		while (true)
		{
			availableRoomsToConnect.clear();
			for_every(r, rooms)
			{
				for_count(int, a, r->doorAmount - r->connectedToRoomIdx.get_size())
				{
					availableRoomsToConnect.push_back(for_everys_index(r));
				}
			}
			if (availableRoomsToConnect.get_size() >= 2)
			{
				int artcAIdx = rg.get_int(availableRoomsToConnect.get_size());
				int rAIdx = availableRoomsToConnect[artcAIdx];
				availableRoomsToConnect.remove_fast_at(artcAIdx);
				int artcBIdx = rg.get_int(availableRoomsToConnect.get_size());
				int rBIdx = availableRoomsToConnect[artcBIdx];
				availableRoomsToConnect.remove_fast_at(artcBIdx);

				RoomInfo::connect(rooms, rAIdx, rBIdx);
			}
			else
			{
				break;
			}
		}
	}

	// connect actual doors, disconnect all first, then connect
	{
		auto* game = Game::get();
		game->perform_sync_world_job(TXT("reconnect"), [&rooms, doorInRooms, &doors]()
			{
				for_every_ptr(door, doorInRooms)
				{
					door->unlink();
				}

				for_every(r, rooms)
				{
					int rAIdx = for_everys_index(r);
					while (!r->connectedToRoomIdx.is_empty())
					{
						int rBIdx = r->connectedToRoomIdx.get_first();
						RoomInfo::disconnect(rooms, rAIdx, rBIdx);

						DoorInRoom* da = nullptr;
						da = rooms[rAIdx].doors.get_last();
						rooms[rAIdx].doors.pop_back();

						DoorInRoom* db = nullptr;
						db = rooms[rBIdx].doors.get_last();
						rooms[rBIdx].doors.pop_back();

						doors.get_first()->link_a(da);
						doors.get_first()->link_b(db);
						doors.pop_front();
					}
				}
			});
	}

}

void RegionLayout::async_reconnect_doors(Region* _inRegion, Optional<int> const& _amount, Optional<Random::Generator> const & _rg)
{
	ASSERT_ASYNC_OR_LOADING_OR_PREVIEW;

	if (_amount.is_set() && _amount.get() <= 0)
	{
		return;
	}

	Array<DoorInRoom*> doors;

	// we can do this as being async prevents from anything else altering the world
	_inRegion->for_every_room([&doors](Room* _room)
		{
			FOR_EVERY_ALL_DOOR_IN_ROOM(door, _room)
			{
				if (door->get_tags().get_tag(RoomLayoutNames::DoorInRoomTags::reconnectable()))
				{
					doors.push_back(door);
				}
			}
		});

	if (doors.get_size() >= 4)
	{
		Array<DoorInRoom*> reconnectDoors;
		reconnectDoors.add_from(doors);

		Random::Generator rg;

		if (_rg.is_set())
		{
			rg = _rg.get();
		}

		if (_amount.is_set())
		{
			while (reconnectDoors.get_size() > _amount.get())
			{
				reconnectDoors.remove_fast_at(rg.get_int(reconnectDoors.get_size()));
			}
			reconnectDoors.make_space_for(_amount.get());
		}

		for_every_ptr(rd, reconnectDoors)
		{
			DoorInRoom* wd = rd;
			while (wd == rd ||
				rd->get_door() == wd->get_door())
			{
				wd = doors[rg.get_int(doors.get_size())];
			}

			auto* game = Game::get();

			game->perform_sync_world_job(TXT("reconnect"), [rd, wd]()
				{
					auto* rd_door = rd->get_door();
					auto* wd_door = wd->get_door();

					rd_door->unlink(rd);
					wd_door->unlink(wd);

					rd_door->link(wd);
					wd_door->link(rd);
				});
		}
	}
}

//--

bool RegionLayout::IncludeRoomLayout::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = true;

	result &= roomLayout.load_from_xml(_node, TXT("roomLayout"), TXT("roomLayoutTagged"), _lc);
	result &= count.load_from_xml(_node, TXT("count"));

	return result;
}

bool RegionLayout::IncludeRoomLayout::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	result &= roomLayout.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);

	return result;
}

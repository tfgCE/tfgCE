#pragma once

#include "pilgrimageInstance.h"

namespace TeaForGodEmperor
{
	class PilgrimageInstanceLinear
	: public PilgrimageInstance
	{
		FAST_CAST_DECLARE(PilgrimageInstanceLinear);
		FAST_CAST_BASE(PilgrimageInstance);
		FAST_CAST_END();
		
		typedef PilgrimageInstance base;
	public:
		static PilgrimageInstanceLinear* get() { return fast_cast<PilgrimageInstanceLinear>(s_current); }

		PilgrimageInstanceLinear(Game* _game, Pilgrimage* _pilgrimage);
		~PilgrimageInstanceLinear();

		virtual void create_and_start(GameState* _fromGameState = nullptr, bool _asNextPilgrimageStart = false); // will add async job
		virtual bool has_started() const { return generatedPilgrimAndStarted; }

	public:
		virtual void pre_advance();
		virtual void advance(float _deltaTime);

	public: // linear
		bool is_linear() const { return linear.active; }

		bool linear__is_station(Framework::Room* _room) const { an_assert(is_linear(), TXT("implement others")); return _room == linear.currentStation.get() || _room == linear.nextStation.get(); }

		void linear__set_part_idx(int _partIdx) { linear.partIdx = _partIdx; }
		int linear__get_part_idx() const { return linear.partIdx; }
		bool linear__may_advance_to_next_part() const;
		void linear__end_part_and_advance_to_next();
		bool linear__may_create_new_part() const;
		void linear__create_part();

		void linear__provide_distance_to_next_station(float _distance);

		//

		void linear__mark_current_open() { an_assert(linear.active); linear.currentOpen = true; }
		bool linear__is_current_open() const { an_assert(linear.active); return linear.currentOpen; }

		Framework::Region* linear__get_current_region() const { an_assert(linear.active); return linear.currentRegion.get(); }
		int linear__get_current_region_idx() const { an_assert(linear.active); return linear.currentRegionIdx; } // not part idx
		bool linear__is_current_station(Framework::Room* _room) const { an_assert(linear.active); return _room == linear.currentStation.get(); }
		bool linear__is_next_station(Framework::Room* _room) const { an_assert(linear.active); return _room == linear.nextStation.get(); }
		Framework::Room* linear__get_current_station() const { an_assert(linear.active); return linear.currentStation.get(); }
		Framework::Room* linear__get_next_station() const { an_assert(linear.active); return linear.nextStation.get(); }
		Framework::Door* linear__get_next_station_entrance_door() const { an_assert(linear.active); return linear.nextStationDoor.get(); }
		bool linear__has_reached_end() const { an_assert(linear.active); return linear.reachedEnd; }

	public:
		virtual void debug_log_environments(LogInfoContext& _log) const;
		virtual void log(LogInfoContext & _log) const;

	private:
		int reachedStations = 0;

		bool generatedStartingStation = false;
		bool generatedPilgrimAndStarted = false;

		struct Linear
		{
			bool active = false;

			int partIdx = 0;
			int partTryIdx = 0;
			bool reachedEnd = false;
			RefCountObjectPtr<PilgrimagePart> currentPart;
			RefCountObjectPtr<PilgrimagePart> nextPart;

			bool currentOpen = false; // if we have open/started next one

			float stationToStationDistance = 0.0f; // first non zero distance
			float currentToStationDistance = 0.0f;

			bool generatedPart = false;

			SafePtr<Framework::Room> currentStation;
			SafePtr<Framework::Door> currentStationDoor; // towards next
			SafePtr<Framework::Region> currentRegion;
			SafePtr<Framework::Room> nextStation;
			SafePtr<Framework::Door> nextStationDoor; // from current
			int currentRegionIdx = NONE; // to start lap, not part idx
		} linear;

		Random::Generator get_station_rg(int _offset = 0) const;
		
		Random::Generator linear__get_part_rg(int _offset = 0) const;
		Random::Generator linear__offset_station_rg_to_entrance(Random::Generator const & _rg) const;
		Random::Generator linear__offset_station_rg_to_exit(Random::Generator const & _rg) const;

		void async_generate_starting_station(int _partIdx = 0);
		void async_add_pilgrim_and_start();

		Framework::Room* create_station(int _offset) const;
		Framework::Door* create_station_door(Framework::Room* currentStation, Name const & _doorPOIName, Framework::DoorType* _doorType, Tags const& _doorInRoomTags) const;
		Framework::Room* create_station_entrance_exit_room(int _offset) const;

		void cancel_next_part();

		virtual String get_random_seed_info() const;
		virtual void add_environment_mixes_if_no_cycles();

		void output_dev_info(int _partIdx) const;

		override_ TransitionRoom async_create_transition_room_for_previous_pilgrimage(PilgrimageInstance* _previousPilgrimage);

		override_ void post_setup_rgi();
	};
};

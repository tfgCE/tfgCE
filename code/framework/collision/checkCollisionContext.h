#pragma once

#include "..\..\core\io\xml.h"
#include "..\..\core\collision\checkCollisionContext.h"

#include "..\library\libraryStored.h"

namespace Framework
{
	interface_class IModulesOwner;
	struct CheckCollisionCache;

	struct CheckCollisionContext
	: public Collision::CheckCollisionContext
	{
		typedef Collision::CheckCollisionContext base;
	public:
		CheckCollisionContext();

		inline void use_cache(CheckCollisionCache const * _cache) { cache = _cache; }
		inline CheckCollisionCache const * swap_cache(CheckCollisionCache const * _cache) { CheckCollisionCache const * oldCache = cache; cache = _cache; return oldCache; }
		inline CheckCollisionCache const * get_cache() const { return cache; }

		inline void avoid(Collision::ICollidableObject const * _object) { base::avoid(_object); }
		void avoid(IModulesOwner* imo, bool _andAttached = false);
		void avoid_up_to(IModulesOwner* imo, IModulesOwner* upTo = nullptr);
		void avoid_up_to_top_instigator(IModulesOwner* imo);

		inline void start_with_nothing_to_check() { doorsOnly = false; checkObjects = false; checkActors = false; checkItems = false; checkScenery = false; checkDoors = false; checkRoom = false; checkRoomScenery = false; checkTemporaryObjects = false; }

		inline bool should_check_objects() const { return checkObjects; }

		inline bool should_check_actors() const { return checkActors; }
		inline void check_actors() { checkActors = true; checkObjects = true; }
		inline void ignore_actors() { checkActors = false; checkObjects = checkActors || checkItems || checkScenery || checkRoomScenery; }

		inline bool should_check_items() const { return checkItems; }
		inline void check_items() { checkItems = true; checkObjects = true; }
		inline void ignore_items() { checkItems = false; checkObjects = checkActors || checkItems || checkScenery || checkRoomScenery; }

		inline bool should_check_scenery() const { return checkScenery; }
		inline void check_scenery() { checkScenery = true; checkObjects = true; }
		inline void ignore_scenery() { checkScenery = false; checkObjects = checkActors || checkItems || checkScenery || checkRoomScenery; }

		inline bool should_check_temporary_objects() const { return checkTemporaryObjects; }
		inline void check_temporary_objects() { checkTemporaryObjects = true; }
		inline void ignore_temporary_objects() { checkTemporaryObjects = false; }

		inline bool should_check_doors() const { return checkDoors; }
		inline void check_doors() { checkDoors = true; }
		inline void ignore_doors() { checkDoors = false; }

		inline bool should_check_room() const { return checkRoom; }
		inline void check_room() { checkRoom = true; }
		inline void ignore_room() { checkRoom = false; }

		inline bool should_check_room_scenery() const { return checkRoomScenery; }
		inline void check_room_scenery() { checkRoomScenery = true; checkObjects = true; }
		inline void ignore_room_scenery() { checkRoomScenery = false; checkObjects = checkActors || checkItems || checkScenery || checkRoomScenery; }

		inline bool should_check_only_doors() const { return doorsOnly; }
		inline void doors_only() { doorsOnly = true; }
		inline void not_only_doors() { doorsOnly = false; }

		inline float get_increase_size_for_item() const { return increaseSizeForItem; }
		inline void set_increase_size_for_item(float _increaseSizeForItem) { increaseSizeForItem = _increaseSizeForItem; }
		
		inline Optional<float> get_non_aligned_gravity_slide_dot_threshold() const { return nonAlignedGravitySlideDotThreshold; }
		inline void set_non_aligned_gravity_slide_dot_threshold(Optional<float> _nonAlignedGravitySlideDotThreshold = NP) { nonAlignedGravitySlideDotThreshold = _nonAlignedGravitySlideDotThreshold; }

	private:
		bool doorsOnly = false;
		bool checkObjects = true; // if not, won't check actors, items or scenery
		bool checkActors = true;
		bool checkItems = true;
		bool checkScenery = true;
		bool checkDoors = true; // collision with doors (frames, wings)
		bool checkRoom = true;
		bool checkRoomScenery = true;
		bool checkTemporaryObjects = true;

		float increaseSizeForItem = 0.0f;

		Optional<float> nonAlignedGravitySlideDotThreshold; // if normal is not aligned with gravity, we should ignore this hit location and continue with next

		CheckCollisionCache const * cache = nullptr;
	};
};

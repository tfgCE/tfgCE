#pragma once

#include "..\..\core\casts.h"
#include "..\..\core\fastCast.h"
#include "..\..\core\collision\collisionFlags.h"
#include "..\..\core\types\optional.h"
#include "..\time\dateTimeFormat.h"

struct Name;

namespace IO
{
	namespace XML
	{
		class Node;
	};
};

namespace Framework
{
	struct GameConfig
	{
		FAST_CAST_DECLARE(GameConfig);
		FAST_CAST_END();

	public:
		GameConfig();
		virtual ~GameConfig() {}

		virtual bool is_option_set(Name const & _option) const;

		virtual void prepare_on_game_start();

		static void initialise_static();
		static void close_static();

		static void update_date_time_format(REF_ DateTimeFormat & _dateTimeFormat);

		static bool load_from_xml(IO::XML::Node const * _node);
		static void save_to_xml(IO::XML::Node * _container, bool _userSpecific = false);

		static bool load_to_be_copied_to_main_config_xml(IO::XML::Node const* _node);
		static void save_to_be_copied_to_main_config_xml(IO::XML::Node* _containerNode);

		static GameConfig* get() { return s_config; }

		template<typename Class>
		static Class* get_as() {
#ifdef AN_DEVELOPMENT
			an_assert(fast_cast<Class>(s_config));
			an_assert(fast_cast<Class>(s_config) == plain_cast<Class>(s_config));
			return fast_cast<Class>(s_config);
#else
			return plain_cast<Class>(s_config);
#endif
		}

		static Collision::Flags const & item_collision_flags() { an_assert(get()); an_assert(get()->prepared); return get()->itemCollisionFlags; }
		static Collision::Flags const & item_collides_with_flags() { an_assert(get()); an_assert(get()->prepared); return get()->itemCollidesWithFlags; }
		static Collision::Flags const & actor_collision_flags() { an_assert(get()); an_assert(get()->prepared); return get()->actorCollisionFlags; }
		static Collision::Flags const & actor_collides_with_flags() { an_assert(get()); an_assert(get()->prepared); return get()->actorCollidesWithFlags; }
		static Collision::Flags const & scenery_collision_flags() { an_assert(get()); an_assert(get()->prepared); return get()->sceneryCollisionFlags; }
		static Collision::Flags const & scenery_collides_with_flags() { an_assert(get()); an_assert(get()->prepared); return get()->sceneryCollidesWithFlags; }
		static Collision::Flags const & temporary_object_collision_flags() { an_assert(get()); an_assert(get()->prepared); return get()->temporaryObjectCollisionFlags; }
		static Collision::Flags const & temporary_object_collides_with_flags() { an_assert(get()); an_assert(get()->prepared); return get()->temporaryObjectCollidesWithFlags; }
		static Collision::Flags const & projectile_collides_with_flags() { an_assert(get()); an_assert(get()->prepared); return get()->projectileCollidesWithFlags; }
		static Collision::Flags const & point_of_interest_collides_with_flags() { an_assert(get()); an_assert(get()->prepared); return get()->pointOfInterestCollidesWithFlags; }

#ifdef AN_DEVELOPMENT_OR_PROFILER
		static bool does_allow_lod() { an_assert(get()); return get()->allowLOD; }
		static void switch_allow_lod() { an_assert(get()); get()->allowLOD = !get()->allowLOD; }
#endif

		static float get_min_lod_size() { an_assert(get()); return get()->minLODSize; }
		static float get_lod_selector_start() { an_assert(get()); return get()->lodSelectorStart; }
		static float get_lod_selector_step() { an_assert(get()); return get()->lodSelectorStep; }

	public:
		DateTimeFormat const & get_date_time_format() const { return dateTimeFormat; }
		DateTimeFormat const & get_system_date_time_format() const { return systemDateTimeFormat; }
		void set_date_time_format(DateTimeFormat const & _dateTimeFormat) { dateTimeFormat = _dateTimeFormat; }

		Optional<int> const & get_max_concurrent_apparent_room_sound_sources() const { return maxConcurrentApparentRoomSoundSources; }
		int get_auto_generate_lods() const { return autoGenerateLODs; }

		int get_advancement_suspension_room_distance() const { return advancementSuspensionRoomDistance; }

	protected:
		virtual bool internal_load_from_xml(IO::XML::Node const * _node);
		virtual void internal_save_to_xml(IO::XML::Node * _container, bool _userSpecific) const;

	protected:
		static GameConfig* s_config;

		DateTimeFormat dateTimeFormat; // overrides if not default
		DateTimeFormat systemDateTimeFormat; // fills all default

		Optional<int> maxConcurrentApparentRoomSoundSources = 2; // one higher than sample's

		int autoGenerateLODs = 0;
		float minLODSize = 1.0f;
		float lodSelectorStart = 0.1f;
		float lodSelectorStep = 0.3f;

		int advancementSuspensionRoomDistance = 5;

		// configurable/loadable although default names should be used!
		Name providedItemCollisionFlags;
		Name providedItemCollidesWithFlags;
		Name providedActorCollisionFlags;
		Name providedActorCollidesWithFlags;
		Name providedSceneryCollidesWithFlags;
		Name providedSceneryCollisionFlags;
		Name providedTemporaryObjectCollisionFlags;
		Name providedTemporaryObjectCollidesWithFlags;
		Name providedProjectileCollidesWithFlags;
		Name providedPointOfInterestCollidesWithFlags;

		// handled - processed at game start
		bool prepared = false;
		Collision::Flags itemCollisionFlags = Collision::Flags::none();
		Collision::Flags itemCollidesWithFlags = Collision::Flags::none();
		Collision::Flags actorCollisionFlags = Collision::Flags::none();
		Collision::Flags actorCollidesWithFlags = Collision::Flags::none();
		Collision::Flags sceneryCollidesWithFlags = Collision::Flags::none();
		Collision::Flags sceneryCollisionFlags = Collision::Flags::none();
		Collision::Flags temporaryObjectCollisionFlags = Collision::Flags::none();
		Collision::Flags temporaryObjectCollidesWithFlags = Collision::Flags::none();
		Collision::Flags projectileCollidesWithFlags = Collision::Flags::none();
		Collision::Flags pointOfInterestCollidesWithFlags = Collision::Flags::none();

#ifdef AN_DEVELOPMENT_OR_PROFILER
		bool allowLOD = true;
#endif
	};

};

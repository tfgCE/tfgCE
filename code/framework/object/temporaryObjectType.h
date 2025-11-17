#pragma once

#include "..\library\libraryStored.h"
#include "..\module\moduleData.h"
#include "..\module\moduleAIData.h"
#ifdef AN_CLANG
#include "..\module\moduleAppearance.h"
#endif
#include "..\module\moduleAppearanceData.h"
#include "..\module\moduleCollisionData.h"
#include "..\module\moduleMovementData.h"
#include "..\module\modulePresenceData.h"
#include "..\module\moduleSoundData.h"
#include "..\module\moduleTemporaryObjectsData.h"

#define HOW_MANY_INACTIVE_TEMPORARY_OBJECTS_DO_WE_WANT_TO_HAVE_BY_DEFAULT 3

namespace Framework
{
	class ModuleController;
	class ModuleCustom;
	class ModuleGameplay;
	class ModuleSound;
	class ModuleTemporaryObjects;

	class TemporaryObjectType
	: public LibraryStored
	//TODO,	public ILibrarySolver
	{
		FAST_CAST_DECLARE(TemporaryObjectType);
		FAST_CAST_BASE(LibraryStored);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef LibraryStored base;
	public:
		TemporaryObjectType(Library * _library, LibraryName const & _name);

		int get_precreate_count() const { return precreateCount; }
		int should_reset_on_reuse() const { return ! noResetOnReuse; }
		bool should_skip_for_non_recently_rendered_room() const { return skipForNonRecentlyRenderedRoom; }
		bool has_particles_based_existence() const { return particlesBasedExistence; }
		bool has_sound_based_existence() const { return soundBasedExistence; }
		bool has_light_sources_based_existence() const { return lightSourcesBasedExistence; }
		
		bool get_spawn_blocking_info(OUT_ float& _blockingDist, OUT_ Optional<float>& _blockingTime, OUT_ Optional<int>& _blockingCount) const;

		void set_defaults(); // set_defaults should define default type and default values for parameters. called when object is created for library

#ifdef AN_DEVELOPMENT
		override_ void ready_for_reload();
#endif
		override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
		override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);
		override_ void prepare_to_unload();

		WheresMyPoint::ToolSet const & get_wheres_my_point_processor_on_init() const { return wheresMyPointProcessorOnInit; }

		// ILibrarySolver
		//TODO LibraryName solve_reference(Name _reference, Name _ofType) { return librarySolver.solve_reference(_reference, _ofType); }

		ModuleDefBase const * get_ai() const { return &ai; }
		Array<ModuleDef<ModuleAppearance, ModuleAppearanceData>> const & get_appearances() const { return appearances; }
		ModuleDefBase const * get_collision() const { return &collision; }
		ModuleDefBase const * get_controller() const { return &controller; }
		Array<ModuleDef<ModuleCustom, ModuleData>> const & get_customs() const { return customs; }
		ModuleDefBase const * get_gameplay() const { return &gameplay; }
		Array<ModuleDef<ModuleMovement, ModuleMovementData>> const & get_movements() const { return movements; }
		ModuleDefBase const * get_presence() const { return &presence; }
		ModuleDefBase const * get_sound() const { return &sound; }
		ModuleDef<ModuleTemporaryObjects, ModuleTemporaryObjectsData>  const * get_companion_temporary_objects() const { return &companionTemporaryObjects; }
		ModuleDefBase const* get_temporary_objects() const { return &temporaryObjects; }

		template <typename Class>
		Class const * get_gameplay_as() const { return fast_cast<Class>(get_gameplay()? get_gameplay()->get_data() : nullptr); }

	public: // LibraryStored
		override_ void log(LogInfoContext & _context) const;

	protected:
		int precreateCount = HOW_MANY_INACTIVE_TEMPORARY_OBJECTS_DO_WE_WANT_TO_HAVE_BY_DEFAULT;
		bool noResetOnReuse = false;
		WheresMyPoint::ToolSet wheresMyPointProcessorOnInit; // to allow modification of parameters in an actual object (once, on initialisation of modules!)

		ModuleDef<ModuleAI, ModuleAIData> ai = ModuleDef<ModuleAI, ModuleAIData>(ModuleDefType::None);
		Array<ModuleDef<ModuleAppearance, ModuleAppearanceData>> appearances;
		ModuleDef<ModuleCollision, ModuleCollisionData> collision;
		ModuleDef<ModuleController, ModuleData> controller;
		Array<ModuleDef<ModuleCustom, ModuleData>> customs;
		ModuleDef<ModuleGameplay, ModuleData> gameplay = ModuleDef<ModuleGameplay, ModuleData>(ModuleDefType::None);
		Array<ModuleDef<ModuleMovement, ModuleMovementData>> movements;
		// no nav element for temporary objects, why would we do that?
		ModuleDef<ModulePresence, ModulePresenceData> presence;
		ModuleDef<ModuleSound, ModuleSoundData> sound;
		ModuleDef<ModuleTemporaryObjects, ModuleTemporaryObjectsData> companionTemporaryObjects; // spawned with our temporary object, limited (will stop on assert)
		ModuleDef<ModuleTemporaryObjects, ModuleTemporaryObjectsData> temporaryObjects;

		bool skipForNonRecentlyRenderedRoom = false;
		bool particlesBasedExistence = false; // if active particles (ac) will become inactive, this object ceases to exist
		bool soundBasedExistence = false; // as above but with sounds
		bool lightSourcesBasedExistence = false; // as above but with sounds
		// if both are set, if hits 0 particles, will stop looped sounds

		struct SpawnBlocking
		{
			Optional<float> dist; // has to be set to become active
			Optional<float> time;
			Optional<int> count;
		};
		SpawnBlocking spawnBlocking;

		virtual ~TemporaryObjectType();
	};
};


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

namespace Framework
{
	class ModuleController;
	class ModuleCustom;
	class ModuleGameplay;
	class ModuleNavElement;
	class ModuleSound;
	class ModuleTemporaryObjects;
	class Object;

	class ObjectType
	: public LibraryStored
	//TODO,	public ILibrarySolver
	{
		FAST_CAST_DECLARE(ObjectType);
		FAST_CAST_BASE(LibraryStored);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef LibraryStored base;
	public:
		ObjectType(Library * _library, LibraryName const & _name);

		bool is_sub_object() const { return subObject; }

		virtual void set_defaults(); // set_defaults should define default type and default values for parameters. called when object is created for library

		virtual Object* create(String const & _name) const; // create object of proper type (actor/item/scenery)

#ifdef AN_DEVELOPMENT
		override_ void ready_for_reload();
#endif
		override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
		override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);
		override_ void prepare_to_unload();

		// ILibrarySolver
		//TODO LibraryName solve_reference(Name _reference, Name _ofType) { return librarySolver.solve_reference(_reference, _ofType); }

		float get_individuality_factor() const { return individualityFactor; }

		bool should_advance_once() const { return advanceOnce; }
		bool is_advancement_suspendable() const { return advancementSuspendable; }

		WheresMyPoint::ToolSet const & get_wheres_my_point_processor_on_init() const { return wheresMyPointProcessorOnInit; }

		ModuleDefBase const * get_ai() const { return &ai; }
		Array<ModuleDef<ModuleAppearance, ModuleAppearanceData>> const & get_appearances() const { return appearances; }
		ModuleDefBase const * get_collision() const { return &collision; }
		ModuleDefBase const * get_controller() const { return &controller; }
		Array<ModuleDef<ModuleCustom, ModuleData>> const & get_customs() const { return customs; }
		ModuleDefBase const * get_gameplay() const { return &gameplay; }
		Array<ModuleDef<ModuleMovement, ModuleMovementData>> const & get_movements() const { return movements; }
		ModuleDefBase const * get_nav_element() const { return &navElement; }
		ModuleDefBase const * get_presence() const { return &presence; }
		ModuleDefBase const * get_sound() const { return &sound; }
		ModuleDefBase const * get_temporary_objects() const { return &temporaryObjects; }

	protected:
		float individualityFactor;

		bool subObject = false; // should be a part of a bigger object, like sub module, not "a turret attached to a ship", more like "an engine being part of a ship"

		bool advanceOnce = false;
		bool advancementSuspendable = true;

		WheresMyPoint::ToolSet wheresMyPointProcessorOnInit; // to allow modification of parameters in an actual object (on init)

		ModuleDef<ModuleAI, ModuleAIData> ai = ModuleDef<ModuleAI, ModuleAIData>(ModuleDefType::None);
		Array<ModuleDef<ModuleAppearance, ModuleAppearanceData>> appearances;
		ModuleDef<ModuleCollision, ModuleCollisionData> collision;
		ModuleDef<ModuleController, ModuleData> controller;
		Array<ModuleDef<ModuleCustom, ModuleData>> customs;
		ModuleDef<ModuleGameplay, ModuleData> gameplay = ModuleDef<ModuleGameplay, ModuleData>(ModuleDefType::None);
		Array<ModuleDef<ModuleMovement, ModuleMovementData>> movements;
		ModuleDef<ModuleNavElement, ModuleData> navElement = ModuleDef<ModuleNavElement, ModuleData>(ModuleDefType::None);
		ModuleDef<ModulePresence, ModulePresenceData> presence;
		ModuleDef<ModuleSound, ModuleSoundData> sound;
		ModuleDef<ModuleTemporaryObjects, ModuleTemporaryObjectsData> temporaryObjects;

		virtual ~ObjectType();
	};
};


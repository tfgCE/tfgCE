#pragma once

#include "module.h"

#include "..\..\core\functionParamsStruct.h"

namespace Concurrency
{
	class JobExecutionContext;
};

namespace Framework
{
	class ModulePresence;
	class ModuleTemporaryObjectsData;
	class TemporaryObject;
	class TemporaryObjectType;
	struct ModuleTemporaryObjectsSingle;

	struct TemporaryObjectSpawnContext
	{
		Optional<Room*> inRoom;
		Optional<Transform> placement;

		mutable Random::Generator rg;

		TemporaryObjectSpawnContext() {}
		TemporaryObjectSpawnContext(Room* _inRoom, Transform const & _placement) : inRoom(_inRoom), placement(_placement) {}

		TemporaryObjectSpawnContext& with(Random::Generator const& _rg) { rg = _rg; return *this; }
	};

	class ModuleTemporaryObjects
	: public Module
	{
		FAST_CAST_DECLARE(ModuleTemporaryObjects);
		FAST_CAST_BASE(Module);
		FAST_CAST_END();

		typedef Module base;
	public:
		static RegisteredModule<ModuleTemporaryObjects> & register_itself();

		ModuleTemporaryObjects(IModulesOwner* _owner);
		virtual ~ModuleTemporaryObjects();

		bool does_handle_temporary_object(Name const & _name) const;

		struct SpawnParams
		{
			ADD_FUNCTION_PARAM(SpawnParams, Name, socket, at_socket);
			ADD_FUNCTION_PARAM_PLAIN_INITIAL(SpawnParams, Transform, relativePlacement, at_relative_placement, Transform::identity);
		};

		// better to use this if no extra params required (returns how many did spawn)
		int spawn_all(Name const & _name, Optional<SpawnParams> const& _spawnParams = NP, Array<SafePtr<IModulesOwner>> * _storeIn = nullptr, IModulesOwner* _of = nullptr);
		int spawn_all_using(Name const & _name, ModuleTemporaryObjectsData const* _usingData = nullptr, Optional<SpawnParams> const& _spawnParams = NP, Array<SafePtr<IModulesOwner>> * _storeIn = nullptr);

		TemporaryObject* spawn(Name const & _name, Optional<SpawnParams> const& _spawnParams = NP); // attached or in room

		TemporaryObject* spawn_attached(Name const & _name, Optional<SpawnParams> const& _spawnParams = NP);
		TemporaryObject* spawn_attached_to(Name const & _name, IModulesOwner* _attachTo, Optional<SpawnParams> const& _spawnParams = NP);

		TemporaryObject* spawn_in_room(Name const & _name, Optional<SpawnParams> const& _spawnParams = NP);
		TemporaryObject* spawn_in_room_attached_to(Name const & _name, IModulesOwner* _attachTo, Optional<SpawnParams> const& _spawnParams = NP); // will be spawned as in_room but will be attached at that placement
		TemporaryObject* spawn_in_room(Name const & _name, Room* _room, Transform const & _placement);

		ModuleTemporaryObjectsSingle const* get_info_for(Name const& _name) const;

	public:
		static void spawn_companions_of(IModulesOwner* _owner, TemporaryObject const * _to, Array<SafePtr<IModulesOwner>> * _storeIn = nullptr, TemporaryObjectSpawnContext const & _spawnContext = TemporaryObjectSpawnContext());
		static void spawn_companions(IModulesOwner* _owner, ModuleTemporaryObjectsData const * _companions, Array<SafePtr<IModulesOwner>> * _storeIn = nullptr, TemporaryObject const * _to = nullptr, TemporaryObjectSpawnContext const & _spawnContext = TemporaryObjectSpawnContext());

	public:	// Module
		override_ void reset();
		override_ void setup_with(ModuleData const * _moduleData);
		override_ void initialise();
		override_ void activate();

	private: friend class ModulePresence;
		void spawn_auto_spawn(); // only if not spawned so far

	private:
		Random::Generator rg;

		bool spawnedAutoSpawn = false;

		static TemporaryObject* static_spawn_attached(IModulesOwner* _owner, ModuleTemporaryObjectsSingle const * _to, Array<SafePtr<IModulesOwner>> * _storeIn = nullptr, IModulesOwner* _attachTo = nullptr, Optional<Name> const & _socket = NP, Transform const & _relativePlacement = Transform::identity, TemporaryObjectSpawnContext const & _spawnContext = TemporaryObjectSpawnContext());
		static TemporaryObject* static_spawn_in_room(IModulesOwner* _owner, ModuleTemporaryObjectsSingle const * _to, Array<SafePtr<IModulesOwner>> * _storeIn = nullptr, IModulesOwner* _attachTo = nullptr, Optional<Name> const & _socket = NP, Transform const & _relativePlacement = Transform::identity, TemporaryObjectSpawnContext const & _spawnContext = TemporaryObjectSpawnContext());

		static bool does_have_companions_to_spawn(TemporaryObject const * _to);
		static bool does_have_companions_to_spawn(TemporaryObjectType const * _to);
		static void cant_spawn_companions_of(TemporaryObject const * _to);

		static void copy_variables(IModulesOwner* _owner, TemporaryObject* to, Array<Name> const & _copyVariables, SimpleVariableStorage const & _setVariables, Array<Name> const & _modulesAffectedByVariables);

		ModuleTemporaryObjectsData const * moduleTemporaryObjectsData = nullptr;
	};
};

 
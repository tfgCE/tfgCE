#include "moduleTemporaryObjects.h"

#include "moduleTemporaryObjectsData.h"

#include "modules.h"
#include "registeredModule.h"

#include "..\world\subWorld.h"

#include "..\..\core\debug\extendedDebug.h"
#include "..\..\core\performance\performanceUtils.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#ifdef AN_DEVELOPMENT_OR_PROFILER
//#define MEASURE_TO_TIMES
#endif

//

using namespace Framework;

//

#define SOCKET_FROM(_spawnParams) _spawnParams.is_set() ? _spawnParams.get().socket : NP
#define RELATIVE_PLACEMENT_FROM(_spawnParams) _spawnParams.is_set() ? _spawnParams.get().relativePlacement : Transform::identity

REGISTER_FOR_FAST_CAST(ModuleTemporaryObjects);

static ModuleTemporaryObjects* create_module(IModulesOwner* _owner)
{
	return new ModuleTemporaryObjects(_owner);
}

RegisteredModule<ModuleTemporaryObjects> & ModuleTemporaryObjects::register_itself()
{
	return Modules::temporaryObjects.register_module(String(TXT("base")), create_module);
}

ModuleTemporaryObjects::ModuleTemporaryObjects(IModulesOwner* _owner)
: Module(_owner)
{
}

ModuleTemporaryObjects::~ModuleTemporaryObjects()
{
}

void ModuleTemporaryObjects::reset()
{
	Module::reset();
	spawnedAutoSpawn = false;
}

void ModuleTemporaryObjects::setup_with(ModuleData const * _moduleData)
{
	Module::setup_with(_moduleData);

	moduleTemporaryObjectsData = fast_cast<ModuleTemporaryObjectsData>(_moduleData);
}

void ModuleTemporaryObjects::initialise()
{
	base::initialise();
}

void ModuleTemporaryObjects::activate()
{
	base::activate();

	spawn_auto_spawn();
}

void ModuleTemporaryObjects::spawn_auto_spawn()
{
	if (spawnedAutoSpawn)
	{
		return;
	}

	if (!get_owner()->get_presence()->get_in_room())
	{
		return;
	}

	spawnedAutoSpawn = true;
	if (moduleTemporaryObjectsData)
	{
		for_every_ref(temporaryObject, moduleTemporaryObjectsData->get_temporary_objects())
		{
			if (temporaryObject->autoSpawn.get(false))
			{
				if (temporaryObject->spawnDetached)
				{
					static_spawn_in_room(get_owner(), temporaryObject, nullptr, nullptr, NP, Transform::identity, TemporaryObjectSpawnContext().with(rg.spawn()));
				}
				else
				{
					static_spawn_attached(get_owner(), temporaryObject, nullptr, nullptr, NP, Transform::identity, TemporaryObjectSpawnContext().with(rg.spawn()));
				}
			}
		}
	}
}

bool ModuleTemporaryObjects::does_handle_temporary_object(Name const & _name) const
{
	if (moduleTemporaryObjectsData)
	{
		for_every_ref(temporaryObject, moduleTemporaryObjectsData->get_temporary_objects())
		{
			if (temporaryObject->id == _name)
			{
				return true;
			}
		}
	}

	return false;
}

#define FOLLOWING_OR_NOT true

void ModuleTemporaryObjects::spawn_companions_of(IModulesOwner* _owner, TemporaryObject const * _to, Array<SafePtr<IModulesOwner>> * _storeIn, TemporaryObjectSpawnContext const & _spawnContext)
{
	if (_to &&
		_to->get_object_type())
	{
		spawn_companions(_owner, fast_cast<ModuleTemporaryObjectsData>(_to->get_object_type()->get_companion_temporary_objects()->get_data()), _storeIn, _to, _spawnContext);
	}
}

bool ModuleTemporaryObjects::does_have_companions_to_spawn(TemporaryObject const * _to)
{
	if (_to &&
		_to->get_object_type())
	{
		return does_have_companions_to_spawn(_to->get_object_type());
	}
	return false;
}

bool ModuleTemporaryObjects::does_have_companions_to_spawn(TemporaryObjectType const * _to)
{
	if (_to)
	{
		if (auto* companions = fast_cast<ModuleTemporaryObjectsData>(_to->get_companion_temporary_objects()->get_data()))
		{
			return !companions->get_temporary_objects().is_empty();
		}
	}
	return false;
}

void ModuleTemporaryObjects::cant_spawn_companions_of(TemporaryObject const * _to)
{
#ifdef AN_DEVELOPMENT
	if (_to &&
		_to->get_object_type())
	{
		if (auto* companions = fast_cast<ModuleTemporaryObjectsData>(_to->get_object_type()->get_companion_temporary_objects()->get_data()))
		{
			an_assert(companions->get_temporary_objects().is_empty());
		}
	}
#endif
}

void ModuleTemporaryObjects::spawn_companions(IModulesOwner* _owner, ModuleTemporaryObjectsData const * _companions, Array<SafePtr<IModulesOwner>> * _storeIn, TemporaryObject const * _to, TemporaryObjectSpawnContext const & _spawnContext)
{
	if (_companions)
	{
		for_every_ref(temporaryObject, _companions->get_temporary_objects())
		{
			for_count(int, i, _spawnContext.rg.get(temporaryObject->companionCount))
			{
				TemporaryObject* to = nullptr;
				if (temporaryObject->spawnDetached)
				{
					to = static_spawn_in_room(_owner, temporaryObject, _storeIn, nullptr, NP, Transform::identity, _spawnContext);
				}
				else
				{
					to = static_spawn_attached(_owner, temporaryObject, _storeIn, nullptr, NP, Transform::identity, _spawnContext);
				}
				spawn_companions_of(_owner, to, _storeIn, _spawnContext);
			}
		}
	}
}

int ModuleTemporaryObjects::spawn_all(Name const& _name, Optional<SpawnParams> const& _spawnParams, Array<SafePtr<IModulesOwner>>* _storeIn, IModulesOwner* _of)
{
	auto* useData = moduleTemporaryObjectsData;
	if (_of)
	{
		if (auto* to = _of->get_temporary_objects())
		{
			useData = to->moduleTemporaryObjectsData;
		}
	}
	return spawn_all_using(_name, useData, _spawnParams, _storeIn);
}

int ModuleTemporaryObjects::spawn_all_using(Name const& _name, ModuleTemporaryObjectsData const* _usingData, Optional<SpawnParams> const& _spawnParams, Array<SafePtr<IModulesOwner>>* _storeIn)
{
	int spawned = 0;
	if (_usingData)
	{
		bool found = true;
		for_every_ref(temporaryObject, _usingData->get_temporary_objects())
		{
			if (temporaryObject->id == _name)
			{
#ifdef MEASURE_TO_TIMES
				MEASURE_AND_OUTPUT_PERFORMANCE(TXT("spawn all using, spawn \"%S\""), temporaryObject->id.to_char());
#endif

				found = true;
				TemporaryObject* to = nullptr;
				if (temporaryObject->spawnDetached)
				{
					to = static_spawn_in_room(get_owner(), temporaryObject, _storeIn, nullptr, SOCKET_FROM(_spawnParams), RELATIVE_PLACEMENT_FROM(_spawnParams), TemporaryObjectSpawnContext().with(rg.spawn()));
				}
				else
				{
					to = static_spawn_attached(get_owner(), temporaryObject, _storeIn, nullptr, SOCKET_FROM(_spawnParams), RELATIVE_PLACEMENT_FROM(_spawnParams), TemporaryObjectSpawnContext().with(rg.spawn()));
				}
				++ spawned;
			}
		}
		if (!found)
		{
#ifdef AN_ALLOW_EXTENDED_DEBUG
			IF_EXTENDED_DEBUG(MissingTemporaryObjects)
			{
				warn(TXT("couldn't find temporary objects \"%S\" for \"%S\""), _name.to_char(), get_owner()->ai_get_name().to_char());
			}
#endif
		}
	}
	return spawned;
}

ModuleTemporaryObjectsSingle const* ModuleTemporaryObjects::get_info_for(Name const& _name) const
{
	if (moduleTemporaryObjectsData)
	{
		for_every_ref(temporaryObject, moduleTemporaryObjectsData->get_temporary_objects())
		{
			if (temporaryObject->id == _name)
			{
				return temporaryObject;
			}
		}
	}

	return nullptr;
}

TemporaryObject* ModuleTemporaryObjects::spawn(Name const & _name, Optional<SpawnParams> const& _spawnParams)
{
	TemporaryObject* to = nullptr;
	if (moduleTemporaryObjectsData)
	{
		bool found = false;
		for_every_ref(temporaryObject, moduleTemporaryObjectsData->get_temporary_objects())
		{
			if (temporaryObject->id == _name)
			{
				found = true;
				if (temporaryObject->spawnDetached)
				{
					to = static_spawn_in_room(get_owner(), temporaryObject, nullptr, nullptr, SOCKET_FROM(_spawnParams), RELATIVE_PLACEMENT_FROM(_spawnParams), TemporaryObjectSpawnContext().with(rg.spawn()));
				}
				else
				{
					to = static_spawn_attached(get_owner(), temporaryObject, nullptr, nullptr, SOCKET_FROM(_spawnParams), RELATIVE_PLACEMENT_FROM(_spawnParams), TemporaryObjectSpawnContext().with(rg.spawn()));
				}
				break;
			}
		}
		if (! found)
		{
#ifdef AN_ALLOW_EXTENDED_DEBUG
			IF_EXTENDED_DEBUG(MissingTemporaryObjects)
			{
				warn(TXT("couldn't find temporary objects \"%S\" for \"%S\""), _name.to_char(), get_owner()->ai_get_name().to_char());
			}
#endif
		}
	}

	return to;
}

TemporaryObject* ModuleTemporaryObjects::spawn_attached(Name const & _name, Optional<SpawnParams> const& _spawnParams)
{
	TemporaryObject* to = nullptr;
	if (moduleTemporaryObjectsData)
	{
		bool found = false;
		for_every_ref(temporaryObject, moduleTemporaryObjectsData->get_temporary_objects())
		{
			if (temporaryObject->id == _name)
			{
				found = true;
				to = static_spawn_attached(get_owner(), temporaryObject, nullptr, nullptr, SOCKET_FROM(_spawnParams), RELATIVE_PLACEMENT_FROM(_spawnParams), TemporaryObjectSpawnContext().with(rg.spawn()));
				break;
			}
		}
		if (!found)
		{
#ifdef AN_ALLOW_EXTENDED_DEBUG
			IF_EXTENDED_DEBUG(MissingTemporaryObjects)
			{
				warn(TXT("couldn't find temporary objects \"%S\" for \"%S\""), _name.to_char(), get_owner()->ai_get_name().to_char());
			}
#endif
		}
	}

	return to;
}

TemporaryObject* ModuleTemporaryObjects::spawn_attached_to(Name const & _name, IModulesOwner* _attachTo, Optional<SpawnParams> const& _spawnParams)
{
	TemporaryObject* to = nullptr;
	if (moduleTemporaryObjectsData)
	{
		bool found = false;
		for_every_ref(temporaryObject, moduleTemporaryObjectsData->get_temporary_objects())
		{
			if (temporaryObject->id == _name)
			{
				found = true;
				to = static_spawn_attached(get_owner(), temporaryObject, nullptr, _attachTo, SOCKET_FROM(_spawnParams), RELATIVE_PLACEMENT_FROM(_spawnParams), TemporaryObjectSpawnContext().with(rg.spawn()));
				break;
			}
		}
		if (!found)
		{
#ifdef AN_ALLOW_EXTENDED_DEBUG
			IF_EXTENDED_DEBUG(MissingTemporaryObjects)
			{
				warn(TXT("couldn't find temporary objects \"%S\" for \"%S\""), _name.to_char(), get_owner()->ai_get_name().to_char());
			}
#endif
		}
	}

	return to;
}

TemporaryObject* ModuleTemporaryObjects::spawn_in_room(Name const & _name, Optional<SpawnParams> const& _spawnParams)
{
	TemporaryObject* to = nullptr;
	if (moduleTemporaryObjectsData)
	{
		bool found = false;
		for_every_ref(temporaryObject, moduleTemporaryObjectsData->get_temporary_objects())
		{
			if (temporaryObject->id == _name)
			{
				found = true;
				to = static_spawn_in_room(get_owner(), temporaryObject, nullptr, nullptr, SOCKET_FROM(_spawnParams), RELATIVE_PLACEMENT_FROM(_spawnParams), TemporaryObjectSpawnContext().with(rg.spawn()));
				break;
			}
		}
		if (!found)
		{
#ifdef AN_ALLOW_EXTENDED_DEBUG
			IF_EXTENDED_DEBUG(MissingTemporaryObjects)
			{
				warn(TXT("couldn't find temporary objects \"%S\" for \"%S\""), _name.to_char(), get_owner()->ai_get_name().to_char());
			}
#endif
		}
	}

	return to;
}

TemporaryObject* ModuleTemporaryObjects::spawn_in_room_attached_to(Name const & _name, IModulesOwner* _attachTo, Optional<SpawnParams> const& _spawnParams)
{
	TemporaryObject* to = nullptr;
	if (moduleTemporaryObjectsData)
	{
		bool found = false;
		for_every_ref(temporaryObject, moduleTemporaryObjectsData->get_temporary_objects())
		{
			if (temporaryObject->id == _name)
			{
				found = true;
				to = static_spawn_in_room(get_owner(), temporaryObject, nullptr, _attachTo, SOCKET_FROM(_spawnParams), RELATIVE_PLACEMENT_FROM(_spawnParams), TemporaryObjectSpawnContext().with(rg.spawn()));
				break;
			}
		}
		if (!found)
		{
#ifdef AN_ALLOW_EXTENDED_DEBUG
			IF_EXTENDED_DEBUG(MissingTemporaryObjects)
			{
				warn(TXT("couldn't find temporary objects \"%S\" for \"%S\""), _name.to_char(), get_owner()->ai_get_name().to_char());
			}
#endif
		}
	}

	return to;
}

TemporaryObject* ModuleTemporaryObjects::spawn_in_room(Name const & _name, Room* _room, Transform const & _placement)
{
	TemporaryObject* to = nullptr;
	if (moduleTemporaryObjectsData)
	{
		bool found = false;
		for_every_ref(temporaryObject, moduleTemporaryObjectsData->get_temporary_objects())
		{
			if (temporaryObject->id == _name)
			{
				found = true;
				if (temporaryObject->systemTagRequired.is_empty() ||
					temporaryObject->systemTagRequired.check(System::Core::get_system_tags()))
				{
					if (auto* subWorld = _room->get_in_sub_world())
					{
						if ((to = subWorld->get_one_for(temporaryObject->get_type(), _room)))
						{
							Transform placement = _placement;
							placement = placement.to_world(temporaryObject->ready_placement(&rg));
							to->set_creator(get_owner());
							to->set_instigator(get_owner());
							copy_variables(get_owner(), to, temporaryObject->copyVariables, temporaryObject->variables, temporaryObject->modulesAffectedByVariables);
							to->on_activate_place_in(_room, placement);
							to->mark_to_activate();
							spawn_companions_of(get_owner(), to, nullptr, TemporaryObjectSpawnContext(_room, placement));
						}
					}
				}
				break;
			}
		}
		if (!found)
		{
#ifdef AN_ALLOW_EXTENDED_DEBUG
			IF_EXTENDED_DEBUG(MissingTemporaryObjects)
			{
				warn(TXT("couldn't find temporary objects \"%S\" for \"%S\""), _name.to_char(), get_owner()->ai_get_name().to_char());
			}
#endif
		}
	}

	return to;
}

TemporaryObject* ModuleTemporaryObjects::static_spawn_attached(IModulesOwner* _owner, ModuleTemporaryObjectsSingle const * _to, Array<SafePtr<IModulesOwner>> * _storeIn, IModulesOwner* _attachTo, Optional<Name> const & _socket, Transform const & _relativePlacement, TemporaryObjectSpawnContext const & _spawnContext)
{
	an_assert_immediate(_relativePlacement.is_ok());
#ifdef MEASURE_TO_TIMES
	MEASURE_AND_OUTPUT_PERFORMANCE(TXT("static_spawn_attached \"%S\""), _to->get_type()->get_name().to_string().to_char());
#endif

	PERFORMANCE_GUARD_LIMIT(0.005f, TXT("ModuleTemporaryObjects::static_spawn_attached"));

	if (!_to->systemTagRequired.is_empty() &&
		!_to->systemTagRequired.check(System::Core::get_system_tags()))
	{
		return nullptr;
	}

	if (!_attachTo)
	{
		_attachTo = _owner;
	}
	if (auto* room = _spawnContext.inRoom.get(_owner->get_presence()->get_in_room()))
	{
		if (auto* subWorld = room->get_in_sub_world())
		{
			if (!_socket.is_set() && _to->socketPrefix.is_valid() && _owner)
			{
				TemporaryObject* lastTO = nullptr;
				an_assert(_owner->get_appearances().get_size() == 1, TXT("implemented for single appearance ATM"));
				if (auto* ap = _owner->get_appearance())
				{
					ap->for_every_socket_starting_with(_to->socketPrefix,
						[&lastTO, &_storeIn, _spawnContext, _to, subWorld, room, _owner, _relativePlacement, _attachTo](Name const& _socket)
					{
						if (auto* to = subWorld->get_one_for(_to->get_type(), room))
						{
							Transform relativePlacement = _relativePlacement;
							relativePlacement = relativePlacement.to_world(_to->ready_placement(&_spawnContext.rg));
							to->set_creator(_attachTo); // or _owner?
							to->set_instigator(_attachTo);
							copy_variables(_owner, to, _to->copyVariables, _to->variables, _to->modulesAffectedByVariables);
							{
								to->on_activate_attach_to_socket(_attachTo, _socket, FOLLOWING_OR_NOT, relativePlacement);
							}
							to->mark_to_activate();
							cant_spawn_companions_of(to);
							if (_storeIn)
							{
								_storeIn->push_back(SafePtr<IModulesOwner>(to));
							}
							lastTO = to;
						}
					});
				}
				return lastTO;

			}
			else
			{
				if (auto* to = subWorld->get_one_for(_to->get_type(), room))
				{
					Transform relativePlacement = _relativePlacement;
					relativePlacement = relativePlacement.to_world(_to->ready_placement(&_spawnContext.rg));
					to->set_creator(_attachTo); // or _owner?
					to->set_instigator(_attachTo);
					copy_variables(_owner, to, _to->copyVariables, _to->variables, _to->modulesAffectedByVariables);
					if ((_socket.is_set() && _socket.get().is_valid()) || _to->socket.is_valid())
					{
						to->on_activate_attach_to_socket(_attachTo, _socket.is_set() ? _socket.get() : _to->socket, FOLLOWING_OR_NOT, relativePlacement);
					}
					else if (_to->bone.is_valid())
					{
						to->on_activate_attach_to_bone(_attachTo, _to->bone, FOLLOWING_OR_NOT, relativePlacement);
					}
					else
					{
						to->on_activate_attach_to(_attachTo, FOLLOWING_OR_NOT, relativePlacement);
					}
					to->mark_to_activate();
					cant_spawn_companions_of(to);
					if (_storeIn)
					{
						_storeIn->push_back(SafePtr<IModulesOwner>(to));
					}
					return to;
				}
			}
		}
	}
	return nullptr;
}

void for_every_socket_starting_with(IModulesOwner* _owner, Name const& _socketPrefix, std::function<void(Name const & _socket)> _do)
{
	if (auto* ap = _owner->get_appearance())
	{
		if (auto* mesh = ap->get_mesh())
		{
			for_every(socket, mesh->get_sockets().get_sockets())
			{
				if (socket->get_name().to_string().has_prefix(_socketPrefix.to_string()))
				{
					_do(socket->get_name());
				}
			}
		}
	}
}

TemporaryObject* ModuleTemporaryObjects::static_spawn_in_room(IModulesOwner* _owner, ModuleTemporaryObjectsSingle const * _to, Array<SafePtr<IModulesOwner>> * _storeIn, IModulesOwner* _attachTo, Optional<Name> const & _socket, Transform const & _relativePlacement, TemporaryObjectSpawnContext const & _spawnContext)
{
	an_assert_immediate(_relativePlacement.is_ok());
#ifdef MEASURE_TO_TIMES
	MEASURE_AND_OUTPUT_PERFORMANCE(TXT("static_spawn_in_room \"%S\""), _to->get_type()->get_name().to_string().to_char());
#endif

	PERFORMANCE_GUARD_LIMIT(0.005f, TXT("ModuleTemporaryObjects::static_spawn_in_room"));

	if (!_to->systemTagRequired.is_empty() &&
		!_to->systemTagRequired.check(System::Core::get_system_tags()))
	{
		return nullptr;
	}

	if (auto* room = _spawnContext.inRoom.get(_owner->get_presence()->get_in_room()))
	{
		if (auto* subWorld = room->get_in_sub_world())
		{
			Transform relativePlacement = _relativePlacement;
			// we calculate our own socket:
			// for _attachTo, attaching to different object
			// for not _attachTo, we place at socket
			if (_attachTo || does_have_companions_to_spawn(_to->get_type()))
			{
				// if not attaching and does not have companions, we will use on_activate_place_at and we don't want to have sockest interferring here.
				if (_socket.is_set() && _socket.get().is_valid())
				{
					relativePlacement = _owner->get_appearance()->calculate_socket_os(_socket.get()).to_world(relativePlacement);
				}
				else if (_to->socket.is_valid())
				{
					relativePlacement = _owner->get_appearance()->calculate_socket_os(_to->socket).to_world(relativePlacement);
				}
			}
			if (_attachTo)
			{
				// now we should change relative placement
				if (_attachTo->get_presence()->get_in_room() == room)
				{
					relativePlacement = _attachTo->get_presence()->get_placement().to_local(_spawnContext.placement.get(_owner->get_presence()->get_placement()).to_world(relativePlacement));
				}
				else
				{
					warn(TXT("spawning in different room"));
					return nullptr;
				}
				relativePlacement = relativePlacement.to_world(_to->ready_placement(&_spawnContext.rg));
				if (auto* to = subWorld->get_one_for(_to->get_type(), room))
				{
					to->set_creator(_owner);
					to->set_instigator(_owner);
					copy_variables(_owner, to, _to->copyVariables, _to->variables, _to->modulesAffectedByVariables);
					to->on_activate_attach_to(_attachTo, FOLLOWING_OR_NOT, relativePlacement);
					to->mark_to_activate();
					cant_spawn_companions_of(to);
					if (_storeIn)
					{
						_storeIn->push_back(SafePtr<IModulesOwner>(to));
					}
					return to;
				}
			}
			else
			{
				if (! _socket.is_set() && _to->socketPrefix.is_valid() && _owner)
				{
					TemporaryObject* lastTO = nullptr;
					an_assert(_owner->get_appearances().get_size() == 1, TXT("implemented for single appearance ATM"));
					if (auto* ap = _owner->get_appearance())
					{
						ap->for_every_socket_starting_with(_to->socketPrefix,
							[&lastTO, &_storeIn, _spawnContext, _to, subWorld, room, _owner, relativePlacement](Name const& _socket)
						{
							if (auto* to = subWorld->get_one_for(_to->get_type(), room))
							{
#ifdef MEASURE_TO_TIMES
								MEASURE_AND_OUTPUT_PERFORMANCE(TXT("setup (socket) \"%S\""), _to->get_type()->get_name().to_string().to_char());
#endif
								to->set_creator(_owner);
								to->set_instigator(_owner);
								copy_variables(_owner, to, _to->copyVariables, _to->variables, _to->modulesAffectedByVariables);
								Transform useRelativePlacement = relativePlacement.to_world(_to->ready_placement(&_spawnContext.rg));
								//Transform placement = _spawnContext.placement.get(_owner->get_presence()->get_placement()).to_world(useRelativePlacement);
								{
									to->on_activate_place_at(_owner, _socket, useRelativePlacement);
									cant_spawn_companions_of(to);
								}
								to->mark_to_activate();
								if (_storeIn)
								{
									_storeIn->push_back(SafePtr<IModulesOwner>(to));
								}
								lastTO = to;
							}
						});
					}
					return lastTO;
				}
				else
				{
					if (auto* to = subWorld->get_one_for(_to->get_type(), room))
					{
#ifdef MEASURE_TO_TIMES
						MEASURE_AND_OUTPUT_PERFORMANCE(TXT("setup (non socket) \"%S\""), _to->get_type()->get_name().to_string().to_char());
#endif
						to->set_creator(_owner);
						to->set_instigator(_owner);
						copy_variables(_owner, to, _to->copyVariables, _to->variables, _to->modulesAffectedByVariables);
						relativePlacement = relativePlacement.to_world(_to->ready_placement(&_spawnContext.rg));
						Transform placement = _spawnContext.placement.get(_owner->get_presence()->get_placement()).to_world(relativePlacement);
						if (((_socket.is_set() && _socket.get().is_valid()) || _to->socket.is_valid()) && !does_have_companions_to_spawn(to))
						{
							to->on_activate_place_at(_owner, _socket.is_set() ? _socket.get() : _to->socket, relativePlacement);
							cant_spawn_companions_of(to);
						}
						else
						{
							to->on_activate_place_in(room, placement);
#ifdef MEASURE_TO_TIMES
							MEASURE_AND_OUTPUT_PERFORMANCE(TXT("spawn_companions_of \"%S\""), _to->get_type()->get_name().to_string().to_char());
#endif
							spawn_companions_of(_owner, to, _storeIn, TemporaryObjectSpawnContext(room, placement));
						}
						{
#ifdef MEASURE_TO_TIMES
							MEASURE_AND_OUTPUT_PERFORMANCE(TXT("mark_to_activate \"%S\""), _to->get_type()->get_name().to_string().to_char());
#endif
							to->mark_to_activate();
						}
						if (_storeIn)
						{
							_storeIn->push_back(SafePtr<IModulesOwner>(to));
						}
						return to;
					}
				}
			}
		}
	}
	return nullptr;
}

void ModuleTemporaryObjects::copy_variables(IModulesOwner* _owner, TemporaryObject* to, Array<Name> const & _copyVariables, SimpleVariableStorage const & _setVariables, Array<Name> const& _modulesAffectedByVariables)
{
	if (!_copyVariables.is_empty() ||
		!_setVariables.is_empty())
	{
#ifdef MEASURE_TO_TIMES
		MEASURE_AND_OUTPUT_PERFORMANCE(TXT("copy_variables"));
#endif

		for_every(var, _copyVariables)
		{
			TypeId varType;
			void const* value;
			if (_owner->get_variables().get_existing_of_any_type_id(*var, varType, value))
			{
				RegisteredType::copy(varType, to->access_variables().access(*var, varType), value);
			}
		}
		for_every(varInfo, _setVariables.get_all())
		{
			RegisteredType::copy(varInfo->type_id(), to->access_variables().access(varInfo->get_name(), varInfo->type_id()), varInfo->get_raw());
		}
		to->resetup(_modulesAffectedByVariables);
	}
}

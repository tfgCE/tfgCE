#include "door.h"

#include "doorInRoom.h"
#include "world.h"
#include "room.h"
#include "presenceLink.h"

#include "..\advance\advanceContext.h"
#include "..\debug\debugVisualiserUtils.h"
#include "..\debug\previewGame.h"
#include "..\game\game.h"
#include "..\module\moduleCollision.h"
#include "..\object\object.h"

#include "..\..\core\debug\debugVisualiser.h"
#include "..\..\core\performance\performanceUtils.h"
#include "..\..\core\vr\vrZone.h"

#include "..\library\library.h"
#include "..\library\usedLibraryStored.inl"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

//

#ifdef AN_DEVELOPMENT
	#define CATCH_NESTED_VR_CORRIDORS
	#define AN_DEBUG_DOOR_TYPE_REPLACER

	//#define OUTPUT_GENERATION_DEBUG_VISUALISER
	#define ALLOW_OUTPUT_GENERATION_DEBUG_VISUALISER
	//#define ALLOW_OUTPUT_GENERATION_DEBUG_VISUALISER_DETAILED
	//#define DEBUG_VISUALISE_LOCAL
#else
	#ifdef AN_PROFILER
		//#define AN_DEBUG_DOOR_TYPE_REPLACER

		//#define OUTPUT_GENERATION_DEBUG_VISUALISER
		//#define ALLOW_OUTPUT_GENERATION_DEBUG_VISUALISER
		//#define ALLOW_OUTPUT_GENERATION_DEBUG_VISUALISER_DETAILED
	#endif
#endif

#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
	#ifndef ALLOW_OUTPUT_GENERATION_DEBUG_VISUALISER
		#define ALLOW_OUTPUT_GENERATION_DEBUG_VISUALISER
	#endif
#endif

#ifdef AN_ALLOW_EXTENSIVE_LOGS
	#ifdef AN_OUTPUT_WORLD_GENERATION
		#define OUTPUT_GENERATION
	#endif
	#ifdef ALLOW_DETAILED_DEBUG
		//#define INSPECT_DOOR_NOMINAL_SIZE
	#endif
#endif

#ifndef INVESTIGATE_SOUND_SYSTEM
	#ifndef BUILD_PUBLIC_RELEASE
		#ifndef DEBUG_WORLD_LIFETIME
			#define DEBUG_WORLD_LIFETIME
		#endif
	#endif
#endif

//

DEFINE_STATIC_NAME(avoidElevators);

// room tags
DEFINE_STATIC_NAME(vrCorridor);
DEFINE_STATIC_NAME(vrCorridorNested);
DEFINE_STATIC_NAME(vrCorridorUsing180);

// door tags
DEFINE_STATIC_NAME(disallow180);

// generation issues
DEFINE_STATIC_NAME(door_notSureAboutThisOne);

// shader param
DEFINE_STATIC_NAME(doorHoleCentre);

//

void DoorSoundsPlayer::stop()
{
	if (openingSoundSource.is_set() && openingSoundSource->is_active())
	{
		openingSoundSource->stop();
		openingSoundSource = nullptr;
	}
	if (closingSoundSource.is_set() && closingSoundSource->is_active())
	{
		closingSoundSource->stop();
		closingSoundSource = nullptr;
	}
}

void DoorSoundsPlayer::advance(Door* _door, DoorSounds const& _sounds, float _openTarget, float _openFactor, Transform const& _relPlacement)
{
	if (!_door->can_be_closed())
	{
		return;
	}

	if (_door->mute || !advancedOnce)
	{
		stop();
	}
	else
	{
		// play start sounds, check if we moved and if we were at start/close location
		if (prevOpenFactor != _openFactor)
		{
			if (_openTarget == 0.0f && abs(prevOpenFactor) == 1.0f)
			{
				if (auto* sample = _sounds.get_close_sound_sample())
				{
					_door->sounds.play(sample, _door, _relPlacement);
				}
			}
			else if (abs(_openTarget) == 1.0f && prevOpenFactor == 0.0f)
			{
				if (auto* sample = _sounds.get_open_sound_sample())
				{
					_door->sounds.play(sample, _door, _relPlacement);
				}
			}
		}

		// play sounds for reaching the target (only when reached the target!)
		if (_openTarget == _openFactor &&
			_openFactor != prevOpenFactor)
		{
			if (_openTarget == 0.0f)
			{
				if (auto* sample = _sounds.get_closed_sound_sample())
				{
					_door->sounds.play(sample, _door, _relPlacement);
				}
			}
			else if (abs(_openTarget) == 1.0f)
			{
				if (auto* sample = _sounds.get_opened_sound_sample())
				{
					_door->sounds.play(sample, _door, _relPlacement);
				}
			}
		}

		// stop looping sounds if we're at the target
		if (_openTarget == _openFactor)
		{
			if (openingSoundSource.is_set() && openingSoundSource->is_active() && openingSoundSource->get_sample_instance().get_sample()->get_sample()->is_looped())
			{
				openingSoundSource->stop();
				openingSoundSource = nullptr;
			}
			if (closingSoundSource.is_set() && closingSoundSource->is_active() && closingSoundSource->get_sample_instance().get_sample()->get_sample()->is_looped())
			{
				closingSoundSource->stop();
				closingSoundSource = nullptr;
			}
		}
		// otherwise, check if we should play loop sounds (to fit what we're doing)
		else if (_openFactor != prevOpenFactor)
		{
			if (abs(_openFactor) > abs(prevOpenFactor))
			{
				// opening
				// stop closing sound, play opening
				if (closingSoundSource.is_set() && closingSoundSource->is_active())
				{
					closingSoundSource->stop();
					closingSoundSource = nullptr;
				}
				if (!openingSoundSource.is_set() || !openingSoundSource->is_active())
				{
					if (auto* sample = _sounds.get_opening_sound_sample())
					{
						openingSoundSource = _door->sounds.play(sample, _door, _relPlacement);
					}
				}
			}
			else
			{
				// closing
				// stop opening sounds, play closing
				if (openingSoundSource.is_set() && openingSoundSource->is_active())
				{
					openingSoundSource->stop();
					openingSoundSource = nullptr;
				}
				if (!closingSoundSource.is_set() || !closingSoundSource->is_active())
				{
					if (auto* sample = _sounds.get_closing_sound_sample())
					{
						closingSoundSource = _door->sounds.play(sample, _door, _relPlacement);
					}
				}
			}
		}
	}

	advancedOnce = true;
	prevOpenTarget = _openTarget;
	prevOpenFactor = _openFactor;
	prevMute = _door->mute;
}

//

REGISTER_FOR_FAST_CAST(Door);

// [demo hack]
bool Door::demoHackLockAutoDoors = false;
bool Door::hackCloseAllDoor = false;

Optional<float> Door::nominalDoorWidth;
float Door::nominalDoorHeight = Door::get_default_nominal_door_height();

Transform Door::reverse_door_placement(Transform const & _doorPlacement)
{
	scoped_call_stack_info(TXT("Door::reverse_door_placement"));
	Matrix44 doorPlacementMatrix = _doorPlacement.to_matrix();
	Matrix44 doorPlacementRevMatrix = matrix_from_axes_orthonormal_check(-doorPlacementMatrix.get_x_axis(), -doorPlacementMatrix.get_y_axis(), doorPlacementMatrix.get_z_axis(), doorPlacementMatrix.get_translation());
	return doorPlacementRevMatrix.to_transform();
}

Door::Door(SubWorld* _inSubWorld, DoorType const * _doorType)
: SafeObject<Door>(this)
, inSubWorld(nullptr)
, individual(Random::get_float(0.0f, 1.0f))
, doorTypeForced(nullptr)
, doorTypeOriginal(_doorType)
, doorType(_doorType)
, secondaryDoorType(nullptr)
, blocksEnvironmentPropagation(false)
, linkedDoorA(nullptr)
, linkedDoorB(nullptr)
, openTarget(0.0f)
, openFactor(0.0f)
, openTime(0.0f)
{
#ifdef DEBUG_WORLD_LIFETIME
	output(TXT("Door [new] d%p"), this);
#endif

	an_assert(doorType);
	an_assert(_inSubWorld && _inSubWorld->get_in_world());
	_inSubWorld->get_in_world()->add(this);
	_inSubWorld->add(this);

	on_new_door_type();
}

Door::~Door()
{
#ifdef DEBUG_WORLD_LIFETIME
	output(TXT("Door [delete] d%p"), this);
#endif

	make_safe_object_unavailable();
	if (linkedDoorA)
	{
		linkedDoorA->unlink();
	}
	if (linkedDoorB)
	{
		linkedDoorB->unlink();
	}
	if (inSubWorld)
	{
		inSubWorld->remove(this);
	}
	if (auto* inWorld = get_in_world())
	{
		inWorld->remove(this);
	}
}

void Door::set_mute(bool _mute)
{
	mute = _mute;
}

void Door::on_connection()
{
#ifdef AN_DEVELOPMENT
	if (is_world_active())
	{
		ASSERT_SYNC;
	}
#endif
	if (linkedDoorA && linkedDoorB)
	{
		update_type();
		linkedDoorA->on_connection_through_door_created();
		linkedDoorB->on_connection_through_door_created();
	}
	if (is_important_vr_door())
	{
		if (linkedDoorA && linkedDoorB)
		{
			if (linkedDoorA->is_vr_space_placement_set())
			{
				if (!linkedDoorB->is_vr_space_placement_set())
				{
					scoped_call_stack_info(TXT("Door::on_connection A=vr B!vr"));
					DOOR_IN_ROOM_HISTORY(linkedDoorB, TXT("set vr placement [door, on connection, from A]"));
					linkedDoorB->set_vr_space_placement(Door::reverse_door_placement(linkedDoorA->get_vr_space_placement()));
				}
				if (linkedDoorA->is_vr_placement_immovable())
				{
					linkedDoorB->make_vr_placement_immovable();
				}
			}
			else if (linkedDoorB->is_vr_space_placement_set())
			{
				if (!linkedDoorA->is_vr_space_placement_set())
				{
					an_assert_log_always(linkedDoorB->is_vr_space_placement_set(), TXT("we assume that B has vr placement set, a bold guess I must say!"));
					scoped_call_stack_info(TXT("Door::on_connection A!vr B?vr"));
					DOOR_IN_ROOM_HISTORY(linkedDoorA, TXT("set vr placement [door, on connection, from B]"));
					linkedDoorA->set_vr_space_placement(Door::reverse_door_placement(linkedDoorB->get_vr_space_placement()));
				}
				if (linkedDoorB->is_vr_placement_immovable())
				{
					linkedDoorA->make_vr_placement_immovable();
				}
			}
		}
		an_assert_log_always(!linkedDoorA || linkedDoorA->is_vr_space_placement_set());
		an_assert_log_always(!linkedDoorB || linkedDoorB->is_vr_space_placement_set());
	}
#ifdef AN_DEVELOPMENT
	dev_check_if_ok();
#endif
}

void Door::link(DoorInRoom* _dir)
{
#ifdef AN_DEVELOPMENT
	if (is_world_active())
	{
		ASSERT_SYNC;
	}
#endif
	an_assert(linkedDoorA == nullptr || linkedDoorB == nullptr, TXT("there should be at least one door available"));
	if (linkedDoorA == nullptr)
	{
		DOOR_IN_ROOM_HISTORY(_dir, TXT("linked as A (genlink) to %p"), this);
		linkedDoorA = _dir;
		linkedDoorA->update_movement_info();
	}
	else if (linkedDoorB == nullptr)
	{
		DOOR_IN_ROOM_HISTORY(_dir, TXT("linked as B (genlink) to %p"), this);
		linkedDoorB = _dir;
		linkedDoorB->update_movement_info();
	}
	else
	{
		an_assert(false, TXT("no place to link"));
	}
	if (!_dir->is_linked(this))
	{
		an_assert(_dir->get_door() == nullptr);
		_dir->link(this);
	}
	on_connection();
}

void Door::link_a(DoorInRoom* _dir)
{
#ifdef AN_DEVELOPMENT
	if (is_world_active())
	{
		ASSERT_SYNC;
	}
#endif
	an_assert(linkedDoorA == nullptr, TXT("already connected!"));
	DOOR_IN_ROOM_HISTORY(_dir, TXT("linked as A (direct) to %p"), this);
	linkedDoorA = _dir;
	linkedDoorA->update_movement_info();
	if (!_dir->is_linked(this))
	{
		an_assert(_dir->get_door() == nullptr);
		_dir->link_as_a(this);
	}
	on_connection();
}

void Door::link_b(DoorInRoom* _dir)
{
#ifdef AN_DEVELOPMENT
	if (is_world_active())
	{
		ASSERT_SYNC;
	}
#endif
	an_assert(linkedDoorB == nullptr, TXT("already connected!"));
	DOOR_IN_ROOM_HISTORY(_dir, TXT("linked as B (direct) to %p"), this);
	linkedDoorB = _dir;
	linkedDoorB->update_movement_info();
	if (!_dir->is_linked(this))
	{
		an_assert(_dir->get_door() == nullptr);
		_dir->link_as_b(this);
	}
	on_connection();
}

void Door::unlink(DoorInRoom* _dir)
{
#ifdef AN_DEVELOPMENT
	if (is_world_active())
	{
		ASSERT_SYNC_OR(! get_in_world() || get_in_world()->is_being_destroyed());
	}
#endif
	an_assert(linkedDoorA == _dir || linkedDoorB == _dir, TXT("should not unlink door in room that is not linked"));
	if (linkedDoorA)
	{
		linkedDoorA->on_connection_through_door_broken();
	}
	if (linkedDoorB)
	{
		linkedDoorB->on_connection_through_door_broken();
	}
	if (linkedDoorA == _dir)
	{
		DOOR_IN_ROOM_HISTORY(_dir, TXT("unlinked as A from %p"), this);
		linkedDoorA = nullptr;
	}
	if (linkedDoorB == _dir)
	{
		DOOR_IN_ROOM_HISTORY(_dir, TXT("unlinked as B from %p"), this);
		linkedDoorB = nullptr;
	}
	if (_dir->is_linked(this))
	{
		_dir->unlink();
	}
}

void Door::set_initial_operation(Optional<DoorOperation::Type> const& _operation)
{
	if (initialOperation == _operation)
	{
		return;
	}
	initialOperation = _operation;
	on_new_door_type();
}

void Door::set_operation(DoorOperation::Type _operation, DoorOperationParams const & _params)
{
	Concurrency::ScopedSpinLock lock(operationLock, true);
	DoorOperation::Type newOperation = _operation;
	if (_params.allowAuto.get(false))
	{
		if (auto* dt = get_door_type())
		{
			if (dt->get_operation() == DoorOperation::Auto)
			{
				newOperation = DoorOperation::Auto;
			}
		}
	}

	if (! operationLocked || _params.forceOperation.get(false))
	{
		operation = newOperation;
	}
	operationIfUnlocked = newOperation;
}

void Door::set_operation_lock(bool _operationLock)
{
	Concurrency::ScopedSpinLock lock(operationLock, true);
	bool wasLocked = operationLocked;
	operationLocked = _operationLock;
	if (wasLocked && !operationLocked && operationIfUnlocked.is_set())
	{
		set_operation(operationIfUnlocked.get());
	}
}

void Door::set_auto_operation_distance(Optional<float> const& _autoOperationDistance)
{
	Concurrency::ScopedSpinLock lock(operationLock, true);
	autoOperationDistance = _autoOperationDistance;
}

void Door::advance__operation(IMMEDIATE_JOB_PARAMS)
{
	FOR_EVERY_IMMEDIATE_JOB_DATA(Door, self)
	{
		an_assert(self->doorType != nullptr);
		DoorOperation::Type operation = self->get_operation();
		if (hackCloseAllDoor && self->doorType->can_be_closed())
		{
			self->close();
		}
		else if (operation == DoorOperation::StayOpen || operation == DoorOperation::Unclosable)
		{
			self->open();
		}
		else if (operation == DoorOperation::StayClosed ||
				 operation == DoorOperation::StayClosedTemporarily)
		{
			self->close();
		}
		else if (operation == DoorOperation::Auto ||
				 operation == DoorOperation::AutoEagerToClose)
		{
			Optional<float> autoCloseDelay;
			if (!autoCloseDelay.is_set() && self->doorType->get_auto_close_delay().is_set())
			{
				autoCloseDelay = self->doorType->get_auto_close_delay().get();
			}
			if (!autoCloseDelay.is_set() && self->secondaryDoorType && self->secondaryDoorType->get_auto_close_delay().is_set())
			{
				autoCloseDelay = self->secondaryDoorType->get_auto_close_delay().get();
			}
			autoCloseDelay.set_if_not_set(1.0f);
			if (operation == DoorOperation::AutoEagerToClose)
			{
				autoCloseDelay = min(0.05f, autoCloseDelay.get());
			}
			if (!self->is_opening() && (!self->is_open() || self->openTime >= autoCloseDelay.get()))
			{
				bool openNow = false;
				bool isOpen = self->openFactor > 0.0f; // anything will keep it open
				Room const* openInto = nullptr;
				for (int side = 0; side < 2; ++side)
				{
					if (demoHackLockAutoDoors)
					{
						continue;
					}
					if (DoorInRoom const* doorInRoom = side == 0 ? self->linkedDoorA : self->linkedDoorB)
					{
						an_assert(doorInRoom->get_in_room() != nullptr);
						if (Room const* inRoom = doorInRoom->get_in_room())
						{
							PresenceLink const* presenceLink = inRoom->get_presence_links();
							while (presenceLink)
							{
								if (IModulesOwner const* modulesOwner = presenceLink->get_modules_owner())
								{
									scoped_call_stack_ptr(modulesOwner);
									// temporary objects should not open doors
									if (auto* asObject = modulesOwner->get_as_object())
									{
										if (ModuleCollision const* objectCollision = modulesOwner->get_collision())
										{
											float distance = self->autoOperationDistance.get(self->doorType->get_auto_operation_distance());
											if (operation == DoorOperation::AutoEagerToClose)
											{
												distance = 0.01f; // one centimeter + collision primitive radius
											}
											if (objectCollision->is_collision_in_radius(doorInRoom, presenceLink->get_placement_in_room(), distance))
											{
												if (auto* r = doorInRoom->get_world_active_room_on_other_side())
												{
													bool ok = isOpen || self->doorType->get_auto_operation_tag_condition().is_empty();
													if (!ok)
													{
														if (self->doorType->get_auto_operation_tag_condition().check(asObject->get_tags()))
														{
															ok = true;
														}
													}
													if (ok)
													{
														openNow = true;
														openInto = r;
														break;
													}
												}
											}
										}
									}
								}
								presenceLink = presenceLink->get_next_in_room();
							}
						}
					}
				}
				if (openNow)
				{
					self->open_into(openInto);
				}
				else
				{
					self->close();
				}
			}
		}
	}
}

bool Door::is_new_placement_requested() const
{
	return (linkedDoorA && linkedDoorA->is_new_placement_requested()) ||
		   (linkedDoorB && linkedDoorB->is_new_placement_requested());
}

void Door::advance__physically(IMMEDIATE_JOB_PARAMS)
{
	FOR_EVERY_IMMEDIATE_JOB_DATA(Door, self)
	{
		if (self->visiblePending.is_set())
		{
			self->visible = self->visiblePending.get();
			self->visiblePending.clear();
		}
		an_assert(self->doorType != nullptr);
		if (self->linkedDoorA && self->linkedDoorA->is_new_placement_requested())
		{
			self->linkedDoorA->update_placement_to_requested();
		}
		if (self->linkedDoorB && self->linkedDoorB->is_new_placement_requested())
		{
			self->linkedDoorB->update_placement_to_requested();
		}
		AdvanceContext const* ac = plain_cast<AdvanceContext>(_executionContext);
		float prevOpenFactor = self->openFactor;
		if (! self->can_be_closed())
		{
			// no need to advance
			self->openFactor = self->openTarget = 1.0f;
		}
		else
		{
			if (self->prevOpenTarget != self->openTarget)
			{
				self->prevOpenTarget = self->openTarget;
			}
			float const absOpenFactor = abs(self->openFactor);
			float const absOpenTarget = abs(self->openTarget);
			bool const isOpening = absOpenFactor < absOpenTarget;
			float const operationTime =
				(self->doorType->has_wings() || !self->secondaryDoorType)
				? (isOpening ? self->doorType->get_wings_opening_time() : self->doorType->get_wings_closing_time())
				: (isOpening ? self->secondaryDoorType->get_wings_opening_time() : self->secondaryDoorType->get_wings_closing_time());
			float const diffLeftPct = abs(absOpenTarget - absOpenFactor); // it's automatically pct as length is 1.0
			float const advancePct = operationTime != 0.0f ? ac->get_delta_time() / operationTime : 1.0f;
			if (advancePct > diffLeftPct || operationTime == 0.0f)
			{
				self->openFactor = self->openTarget;
			}
			else
			{
				an_assert(operationTime != 0.0f);
				float const operationSpeed = 1.0f / operationTime;
				float const operationDir = self->openTarget > self->openFactor ? 1.0f : -1.0f;
				self->openFactor += operationDir * operationSpeed * ac->get_delta_time();
			}
		}
		if (ac->get_delta_time() != 0.0f)
		{
			self->soundsPlayer.advance(self, self->doorType->get_sounds(), self->openTarget, self->openFactor);
			if (self->secondaryDoorType)
			{
				self->soundsPlayerForSecondaryDoorType.advance(self, self->secondaryDoorType->get_sounds(), self->openTarget, self->openFactor);
			}
			if (self->doorType->has_wings())
			{
				auto& wings = self->doorType->get_wings();
				auto* wing = wings.begin_const();
				auto* wingSoundsPlayer = self->wingSoundsPlayer.begin();
				for_count(int, idx, min(wings.get_size(), self->wingSoundsPlayer.get_size()))
				{
					float mappedOpenFactor = clamp(wing->mapOpenPt.get_pt_from_value(abs(self->openFactor)), 0.0f, 1.0f) * sign(self->openFactor);
					wingSoundsPlayer->advance(self, wing->get_sounds(), self->openTarget, mappedOpenFactor, wing->placement);
					++wing;
					++wingSoundsPlayer;
				}
			}
		}
		if (self->holeTimeScale != self->holeTimeScaleTarget)
		{
			todo_note(TXT("if hole scaling is part of closing door, use opening/closing time or even let hole scale to be from openFactor"))
			AdvanceContext const * ac = plain_cast<AdvanceContext>(_executionContext);
			self->holeTimeScale = blend_to_using_speed_based_on_time(self->holeTimeScale, self->holeTimeScaleTarget, self->holeTimeScaleTime, ac->get_delta_time());
		}
		if (self->doorTimeScale != self->doorTimeScaleTarget)
		{
			AdvanceContext const * ac = plain_cast<AdvanceContext>(_executionContext);
			self->doorTimeScale = blend_to_using_speed_based_on_time(self->doorTimeScale, self->doorTimeScaleTarget, self->doorTimeScaleTime, ac->get_delta_time());
			if (self->linkedDoorA)
			{
				self->linkedDoorA->update_scaled_placement_matrix();
			}
			if (self->linkedDoorB)
			{
				self->linkedDoorB->update_scaled_placement_matrix();
			}
		}
		if (prevOpenFactor != self->openFactor)
		{
			if (self->linkedDoorA)
			{
				self->linkedDoorA->update_wings();
			}
			if (self->linkedDoorB)
			{
				self->linkedDoorB->update_wings();
			}
		}
		if (self->is_open())
		{
			self->openTime += plain_cast<AdvanceContext>(_executionContext)->get_delta_time();
		}
		else
		{
			self->openTime = 0.0f;
		}
	}
}

void Door::advance__finalise_frame(IMMEDIATE_JOB_PARAMS)
{
	AdvanceContext const * ac = plain_cast<AdvanceContext const>(_executionContext);
	FOR_EVERY_IMMEDIATE_JOB_DATA(Door, self)
	{
		MEASURE_PERFORMANCE(doorFinaliseFrame);

		float const deltaTime = ac->get_delta_time();

		self->sounds.advance(deltaTime);
	}
}

DoorOperation::Type Door::get_operation() const
{
	return operation.is_set() ? operation.get() : doorType->get_operation();
}

void Door::open_into(Room const * _room)
{
	open(((doorType && doorType->can_open_in_two_directions()) || (secondaryDoorType && secondaryDoorType->can_open_in_two_directions())) && linkedDoorB && linkedDoorB->get_in_room() == _room ? -1.0f : 1.0f);
}

void Door::close()
{
	openTarget = 0.0f;
}

void Door::open(float _side)
{
	openTarget = _side;
}

void Door::ready_to_world_activate()
{
	WorldObject::ready_to_world_activate();
	if (auto* dt = cast_to_nonconst(get_door_type()))
	{
		dt->load_data_on_demand_if_required();
	}
}

void Door::world_activate()
{
	ActivationGroupPtr ag = ActivationGroupPtr(get_activation_group());
	WorldObject::world_activate();
	if (auto* inWorld = get_in_world())
	{
		inWorld->activate(this);
	}
	if (inSubWorld)
	{
		inSubWorld->activate(this);
	}
	world_activate_linked_door_in_rooms(ag.get());
}

void Door::world_activate_linked_door_in_rooms(ActivationGroup* _ag)
{
	if (linkedDoorA && linkedDoorA->does_belong_to(_ag))
	{
		linkedDoorA->world_activate();
	}
	if (linkedDoorB && linkedDoorB->does_belong_to(_ag))
	{
		linkedDoorB->world_activate();
	}
}

void Door::scale_door_time(float _scale, float _scaleTime)
{
	doorTimeScaleTarget = _scale;
	doorTimeScaleTime = _scaleTime;
	if (_scaleTime == 0.0f)
	{
		doorTimeScale = doorTimeScaleTarget;
		if (linkedDoorA)
		{
			linkedDoorA->update_scaled_placement_matrix();
		}
		if (linkedDoorB)
		{
			linkedDoorB->update_scaled_placement_matrix();
		}
	}
}

void Door::scale_hole_time(float _scale, float _scaleTime)
{
	holeTimeScaleTarget = _scale;
	holeTimeScaleTime = _scaleTime;
	if (_scaleTime == 0.0f)
	{
		holeTimeScale = holeTimeScaleTarget;
	}
}

float Door::get_door_vr_corridor_priority(Random::Generator& _rg, float _default) const
{
	float temp;
	if (doorVRCorridor.get_priority(_rg, temp)) return temp;
	if (doorType)
	{
		float result = 0.0f;
		if (doorType->get_door_vr_corridor_priority(_rg, OUT_ result))
		{
			return result;
		}
	}
	return _default;
}

RoomType const* Door::get_door_vr_corridor_room_room_type(Random::Generator& _rg) const
{
	RoomType const* result = doorVRCorridor.get_room(_rg);
	if (!result && doorType)
	{
		result = doorType->get_door_vr_corridor_room_room_type(_rg);
	}
	return result;
}

RoomType const* Door::get_door_vr_corridor_elevator_room_type(Random::Generator& _rg) const
{
	RoomType const* result = doorVRCorridor.get_elevator(_rg);
	if (!result && doorType)
	{
		result = doorType->get_door_vr_corridor_elevator_room_type(_rg);
	}
	return result;
}

DoorInRoom* Door::change_into_room(RoomType const * _roomType, Room * _towardsRoom, DoorType const * _changeDoorToTowardsRoomToDoorType, tchar const * _proposedName)
{
	if (Game::get()->does_want_to_cancel_creation())
	{
		// skip this
		return nullptr;
	}

#ifdef OUTPUT_GENERATION
	output(TXT("change into room d%p"), this);
#endif

	Room* newRoom;
	Door* secondDoor;
	DoorInRoom* thisDIR;
	DoorInRoom* secondDIR;

	Room* baseOnRoom = _towardsRoom;

	Random::Generator nrg = _towardsRoom->get_individual_random_generator();
	nrg.advance_seed(23974, 29234);

	bool passWorldSeparatorMarkToSecondDoor = false;

	// if we separate a room from the world and that room is towards which room we change into room, we should pass being world separator there
	if (is_world_separator_door())
	{
		auto* r = get_room_separated_from_world();
		if (r)
		{
#ifdef OUTPUT_GENERATION
			output(TXT("d%p separates world from r%p"), this, r);
#endif
			if (r == _towardsRoom)
			{
#ifdef OUTPUT_GENERATION
				output(TXT("as we want to grow in that dir, we should pass world separator to the new door"));
#endif
				passWorldSeparatorMarkToSecondDoor = true;
				auto* da = get_linked_door_a();
				auto* db = get_linked_door_b();
				if (da && db &&
					da->get_in_room() &&
					db->get_in_room())
				{
					if (da->get_in_room() == _towardsRoom)
					{
						// we want to grow towards room we separate from, make new room belong to the other room
						baseOnRoom = db->get_in_room();
					}
					else if (db->get_in_room() == _towardsRoom)
					{
						// as above
						baseOnRoom = da->get_in_room();
					}
				}
			}
		}
	}

	an_assert(baseOnRoom == _towardsRoom, TXT("the whole thing was created with growing towards room that we're not separating world from, make sure this thing works for other case, as this one we just caught with this very assert"));

	// create new room
	if (get_activation_group())
	{
		Game::get()->push_activation_group(get_activation_group());
	}
	Game::get()->perform_sync_world_job(TXT("change into room"), [&newRoom, &secondDoor, &thisDIR, &secondDIR, this, _roomType, baseOnRoom, _changeDoorToTowardsRoomToDoorType, nrg, passWorldSeparatorMarkToSecondDoor]()
	{
		newRoom = new Room(baseOnRoom->get_in_sub_world(), baseOnRoom->get_in_region(), _roomType, nrg);
		ROOM_HISTORY(newRoom, TXT("change door into room"));
#ifdef OUTPUT_GENERATION
		output(TXT("new room on change into room r%p"), newRoom);
#endif
		Door::set_vr_corridor_tags(newRoom, baseOnRoom, 1);
		if (auto* ld = get_linked_door_a())
		{
			Door::set_vr_corridor_tags(newRoom, ld->get_in_room(), 1);
		}
		if (auto* ld = get_linked_door_b())
		{
			Door::set_vr_corridor_tags(newRoom, ld->get_in_room(), 1);
		}

		// create second door - copy of this one
		secondDoor = new Door(baseOnRoom->get_in_sub_world(), _changeDoorToTowardsRoomToDoorType ? _changeDoorToTowardsRoomToDoorType : get_door_type());
#ifdef OUTPUT_GENERATION
		output(TXT("new door on change into room d%p"), secondDoor);
#endif
		thisDIR = new DoorInRoom(newRoom);
		secondDIR = new DoorInRoom(newRoom);
		DOOR_IN_ROOM_HISTORY(thisDIR, TXT("changing door into room"));
		DOOR_IN_ROOM_HISTORY(secondDIR, TXT("changing door into room"));

		if (passWorldSeparatorMarkToSecondDoor)
		{
			pass_being_world_separator_to(secondDoor);
		}
	});

	if (get_activation_group())
	{
		Game::get()->pop_activation_group(get_activation_group());
	}

	// set origin room before doors are linked - during update_type we need to have origin room to get the right door type replacer
	newRoom->set_origin_room(baseOnRoom->get_origin_room());

	DoorInRoom* dirInNewRoom = nullptr;

	an_assert(get_linked_door_a());
	an_assert(get_linked_door_b());
	DoorInRoom* da = get_linked_door_a();
	DoorInRoom* db = get_linked_door_b();
	Game::get()->perform_sync_world_job(TXT("link doors"), [da, db, this, _towardsRoom, secondDoor, &dirInNewRoom, thisDIR, secondDIR]()
	{
		if (da->get_in_room() == _towardsRoom)
		{
			// oldRoom= db -this- da =towardsRoom
			// oldRoom= db -this- thisDIR =newRoom= secondDIR -secondDoor- da =towardsRoom
			DOOR_IN_ROOM_HISTORY(da, TXT("[change into room] da unlink from %p"), this);
			unlink(da);
			DOOR_IN_ROOM_HISTORY(da, TXT("[change into room] da link to %p"), secondDoor);
			da->link_as_a(secondDoor);
			dirInNewRoom = da;
			DOOR_IN_ROOM_HISTORY(thisDIR, TXT("[change into room] td link to %p"), this);
			thisDIR->link_as_a(this);
			DOOR_IN_ROOM_HISTORY(secondDIR, TXT("[change into room] sd link to %p"), secondDoor);
			secondDIR->link_as_b(secondDoor);
		}
		else
		{
			// oldRoom= da -this- db =towardsRoom
			// oldRoom= da -this- thisDIR =newRoom= secondDIR -secondDoor- db =towardsRoom
			DOOR_IN_ROOM_HISTORY(db, TXT("[change into room] db unlink from %p"), this);
			unlink(db);
			DOOR_IN_ROOM_HISTORY(db, TXT("[change into room] db link to %p"), secondDoor);
			db->link_as_b(secondDoor);
			dirInNewRoom = db;
			DOOR_IN_ROOM_HISTORY(thisDIR, TXT("[change into room] td link to %p"), this);
			thisDIR->link_as_b(this);
			DOOR_IN_ROOM_HISTORY(secondDIR, TXT("[change into room] sd link to %p"), secondDoor);
			secondDIR->link_as_a(secondDoor);
		}
	});

	thisDIR->init_meshes();
	secondDIR->init_meshes();

	if (auto* g = Game::get())
	{
		g->on_newly_created_door_in_room(thisDIR);
		g->on_newly_created_door_in_room(secondDIR);
	}

	// no vr placement, no placement, just simple room to be filled

	if (_proposedName)
	{
		newRoom->set_name(String(_proposedName));
	}
	else
	{
		static int idx = 0;
		++idx;
		String roomName = String::printf(TXT("changed into room %i"), idx);
		newRoom->set_name(roomName);
	}

	return dirInNewRoom;
}

void Door::be_important_vr_door()
{
	if (linkedDoorA && linkedDoorB)
	{
		if (!linkedDoorA->is_vr_space_placement_set())
		{
			error(TXT("linked door A is not vr placed"));
			an_assert(false);
		}
		if (!linkedDoorB->is_vr_space_placement_set())
		{
			error(TXT("linked door A is not vr placed"));
			an_assert(false);
		}
		scoped_call_stack_info(TXT("be_important_vr_door"));
		if (!DoorInRoom::same_with_orientation_for_vr(linkedDoorA->get_vr_space_placement(), Door::reverse_door_placement(linkedDoorB->get_vr_space_placement()), 0.005f))
		{
			error(TXT("linked door A and B do not match placement"));
			an_assert(false);
		}
	}
	importantVRDoor = true;
}

void Door::be_world_separator_door(Optional<int> const& _worldSeparatorReason, Room* _separatesRoomFromWorld)
{
	worldSeparatorDoor = true;
	worldSeparatorReason = _worldSeparatorReason.get(0);
	separatesRoomFromWorld = _separatesRoomFromWorld;
#ifdef OUTPUT_GENERATION
	output(TXT("door d%p became world separator door"), this);
#endif
}

void Door::pass_being_world_separator_to(Door* _otherDoor)
{
	an_assert(is_world_separator_door());
	an_assert(! _otherDoor->is_world_separator_door());
#ifdef OUTPUT_GENERATION
	output(TXT("door d%p passed being world separator door to d%p"), this, _otherDoor);
#endif
	_otherDoor->be_world_separator_door(get_world_separator_reason(), get_room_separated_from_world());
	worldSeparatorDoor = false;
	worldSeparatorReason = 0;
	separatesRoomFromWorld.clear();
}

void Door::change_into_vr_corridor(VR::Zone const & _beInZone, float _tileSize)
{
	an_assert(GameStaticInfo::get()->is_sensitive_to_vr_anchor());

	if (!GameStaticInfo::get()->is_sensitive_to_vr_anchor())
	{
		return;
	}

	scoped_call_stack_info(TXT("change into vr corridor"));

	an_assert(!importantVRDoor);
	if (importantVRDoor)
	{
		error(TXT("should not be changed into vr corridor, both door-in-rooms should have the same vr placement!"));
	}
	if (Game::get()->does_want_to_cancel_creation())
	{
		// skip this
		return;
	}

#ifdef OUTPUT_GENERATION
	output(TXT("change into vr corridor d%p%S"), this, is_world_separator_door()? TXT(" world separator door") : TXT(""));
#endif

	scoped_call_stack_info(TXT("change into vr corridor [0]"));

	/**
	 *	Changes door into vr corridor, check make_vr_corridor for more info
	 */
	an_assert(get_linked_door_a());
	an_assert(get_linked_door_b());
	Room* corridor; // new room for corridor/elevator
	Door* secondDoor; // second door (new room requires two door, this one and new one = second door)
	DoorInRoom* corridorD0; // door-in-rooms for this one will remain in rooms where they are, although one will be moved to another door
	DoorInRoom* corridorD1; // these are door-in-rooms in this corridor

	DoorInRoom* da = get_linked_door_a();
	DoorInRoom* db = get_linked_door_b();

	DoorInRoom* dSource = da; // this will be source for room generators etc
	DoorInRoom* dOther = db; // may come handy when it comes to random generator
	float doorVRPriority = 0.0f;
	{
		scoped_call_stack_info(TXT("sort out doors"));
		// check which one has priority and how big it is
		Random::Generator rg = da->get_in_room()->get_individual_random_generator();
		rg.advance_seed(2358976, 5497);
		float daPriority = 0.0f;
		float dbPriority = 0.0f;
		if (da->get_in_room())
		{
			daPriority = da->get_in_room()->get_door_vr_corridor_priority(rg, daPriority);
		}
		daPriority = da->get_door()->get_door_vr_corridor_priority(rg, daPriority);
		if (db->get_in_room())
		{
			dbPriority = db->get_in_room()->get_door_vr_corridor_priority(rg, dbPriority);
		}
		dbPriority = db->get_door()->get_door_vr_corridor_priority(rg, dbPriority);
		if (daPriority > dbPriority)
		{
			dSource = da;
			dOther = db;
			doorVRPriority = daPriority;
		}
		else if (daPriority < dbPriority)
		{
			dSource = db;
			dOther = da;
			doorVRPriority = dbPriority;
		}
		else
		{
			dSource = rg.get_chance(0.5f) ? da : db;
			dOther = dSource == db? da : db;
			doorVRPriority = daPriority;
		}
	}

	scoped_call_stack_info(TXT("change into vr corridor [1]"));

	bool passWorldSeparatorMarkToSecondDoor = false;

	// if we separate a room from the world and that room is room behind dSource, choose other side as source and grow there
	if (is_world_separator_door())
	{
		auto* r = get_room_separated_from_world();
		if (r)
		{
#ifdef OUTPUT_GENERATION
			output(TXT("d%p separates world from r%p"), this, r);
#endif
			if (r == dSource->get_in_room())
			{
#ifdef OUTPUT_GENERATION
				output(TXT("don't grow in that direction"));
#endif
				// swapping is not enough as we will be still carrying the world separator mark, we should pass it to second door
				// this is going to happen later
				swap(dSource, dOther);
				passWorldSeparatorMarkToSecondDoor = true;
				{
					Random::Generator rg = da->get_in_room()->get_individual_random_generator();
					rg.advance_seed(8345, 702086);
					doorVRPriority = dSource->get_in_room()->get_door_vr_corridor_priority(rg, doorVRPriority);
				}
			}
			else
			{
				an_assert(r == dOther->get_in_room(), TXT("if we separate a door, one of our linked door-in-room should point at what we separate"));
				passWorldSeparatorMarkToSecondDoor = true;
			}
		}
	}

	scoped_call_stack_info(TXT("change into vr corridor [2]"));

	if (get_activation_group())
	{
		Game::get()->push_activation_group(get_activation_group());
	}
	Game::get()->perform_sync_world_job(TXT("create vr corridor"), [&corridor, &secondDoor, &corridorD0, &corridorD1, this, dSource, passWorldSeparatorMarkToSecondDoor]()
	{
		// make corridor exist in same region
		corridor = new Room(inSubWorld, dSource->get_in_room()->get_in_region(), nullptr, Random::Generator().be_zero_seed() /* will be replaced */);
		ROOM_HISTORY(corridor, TXT("change door into vr corridor"));
#ifdef OUTPUT_GENERATION
		output(TXT("new room on change into vr corridor r%p"), corridor);
#endif
		Door::set_vr_corridor_tags(corridor, dSource->get_in_room(), 1);
		// create second door - copy of this one
		secondDoor = new Door(inSubWorld, doorType);
#ifdef OUTPUT_GENERATION
		output(TXT("new door on change into vr corridor d%p"), secondDoor);
#endif
		corridorD0 = new DoorInRoom(corridor);
		corridorD1 = new DoorInRoom(corridor);
		DOOR_IN_ROOM_HISTORY(corridorD0, TXT("changing door into vr corridor"));
		DOOR_IN_ROOM_HISTORY(corridorD1, TXT("changing door into vr corridor"));

		if (passWorldSeparatorMarkToSecondDoor)
		{
			pass_being_world_separator_to(secondDoor);
		}
	});
	if (get_activation_group())
	{
		Game::get()->pop_activation_group(get_activation_group());
	}
	
	scoped_call_stack_info(TXT("change into vr corridor [3]"));

	{
		static int corridorIdx = 0;
		++corridorIdx;
		String corridorName = String::printf(TXT("vr corridor %i"), corridorIdx);
		corridor->set_name(corridorName);
	}

	scoped_call_stack_info(TXT("change into vr corridor [4]"));

	corridor->set_door_vr_corridor_priority(doorVRPriority);

	scoped_call_stack_info(TXT("change into vr corridor [5]"));

	Game::get()->perform_sync_world_job(TXT("link doors"), [da, db, secondDoor, corridorD0, corridorD1, this, dSource]()
	{
		bool a_second_corridor_this_b = dSource == db;
		if (a_second_corridor_this_b)
		{
			// (A)=(da) -a-this-b- (db)=(B)
			// (A)=(da) -a-second-b- (cd1)=(corridor)=(cd0) -a-this-b- (db)=(B)
			DOOR_IN_ROOM_HISTORY(da, TXT("[change into vr corridor] da unlink from d%p"), this);
			unlink(da);
			DOOR_IN_ROOM_HISTORY(da, TXT("[change into vr corridor] da link to d%p"), secondDoor);
			da->link_as_a(secondDoor);
			DOOR_IN_ROOM_HISTORY(corridorD0, TXT("[change into vr corridor] c0 link to d%p"), this);
			corridorD0->link_as_a(this);
			DOOR_IN_ROOM_HISTORY(corridorD1, TXT("[change into vr corridor] c1 link to d%p"), secondDoor);
			corridorD1->link_as_b(secondDoor);
		}
		else
		{
			// (A)=(da) -a-this-b- (db)=(B)
			// (A)=(da) -a-this-b- (cd0)=(corridor)=(cd1) -a-second-b- (db)=(B)
			DOOR_IN_ROOM_HISTORY(db, TXT("[change into vr corridor] db unlink from d%p"), this);
			unlink(db);
			DOOR_IN_ROOM_HISTORY(db, TXT("[change into vr corridor] db link to d%p"), secondDoor);
			db->link_as_b(secondDoor);
			DOOR_IN_ROOM_HISTORY(corridorD0, TXT("[change into vr corridor] c0 link to d%p"), this);
			corridorD0->link_as_b(this);
			DOOR_IN_ROOM_HISTORY(corridorD1, TXT("[change into vr corridor] c1 link to d%p"), secondDoor);
			corridorD1->link_as_a(secondDoor);
		}
	});

	scoped_call_stack_info(TXT("change into vr corridor [6]"));

	corridorD0->update_vr_placement_from_door_on_other_side();
	corridorD1->update_vr_placement_from_door_on_other_side();

	scoped_call_stack_info(TXT("change into vr corridor [7]"));

	if (corridorD0->get_door_on_other_side() == dOther)
	{
		// why do we keep this in order? should it matter?
		swap(corridorD0, corridorD1);
	}

	scoped_call_stack_info(TXT("change into vr corridor [8]"));

	// corridorD0 should be the source
	make_vr_corridor(corridor, corridorD0, corridorD1, _beInZone, _tileSize);
}

void Door::set_vr_corridor_tags(Room* _to, Room const* _from, int _nestedOffset)
{
	int vrCorridorNested = _to->get_tags().get_tag_as_int(NAME(vrCorridorNested));
	if (_from)
	{
		int nestedFrom = _from->get_tags().get_tag_as_int(NAME(vrCorridorNested)) + _nestedOffset;
		vrCorridorNested = max(vrCorridorNested, nestedFrom);
	}
	_to->access_tags().set_tag(NAME(vrCorridor));
	_to->access_tags().set_tag(NAME(vrCorridorNested), vrCorridorNested);
}

void Door::make_vr_corridor(Room* corridor, DoorInRoom* corridorD0, DoorInRoom* corridorD1, VR::Zone const& _beInZone, float _tileSize)
{
	an_assert(GameStaticInfo::get()->is_sensitive_to_vr_anchor());

	if (!GameStaticInfo::get()->is_sensitive_to_vr_anchor())
	{
		return;
	}

	scoped_call_stack_info(TXT("make vr corridor"));

#ifdef OUTPUT_GENERATION
	output(TXT("make vr corridor d0:d%p d1:d%p"), corridorD0->get_door(), corridorD1->get_door());
#endif

	/**
	 *	Makes vr corridor for two door-in-rooms (in the same room)
	 *
	 *	Creates room between two doors that would be used to move from one point in vr space to another.
	 *	If it is impossible to create such rooms, gets best solution and allows to create next vr corridor.
	 *
	 *	It handles two cases:
	 *		1.	(actual) corridor - both doors should see each other or at least be not too much in front of another
	 *			possible results:
	 *				|		 |								|
	 *				|		/		.--		.---.		.---'
	 *				|	   |		|		|	|		|
	 *		2. elevator - uses cart to traverse from one point in game world to another while player is stationary
	 *			when door spaces overlap each other and face one dir, it is better to create elevator
	 *			generator on its own decides if it should be vertical elevator, horizontal, turning etc
	 *
	 *	We do not use maxPlayAreaSize.
	 *	The more nested we are the more we use the whole space (_beInZone).
	 *	For first few nested tries we allow doing 180 for door's placement if that makes the door closer - this is to avoid long empty corridors.
	 */

	an_assert(corridorD0->get_in_room() == corridorD1->get_in_room());
	an_assert(!corridorD0->is_vr_placement_immovable() || !corridorD1->is_vr_placement_immovable());
	Door* d0 = corridorD0->get_door();
	Door* d1 = corridorD1->get_door();
	an_assert(!d0->is_world_separator_door() || !d1->is_world_separator_door(), TXT("how come we make vr corridor between two world separator doors?"));
	Door* mainD = d1->is_world_separator_door()? d1 : d0;
	// we prefer room that is not not beyond world separator door. if both are, then at least we may not have issues with a shape
	Room* sourceRoom = d0->is_world_separator_door()? corridorD1->get_room_on_other_side() : corridorD0->get_room_on_other_side();
	Room* otherRoom = d0->is_world_separator_door()? corridorD0->get_room_on_other_side() : corridorD1->get_room_on_other_side();
	Door* sourceDoor = d0->is_world_separator_door()? d1 : d0;
	Door* otherDoor = d0->is_world_separator_door()? d0 : d1;

	SimpleVariableStorage variables;
	Random::Generator baseRG;
	baseRG.set_seed(0, 0);
#ifdef OUTPUT_GENERATION
	output(TXT("make vr corridor, base rg %S"), baseRG.get_seed_string().to_char());
	output(TXT("d0: d%p dt[%S]%S"), d0,
		d0->get_door_type()? d0->get_door_type()->get_name().to_string().to_char() : TXT("--"),
		d0->is_world_separator_door()? TXT(" world separator door") : TXT(""));
	output(TXT("d1: d%p dt[%S]%S"), d1,
		d1->get_door_type() ? d1->get_door_type()->get_name().to_string().to_char() : TXT("--"), 
		d1->is_world_separator_door() ? TXT(" world separator door") : TXT(""));
#endif
	if (!d0->is_world_separator_door() &&
		corridorD0->get_room_on_other_side())
	{
		corridorD0->get_room_on_other_side()->collect_variables(variables);
		Random::Generator rg = corridorD0->get_room_on_other_side()->get_individual_random_generator();
#ifdef OUTPUT_GENERATION
		output(TXT("use d0->rg %S \"%S\""), rg.get_seed_string().to_char(), corridorD0->get_room_on_other_side()->get_name().to_char());
#endif
		baseRG.advance_seed(REF_ rg);
#ifdef OUTPUT_GENERATION
		output(TXT("make vr corridor, d0->rg, base rg %S"), baseRG.get_seed_string().to_char());
#endif
	}
	if (!d1->is_world_separator_door() &&
		corridorD1->get_room_on_other_side())
	{
		corridorD1->get_room_on_other_side()->collect_variables(variables);
		Random::Generator rg = corridorD1->get_room_on_other_side()->get_individual_random_generator();
#ifdef OUTPUT_GENERATION
		output(TXT("use d1->rg %S \"%S\""), rg.get_seed_string().to_char(), corridorD1->get_room_on_other_side()->get_name().to_char());
#endif
		baseRG.advance_seed(REF_ rg);
#ifdef OUTPUT_GENERATION
		output(TXT("make vr corridor, d1->rg, base rg %S"), baseRG.get_seed_string().to_char());
#endif
	}
	bool avoidElevators = variables.get_value(NAME(avoidElevators), false);
	bool encourageElevators = false;

	{
		scoped_call_stack_info(TXT("init meshes for doors"));
		corridorD0->init_meshes();
		corridorD1->init_meshes();
	}

	if (auto* g = Game::get())
	{
		scoped_call_stack_info(TXT("on newly created door in room"));
		g->on_newly_created_door_in_room(corridorD0);
		g->on_newly_created_door_in_room(corridorD1);
	}

	bool beElevator = false;
	float doorWidth = mainD->calculate_vr_width();
	if (doorWidth == 0.0f)
	{
		doorWidth = _tileSize;
	}
	//float elevatorLength = min(doorWidth, _tileSize);
	float halfDoorWidth = doorWidth * 0.5f;

#ifdef AN_DEVELOPMENT
	bool debugThis = false;
	Optional<int> nestedVRCorridor;
#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
	debugThis = true;
#endif
	if (corridor->get_name() == TXT("vr corridor 4"))
	{
		//debugThis = true;
	}
#else
	#ifdef AN_PROFILER
		#ifdef ALLOW_OUTPUT_GENERATION_DEBUG_VISUALISER
			bool debugThis = false;
			#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
				debugThis = true;
			#endif
		#endif
	#endif
#endif

	VR::Zone beInZone = _beInZone; // we add doors to beInZone and use it instead of _beInZone as we don't want the doors to be too close to the edge
	VR::Zone maxBeInZone = Game::get()->get_vr_zone();
	{
		scoped_call_stack_info(TXT("prepare zone"));
		an_assert_log_always(corridorD0->is_vr_space_placement_set());
		an_assert_log_always(corridorD1->is_vr_space_placement_set());

		Transform vrPlacementD0 = corridorD0->get_vr_space_placement();
		Transform vrPlacementD1 = corridorD1->get_vr_space_placement();

		Range2 beInZoneRange = _beInZone.to_range2();

		Vector3 halfDoorOff = Vector3::xAxis * doorWidth * 0.5f;
		Vector3 forwardDoor = Vector3::yAxis * doorWidth;
		beInZoneRange.include(vrPlacementD0.location_to_world( forwardDoor + ( halfDoorOff)).to_vector2());
		beInZoneRange.include(vrPlacementD0.location_to_world( forwardDoor + (-halfDoorOff)).to_vector2());
		beInZoneRange.include(vrPlacementD1.location_to_world( forwardDoor + ( halfDoorOff)).to_vector2());
		beInZoneRange.include(vrPlacementD1.location_to_world( forwardDoor + (-halfDoorOff)).to_vector2());
		beInZoneRange.include(vrPlacementD0.location_to_world(-forwardDoor + ( halfDoorOff)).to_vector2());
		beInZoneRange.include(vrPlacementD0.location_to_world(-forwardDoor + (-halfDoorOff)).to_vector2());
		beInZoneRange.include(vrPlacementD1.location_to_world(-forwardDoor + ( halfDoorOff)).to_vector2());
		beInZoneRange.include(vrPlacementD1.location_to_world(-forwardDoor + (-halfDoorOff)).to_vector2());
		beInZoneRange.expand_by(Vector2::one * 0.1f * max(doorWidth, _tileSize)); // give just a bit more space around

		Range2 maxBeInZoneRange = maxBeInZone.to_range2();
		beInZoneRange.intersect_with(maxBeInZoneRange); // shouldn't be bigger than that (this also works well with generosity in the statement above, when minRange is expanded)

		beInZone.be_range(beInZoneRange);
	}

#ifdef AN_DEVELOPMENT
	// check if doors are inside the play area
	if (!mainD->get_in_sub_world() || !mainD->get_in_sub_world()->is_test_sub_world())
	{
		VR::Zone fullBeInZone = maxBeInZone;

		Transform vrSpacePlacementD0 = corridorD0->get_vr_space_placement();
		Transform vrSpacePlacementD1 = corridorD1->get_vr_space_placement();
		// we check against fullBeInZone as it is the wider one
		if (!fullBeInZone.does_contain(vrSpacePlacementD0.get_translation().to_vector2(), doorWidth * 0.4f) ||
			!fullBeInZone.does_contain(vrSpacePlacementD1.get_translation().to_vector2(), doorWidth * 0.4f))
		{
			DebugVisualiserPtr dv(new DebugVisualiser(corridor->get_name()));
			dv->set_background_colour(Colour::lerp(0.05f, Colour::greyLight, Colour::red));
			dv->activate();
			dv->add_text(Vector2::zero, String(TXT("initial check: door outside!")), Colour::red, 0.3f);
			DebugVisualiserUtils::add_doors_to_debug_visualiser(dv, fullBeInZone,
				corridorD0, vrSpacePlacementD0.get_translation().to_vector2(), vrSpacePlacementD0.get_axis(Axis::Forward).to_vector2(),
				corridorD1, vrSpacePlacementD1.get_translation().to_vector2(), vrSpacePlacementD1.get_axis(Axis::Forward).to_vector2(),
				doorWidth);
			dv->end_gathering_data();
			dv->show_and_wait_for_key();

			an_assert(fullBeInZone.does_contain(vrSpacePlacementD0.get_translation().to_vector2(), doorWidth * 0.4f));
			an_assert(fullBeInZone.does_contain(vrSpacePlacementD1.get_translation().to_vector2(), doorWidth * 0.4f));
		}
	}
#endif

#ifdef AN_DEVELOPMENT
	{
#ifdef CATCH_NESTED_VR_CORRIDORS
		int vrCorridorNested = corridor->get_tags().get_tag_as_int(NAME(vrCorridorNested));
		if (vrCorridorNested > 10)
		{
			debugThis = true;
			nestedVRCorridor = vrCorridorNested;
		}
#endif
	}
#endif

	int vrCorridorNested = 0;
	{
		vrCorridorNested = corridor->get_tags().get_tag_as_int(NAME(vrCorridorNested));
		if (vrCorridorNested > 3)
		{
			avoidElevators = false;
		}
		if (vrCorridorNested > 6)
		{
			encourageElevators = true;
		}
	}

	//bool avoidFailSafeElevators = vrCorridorNested <= 1;

#ifdef ALLOW_OUTPUT_GENERATION_DEBUG_VISUALISER
	DebugVisualiserPtr dv(new DebugVisualiser(corridor->get_name()));
	dv->set_background_colour(Colour::lerp(0.05f, Colour::greyLight, Colour::red));
	if (debugThis)
	{
		dv->activate();
	}
#endif

	if (corridorD1->is_vr_placement_immovable())
	{
#ifdef OUTPUT_GENERATION
		output(TXT("swap d0 and d1 (d1->is_vr_placement_immovable)"));
#endif
		swap(corridorD0, corridorD1);
	}

	struct Do180
	{
		void clear() { do180 = false; }
		void reverse() { do180 = !do180; }
		bool should() const { return do180; }
		Transform if_required(Transform const& _vr) const 
		{
			if (do180)
			{
				return Transform::do180.to_world(_vr);
			}
			else
			{
				return _vr;
			}
		}
		Vector3 apply_to(Vector3 const& _dir)
		{
			return do180 ? -_dir : _dir;
		}
	private:
		bool do180 = false;
	}  do180ForD1;

	enum ResultFit
	{
		Unknown							= 0,
		FailSafeElevator				= 1,
		FailSafeNonElevator				= 2,
		Guessing						= 3,
		NextShouldBeOkGuessingALot		= 4,
		NextShouldBeOkButGuessingABit	= 5,
		NextShouldBeOk					= 7,
		AsIsButLessEncouraged			= 8,
		AsIs							= 10,
		AsIsBetter						= 12,
		AsIsBest						= 18,
	};
	struct ResultFitPenalty
	{
		static bool for_distance(ResultFit _rf)
		{
			return _rf != FailSafeElevator && _rf != AsIsButLessEncouraged;
		}
		static bool for_different_dir(ResultFit _rf)
		{
			return _rf == FailSafeElevator || _rf == AsIsButLessEncouraged;
		}
	};
	
	Optional<Transform> resultPlacementsD1[2];
	ResultFit resultFits[2];
	Do180 resultDo180s[2];
	bool resultBeElevators[2];

	bool allow180 = true;
	if (d0->get_tags().get_tag(NAME(disallow180)) ||
		corridorD0->get_tags().get_tag(NAME(disallow180)) ||
		(corridorD0->get_door_on_other_side() && corridorD0->get_door_on_other_side()->get_tags().get_tag(NAME(disallow180))) ||
		d1->get_tags().get_tag(NAME(disallow180)) ||
		corridorD1->get_tags().get_tag(NAME(disallow180)) ||
		(corridorD1->get_door_on_other_side() && corridorD1->get_door_on_other_side()->get_tags().get_tag(NAME(disallow180))))
	{
		// second index won't be used at all
		allow180 = false;
	}

	for_count(int, try180, allow180? 2 : 1)
	{
		auto& resultDo180 = resultDo180s[try180];
		resultDo180 = do180ForD1;
		resultDo180.clear();
		if (try180)
		{
			resultDo180.reverse();
		}

		auto& resultBeElevator = resultBeElevators[try180];
		resultBeElevator = false;

		Optional<Transform> & resultPlacementD1 = resultPlacementsD1[try180];
		ResultFit & resultFit = resultFits[try180];

		VR::Zone useBeInZone = beInZone;
		{
			int offsetNested = -2;
			int maxNested = 5;
			int maxNestedFull = 8;
			int useNested = max(0, vrCorridorNested + offsetNested);

			Range2 maxBeInZoneRange = maxBeInZone.to_range2();

			if (useNested < maxNested)
			{
				// try to remain within
				float useFull = (float)useNested / (float)maxNested;

				Transform vrPlacementD0 = corridorD0->get_vr_space_placement();
				Transform vrPlacementD1 = resultDo180.if_required(corridorD1->get_vr_space_placement());

				Range2 beInZoneRange = beInZone.to_range2();
			
				Vector3 halfDoorOff = Vector3::xAxis * doorWidth * 0.5f;
				Vector3 forwardDoor = Vector3::yAxis * doorWidth;
				Range2 minRange = Range2::empty;
				minRange.include(vrPlacementD0.location_to_world( forwardDoor + ( halfDoorOff)).to_vector2());
				minRange.include(vrPlacementD0.location_to_world( forwardDoor + (-halfDoorOff)).to_vector2());
				minRange.include(vrPlacementD1.location_to_world( forwardDoor + ( halfDoorOff)).to_vector2());
				minRange.include(vrPlacementD1.location_to_world( forwardDoor + (-halfDoorOff)).to_vector2());
				minRange.include(vrPlacementD0.location_to_world(-forwardDoor + ( halfDoorOff)).to_vector2());
				minRange.include(vrPlacementD0.location_to_world(-forwardDoor + (-halfDoorOff)).to_vector2());
				minRange.include(vrPlacementD1.location_to_world(-forwardDoor + ( halfDoorOff)).to_vector2());
				minRange.include(vrPlacementD1.location_to_world(-forwardDoor + (-halfDoorOff)).to_vector2());
				minRange.expand_by(Vector2::one * 1.05f * max(doorWidth, _tileSize)); // give more space around to allow to fit in (prefer the max value of door width and tile size)

				minRange.intersect_with(maxBeInZoneRange); // shouldn't be bigger than that (this also works well with generosity in the statement above, when minRange is expanded)

				Range2 useRange = lerp(useFull, minRange, beInZoneRange);

				useRange.intersect_with(maxBeInZoneRange); // can't be bigger than the zone, after we lerp(!)

				useBeInZone.be_range(useRange);
			}
			else if (useNested >= maxNested)
			{
				// expand to the max!

				float useFull = (float)(useNested - maxNested) / (float)(maxNestedFull - maxNested);
				
				Range2 useRange = lerp(useFull, beInZone.to_range2(), maxBeInZoneRange);

				useRange.intersect_with(maxBeInZoneRange); // can't be bigger than the zone, after we lerp(!)

				useBeInZone.be_range(useRange);
			}
		}

		{
			Transform vrPlacementD0 = corridorD0->get_vr_space_placement();
			Transform vrPlacementD1 = resultDo180.if_required(corridorD1->get_vr_space_placement());
#ifdef ALLOW_OUTPUT_GENERATION_DEBUG_VISUALISER
			Transform vrPlacementD0DebugLocal = Transform(Vector3(6.0f, 0.0f, 0.0f), Rotator3(0.0f, 180.0f, 0.0f).to_quat());
			Transform vrPlacementD1DebugLocal = vrPlacementD0DebugLocal.to_world(vrPlacementD0.to_local(vrPlacementD1));
#endif

			// always work on outbounds!

			Vector2 vrLocD0 = vrPlacementD0.get_translation().to_vector2();
			Vector2 vrDirD0 = vrPlacementD0.get_axis(Axis::Y).to_vector2();
			Vector2 vrRitD0 = vrDirD0.rotated_right();
			Vector2 vrLocD1 = vrPlacementD1.get_translation().to_vector2();
			Vector2 vrDirD1 = vrPlacementD1.get_axis(Axis::Y).to_vector2();
			Vector2 vrRitD1 = vrDirD1.rotated_right();

#ifdef ALLOW_OUTPUT_GENERATION_DEBUG_VISUALISER
#ifdef DEBUG_VISUALISE_LOCAL
			Vector2 vrLocD0DL = vrPlacementD0DebugLocal.get_translation().to_vector2();
			Vector2 vrDirD0DL = vrPlacementD0DebugLocal.get_axis(Axis::Y).to_vector2();
			Vector2 vrRitD0DL = vrDirD0DL.rotated_right();
			Vector2 vrLocD1DL = vrPlacementD1DebugLocal.get_translation().to_vector2();
			Vector2 vrDirD1DL = vrPlacementD1DebugLocal.get_axis(Axis::Y).to_vector2();
			Vector2 vrRitD1DL = vrDirD1DL.rotated_right();
#endif
#endif

			float vrYawD0 = vrDirD0.angle();
			float vrYawD1 = vrDirD1.angle();
			float yawDiff = relative_angle(vrYawD1 - vrYawD0);
			float absYawDiff = abs(yawDiff);
			Matrix44 vrMatD0 = look_at_matrix(vrLocD0.to_vector3(), (vrLocD0 + vrDirD0).to_vector3(), Vector3::zAxis);
			Matrix44 vrMatD1 = look_at_matrix(vrLocD1.to_vector3(), (vrLocD1 + vrDirD1).to_vector3(), Vector3::zAxis);
			Vector2 relLoc01 = vrMatD0.location_to_local(vrLocD1.to_vector3()).to_vector2();
			Vector2 relDir01 = vrMatD0.vector_to_local(vrDirD1.to_vector3()).to_vector2();
#ifdef AN_DEVELOPMENT_OR_PROFILER
#ifndef AN_CLANG
			Vector2 relLoc10 = vrMatD1.location_to_local(vrLocD0.to_vector3()).to_vector2();
			Vector2 relDir10 = vrMatD1.vector_to_local(vrDirD0.to_vector3()).to_vector2();
#endif
#endif
		
			Range2 doorSizeD0 = corridorD0->get_hole()->calculate_size(corridorD0->get_side(), corridorD0->get_hole_scale());
			Range2 doorSizeD1 = corridorD1->get_hole()->calculate_size(corridorD1->get_side(), corridorD1->get_hole_scale());

			// edges for door
			float makeItNarrower = 0.99f;
			Vector2 leftD0 = vrMatD0.location_to_world(Vector3::xAxis * doorSizeD0.x.min * makeItNarrower).to_vector2();
			Vector2 rightD0 = vrMatD0.location_to_world(Vector3::xAxis * doorSizeD0.x.max * makeItNarrower).to_vector2();
			Vector2 leftD1 = vrMatD1.location_to_world(Vector3::xAxis * doorSizeD1.x.min * makeItNarrower).to_vector2();
			Vector2 rightD1 = vrMatD1.location_to_world(Vector3::xAxis * doorSizeD1.x.max * makeItNarrower).to_vector2();

			Segment2 vrSegD0(leftD0, rightD0);
			Segment2 vrSegD1(leftD1, rightD1);

			// outbound zones
			VR::Zone vrZoneD0;
			vrZoneD0.be_rect(Vector2(doorWidth, doorWidth), Vector2(0.0f, halfDoorWidth));
			vrZoneD0 = vrZoneD0.to_world_of(vrMatD0.to_transform());
			VR::Zone vrZoneD1;
			vrZoneD1.be_rect(Vector2(doorWidth, doorWidth), Vector2(0.0f, halfDoorWidth));
			vrZoneD1 = vrZoneD1.to_world_of(vrMatD1.to_transform());

			// inbound zones
			VR::Zone vrInZoneD0;
			vrInZoneD0.be_rect(Vector2(doorWidth, doorWidth), Vector2(0.0f, -halfDoorWidth));
			vrInZoneD0 = vrInZoneD0.to_world_of(vrMatD0.to_transform());
			VR::Zone vrInZoneD1;
			vrInZoneD1.be_rect(Vector2(doorWidth, doorWidth), Vector2(0.0f, -halfDoorWidth));
			vrInZoneD1 = vrInZoneD1.to_world_of(vrMatD1.to_transform());

#ifdef ALLOW_OUTPUT_GENERATION_DEBUG_VISUALISER
			if (debugThis)
			{
				dv->start_gathering_data();
				dv->clear();
				DebugVisualiserUtils::add_doors_to_debug_visualiser(dv, useBeInZone, corridorD0, vrLocD0, vrDirD0, corridorD1, vrLocD1, vrDirD1, doorWidth);
#ifdef DEBUG_VISUALISE_LOCAL
				DebugVisualiserUtils::add_doors_to_debug_visualiser(dv, useBeInZone, corridorD0, vrLocD0DL, vrDirD0DL, corridorD1, vrLocD1DL, vrDirD1DL, doorWidth);
#endif
#ifdef AN_DEVELOPMENT
				if (nestedVRCorridor.is_set())
				{
					dv->add_text(Vector2::zero, String::printf(TXT("VR NESTED %i"), nestedVRCorridor.get()), Colour::black, 0.3f);
				}
#endif
				dv->end_gathering_data();
				dv->show_and_wait_for_key();
			}
#endif

			// when we change placement of corridorD1, we will have doors placed in different vr space and this should trigger another vr corridor creation
			// that's why it is completely ok just to alter D1 vr placement

			// if doors (zones) intersect (and actually one door intersects with zone of the other, not just zones) and they face samey direction, make them elevator
			if (absYawDiff < 30.0f && // more or less same direction
				relDir01.x * relLoc01.x >= 0.0f && // are facing each other
				(vrInZoneD0.does_intersect_with(vrSegD1) || vrInZoneD1.does_intersect_with(vrSegD0))) // one door (as segment) is within zone of other door
			{
				resultBeElevator = true;
				resultFit = AsIsButLessEncouraged;
#ifdef ALLOW_OUTPUT_GENERATION_DEBUG_VISUALISER
				if (debugThis)
				{
					dv->start_gathering_data();
					dv->clear();
					DebugVisualiserUtils::add_doors_to_debug_visualiser(dv, useBeInZone, corridorD0, vrLocD0, vrDirD0, corridorD1, vrLocD1, vrDirD1, doorWidth);
#ifdef DEBUG_VISUALISE_LOCAL
					DebugVisualiserUtils::add_doors_to_debug_visualiser(dv, useBeInZone, corridorD0, vrLocD0DL, vrDirD0DL, corridorD1, vrLocD1DL, vrDirD1DL, doorWidth);
#endif
					vrInZoneD0.debug_visualise(dv.get(), Colour::lerp(0.5f, Colour::white, Colour::red));
					vrInZoneD1.debug_visualise(dv.get(), Colour::lerp(0.5f, Colour::white, Colour::blue));
					dv->add_text(Vector2::zero, String(TXT("ELEVATOR")), Colour::cyan, 0.3f);
					dv->end_gathering_data();
					dv->show_and_wait_for_key();
				}
#endif
			}
			else
			{
				Random::Generator rg = baseRG;
				rg.advance_seed(4597, 2579245);

				// check if both are in front of each other, if they are, we should always make it work
				float inFrontThreshold = halfDoorWidth; // half door width is good enough
				if (absYawDiff > 179.5f && relLoc01.is_almost_zero())
				{
					// they are placed at same spot!
					resultFit = AsIsBest;
#ifdef ALLOW_OUTPUT_GENERATION_DEBUG_VISUALISER
					if (debugThis)
					{
						dv->start_gathering_data();
						dv->clear();
						DebugVisualiserUtils::add_doors_to_debug_visualiser(dv, useBeInZone, corridorD0, vrLocD0, vrDirD0, corridorD1, vrLocD1, vrDirD1, doorWidth);
#ifdef DEBUG_VISUALISE_LOCAL
						DebugVisualiserUtils::add_doors_to_debug_visualiser(dv, useBeInZone, corridorD0, vrLocD0DL, vrDirD0DL, corridorD1, vrLocD1DL, vrDirD1DL, doorWidth);
#endif
						dv->add_text(Vector2::zero, String(TXT("Why are we doing this if this is same spot?")), Colour::cyan, 0.3f);
						dv->end_gathering_data();
						dv->show_and_wait_for_key();
					}
#endif
					an_assert(false, TXT("we should not create corridor if it is same spot! check why we did call this function"));
					error(TXT("we should not create corridor if it is same spot! check why we did do this?"));
				}
				else if (absYawDiff < 25.0f && // same direction
						 vrMatD0.location_to_local(leftD1.to_vector3()).y <= inFrontThreshold &&
						 vrMatD0.location_to_local(rightD1.to_vector3()).y <= inFrontThreshold &&
						 vrMatD1.location_to_local(leftD0.to_vector3()).y <= inFrontThreshold &&
						 vrMatD1.location_to_local(rightD0.to_vector3()).y <= inFrontThreshold &&
						 abs(relLoc01.x) >= doorWidth - 0.1f) // to allow (2ip / u-turn) or elevator
				{
					if (abs(relLoc01.x) < doorWidth - 0.01f)
					{
						resultBeElevator = true;
						resultFit = AsIsButLessEncouraged;
#ifdef ALLOW_OUTPUT_GENERATION_DEBUG_VISUALISER
						if (debugThis)
						{
							dv->start_gathering_data();
							dv->clear();
							DebugVisualiserUtils::add_doors_to_debug_visualiser(dv, useBeInZone, corridorD0, vrLocD0, vrDirD0, corridorD1, vrLocD1, vrDirD1, doorWidth);
#ifdef DEBUG_VISUALISE_LOCAL
							DebugVisualiserUtils::add_doors_to_debug_visualiser(dv, useBeInZone, corridorD0, vrLocD0DL, vrDirD0DL, corridorD1, vrLocD1DL, vrDirD1DL, doorWidth);
#endif
							dv->add_text(Vector2::zero, String(TXT("almost OK as is (same dir) but too close, ELEVATOR")), Colour::cyan, 0.3f);
							dv->end_gathering_data();
							dv->show_and_wait_for_key();
						}
#endif
					}
					else
					{
						// is ok, they face samey direction, if it would be wrong, they would end up as an elevator
						resultFit = AsIs;
#ifdef ALLOW_OUTPUT_GENERATION_DEBUG_VISUALISER
						if (debugThis)
						{
							dv->start_gathering_data();
							dv->clear();
							DebugVisualiserUtils::add_doors_to_debug_visualiser(dv, useBeInZone, corridorD0, vrLocD0, vrDirD0, corridorD1, vrLocD1, vrDirD1, doorWidth);
#ifdef DEBUG_VISUALISE_LOCAL
							DebugVisualiserUtils::add_doors_to_debug_visualiser(dv, useBeInZone, corridorD0, vrLocD0DL, vrDirD0DL, corridorD1, vrLocD1DL, vrDirD1DL, doorWidth);
#endif
							dv->add_text(Vector2::zero, String(TXT("OK as is (same dir)")), Colour::cyan, 0.3f);
							dv->end_gathering_data();
							dv->show_and_wait_for_key();
						}
#endif
					}
				}
				else if (absYawDiff >= 70.0f && absYawDiff <= 100.0f &&
						 vrMatD0.location_to_local(leftD1.to_vector3()).y <= 0.0f &&
						 vrMatD0.location_to_local(rightD1.to_vector3()).y <= 0.0f &&
						 vrMatD1.location_to_local(leftD0.to_vector3()).y <= 0.0f &&
						 vrMatD1.location_to_local(rightD0.to_vector3()).y <= 0.0f)
				{
					// is ok, they are perpendicular and are in front of each other
					resultFit = AsIsBetter;
#ifdef ALLOW_OUTPUT_GENERATION_DEBUG_VISUALISER
					if (debugThis)
					{
						dv->start_gathering_data();
						dv->clear();
						DebugVisualiserUtils::add_doors_to_debug_visualiser(dv, useBeInZone, corridorD0, vrLocD0, vrDirD0, corridorD1, vrLocD1, vrDirD1, doorWidth);
#ifdef DEBUG_VISUALISE_LOCAL
						DebugVisualiserUtils::add_doors_to_debug_visualiser(dv, useBeInZone, corridorD0, vrLocD0DL, vrDirD0DL, corridorD1, vrLocD1DL, vrDirD1DL, doorWidth);
#endif
						dv->add_text(Vector2::zero, String(TXT("OK as is (perpendicular, in front of each other)")), Colour::cyan, 0.3f);
						dv->end_gathering_data();
						dv->show_and_wait_for_key();
					}
#endif
				}
				else if (absYawDiff >100.0f && // face each other
						 vrMatD0.location_to_local(leftD1.to_vector3()).y <= inFrontThreshold &&
						 vrMatD0.location_to_local(rightD1.to_vector3()).y <= inFrontThreshold &&
						 vrMatD1.location_to_local(leftD0.to_vector3()).y <= inFrontThreshold &&
						 vrMatD1.location_to_local(rightD0.to_vector3()).y <= inFrontThreshold &&
						 (abs(relLoc01.x) < -relLoc01.y * 0.5f || // when facing each other they should be in front of each other
						 -relLoc01.y >= doorWidth - 0.01f)) // or should have enough space to allow 2ip
				{
					// is ok, they face samey direction, if it would be wrong, they would end up as an elevator
					resultFit = AsIsBest;
#ifdef ALLOW_OUTPUT_GENERATION_DEBUG_VISUALISER
					if (debugThis)
					{
						dv->start_gathering_data();
						dv->clear();
						DebugVisualiserUtils::add_doors_to_debug_visualiser(dv, useBeInZone, corridorD0, vrLocD0, vrDirD0, corridorD1, vrLocD1, vrDirD1, doorWidth);
#ifdef DEBUG_VISUALISE_LOCAL
						DebugVisualiserUtils::add_doors_to_debug_visualiser(dv, useBeInZone, corridorD0, vrLocD0DL, vrDirD0DL, corridorD1, vrLocD1DL, vrDirD1DL, doorWidth);
#endif
						dv->add_text(Vector2::zero, String(TXT("OK as is (face e/o)")), Colour::cyan, 0.3f);
						dv->end_gathering_data();
						dv->show_and_wait_for_key();
					}
#endif
				}
				else
				{
					bool handled = false;
					Vector2 newDoorAt = vrLocD0;
					Vector2 newDoorDir = vrDirD0; // will be outbound
					float const zoneThreshold = 0.05f;

					bool notEnoughSpaceNotAligned = relLoc01.y > -doorWidth && relLoc01.y < 0.0f && // in front
						absYawDiff > 135.0f && // opposite direction
						vrInZoneD0.does_intersect_with(vrZoneD1) && // their zones touch each other - they are too close
						abs(relLoc01.x) > 0.3f * -relLoc01.y; // and they are not aligned
					if (encourageElevators &&
						absYawDiff < 30.0f &&
						abs(Vector2::dot(vrLocD1 - vrLocD0, -vrDirD0.rotated_right())) < doorWidth * 2.0f)
					{
						// enforce elevator if the other door is close (right dir speaking) and facing sort of the same dir
						newDoorAt = vrLocD1;
						newDoorDir = vrDirD1;
						resultBeElevator = true;
						handled = true;
						resultFit = AsIsButLessEncouraged;
#ifdef ALLOW_OUTPUT_GENERATION_DEBUG_VISUALISER
						if (debugThis)
						{
							dv->start_gathering_data();
							dv->clear();
							DebugVisualiserUtils::add_doors_to_debug_visualiser(dv, useBeInZone, corridorD0, vrLocD0, vrDirD0, corridorD1, vrLocD1, vrDirD1, doorWidth);
#ifdef DEBUG_VISUALISE_LOCAL
							DebugVisualiserUtils::add_doors_to_debug_visualiser(dv, useBeInZone, corridorD0, vrLocD0DL, vrDirD0DL, corridorD1, vrLocD1DL, vrDirD1DL, doorWidth);
#endif
							vrInZoneD0.debug_visualise(dv.get(), Colour::lerp(0.5f, Colour::white, Colour::red));
							vrInZoneD1.debug_visualise(dv.get(), Colour::lerp(0.5f, Colour::white, Colour::blue));
							dv->add_text(Vector2::zero, String(TXT("ENCOURAGED ELEVATOR")), Colour::cyan, 0.3f);
							dv->end_gathering_data();
							dv->show_and_wait_for_key();
						}
#endif
					}
					if (!handled &&
						(absYawDiff < 45.0f || absYawDiff > 135.0f) && // samey direction or totally opposite
						(relLoc01.y > 0.0f || absYawDiff < 45.0f || // either behind us or same dir
						notEnoughSpaceNotAligned)) // or there is not enough space
					{
						// there are two cases
						//	1. other door is behind us (no matter dir), we will be looking for a door on our plane looking in same direction
						//	2. other door is in front of us facing same direction, we will be looking for a door on their plane but with reversed direction
						bool findAtD1Plane = relLoc01.y <= 0.0f && ! notEnoughSpaceNotAligned; // check if we should find at D1 plane or at D0 plane
						bool sameyDirection = absYawDiff < 45.0f;
						bool reverseDirE = false;
						if (findAtD1Plane && sameyDirection) // if we have same direction, we want to get U-turn
						{
							reverseDirE = true;
						}
							auto const & vrLocS = findAtD1Plane ? vrLocD0 : vrLocD1;
							auto const & vrDirS = findAtD1Plane ? vrDirD0 : vrDirD1;
						//	auto const & vrRitS = findAtD1Plane ? vrRitD0 : vrRitD1;
						//	auto const & vrMatS = findAtD1Plane ? vrMatD0 : vrMatD1;
							auto const & vrLocE = findAtD1Plane ? vrLocD1 : vrLocD0;
							auto const & vrDirE = findAtD1Plane ? vrDirD1 : vrDirD0;
							auto const & vrRitE = findAtD1Plane ? vrRitD1 : vrRitD0;
						//	auto const & vrMatE = findAtD1Plane ? vrMatD1 : vrMatD0;
						VR::Zone vrZoneSRev; // reversed - to catch elevators
						vrZoneSRev.be_rect(Vector2(doorWidth, doorWidth), Vector2(0.0f, halfDoorWidth));
						vrZoneSRev.place_at(vrLocS, -vrDirS);
						// check space on both sides of other door and place new door on one side or the other
						// check distance to new loc, get closer one
						// 0 left, 1 right
						bool inside[2];
						float dist[2];
						for_count(int, trySide, 2)
						{
							Vector2 tryLoc = vrLocE + vrRitE * doorWidth * (trySide ? 1.0f : -1.0f);
							VR::Zone tryDoorZone;
							tryDoorZone.be_rect(Vector2(doorWidth, doorWidth), Vector2(0.0f, halfDoorWidth));
							tryDoorZone.place_at(tryLoc, vrDirE * (reverseDirE ? -1.0f : 1.0f));
							inside[trySide] = useBeInZone.does_contain(tryDoorZone, -zoneThreshold);
							dist[trySide] = (tryLoc - vrLocS).length();
						}

						int goodSide = NONE;
						if (dist[0] < dist[1] && inside[0])
						{
							goodSide = 0;
						}
						if (dist[1] < dist[0] && inside[1])
						{
							goodSide = 1;
						}
						if (abs(dist[1] - dist[0]) < doorWidth * 0.3f && inside[0] && inside[1])
						{
							goodSide = rg.get_int(2);
						}
						if (goodSide == NONE)
						{
							if (inside[0]) goodSide = 0;
							if (inside[1]) goodSide = 1;
						}
						if (goodSide != NONE)
						{
							float dirSide = (goodSide ? 1.0f : -1.0f);
							// check if intersection (on side we've found place) is ok
							Vector2 intersection;
							Vector2 dirI = dirSide * vrRitE;
							Vector2 startIAt = vrLocE + dirI * doorWidth; // this is the closest point on other door's line so we won't cross with the doors
#ifdef ALLOW_OUTPUT_GENERATION_DEBUG_VISUALISER
#ifdef ALLOW_OUTPUT_GENERATION_DEBUG_VISUALISER_DETAILED
							if (debugThis)
							{
								dv->start_gathering_data();
								dv->clear();
								vrZoneSRev.debug_visualise(dv.get(), Colour::green);
								DebugVisualiserUtils::add_doors_to_debug_visualiser(dv, useBeInZone, corridorD0, vrLocD0, vrDirD0, corridorD1, vrLocD1, vrDirD1, doorWidth);
#ifdef DEBUG_VISUALISE_LOCAL
								DebugVisualiserUtils::add_doors_to_debug_visualiser(dv, useBeInZone, corridorD0, vrLocD0DL, vrDirD0DL, corridorD1, vrLocD1DL, vrDirD1DL, doorWidth);
#endif
								DebugVisualiserUtils::add_door_to_debug_visualiser(dv, nullptr, newDoorAt, newDoorDir, doorWidth, Colour::greenDark);
#ifdef DEBUG_VISUALISE_LOCAL
								DebugVisualiserUtils::add_door_to_debug_visualiser(dv, nullptr, vrPlacementD0DebugLocal.location_to_world(vrPlacementD0.location_to_local(newDoorAt.to_vector3())).to_vector2(),
																								vrPlacementD0DebugLocal.vector_to_world(vrPlacementD0.vector_to_local(newDoorDir.to_vector3())).to_vector2(),
																								doorWidth, Colour::greenDark);
#endif
								Vector2::calc_intersection(vrLocS, vrDirS, startIAt, dirI, intersection);
								dv->add_arrow(vrLocS, vrLocS + vrDirS * 10.0f, Colour::red);
								dv->add_arrow(startIAt, startIAt + dirI * 10.0f, Colour::blue);
								dv->add_circle(intersection, 0.02f, Colour::purple);
								dv->add_text(Vector2::zero, String(TXT("try catched up (intersection)")), Colour::greenDark, 0.3f);
								dv->end_gathering_data();
								dv->show_and_wait_for_key();
							}
#endif
#endif
							if (Vector2::calc_intersection(vrLocS, vrDirS, startIAt, dirI, intersection))
							{
								if (Vector2::dot(intersection - startIAt, dirI) < 0.001f &&
									Vector2::dot(intersection - startIAt, dirI) >= -doorWidth * 0.5f)
								{
									// if at least one is in front of the other
									if (Vector2::dot(vrDirS, vrLocE - vrLocS) < 0.0f ||
										Vector2::dot(vrDirE, vrLocS - vrLocE) < 0.0f)									
									{
										// move it so it is valid, it may work a bit as opposite version of "next to"
										intersection = startIAt + dirI * 0.001f;
									}
								}
								if (Vector2::dot(intersection - startIAt, dirI) >= -0.001f)
								{
									VR::Zone tryDoorZone;
									tryDoorZone.be_rect(Vector2(doorWidth, doorWidth * 2.0f), Vector2::zero);
									tryDoorZone.place_at(intersection, vrDirE * (reverseDirE ? -1.0f : 1.0f));
									Vector2 relTryLoc = vrMatD0.location_to_local(intersection.to_vector3()).to_vector2();
#ifdef ALLOW_OUTPUT_GENERATION_DEBUG_VISUALISER
#ifdef ALLOW_OUTPUT_GENERATION_DEBUG_VISUALISER_DETAILED
									if (debugThis)
									{
										dv->start_gathering_data();
										dv->clear();
										tryDoorZone.debug_visualise(dv.get(), Colour::yellow);
										DebugVisualiserUtils::add_doors_to_debug_visualiser(dv, useBeInZone, corridorD0, vrLocD0, vrDirD0, corridorD1, vrLocD1, vrDirD1, doorWidth);
#ifdef DEBUG_VISUALISE_LOCAL
										DebugVisualiserUtils::add_doors_to_debug_visualiser(dv, useBeInZone, corridorD0, vrLocD0DL, vrDirD0DL, corridorD1, vrLocD1DL, vrDirD1DL, doorWidth);
#endif
										DebugVisualiserUtils::add_door_to_debug_visualiser(dv, nullptr, newDoorAt, newDoorDir, doorWidth, Colour::greenDark);
#ifdef DEBUG_VISUALISE_LOCAL
										DebugVisualiserUtils::add_door_to_debug_visualiser(dv, nullptr, vrPlacementD0DebugLocal.location_to_world(vrPlacementD0.location_to_local(newDoorAt.to_vector3())).to_vector2(),
																								vrPlacementD0DebugLocal.vector_to_world(vrPlacementD0.vector_to_local(newDoorDir.to_vector3())).to_vector2(),
																								doorWidth, Colour::greenDark);
#endif
										dv->add_text(Vector2::zero, String(TXT("try catched up (intersection) fits?")), Colour::greenDark, 0.3f);
										dv->end_gathering_data();
										output(TXT("relTryLoc.x %.3f"), relTryLoc.x);
										output(TXT("relTryLoc.y %.3f"), relTryLoc.y);
										dv->show_and_wait_for_key();
									}
#endif
#endif
									if (useBeInZone.does_contain(tryDoorZone, -zoneThreshold) &&
										(!findAtD1Plane || // on our plane (u-turn)
										 abs(relTryLoc.x) < -relTryLoc.y * 0.5f || // in front
										 -relTryLoc.y >= doorWidth - 0.01f)) // enough space for 2ip
									{
										bool allowElevator = rg.get_chance(0.2f) && ! avoidElevators;
#ifdef ALLOW_OUTPUT_GENERATION_DEBUG_VISUALISER
#ifdef ALLOW_OUTPUT_GENERATION_DEBUG_VISUALISER_DETAILED
										if (debugThis)
										{
											dv->start_gathering_data();
											dv->clear();
											vrZoneSRev.debug_visualise(dv.get(), Colour::green);
											tryDoorZone.debug_visualise(dv.get(), Colour::yellow);
											DebugVisualiserUtils::add_doors_to_debug_visualiser(dv, useBeInZone, corridorD0, vrLocD0, vrDirD0, corridorD1, vrLocD1, vrDirD1, doorWidth);
#ifdef DEBUG_VISUALISE_LOCAL
											DebugVisualiserUtils::add_doors_to_debug_visualiser(dv, useBeInZone, corridorD0, vrLocD0DL, vrDirD0DL, corridorD1, vrLocD1DL, vrDirD1DL, doorWidth);
#endif
											DebugVisualiserUtils::add_door_to_debug_visualiser(dv, nullptr, newDoorAt, newDoorDir, doorWidth, Colour::greenDark);
#ifdef DEBUG_VISUALISE_LOCAL
											DebugVisualiserUtils::add_door_to_debug_visualiser(dv, nullptr, vrPlacementD0DebugLocal.location_to_world(vrPlacementD0.location_to_local(newDoorAt.to_vector3())).to_vector2(),
																									vrPlacementD0DebugLocal.vector_to_world(vrPlacementD0.vector_to_local(newDoorDir.to_vector3())).to_vector2(),
																									doorWidth, Colour::greenDark);
#endif
											dv->add_arrow(vrLocS, vrLocS + vrDirS, Colour::red);
											dv->add_arrow(vrLocS, vrLocS + vrDirE * (reverseDirE ? -1.0f : 1.0f), Colour::blue);
											dv->add_text(Vector2::zero, String(TXT("try catched up (intersection) elevator?")), Colour::greenDark, 0.3f);
											dv->end_gathering_data();
											dv->show_and_wait_for_key();
										}
#endif
#endif
										if (allowElevator
											|| // try to avoid elevators
											(!tryDoorZone.does_intersect_with(vrZoneSRev) ||
											 Vector2::dot(vrDirS, vrDirE * (reverseDirE ? -1.0f : 1.0f)) > 0.7f)
											|| // maybe just right in front or enough space for 2ip
											(Vector2::dot(vrDirS, vrDirE * (reverseDirE ? -1.0f : 1.0f)) < -0.7f && // new door would be opposite to us
											 (abs(relTryLoc.x) < -relTryLoc.y * 0.2f || -relTryLoc.y >= doorWidth - 0.01f)) // are right in front or there is enough space for 2ip
											)
										{
											newDoorAt = intersection;
											newDoorDir = reverseDirE ? -vrDirE : vrDirE;
											handled = true;
											resultFit = reverseDirE? NextShouldBeOk : NextShouldBeOkButGuessingABit;
#ifdef ALLOW_OUTPUT_GENERATION_DEBUG_VISUALISER
											if (debugThis)
											{
												dv->start_gathering_data();
												dv->clear();
												DebugVisualiserUtils::add_doors_to_debug_visualiser(dv, useBeInZone, corridorD0, vrLocD0, vrDirD0, corridorD1, vrLocD1, vrDirD1, doorWidth);
#ifdef DEBUG_VISUALISE_LOCAL
												DebugVisualiserUtils::add_doors_to_debug_visualiser(dv, useBeInZone, corridorD0, vrLocD0DL, vrDirD0DL, corridorD1, vrLocD1DL, vrDirD1DL, doorWidth);
#endif
												DebugVisualiserUtils::add_door_to_debug_visualiser(dv, nullptr, newDoorAt, newDoorDir, doorWidth, Colour::greenDark);
#ifdef DEBUG_VISUALISE_LOCAL
												DebugVisualiserUtils::add_door_to_debug_visualiser(dv, nullptr, vrPlacementD0DebugLocal.location_to_world(vrPlacementD0.location_to_local(newDoorAt.to_vector3())).to_vector2(),
																								vrPlacementD0DebugLocal.vector_to_world(vrPlacementD0.vector_to_local(newDoorDir.to_vector3())).to_vector2(),
																								doorWidth, Colour::greenDark);
#endif
												dv->add_text(Vector2::zero, String::printf(TXT("catched up (intersection, %S)"), reverseDirE? TXT("u-turn") : TXT("guessing a bit")), Colour::greenDark, 0.3f);
												tryDoorZone.debug_visualise(dv.get(), Colour::magenta);
												vrZoneSRev.debug_visualise(dv.get(), Colour::c64Brown);
												dv->end_gathering_data();
												dv->show_and_wait_for_key();
											}
#endif
										}
									}
								}
							}
							if (!handled)
							{
								// try bunch of random ones on that side, first check how far can we go
								Vector2 asFarAs;
								if (useBeInZone.calc_intersection(startIAt, dirI, asFarAs))
								{
									bool allowElevator = rg.get_chance(0.2f) && !avoidElevators;
									Vector2 offset = asFarAs - startIAt;
									int triesLeft = 200;
									float maxDistPt = 0.5f;
									while (triesLeft--)
									{
										float randOffset = rg.get_float(0.0f, maxDistPt);
										if (rg.get_chance(0.3f))
										{
											// try to move as far as possible as it is more likely we will 
											randOffset = maxDistPt;
										}
										Vector2 tryLoc = startIAt + offset * randOffset;
										VR::Zone tryDoorZone;
										tryDoorZone.be_rect(Vector2(doorWidth, doorWidth * 2.0f), Vector2::zero);
										tryDoorZone.place_at(tryLoc, vrDirE * (reverseDirE ? -1.0f : 1.0f));
										Vector2 relTryLoc = vrMatD0.location_to_local(tryLoc.to_vector3()).to_vector2();
										if (useBeInZone.does_contain(tryDoorZone, -zoneThreshold) &&
											(!findAtD1Plane || // on our plane (u-turn)
											 abs(relTryLoc.x) < -relTryLoc.y * 0.5f || // in front
											 -relTryLoc.y >= doorWidth - 0.01f)) // enough space for 2ip
										{
#ifdef ALLOW_OUTPUT_GENERATION_DEBUG_VISUALISER
#ifdef ALLOW_OUTPUT_GENERATION_DEBUG_VISUALISER_DETAILED
											if (debugThis)
											{
												dv->start_gathering_data();
												dv->clear();
												tryDoorZone.debug_visualise(dv.get(), Colour::yellow);
												vrZoneSRev.debug_visualise(dv.get(), Colour::green);
												DebugVisualiserUtils::add_doors_to_debug_visualiser(dv, useBeInZone, corridorD0, vrLocD0, vrDirD0, corridorD1, vrLocD1, vrDirD1, doorWidth);
#ifdef DEBUG_VISUALISE_LOCAL
												DebugVisualiserUtils::add_doors_to_debug_visualiser(dv, useBeInZone, corridorD0, vrLocD0DL, vrDirD0DL, corridorD1, vrLocD1DL, vrDirD1DL, doorWidth);
#endif
												DebugVisualiserUtils::add_door_to_debug_visualiser(dv, nullptr, tryLoc, reverseDirE ? -vrDirE : vrDirE, doorWidth, Colour::yellow);
#ifdef DEBUG_VISUALISE_LOCAL
												DebugVisualiserUtils::add_door_to_debug_visualiser(dv, nullptr, vrPlacementD0DebugLocal.location_to_world(vrPlacementD0.location_to_local(newDoorAt.to_vector3())).to_vector2(),
																								vrPlacementD0DebugLocal.vector_to_world(vrPlacementD0.vector_to_local(newDoorDir.to_vector3())).to_vector2(),
																								doorWidth, Colour::greenDark);
#endif
												dv->add_text(Vector2::zero, String(TXT("try catched up (random)")), Colour::greenDark, 0.3f);
												dv->end_gathering_data();
												bool doesIntersect = tryDoorZone.does_intersect_with(vrZoneSRev);
												float dot = Vector2::dot(vrDirS, vrDirE * (reverseDirE ? -1.0f : 1.0f));
												output(TXT("doesIntersect %c   dot %.3f"), doesIntersect ? '+' : '-', dot);
												dv->show_and_wait_for_key();
											}
#endif
#endif
											if (allowElevator ||
												(!tryDoorZone.does_intersect_with(vrZoneSRev) // does not intersect, won't be an elevator
												 || (Vector2::dot(vrDirS, vrDirE * (reverseDirE ? -1.0f : 1.0f)) < -0.7f && -relTryLoc.y >= doorWidth - 0.01f) // the point towards each other and are in front of each other
												)
											   )
											{
												newDoorAt = tryLoc;
												newDoorDir = reverseDirE ? -vrDirE : vrDirE;
												handled = true; 
												resultFit = reverseDirE? NextShouldBeOk : NextShouldBeOkGuessingALot;
#ifdef ALLOW_OUTPUT_GENERATION_DEBUG_VISUALISER
												if (debugThis)
												{
													dv->start_gathering_data();
													dv->clear();
													tryDoorZone.debug_visualise(dv.get(), Colour::yellow);
													DebugVisualiserUtils::add_doors_to_debug_visualiser(dv, useBeInZone, corridorD0, vrLocD0, vrDirD0, corridorD1, vrLocD1, vrDirD1, doorWidth);
#ifdef DEBUG_VISUALISE_LOCAL
													DebugVisualiserUtils::add_doors_to_debug_visualiser(dv, useBeInZone, corridorD0, vrLocD0DL, vrDirD0DL, corridorD1, vrLocD1DL, vrDirD1DL, doorWidth);
#endif
													DebugVisualiserUtils::add_door_to_debug_visualiser(dv, nullptr, newDoorAt, newDoorDir, doorWidth, Colour::greenDark);
#ifdef DEBUG_VISUALISE_LOCAL
													DebugVisualiserUtils::add_door_to_debug_visualiser(dv, nullptr, vrPlacementD0DebugLocal.location_to_world(vrPlacementD0.location_to_local(newDoorAt.to_vector3())).to_vector2(),
																								vrPlacementD0DebugLocal.vector_to_world(vrPlacementD0.vector_to_local(newDoorDir.to_vector3())).to_vector2(),
																								doorWidth, Colour::greenDark);
#endif
													dv->add_text(Vector2::zero, String::printf(TXT("catched up (random %S %S)"), baseRG.get_seed_string().to_char(), reverseDirE ? TXT("u-turn") : TXT("guessing a bit")), Colour::greenDark, 0.3f);
													dv->end_gathering_data();
													dv->show_and_wait_for_key();
												}
#endif
												break;
											}
										}
										maxDistPt = min(1.0f, maxDistPt + 0.1f);
									}
								}
							}
							{
								for_count(int, trySide, 2)
								{
									if (!handled && !sameyDirection) // only if opposite
									{
										// fallback, use next to other door
										newDoorAt = vrLocE + vrRitE * doorWidth * dirSide;
										newDoorDir = reverseDirE ? -vrDirE : vrDirE;
										VR::Zone tryDoorZone;
										tryDoorZone.be_rect(Vector2(doorWidth, doorWidth), Vector2(0.0f, halfDoorWidth));
										tryDoorZone.place_at(newDoorAt, newDoorDir);
										// check if we would end up with an elevator
										if (! tryDoorZone.does_intersect_with(vrZoneSRev))
										{
											// it's ok
											// check if we fit in the zone etc
											Vector2 relTryLoc = vrMatD0.location_to_local(newDoorAt.to_vector3()).to_vector2();
											if (!findAtD1Plane || // on our plane (u-turn)
												abs(relTryLoc.x) < -relTryLoc.y * 0.5f || // in front
												-relTryLoc.y >= doorWidth - 0.01f) // enough space for 2ip
											{
												VR::Zone tryDoorZoneDoubleSide;
												tryDoorZoneDoubleSide.be_rect(Vector2(doorWidth, doorWidth * 2.0f), Vector2(0.0f, 0.0f));
												tryDoorZoneDoubleSide.place_at(newDoorAt, newDoorDir);
												if (useBeInZone.does_contain(tryDoorZoneDoubleSide, -zoneThreshold))
												{
													handled = true;
													resultFit = NextShouldBeOk;
#ifdef ALLOW_OUTPUT_GENERATION_DEBUG_VISUALISER
													if (debugThis)
													{
														dv->start_gathering_data();
														dv->clear();
														DebugVisualiserUtils::add_doors_to_debug_visualiser(dv, useBeInZone, corridorD0, vrLocD0, vrDirD0, corridorD1, vrLocD1, vrDirD1, doorWidth);
#ifdef DEBUG_VISUALISE_LOCAL
														DebugVisualiserUtils::add_doors_to_debug_visualiser(dv, useBeInZone, corridorD0, vrLocD0DL, vrDirD0DL, corridorD1, vrLocD1DL, vrDirD1DL, doorWidth);
#endif
														DebugVisualiserUtils::add_door_to_debug_visualiser(dv, nullptr, newDoorAt, newDoorDir, doorWidth, Colour::greenDark);
#ifdef DEBUG_VISUALISE_LOCAL
														DebugVisualiserUtils::add_door_to_debug_visualiser(dv, nullptr, vrPlacementD0DebugLocal.location_to_world(vrPlacementD0.location_to_local(newDoorAt.to_vector3())).to_vector2(),
																								vrPlacementD0DebugLocal.vector_to_world(vrPlacementD0.vector_to_local(newDoorDir.to_vector3())).to_vector2(),
																								doorWidth, Colour::greenDark);
#endif
														dv->add_text(Vector2::zero, String(TXT("catched up (next to)")), Colour::greenDark, 0.3f);
														dv->end_gathering_data();
														dv->show_and_wait_for_key();
													}
#endif
												}
											}
										}
										if (!handled)
										{
											dirSide = -dirSide;
										}
									}
								}
							}
							if (handled && !findAtD1Plane)
							{
								if ((newDoorAt - vrLocD0).length() < doorWidth * 0.98f)
								{
									resultBeElevator = true;
#ifdef ALLOW_OUTPUT_GENERATION_DEBUG_VISUALISER
									if (debugThis)
									{
										dv->start_gathering_data();
										dv->clear();
										DebugVisualiserUtils::add_doors_to_debug_visualiser(dv, useBeInZone, corridorD0, vrLocD0, vrDirD0, corridorD1, vrLocD1, vrDirD1, doorWidth);
#ifdef DEBUG_VISUALISE_LOCAL
										DebugVisualiserUtils::add_doors_to_debug_visualiser(dv, useBeInZone, corridorD0, vrLocD0DL, vrDirD0DL, corridorD1, vrLocD1DL, vrDirD1DL, doorWidth);
#endif
										DebugVisualiserUtils::add_door_to_debug_visualiser(dv, nullptr, newDoorAt, newDoorDir, doorWidth, Colour::greenDark);
#ifdef DEBUG_VISUALISE_LOCAL
										DebugVisualiserUtils::add_door_to_debug_visualiser(dv, nullptr, vrPlacementD0DebugLocal.location_to_world(vrPlacementD0.location_to_local(newDoorAt.to_vector3())).to_vector2(),
																								vrPlacementD0DebugLocal.vector_to_world(vrPlacementD0.vector_to_local(newDoorDir.to_vector3())).to_vector2(),
																								doorWidth, Colour::greenDark);
#endif
										dv->add_text(Vector2::zero, String(TXT("be elevator")), Colour::greenDark, 0.3f);
										dv->end_gathering_data();
										dv->show_and_wait_for_key();
									}
#endif
								}
							}
						}
					}
					if (!handled && absYawDiff > 45.0f && absYawDiff < 135.0f)
					{
						float d1OnRightOfD0 = Vector2::dot(vrLocD1 - vrLocD0, -vrDirD0.rotated_right());
						// check various scenarios, always care about d1 relative to d0 - this means that we will consider same case (d0 in front of d1, d1 in front of d0) twice, separately
						// just try turning around
						{
							// D1 is behind us
							// now we have two options - turn by 90' or 180' - check how much space do we have
							bool turnRight = d1OnRightOfD0 > 0.0f;
							if (abs(d1OnRightOfD0) < halfDoorWidth * 1.05f)
							{
								turnRight = yawDiff > 0.0f;
							}
							for_count(int, allowElevators, 2)
							{
								if (!avoidElevators)
								{
									allowElevators = 1;
								}
								for_count(int, dir, 2)
								{
									Vector2 baseNewDoorAt;
									Vector2 baseNewDoorDir;
									// remember we're working with outbound
									if (turnRight)
									{
										baseNewDoorAt = vrMatD0.location_to_world(Vector3(-halfDoorWidth, -halfDoorWidth, 0.0f)).to_vector2();
										baseNewDoorDir = -vrDirD0.rotated_right();
									}
									else
									{
										baseNewDoorAt = vrMatD0.location_to_world(Vector3(halfDoorWidth, -halfDoorWidth, 0.0f)).to_vector2();
										baseNewDoorDir = vrDirD0.rotated_right();
									}

									float minDist = 0.0f;
									float maxDist = 0.0f;
									float maxDistRandomCoef = 1.0f; // see usage below to learn about this variable
									{
										// calculate how much place do we have from turning point right to the end of the room
										Vector2 intersection;
										if (useBeInZone.calc_intersection(baseNewDoorAt, -vrDirD0, intersection))
										{
											maxDist = max(0.0f, Vector2::dot(intersection - baseNewDoorAt, -vrDirD0));
											maxDist = max(0.0f, maxDist - halfDoorWidth); // because we will never be able to put it outside, why should we try it then?
											// check if it is possible to put it before the other door
											float otherDoorDist = Vector2::dot(vrLocD1 - baseNewDoorAt, -vrDirD0);
											otherDoorDist -= doorWidth; // to cover both halves (new door's and d1's)
											if (otherDoorDist >= 0.0f)
											{
												maxDist = min(maxDist, otherDoorDist);
												maxDistRandomCoef = 3.0f;
											}
											else
											{
												// maybe beyond it then?
												otherDoorDist += doorWidth * 2.0f; // to cover both halves (new door's and d1's)
												if (otherDoorDist >= 0.0f && otherDoorDist <= maxDist + MARGIN)
												{
													minDist = min(otherDoorDist, maxDist);
												}
											}
										}									
									}
									float const inwardsABit = (halfDoorWidth * 0.02f); // to counter making narrower
									Vector2 const inwardsD1ABit = vrSegD1.get_right_dir() * inwardsABit;
									Segment2 vrSegD1MovedABit(vrSegD1.get_start() + inwardsD1ABit, vrSegD1.get_end() + inwardsD1ABit);
									int triesLeft = 20;
									bool orgTurnRight = turnRight;
									while (triesLeft--)
									{
										newDoorAt = baseNewDoorAt;
										newDoorDir = baseNewDoorDir;
										float moveBy = clamp(rg.get_float(minDist, maxDist * maxDistRandomCoef), minDist, maxDist); // give more chance to hit max dist
										newDoorAt += -vrDirD0 * moveBy;

										Vector2 inwardsNewABit = -newDoorDir * inwardsABit; // reversed!
										Segment2 newSegMovedABit(newDoorAt - newDoorDir * halfDoorWidth + inwardsNewABit, newDoorAt + newDoorDir * halfDoorWidth + inwardsNewABit);
										VR::Zone newDoorZone;
										newDoorZone.be_rect(Vector2(doorWidth, doorWidth * 2.0f), Vector2::zero);
										newDoorZone.place_at(newDoorAt, newDoorDir);
										if (useBeInZone.does_contain(newDoorZone, -zoneThreshold))
										{
											if (!allowElevators)
											{
												if (Vector2::dot(-newDoorDir, vrDirD1) > 0.7f &&
													(newDoorZone.does_intersect_with(vrSegD1MovedABit) || vrZoneD1.does_intersect_with(newSegMovedABit)))
												{
													// skip, we would end with an elevator, maybe other direction or placing somewhere else will help
													continue;
												}
											}
											handled = true;
											resultFit = NextShouldBeOkButGuessingABit;
#ifdef ALLOW_OUTPUT_GENERATION_DEBUG_VISUALISER
											if (debugThis)
											{
												dv->start_gathering_data();
												dv->clear();
												DebugVisualiserUtils::add_doors_to_debug_visualiser(dv, useBeInZone, corridorD0, vrLocD0, vrDirD0, corridorD1, vrLocD1, vrDirD1, doorWidth);
#ifdef DEBUG_VISUALISE_LOCAL
												DebugVisualiserUtils::add_doors_to_debug_visualiser(dv, useBeInZone, corridorD0, vrLocD0DL, vrDirD0DL, corridorD1, vrLocD1DL, vrDirD1DL, doorWidth);
#endif
												DebugVisualiserUtils::add_door_to_debug_visualiser(dv, nullptr, newDoorAt, newDoorDir, doorWidth, Colour::yellow);
#ifdef DEBUG_VISUALISE_LOCAL
												DebugVisualiserUtils::add_door_to_debug_visualiser(dv, nullptr, vrPlacementD0DebugLocal.location_to_world(vrPlacementD0.location_to_local(newDoorAt.to_vector3())).to_vector2(),
																								vrPlacementD0DebugLocal.vector_to_world(vrPlacementD0.vector_to_local(newDoorDir.to_vector3())).to_vector2(),
																								doorWidth, Colour::greenDark);
#endif
												newDoorZone.debug_visualise(dv.get(), Colour::yellow);
												dv->add_text(Vector2::zero, String(TXT("turned")), Colour::greenDark, 0.3f);
												dv->end_gathering_data();
												dv->show_and_wait_for_key();
											}
#endif
											break;
										}
									}
									if (handled)
									{
										break;
									}
									turnRight = rg.get_chance(0.8f)? orgTurnRight : !orgTurnRight;
								}
								if (handled)
								{
									break;
								}
							}
						}
					}
					if (!handled)
					{
						float d1OnRightOfD0 = Vector2::dot(vrLocD1 - vrLocD0, -vrDirD0.rotated_right());

						// try adding elevator as they are
						if (absYawDiff < 30.0f && abs(d1OnRightOfD0) < doorWidth)
						{
							newDoorAt = vrLocD1;
							newDoorDir = vrDirD1.axis_aligned();

							// check if elevator would fit
							VR::Zone tryDoorZone;
							tryDoorZone.be_rect(Vector2(doorWidth, doorWidth * 2.0f), Vector2::zero);
							tryDoorZone.place_at(newDoorAt, newDoorDir);
							// both sides fit
							if (useBeInZone.does_contain(tryDoorZone, -zoneThreshold))
							{
								handled = true;
								resultBeElevator = true;
								resultFit = FailSafeElevator; // it is a failsafe elevator no matter you say
#ifdef ALLOW_OUTPUT_GENERATION_DEBUG_VISUALISER
								if (debugThis)
								{
									dv->start_gathering_data();
									dv->clear();
									DebugVisualiserUtils::add_doors_to_debug_visualiser(dv, useBeInZone, corridorD0, vrLocD0, vrDirD0, corridorD1, vrLocD1, vrDirD1, doorWidth);
#ifdef DEBUG_VISUALISE_LOCAL
									DebugVisualiserUtils::add_doors_to_debug_visualiser(dv, useBeInZone, corridorD0, vrLocD0DL, vrDirD0DL, corridorD1, vrLocD1DL, vrDirD1DL, doorWidth);
#endif
									DebugVisualiserUtils::add_door_to_debug_visualiser(dv, nullptr, newDoorAt, newDoorDir, doorWidth, Colour::greenDark);
#ifdef DEBUG_VISUALISE_LOCAL
									DebugVisualiserUtils::add_door_to_debug_visualiser(dv, nullptr, vrPlacementD0DebugLocal.location_to_world(vrPlacementD0.location_to_local(newDoorAt.to_vector3())).to_vector2(),
										vrPlacementD0DebugLocal.vector_to_world(vrPlacementD0.vector_to_local(newDoorDir.to_vector3())).to_vector2(),
										doorWidth, Colour::greenDark);
#endif
									dv->add_text(Vector2::zero, String(TXT("failsafe elevator as they are")), Colour::greenDark, 0.3f);
									dv->end_gathering_data();
									dv->show_and_wait_for_key();
								}
#endif
							}
						}
					}
					if (!handled)
					{
						// try adding elevator, but align axis and move to one side
						newDoorAt = vrLocD0;
						newDoorDir = vrDirD0.axis_aligned();
						float tryDir = rg.get_chance(0.5f) ? 1.0f : -1.0f;
						for_count(int, tryDirIdx, 2)
						{
							Vector2 inDir = newDoorDir.rotated_right() * tryDir;
							Vector2 furthest;
							if (useBeInZone.calc_intersection(newDoorAt, inDir, furthest))
							{
								float maxDist = max(0.0f, (furthest - newDoorAt).length() - halfDoorWidth);
								float atDist = min(maxDist, rg.get_float(halfDoorWidth * 2.0f, halfDoorWidth * 6.0f));
#ifdef ALLOW_OUTPUT_GENERATION_DEBUG_VISUALISER
								if (debugThis)
								{
									dv->start_gathering_data();
									dv->clear();
									DebugVisualiserUtils::add_doors_to_debug_visualiser(dv, useBeInZone, corridorD0, vrLocD0, vrDirD0, corridorD1, vrLocD1, vrDirD1, doorWidth);
#ifdef DEBUG_VISUALISE_LOCAL
									DebugVisualiserUtils::add_doors_to_debug_visualiser(dv, useBeInZone, corridorD0, vrLocD0DL, vrDirD0DL, corridorD1, vrLocD1DL, vrDirD1DL, doorWidth);
#endif
									dv->add_arrow(vrLocD0, newDoorAt, Colour::green);
									dv->add_arrow(newDoorAt, newDoorAt + inDir * atDist, Colour::red);
									dv->add_arrow(newDoorAt + inDir * atDist, furthest - inDir * halfDoorWidth, Colour::blue);
									dv->end_gathering_data();
									dv->show_and_wait_for_key();
								}
#endif
								newDoorAt = newDoorAt + inDir * atDist;
								VR::Zone tryDoorZone;
								tryDoorZone.be_rect(Vector2(doorWidth, doorWidth * 2.0f), Vector2::zero);
								tryDoorZone.place_at(newDoorAt, newDoorDir);
								// both sides fit
								if (useBeInZone.does_contain(tryDoorZone, -zoneThreshold))
								{
									handled = true;
									resultBeElevator = (newDoorAt - vrLocD0).length() < doorWidth * 0.98f;
									resultFit = resultBeElevator? FailSafeElevator : FailSafeNonElevator;
#ifdef ALLOW_OUTPUT_GENERATION_DEBUG_VISUALISER
									if (debugThis)
									{
										dv->start_gathering_data();
										dv->clear();
										tryDoorZone.debug_visualise(dv.get(), Colour::yellow);
										DebugVisualiserUtils::add_doors_to_debug_visualiser(dv, useBeInZone, corridorD0, vrLocD0, vrDirD0, corridorD1, vrLocD1, vrDirD1, doorWidth);
#ifdef DEBUG_VISUALISE_LOCAL
										DebugVisualiserUtils::add_doors_to_debug_visualiser(dv, useBeInZone, corridorD0, vrLocD0DL, vrDirD0DL, corridorD1, vrLocD1DL, vrDirD1DL, doorWidth);
#endif
										DebugVisualiserUtils::add_door_to_debug_visualiser(dv, nullptr, newDoorAt, newDoorDir, doorWidth, Colour::greenDark);
#ifdef DEBUG_VISUALISE_LOCAL
										DebugVisualiserUtils::add_door_to_debug_visualiser(dv, nullptr, vrPlacementD0DebugLocal.location_to_world(vrPlacementD0.location_to_local(newDoorAt.to_vector3())).to_vector2(),
																								vrPlacementD0DebugLocal.vector_to_world(vrPlacementD0.vector_to_local(newDoorDir.to_vector3())).to_vector2(),
																								doorWidth, Colour::greenDark);
#endif
										if (resultBeElevator)
										{
											dv->add_text(Vector2::zero, String(TXT("failsafe elevator")), Colour::orange, 0.3f);
										}
										else
										{
											dv->add_text(Vector2::zero, String(TXT("failsafe")), Colour::orange, 0.3f);
										}
										dv->end_gathering_data();
										dv->show_and_wait_for_key();
									}
#endif
									break;
								}
							}
							tryDir = -tryDir;
						}
					}
					if (!handled)
					{
						// we haven't handled yet, time to use random stuff just to try to get out of this
						// 1. if there's enough place, make a turn 90' to left or right
						// 2. do S corridor, move forward by door width and to the sides, check how much can we move
						// 3. do U turn (sharp)
						// 3. add elevator just right where we are
#ifdef ALLOW_OUTPUT_GENERATION_DEBUG_VISUALISER
						String handledResolution;
#endif
						int triesLeft = 50;
						while (triesLeft > 0 && !handled)
						{
							--triesLeft;
							if (rg.get_chance(0.5f))
							{
								// try turning to sides
								float offset = doorWidth * 0.51f;
								newDoorDir = vrDirD0.rotated_by_angle(rg.get_bool() ? 90.0f : -90.0f);
								newDoorAt = vrLocD0 - vrDirD0 * offset + newDoorDir * offset;
								VR::Zone tryDoorZone;
								tryDoorZone.be_rect(Vector2(doorWidth, doorWidth * 2.0f), Vector2::zero);
								tryDoorZone.place_at(newDoorAt, newDoorDir);
								// both sides fit
								if (useBeInZone.does_contain(tryDoorZone, -zoneThreshold))
								{
									handled = true;
									resultBeElevator = false;
									resultFit = FailSafeNonElevator;
#ifdef ALLOW_OUTPUT_GENERATION_DEBUG_VISUALISER
									handledResolution = String(TXT("failsafe ][ 90' turn"));
#endif
								}
							}
							else
							{
								// S-corridor or U-turn
								if (rg.get_chance(0.5f))
								{
									// move forward a width of door and try sides
									float spaceBetween = doorWidth * 1.01f;
									newDoorDir = -vrDirD0;
									newDoorAt = vrLocD0 - vrDirD0 * spaceBetween + vrDirD0.rotated_right() * rg.get_float(-1.0f, 1.0f) * doorWidth;
									VR::Zone tryDoorZone;
									tryDoorZone.be_rect(Vector2(doorWidth, doorWidth * 2.0f), Vector2::zero);
									tryDoorZone.place_at(newDoorAt, newDoorDir);
									// both sides fit
									if (useBeInZone.does_contain(tryDoorZone, -zoneThreshold))
									{
										handled = true;
										resultBeElevator = false;
										resultFit = FailSafeNonElevator;
#ifdef ALLOW_OUTPUT_GENERATION_DEBUG_VISUALISER
										handledResolution = String(TXT("failsafe ][ S-corridor"));
#endif
									}
								}
								else
								{
									// to the side, sharp u-turn
									float offsetRel = 1.01f;
									newDoorDir = vrDirD0;
									newDoorAt = vrLocD0 + vrDirD0.rotated_right() * (rg.get_bool()? -offsetRel : offsetRel) * doorWidth;
									VR::Zone tryDoorZone;
									tryDoorZone.be_rect(Vector2(doorWidth, doorWidth * 2.0f), Vector2::zero);
									tryDoorZone.place_at(newDoorAt, newDoorDir);
									// both sides fit
									if (useBeInZone.does_contain(tryDoorZone, -zoneThreshold))
									{
										handled = true;
										resultBeElevator = false;
										resultFit = FailSafeNonElevator;
#ifdef ALLOW_OUTPUT_GENERATION_DEBUG_VISUALISER
										handledResolution = String(TXT("failsafe ][ U-turn"));
#endif
									}
								}
							}
						}

						if (!handled)
						{
							// just get on with an elevator
							newDoorDir = vrDirD0;
							newDoorAt = vrLocD0;
							handled = true;
							resultBeElevator = true;
							resultFit = FailSafeElevator;
#ifdef ALLOW_OUTPUT_GENERATION_DEBUG_VISUALISER
							handledResolution = String(TXT("failsafe ][ elevator"));
#endif
						}

#ifdef ALLOW_OUTPUT_GENERATION_DEBUG_VISUALISER
						if (handled)
						{
							if (debugThis)
							{
								VR::Zone tryDoorZone;
								tryDoorZone.be_rect(Vector2(doorWidth, doorWidth * 2.0f), Vector2::zero);
								tryDoorZone.place_at(newDoorAt, newDoorDir);

								dv->start_gathering_data();
								dv->clear();
								tryDoorZone.debug_visualise(dv.get(), Colour::yellow);
								DebugVisualiserUtils::add_doors_to_debug_visualiser(dv, useBeInZone, corridorD0, vrLocD0, vrDirD0, corridorD1, vrLocD1, vrDirD1, doorWidth);
#ifdef DEBUG_VISUALISE_LOCAL
								DebugVisualiserUtils::add_doors_to_debug_visualiser(dv, useBeInZone, corridorD0, vrLocD0DL, vrDirD0DL, corridorD1, vrLocD1DL, vrDirD1DL, doorWidth);
#endif
								DebugVisualiserUtils::add_door_to_debug_visualiser(dv, nullptr, newDoorAt, newDoorDir, doorWidth, Colour::greenDark);
#ifdef DEBUG_VISUALISE_LOCAL
								DebugVisualiserUtils::add_door_to_debug_visualiser(dv, nullptr, vrPlacementD0DebugLocal.location_to_world(vrPlacementD0.location_to_local(newDoorAt.to_vector3())).to_vector2(),
									vrPlacementD0DebugLocal.vector_to_world(vrPlacementD0.vector_to_local(newDoorDir.to_vector3())).to_vector2(),
									doorWidth, Colour::greenDark);
#endif
								dv->add_text(Vector2::zero, handledResolution, Colour::orange, 0.3f);
								dv->end_gathering_data();
								dv->show_and_wait_for_key();
							}
						}
#endif
					}
					//an_assert(handled);
					if (!handled)
					{
#ifdef ALLOW_OUTPUT_GENERATION_DEBUG_VISUALISER
						error_dev_ignore(TXT("door not handled!")); // ignorable as we're going to investigate it with debugThis
						debugThis = true;
						dv->activate(); // could be not activated
#else
						error(TXT("door not handled!"));
#endif
					}

#ifdef ALLOW_OUTPUT_GENERATION_DEBUG_VISUALISER
					if (debugThis)
					{
						if (handled)
						{
							dv->start_gathering_data();
							dv->clear();
							DebugVisualiserUtils::add_doors_to_debug_visualiser(dv, useBeInZone, corridorD0, vrLocD0, vrDirD0, corridorD1, vrLocD1, vrDirD1, doorWidth);
#ifdef DEBUG_VISUALISE_LOCAL
							DebugVisualiserUtils::add_doors_to_debug_visualiser(dv, useBeInZone, corridorD0, vrLocD0DL, vrDirD0DL, corridorD1, vrLocD1DL, vrDirD1DL, doorWidth);
#endif
							DebugVisualiserUtils::add_door_to_debug_visualiser(dv, nullptr, newDoorAt, newDoorDir, doorWidth, Colour::greenDark);
#ifdef DEBUG_VISUALISE_LOCAL
							DebugVisualiserUtils::add_door_to_debug_visualiser(dv, nullptr, vrPlacementD0DebugLocal.location_to_world(vrPlacementD0.location_to_local(newDoorAt.to_vector3())).to_vector2(),
																								vrPlacementD0DebugLocal.vector_to_world(vrPlacementD0.vector_to_local(newDoorDir.to_vector3())).to_vector2(),
																								doorWidth, Colour::greenDark);
#endif
							dv->add_text(Vector2::zero, String(TXT("OK")), Colour::greenDark, 0.3f);
							dv->end_gathering_data();
							dv->show_and_wait_for_key();
						}
						else
						{
							dv->start_gathering_data();
							dv->clear();
							DebugVisualiserUtils::add_doors_to_debug_visualiser(dv, useBeInZone, corridorD0, vrLocD0, vrDirD0, corridorD1, vrLocD1, vrDirD1, doorWidth);
#ifdef DEBUG_VISUALISE_LOCAL
							DebugVisualiserUtils::add_doors_to_debug_visualiser(dv, useBeInZone, corridorD0, vrLocD0DL, vrDirD0DL, corridorD1, vrLocD1DL, vrDirD1DL, doorWidth);
#endif
							dv->add_text(Vector2::zero, String(TXT("NOT HANDLED")), Colour::red, 0.3f);
							dv->end_gathering_data();
							dv->show_and_wait_for_key();
						}
					}
#endif
					if (handled)
					{
						Matrix44 newDoorMat = look_at_matrix(newDoorAt.to_vector3(), (newDoorAt + newDoorDir).to_vector3(), Vector3::zAxis);
#ifdef AN_DEVELOPMENT_OR_GENERATION_ISSUES
#ifdef AN_ASSERT
						Vector2 leftDN = newDoorMat.location_to_world(Vector3::xAxis * doorSizeD1.x.min * makeItNarrower).to_vector2();
						Vector2 rightDN = newDoorMat.location_to_world(Vector3::xAxis * doorSizeD1.x.max * makeItNarrower).to_vector2();
#ifndef AN_CLANG
						Vector2 lN_0 = vrMatD0.location_to_local(leftDN.to_vector3()).to_vector2();
						Vector2 rN_0 = vrMatD0.location_to_local(rightDN.to_vector3()).to_vector2();
#endif
#endif
#ifndef AN_CLANG
						Vector2 l0_N = newDoorMat.location_to_local(leftD0.to_vector3()).to_vector2();
						Vector2 r0_N = newDoorMat.location_to_local(rightD0.to_vector3()).to_vector2();
#endif
						Vector2 relLoc0N = vrMatD0.location_to_local(newDoorAt.to_vector3()).to_vector2();
						float newAbsYawDiff = abs(relative_angle(newDoorDir.angle() - vrYawD0));
						if (!resultBeElevator)
						{
#ifdef AN_ASSERT
							if (vrDirD0.is_axis_aligned() &&
								vrDirD1.is_axis_aligned())
							{
								// this condition makes sense only for axis aligned doors
								an_assert(vrMatD0.location_to_local(leftDN.to_vector3()).y <= inFrontThreshold, TXT("should be %.3f <= %.3f"), vrMatD0.location_to_local(leftDN.to_vector3()).y, inFrontThreshold);
								an_assert(vrMatD0.location_to_local(rightDN.to_vector3()).y <= inFrontThreshold, TXT("should be %.3f <= %.3f"), vrMatD0.location_to_local(rightDN.to_vector3()).y, inFrontThreshold);
							}
#endif
							an_assert(newDoorMat.location_to_local(leftD0.to_vector3()).y <= inFrontThreshold);
							an_assert(newDoorMat.location_to_local(rightD0.to_vector3()).y <= inFrontThreshold);
							todo_note(TXT("this assert below this line gets triggered but it still looks fine"));
							an_assert_info(abs(relLoc0N.x) < -relLoc0N.y * 0.5f || relLoc0N.y < -doorWidth || newAbsYawDiff < 135.0f, TXT("ignore or investigate me, I won't break anything")); // check if they are in front of each other or if there is enough space between them to add 2ip
#ifdef AN_HANDLE_GENERATION_ISSUES
							if (!(abs(relLoc0N.x) < -relLoc0N.y * 0.5f || relLoc0N.y < -doorWidth || newAbsYawDiff < 135.0f))
							{
#ifndef AN_DEVELOPMENT
								Game::get()->add_generation_issue(NAME(door_notSureAboutThisOne));
#endif
							}
#endif
						}
#endif
						// clear 180 turn as we're going to keep it the way it is now
						an_assert(resultFit > Unknown);
						resultPlacementD1 = newDoorMat.to_transform();
						resultDo180.clear();
					}
					todo_note(TXT("consider corridorD1 as for different placement"));
					todo_note(TXT("consider situation where both doors share place - cart based corridor"));
				}
			}
		}
	}

	// select the better fit
	{
		int betterIdx = 0;
#ifdef ALLOW_OUTPUT_GENERATION_DEBUG_VISUALISER
		int resultFitsFinal[2];
		resultFitsFinal[0] = resultFits[0];
		resultFitsFinal[1] = resultFits[1];
#endif
		if (allow180)
		{
			int r1 = resultFits[1];
			int r0 = resultFits[0];
			{
				float distThreshold[2];
				float distDivider[2];

				float unitLength = doorWidth;
				distThreshold[0] = 0.0f;
				distDivider[0] = unitLength * 1.0f;

				if (r0 == r1)
				{
					distThreshold[0] = 0.0f;
					distDivider[0] = unitLength;
					distDivider[1] = distDivider[0];
					distThreshold[1] = distThreshold[0];
				}
				else
				{
					distDivider[1] = distDivider[0];
					distThreshold[1] = distThreshold[0];

					int i = r0 > r1 ? 0 : 1;
					distThreshold[i] = unitLength * 0.5f;
					distDivider[i] = unitLength * 1.5f;
				}

				// prefer closer
				if (vrCorridorNested < 6)
				{
					// check against BOTH doors, d0 and result
					for_count(int, tgtIdx, 2)
					{
						for_count(int, rIdx, 2)
						{
							Transform dp;
							if (tgtIdx == 0)
							{
								dp = corridorD0->get_vr_space_placement();
							}
							else
							{
								dp = (rIdx == 1 ? Transform::do180 : Transform::identity).to_world(corridorD1->get_vr_space_placement());
							}
							Vector2 dpLoc = dp.get_translation().to_vector2().normal();
							Vector2 dpFwd = -dp.get_axis(Axis::Forward).to_vector2().normal();

							auto& r = rIdx == 0 ? r0 : r1;
							Transform drp;
							if (rIdx == 0)
							{
								drp = resultPlacementsD1[rIdx].get(corridorD1->get_vr_space_placement());
							}
							else
							{
								drp = resultPlacementsD1[rIdx].get(Transform::do180.to_world(corridorD1->get_vr_space_placement()));
							}
							Vector2 drLoc = drp.get_translation().to_vector2();
							Vector2 drFwd = -drp.get_axis(Axis::Forward).to_vector2();

							if (ResultFitPenalty::for_distance(resultFits[rIdx]))
							{
								float dist = (drLoc - dpLoc).length();
								dist = max(0.0f, dist - distThreshold[rIdx]);
								int qd = TypeConversions::Normal::f_i_closest(dist / distDivider[rIdx]);
								r -= qd;
							}
							if (ResultFitPenalty::for_different_dir(resultFits[rIdx]))
							{
								float dot = Vector2::dot(dpFwd, drFwd);
								// 1 aligned! -1 not aligned at all
								float penalty = remap_value(dot, 1.0f, -1.0f, 0.0f, 1.0f);
								int qd = TypeConversions::Normal::f_i_closest(penalty * 6.0f);
								r -= qd;
							}
						}
					}

					// check if the exits lead to something bad, like putting the result on top of the next door, if so, give a penalty
				}
			}
#ifdef ALLOW_OUTPUT_GENERATION_DEBUG_VISUALISER
			resultFitsFinal[0] = r0;
			resultFitsFinal[1] = r1;
#endif
			betterIdx = r1 > r0 ? 1 : 0;
			if (r0 == r1)
			{
				Random::Generator nrg = baseRG;
				nrg.advance_seed(808345, 297295);
				betterIdx = nrg.get_int(2);
			}
		}
		auto & resultPlacementD1 = resultPlacementsD1[betterIdx];

#ifdef ALLOW_OUTPUT_GENERATION_DEBUG_VISUALISER
		if (debugThis)
		{
			Transform vrPlacementD0 = corridorD0->get_vr_space_placement();
			Transform vrPlacementD1 = (betterIdx == 1? Transform::do180 : Transform::identity).to_world(corridorD1->get_vr_space_placement());
			Vector2 vrLocD0 = vrPlacementD0.get_translation().to_vector2();
			Vector2 vrDirD0 = vrPlacementD0.get_axis(Axis::Y).to_vector2();
			Vector2 vrRitD0 = vrDirD0.rotated_right();
			Vector2 vrLocD1 = vrPlacementD1.get_translation().to_vector2();
			Vector2 vrDirD1 = vrPlacementD1.get_axis(Axis::Y).to_vector2();
			Vector2 vrRitD1 = vrDirD1.rotated_right();

			Vector2 newDoorAt = resultPlacementD1.get(vrPlacementD1).get_translation().to_vector2();
			Vector2 newDoorDir = resultPlacementD1.get(vrPlacementD1).get_axis(Axis::Y).to_vector2();

			dv->start_gathering_data();
			dv->clear();
			DebugVisualiserUtils::add_doors_to_debug_visualiser(dv, maxBeInZone, corridorD0, vrLocD0, vrDirD0, corridorD1, vrLocD1, vrDirD1, doorWidth);
			DebugVisualiserUtils::add_door_to_debug_visualiser(dv, nullptr, newDoorAt, newDoorDir, doorWidth, Colour::greenDark);
			dv->add_text(Vector2::zero, String::printf(TXT("RESULT (%S;%i(%i)v%i(%i))%S"), betterIdx == 0 ? TXT("did as is") : TXT("did 180"), resultFitsFinal[0], resultFits[0], resultFitsFinal[1], resultFits[1], resultBeElevators[betterIdx]? TXT(" elevator") : TXT("")), Colour::greenDark, 0.3f);
			dv->end_gathering_data();
			dv->show_and_wait_for_key();
		}
#endif

		if (resultPlacementD1.is_set())
		{
			DOOR_IN_ROOM_HISTORY(corridorD1, TXT("set vr placement [make vr corridor (1)]"));
			corridorD1->set_vr_space_placement(resultPlacementD1.get(), true /* to force the placement in given 180 turn */);
		}
		do180ForD1 = resultDo180s[betterIdx];
		beElevator = resultBeElevators[betterIdx];
	}


	{	// store in one same space
		// check if we rotated, if so, store turned 180
		if (do180ForD1.should())
		{
			// keep them in the same space
			DOOR_IN_ROOM_HISTORY(corridorD1, TXT("set vr placement [make vr corridor (2)]"));
			corridorD1->set_vr_space_placement(Transform::do180.to_world(corridorD1->get_vr_space_placement()), true /* to force it */);
		}
		do180ForD1.clear();
	}

	// place doors as they are placed in vr space (apply 180!
	Transform vrSpacePlacementD0 = corridorD0->get_vr_space_placement();
	Transform vrSpacePlacementD1 = do180ForD1.if_required(corridorD1->get_vr_space_placement());

	corridor->access_tags().set_tag(NAME(vrCorridorUsing180), do180ForD1.should()? 1.0f : 0.0f); // marked to remember that it went better with reversed door

	corridorD0->set_placement(vrSpacePlacementD0);
	corridorD1->set_placement(vrSpacePlacementD1);

#ifdef AN_DEVELOPMENT
	if (!mainD->get_in_sub_world() || !mainD->get_in_sub_world()->is_test_sub_world())
	{
		// we check against maxBeInZone as it is the wider one
		if (!maxBeInZone.does_contain(vrSpacePlacementD0.get_translation().to_vector2(), doorWidth * 0.4f) ||
			!maxBeInZone.does_contain(vrSpacePlacementD1.get_translation().to_vector2(), doorWidth * 0.4f))
		{
			DebugVisualiserPtr dv(new DebugVisualiser(corridor->get_name()));
			dv->set_background_colour(Colour::lerp(0.05f, Colour::greyLight, Colour::red));
			dv->activate();
			dv->add_text(Vector2::zero, String(TXT("door outside!")), Colour::red, 0.3f);
			DebugVisualiserUtils::add_doors_to_debug_visualiser(dv, maxBeInZone,
				corridorD0, vrSpacePlacementD0.get_translation().to_vector2(), vrSpacePlacementD0.get_axis(Axis::Forward).to_vector2(),
				corridorD1, vrSpacePlacementD1.get_translation().to_vector2(), vrSpacePlacementD1.get_axis(Axis::Forward).to_vector2(),
				doorWidth);
			dv->end_gathering_data();
			dv->show_and_wait_for_key();

			an_assert(maxBeInZone.does_contain(vrSpacePlacementD0.get_translation().to_vector2(), doorWidth * 0.4f));
			an_assert(maxBeInZone.does_contain(vrSpacePlacementD1.get_translation().to_vector2(), doorWidth * 0.4f));
		}
	}
#endif

	if (beElevator)
	{
		corridor->set_name(corridor->get_name() + TXT(" (elevator)"));
	}

	// ready for further generation if we had to generate again
	Random::Generator nrg = baseRG;
#ifdef OUTPUT_GENERATION
	output(TXT("nrg %S"), nrg.get_seed_string().to_char());
#endif
	nrg.advance_seed(89234, 117947);
#ifdef OUTPUT_GENERATION
	output(TXT("nrg [2] %S"), nrg.get_seed_string().to_char());
#endif
	corridor->set_origin_room(sourceRoom->get_origin_room());
	corridor->set_individual_random_generator(nrg.spawn());
#ifdef OUTPUT_GENERATION
	output(TXT("corridor rg %S"), corridor->get_individual_random_generator().get_seed_string().to_char());
#endif
	// do not call run_wheres_my_point_processor_on_generate as we can't tell which room should we use

	bool generated = false;
	if (! generated)
	{
		RoomType const * roomType = nullptr;
		Random::Generator rg = corridor->get_individual_random_generator();
		rg.advance_seed(8954, 2397534);
		// let's depend on doors first as they might be very important and want to override, if they do, we assume that provided types will have such overrides that will carry on;
		// then on rooms, maybe they have some overrides, and one of them should point at region in which we are, so that could be the case of asking "corridor" room
		// try room beyond source door first

		// we trust source more as other may be unreliable if other is world separator door - world separator doors break world into cells
		if (!roomType)
		{
			roomType = beElevator ? sourceDoor->get_door_vr_corridor_elevator_room_type(rg)
								  : sourceDoor->get_door_vr_corridor_room_room_type(rg);
		}
		if (!roomType && ! otherDoor->is_world_separator_door()) // at this moment only trust it if it is not world separator door
		{
			roomType = beElevator ? otherDoor->get_door_vr_corridor_elevator_room_type(rg)
								  : otherDoor->get_door_vr_corridor_room_room_type(rg);
		}
		if (!roomType && sourceRoom)
		{
			roomType = beElevator ? sourceRoom->get_door_vr_corridor_elevator_room_type(rg)
								  : sourceRoom->get_door_vr_corridor_room_room_type(rg);
		}
		// try beyond other door
		if (!roomType)
		{
			roomType = beElevator ? otherDoor->get_door_vr_corridor_elevator_room_type(rg)
								  : otherDoor->get_door_vr_corridor_room_room_type(rg);
		}
		if (!roomType && otherRoom)
		{
			roomType = beElevator ? otherRoom->get_door_vr_corridor_elevator_room_type(rg)
								  : otherRoom->get_door_vr_corridor_room_room_type(rg);
		}
		if (roomType)
		{
			corridor->set_room_type(roomType);

			{	// should have an environment (create it before generating, to generate meshes etc)
				if (!corridor->get_environment() && sourceRoom)
				{
					corridor->set_environment(sourceRoom->get_environment(), sourceRoom->get_environment_requested_placement());
				}
				if (!corridor->get_environment() && otherRoom)
				{
					corridor->set_environment(otherRoom->get_environment(), otherRoom->get_environment_requested_placement());
				}
			}

			{	// should have a reverb
				if (!corridor->get_reverb() && sourceRoom)
				{
					corridor->set_reverb(sourceRoom->get_reverb());
				}
				if (!corridor->get_reverb() && otherRoom)
				{
					corridor->set_reverb(otherRoom->get_reverb());
				}
			}

			//corridor->create_own_copy_of_environment(); // create own copy to alter bits

			generated = corridor->generate();
			an_assert(corridor->is_vr_arranged(), TXT("generated corridor should be at least vr arranged"));
		}
		else
		{
			error(TXT("no generator found!"));
		}
	}

	if (!generated)
	{
		scoped_call_stack_info(TXT("no generator found or not generated properly, using fallback!"));
		error(TXT("no generator found or not generated properly, using fallback!"));
#ifdef ALLOW_OUTPUT_GENERATION_DEBUG_VISUALISER
		if (debugThis)
		{
			dv->start_gathering_data();
			dv->clear();
			dv->add_text(Vector2::zero, String(TXT("couldn't generate!")), Colour::red, 0.3f);
			dv->end_gathering_data();
			dv->show_and_wait_for_key();
		}
#endif
		// we're about to generate a plane, just place doors on the plane
		for_every_ptr(door, corridor->get_all_doors())
		{
			if (door->is_visible() &&
				door->get_door_on_other_side() &&
				door->get_door_on_other_side()->is_vr_space_placement_set())
			{
				DOOR_IN_ROOM_HISTORY(door, TXT("set vr placement [make vr corridor (3)]"));
				scoped_call_stack_info(TXT("set vr placement [make vr corridor (3)]"));
				door->set_vr_space_placement(Door::reverse_door_placement(door->get_door_on_other_side()->get_vr_space_placement()));
				door->set_placement(door->get_vr_space_placement());
			}
		}

		Framework::Material * floorMaterial = Library::get_current()->get_materials().find(hardcoded Framework::LibraryName(String(TXT("materials:main"))));
		corridor->generate_plane_room(floorMaterial, nullptr, !corridor->get_environment() || !corridor->get_environment()->get_info().get_sky_box_material());
	}
}

float Door::calculate_vr_width() const
{
	return doorType->calculate_vr_width() * holeWholeScale.x;
}

void Door::on_new_door_type()
{
	operation = doorType->get_operation();
	if (initialOperation.is_set())
	{
		operation = initialOperation;
	}

	tags.base_on(&doorType->get_tags());

	if (!can_be_closed())
	{
		openFactor = openTarget = 1.0f;
	}

	if (operation.is_set())
	{
		if (operation.get() == DoorOperation::StayClosed ||
			operation.get() == DoorOperation::StayClosedTemporarily)
		{
			openFactor = 0.0f;
		}
		else if (operation.get() == DoorOperation::StayOpen)
		{
			openFactor = 1.0f;
		}
	}

	if (auto* sp = doorType->get_door_hole_shader_program())
	{
		doorHoleShaderProgramInstance.set_shader_program(sp->get_shader_program());
		doorHoleShaderProgramInstance.set_uniform(NAME(doorHoleCentre), doorType->get_hole()->get_centre(DoorSide::A) /* should be symmetrical */);
	}
	else
	{
		doorHoleShaderProgramInstance.set_shader_program(nullptr);
	}

	wingSoundsPlayer.set_size(doorType->get_wings().get_size());
}

void Door::force_door_type(DoorType const* _doorType)
{
	ASSERT_ASYNC_OR_LOADING_OR_PREVIEW;

	doorTypeForced = _doorType;
	update_type();
#ifdef AN_DEVELOPMENT
	dev_check_if_ok();
#endif
}

#ifdef AN_DEVELOPMENT
void Door::dev_check_if_ok() const
{
	an_assert(!linkedDoorA || !linkedDoorA->has_world_active_room_on_other_side() || linkedDoorA->get_other_room_transform().is_ok() || linkedDoorA->get_other_room_transform().is_zero());
	an_assert(!linkedDoorB || !linkedDoorB->has_world_active_room_on_other_side() || linkedDoorB->get_other_room_transform().is_ok() || linkedDoorB->get_other_room_transform().is_zero());
}
#endif

bool Door::update_type()
{
#ifdef AN_DEVELOPMENT
	dev_check_if_ok();
#endif

	bool changed = false;
	DoorType const* newDoorType = doorTypeOriginal;
#ifdef AN_DEBUG_DOOR_TYPE_REPLACER
	output(TXT("updating type for door d%p (a:dr'%p, b:dr'%p)"), this, linkedDoorA, linkedDoorB);
#endif
	if (doorTypeForced)
	{
		newDoorType = doorTypeForced;
	}
	else
	{
		bool keepDoorType = false;
		if (linkedDoorA) keepDoorType |= linkedDoorA->get_in_room()->should_keep_door_types();
		if (linkedDoorB) keepDoorType |= linkedDoorB->get_in_room()->should_keep_door_types();
		if (!keepDoorType &&
			(!is_world_separator_door() || (linkedDoorA && linkedDoorB)))
		{
			an_assert(linkedDoorA);
			an_assert(linkedDoorA->get_in_room());
			an_assert(linkedDoorB);
			an_assert(linkedDoorB->get_in_room());

#ifdef AN_DEBUG_DOOR_TYPE_REPLACER
			output(TXT("a : %S [%S]"), linkedDoorA->get_in_room()->get_name().to_char(), linkedDoorA->get_in_room()->get_room_type() ? linkedDoorA->get_in_room()->get_room_type()->get_name().to_string().to_char() : TXT("--"));
			output(TXT("    tagged \"%S\""), linkedDoorA->get_tags().to_string().to_char());
			output(TXT("    %S"), linkedDoorA->get_in_room()->get_door_type_replacer() ? linkedDoorA->get_in_room()->get_door_type_replacer()->get_name().to_string().to_char() : TXT("--"));
			output(TXT("b : %S [%S]"), linkedDoorB->get_in_room()->get_name().to_char(), linkedDoorB->get_in_room()->get_room_type() ? linkedDoorB->get_in_room()->get_room_type()->get_name().to_string().to_char() : TXT("--"));
			output(TXT("    tagged \"%S\""), linkedDoorB->get_tags().to_string().to_char());
			output(TXT("    %S"), linkedDoorB->get_in_room()->get_door_type_replacer() ? linkedDoorB->get_in_room()->get_door_type_replacer()->get_name().to_string().to_char() : TXT("--"));
			output(TXT("original door type :: %S"), doorTypeOriginal ? doorTypeOriginal->get_name().to_string().to_char() : TXT("--"));
			output(TXT("door tags %S"), get_tags().to_string().to_char());
#endif
			newDoorType = DoorTypeReplacer::process(cast_to_nonconst(doorTypeOriginal),
				DoorTypeReplacerContext().is_world_separator(is_world_separator_door()),
				linkedDoorA, linkedDoorA->get_in_room()->get_door_type_replacer(),
				linkedDoorB, linkedDoorB->get_in_room()->get_door_type_replacer());
#ifdef AN_DEBUG_DOOR_TYPE_REPLACER
			output(TXT("new : %S"), newDoorType ? newDoorType->get_name().to_string().to_char() : TXT("--"));
#endif
		}
	}

	changed = doorType != newDoorType;

#ifdef AN_DEBUG_DOOR_TYPE_REPLACER
	if (changed)
	{
		output(TXT("changed door type to : dt\"%S\""), newDoorType ? newDoorType->get_name().to_string().to_char() : TXT("--"));
		output(TXT("        changed from : dt\"%S\""), doorType ? doorType->get_name().to_string().to_char() : TXT("--"));
	}
#endif
	doorType = newDoorType;

	if (changed)
	{
		on_new_door_type();

		if (linkedDoorA)
		{
			DOOR_IN_ROOM_HISTORY(linkedDoorA, TXT("door type = \"%S\""), doorType ? doorType->get_name().to_string().to_char() : TXT("--"));
			linkedDoorA->reinit_meshes_if_init();
			linkedDoorA->update_movement_info();
		}
		if (linkedDoorB)
		{
			DOOR_IN_ROOM_HISTORY(linkedDoorB, TXT("door type = \"%S\""), doorType ? doorType->get_name().to_string().to_char() : TXT("--"));
			linkedDoorB->reinit_meshes_if_init();
			linkedDoorB->update_movement_info();
		}

		if (auto* g = Game::get())
		{
			g->on_changed_door_type(this);
		}
	}

#ifdef AN_DEVELOPMENT
	dev_check_if_ok();
#endif

	{
		soundsPlayer.stop();
		soundsPlayerForSecondaryDoorType.stop();
		for_every(wsp, wingSoundsPlayer)
		{
			wsp->stop();
		}
	}

	return changed;
}

float Door::adjust_sound_level(float _soundLevel) const
{
	float coef = 1.0f;
	if (doorType)
	{
		coef = min(coef, doorType->get_sound_hearable_when_open().calculate_at(get_abs_open_factor()));
	}
	if (secondaryDoorType)
	{
		coef = min(coef, secondaryDoorType->get_sound_hearable_when_open().calculate_at(get_abs_open_factor()));
	}
	return _soundLevel * coef;
}

void Door::set_secondary_door_type(DoorType const * _doorType)
{
	if (secondaryDoorType == _doorType)
	{
		return;
	}

	secondaryDoorType = _doorType;
	
	if (linkedDoorA && linkedDoorB)
	{
		update_type();
	}

	// in any case, secondary remains same
	if (linkedDoorA)
	{
		linkedDoorA->reinit_meshes_if_init();
	}
	if (linkedDoorB)
	{
		linkedDoorB->reinit_meshes_if_init();
	}
#ifdef AN_DEVELOPMENT
	dev_check_if_ok();
#endif

	{
		soundsPlayer.stop();
		soundsPlayerForSecondaryDoorType.stop();
		for_every(wsp, wingSoundsPlayer)
		{
			wsp->stop();
		}
	}
}

void Door::set_nominal_door_width(float _nominalDoorWidth)
{
	set_nominal_door_size(_nominalDoorWidth, NP);
}

void Door::set_nominal_door_height(float _nominalDoorHeight)
{
	set_nominal_door_size(NP, _nominalDoorHeight);
}

void Door::set_nominal_door_size(Optional<float> const & _width, Optional<float> const & _height)
{
	if ((! _width.is_set() || (nominalDoorWidth.is_set() && nominalDoorWidth.get() == _width.get())) &&
		(!_height.is_set() || nominalDoorHeight == _height.get()))
	{
		return;
	}

	if (_width.is_set())
	{
		nominalDoorWidth = _width.get();
	}

	if (_height.is_set())
	{
		nominalDoorHeight = _height.get();
	}

#ifdef INSPECT_DOOR_NOMINAL_SIZE
	output(TXT("updating nominal door size for existing door types..."));
#endif
	DoorType::generate_all_meshes();
#ifdef INSPECT_DOOR_NOMINAL_SIZE
	output(TXT("updated"));
#endif
}

DoorInRoom* Door::get_linked_door_in(Room* _room) const
{
	if (linkedDoorA && linkedDoorA->get_in_room() == _room)
	{
		return linkedDoorA;
	}
	if (linkedDoorB && linkedDoorB->get_in_room() == _room)
	{
		return linkedDoorB;
	}
	return nullptr;
}

void Door::mark_cant_move_through()
{
	canMoveThrough = false;
	if (linkedDoorA) linkedDoorA->update_movement_info();
	if (linkedDoorB) linkedDoorB->update_movement_info();
}

bool Door::can_move_through() const
{
	return canMoveThrough && doorType && !doorType->is_fake_one_way_window();
}

bool Door::is_relevant_for_movement() const
{
	return doorType && doorType->is_relevant_for_movement();
}

bool Door::is_relevant_for_vr() const
{
	return doorType && doorType->is_relevant_for_vr();
}

void Door::set_game_related_system_flag(int _flag)
{
	ASSERT_SYNC;
	set_flag(gameRelatedSystemFlags, _flag);
}

void Door::clear_game_related_system_flag(int _flag)
{
	ASSERT_SYNC;
	clear_flag(gameRelatedSystemFlags, _flag);
}

void Door::set_visible(bool _visible)
{
	if (visiblePending.get(!_visible) != _visible)
	{
		visiblePending = _visible;
	}
	else
	{
		visiblePending.clear();
	}
}

Optional<float> const& Door::get_vr_elevator_altitude() const
{
	if (linkedDoorA && linkedDoorA->get_vr_elevator_altitude().is_set()) return linkedDoorA->get_vr_elevator_altitude();
	if (linkedDoorB && linkedDoorB->get_vr_elevator_altitude().is_set()) return linkedDoorB->get_vr_elevator_altitude();
	return vrElevatorAltitude;
}

Range2 Door::calculate_compatible_size(DoorSide::Type _side) const
{
	Range2 result = Range2::empty;
	if (result.is_empty())
	{
		if (auto* dt = get_door_type())
		{
			result.include(dt->calculate_compatible_size(_side, get_hole_scale()));
		}
	}
	if (result.is_empty())
	{
		if (auto* dt = get_secondary_door_type())
		{
			result.include(dt->calculate_compatible_size(_side, get_hole_scale()));
		}
	}
	return result;
}

bool Door::is_replacable_by_door_type_replacer() const
{
	if (replacableByDoorTypeReplacer.is_set())
	{
		return replacableByDoorTypeReplacer.get();
	}
	if (doorType)
	{
		return doorType->is_replacable_by_door_type_replacer();
	}
	return true;
}

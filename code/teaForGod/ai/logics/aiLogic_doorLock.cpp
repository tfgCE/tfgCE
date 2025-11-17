#include "aiLogic_doorLock.h"

#include "..\..\game\game.h"
#include "..\..\modules\gameplay\modulePilgrim.h"
#include "..\..\modules\custom\mc_emissiveControl.h"
#include "..\..\modules\custom\mc_hitIndicator.h"
#include "..\..\pilgrimage\pilgrimageInstanceOpenWorld.h"

#include "..\..\..\core\system\video\video3dPrimitives.h"

#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\display\display.h"
#include "..\..\..\framework\display\displayDrawCommands.h"
#include "..\..\..\framework\game\gameUtils.h"
#include "..\..\..\framework\module\moduleMovementSwitch.h"
#include "..\..\..\framework\module\modulePresence.h"
#include "..\..\..\framework\module\moduleSound.h"
#include "..\..\..\framework\module\custom\mc_display.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\framework\object\actor.h"
#include "..\..\..\framework\world\world.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

// variables
DEFINE_STATIC_NAME(interactiveDeviceId);
DEFINE_STATIC_NAME(pilgrimLock);

// emissive layers
DEFINE_STATIC_NAME(ready);
DEFINE_STATIC_NAME(pulse);
DEFINE_STATIC_NAME(active);

// sounds
DEFINE_STATIC_NAME(locked);

//

REGISTER_FOR_FAST_CAST(DoorLock);

DoorLock::DoorLock(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
{
	doorLockData = fast_cast<DoorLockData>(_logicData);
}

DoorLock::~DoorLock()
{
	if (display)
	{
		display->use_background(nullptr);
		display->set_on_update_display(this, nullptr);
	}
}

void DoorLock::advance(float _deltaTime)
{
	base::advance(_deltaTime);

	if (auto* dir = doorInRoom.get())
	{
		if (auto* d = dir->get_door())
		{
			doorShouldRemainLocked = d->get_operation() == Framework::DoorOperation::StayClosed;
			float doorOpenFactorNow = abs(d->get_open_factor());
			bool doorLockedNow = doorShouldRemainLocked && doorOpenFactorNow == 0.0f;
			if (doorLockedNow ^ doorLocked)
			{
				timeSinceAnyChange = 0.0f;
				doorLocked = doorLockedNow;
				update_emissive(doorLocked, doorShouldRemainLocked);
				if (doorLocked)
				{
					if (auto* s = get_mind()->get_owner_as_modules_owner()->get_sound())
					{
						s->play_sound(NAME(locked));
					}
				}
			}
			if (doorOpenFactorNow > doorOpenFactor || doorOpenFactorNow == 1.0f)
			{
				if (!doorOpen) timeSinceAnyChange = 0.0f;
				doorOpen = true;
			}
			if (doorOpenFactorNow < doorOpenFactor || doorOpenFactorNow == 0.0f)
			{
				if (doorOpen) timeSinceAnyChange = 0.0f;
				doorOpen = false;
			}
			if (!doorShouldRemainLocked && pilgrimLock && pilgrimIn &&
				(doorOpenFactorNow < doorOpenFactor && doorOpenFactorNow == 0.0f))
			{
				lock_door(true);
			}
			doorOpenFactor = doorOpenFactorNow;
		}
	}

	if (updateEmissiveOnce)
	{
		updateEmissiveOnce = false;
		update_emissive(doorLocked, doorShouldRemainLocked);
	}
	timeSinceAnyChange += _deltaTime;
	timeToRedraw -= _deltaTime;
	if (timeToRedraw <= 0.0f)
	{
		float interval = clamp(0.1f * timeSinceAnyChange * 0.3f, 0.02f, 0.6f);
		timeToRedraw = Random::get_float(interval * 0.6f, interval * 1.2f);
		redrawNow = true;
	}

	if (! display)
	{
		if (auto* mind = get_mind())
		{
			if (auto* imo = mind->get_owner_as_modules_owner())
			{
				if (auto* displayModule = imo->get_custom<::Framework::CustomModules::Display>())
				{
					display = displayModule->get_display();

					if (display)
					{
						display->draw_all_commands_immediately();
						display->set_on_update_display(this,
						[this](Framework::Display* _display)
						{
							if (!redrawNow)
							{
								return;
							}
							redrawNow = false;

							_display->drop_all_draw_commands();

							Colour drawColour = (doorShouldRemainLocked || doorLocked) ? doorLockData->lockedColour : (doorOpen ? doorLockData->autoOpenColour : doorLockData->autoClosedColour);
							VectorInt2 cellSize = doorLockData->cellSize;

							if (drawTS.get_time_since() > 2.0f)
							{
								_display->add((new Framework::DisplayDrawCommands::CLS(drawColour)));
							}
							drawTS.reset();

							_display->add((new Framework::DisplayDrawCommands::Custom())->use_simple(
								[drawColour, cellSize](Framework::Display* _display, ::System::Video3D* _v3d)
								{
									Vector2 bl = _display->get_left_bottom_of_screen();
									Vector2 tr = _display->get_right_top_of_screen();

									for_count(int, i, Random::get_int_from_range(5, 10))
									{
										VectorInt2 cellCount = TypeConversions::Normal::f_i_cells((tr - bl) / cellSize.to_vector2());
										VectorInt2 cellAt;
										cellAt.x = Random::get_int_from_range(0, cellCount.x);
										cellAt.y = Random::get_int_from_range(0, cellCount.y);

										Vector2 cbl = bl + (cellAt * cellSize).to_vector2();
										Vector2 ctr = cbl + (cellSize - VectorInt2::one).to_vector2();

										::System::Video3DPrimitives::fill_rect_2d(drawColour, cbl, ctr);
									}
								}));
						});
					}
				}
			}
		}
	}
}

void DoorLock::learn_from(SimpleVariableStorage & _parameters)
{
	base::learn_from(_parameters);

	pilgrimLock = _parameters.get_value<bool>(NAME(pilgrimLock), pilgrimLock);
}

void DoorLock::update_emissive(bool _locked, bool _locking)
{
	ai_log(this, TXT("update emissive [%S]"), _locked ? TXT("locked") : TXT("not locked"));

	if (button.get())
	{
		if (auto* ec = button->get_custom<CustomModules::EmissiveControl>())
		{
			if (_locked)
			{

				ec->emissive_activate(NAME(active));
				ec->emissive_deactivate(NAME(pulse));
			}
			else
			{
				ec->emissive_deactivate(NAME(active));
				if (_locking)
				{
					ec->emissive_activate(NAME(pulse));
				}
				else
				{
					ec->emissive_deactivate(NAME(pulse));
				}
			}

			// ready is always active - as a background
			ec->emissive_activate(NAME(ready));
		}
	}
}

void DoorLock::lock_door(Optional<bool> const& _lock)
{
	if (auto* dir = doorInRoom.get())
	{
		if (auto* d = dir->get_door())
		{
			if (d->get_operation() == Framework::DoorOperation::StayClosed)
			{
				if (!_lock.is_set() || ! _lock.get())
				{
					d->set_operation(Framework::DoorOperation::StayOpen, Framework::Door::DoorOperationParams().allow_auto());
					update_emissive(false, false);
				}
			}
			else
			{
				if (!_lock.is_set() ||  _lock.get())
				{
					d->set_operation(Framework::DoorOperation::StayClosed);
					update_emissive(doorLocked, true);
				}
			}
		}
	}
}

LATENT_FUNCTION(DoorLock::execute_logic)
{
	PERFORMANCE_GUARD_LIMIT(0.005f, TXT("[ai door lock] execute logic"));

	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("execute logic"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(bool, buttonPressed);

	LATENT_VAR(float, pilgrimCheckTimeLeft);

	auto * imo = mind->get_owner_as_modules_owner();
	auto * self = fast_cast<DoorLock>(logic);

	pilgrimCheckTimeLeft -= LATENT_DELTA_TIME;

	LATENT_BEGIN_CODE();

	buttonPressed = false;
	pilgrimCheckTimeLeft = 0.0f;
	self->pilgrimIn = false;

	ai_log(self, TXT("door lock, hello!"));

	if (imo)
	{
		ai_log(self, TXT("get button"));

#ifdef AN_USE_AI_LOG
		if (auto* id = imo->get_variables().get_existing<int>(NAME(interactiveDeviceId)))
		{
			ai_log(self, TXT("id = %i"), *id);
		}
#endif

		while (!self->button.get())
		{
			if (auto* id = imo->get_variables().get_existing<int>(NAME(interactiveDeviceId)))
			{
				ai_log(self, TXT("id = %i"), *id);
				auto* world = imo->get_in_world();
				an_assert(world);

				world->for_every_object_with_id(NAME(interactiveDeviceId), *id,
					[self, imo](Framework::Object* _object)
					{
						if (_object != imo)
						{
							self->button = _object;
							return true; // one is enough
						}
						return false; // keep going on
					});
			}
			LATENT_WAIT(Random::get_float(0.2f, 0.5f));
		}

		while (!self->doorInRoom.get())
		{
			if (auto* inRoom = imo->get_presence()->get_in_room())
			{
				Vector3 imoLoc = imo->get_presence()->get_centre_of_presence_WS();
				float bestDistSq = 0.0f;
				Framework::DoorInRoom* bestDoorInRoom = nullptr;
				//FOR_EVERY_ALL_DOOR_IN_ROOM(door, inRoom)
				for_every_ptr(door, inRoom->get_doors())
				{
					if (!door->is_visible()) continue;
					float distSq = (door->get_hole_centre_WS() - imoLoc).length_squared();
					if (bestDistSq > distSq || !bestDoorInRoom)
					{
						bestDoorInRoom = door;
						bestDistSq = distSq;
					}
				}
				self->doorInRoom = bestDoorInRoom;
			}
			LATENT_WAIT(Random::get_float(0.2f, 0.5f));
		}

		ai_log(self, TXT("button and door in room accessed"));

		if (self->button.get())
		{
			if (auto* ec = self->button->get_custom<CustomModules::EmissiveControl>())
			{
				{
					auto& layer = ec->emissive_access(NAME(active));
					layer.set_colour(self->doorLockData->buttonColourLocked);
				}
				{
					auto& layer = ec->emissive_access(NAME(ready));
					layer.set_colour(self->doorLockData->buttonColourNotLocked);
				}
				{
					auto& layer = ec->emissive_access(NAME(pulse));
					layer.set_colour(self->doorLockData->buttonColourLocked);
				}
			}
		}
	}

	while (true)
	{
		if (pilgrimCheckTimeLeft <= 0.0f)
		{
			pilgrimCheckTimeLeft = Random::get_float(0.1f, 0.4f);
			{
				bool isPilgrimInNow = false;
				{
					//FOR_EVERY_OBJECT_IN_ROOM(object, imo->get_presence()->get_in_room())
					for_every_ptr(object, imo->get_presence()->get_in_room()->get_objects())
					{
						if (object->get_gameplay_as<ModulePilgrim>())
						{
							isPilgrimInNow = true;
							break;
						}
					}
				}

				self->pilgrimIn = isPilgrimInNow;
				if (!self->pilgrimIn)
				{
					self->lock_door(false);
				}
			}
		}
		{
			bool newButtonPressed = false;
			if (self->button.get())
			{
				if (auto* mms = fast_cast<Framework::ModuleMovementSwitch>(self->button->get_movement()))
				{
					if (mms->is_active_at(1))
					{
						newButtonPressed = true;
					}
				}
			
				if (newButtonPressed && !buttonPressed)
				{
					self->lock_door();
				}
			}

			buttonPressed = newButtonPressed;
		}
		LATENT_WAIT(Random::get_float(0.1f, 0.2f));
	}

	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}

//

REGISTER_FOR_FAST_CAST(DoorLockData);

bool DoorLockData::load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	buttonColourNotLocked.load_from_xml_child_node(_node, TXT("buttonColourNotLocked"));
	buttonColourLocked.load_from_xml_child_node(_node, TXT("buttonColourLocked"));
	autoOpenColour.load_from_xml_child_node(_node, TXT("autoOpenColour"));
	autoClosedColour.load_from_xml_child_node(_node, TXT("autoClosedColour"));
	lockedColour.load_from_xml_child_node(_node, TXT("lockedColour"));

	cellSize.load_from_xml_child_or_attr(_node, TXT("cellSize"));

	return result;
}

bool DoorLockData::prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

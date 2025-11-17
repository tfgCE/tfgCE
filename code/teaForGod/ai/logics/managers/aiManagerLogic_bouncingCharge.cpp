#include "aiManagerLogic_bouncingCharge.h"

#include "..\..\..\game\game.h"
#include "..\..\..\game\gameDirector.h"
#include "..\..\..\modules\custom\health\mc_health.h"
#include "..\..\..\modules\gameplay\modulePilgrim.h"

#include "..\..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\..\framework\library\library.h"
#include "..\..\..\..\framework\library\usedLibraryStored.inl"
#include "..\..\..\..\framework\module\moduleAI.h"
#include "..\..\..\..\framework\module\moduleMovementPath.h"
#include "..\..\..\..\framework\module\modulePresence.h"
#include "..\..\..\..\framework\module\moduleTemporaryObjects.h"
#include "..\..\..\..\framework\modulesOwner\modulesOwner.h"
#include "..\..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\..\framework\world\room.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;
using namespace Managers;

//

// params/variables
DEFINE_STATIC_NAME(inRoom);

// temporary object
DEFINE_STATIC_NAME(explode);

// object tags
DEFINE_STATIC_NAME(smallActor);

//

REGISTER_FOR_FAST_CAST(BouncingChargeManager);

BouncingChargeManager::BouncingChargeManager(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
, bouncingChargeManagerData(fast_cast<BouncingChargeManagerData>(_logicData))
{
	random = _mind->get_owner_as_modules_owner()->get_individual_random_generator();
	random.advance_seed(2397, 9752904);
}

BouncingChargeManager::~BouncingChargeManager()
{
}

LATENT_FUNCTION(BouncingChargeManager::execute_logic)
{
	LATENT_SCOPE();

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::Logic*, logic); // auto

	LATENT_END_PARAMS();

	auto * self = fast_cast<BouncingChargeManager>(logic);
	auto * imo = mind->get_owner_as_modules_owner();

	LATENT_BEGIN_CODE();

	self->okToAdvance = false;

	// wait a bit to make sure POIs are there
	LATENT_WAIT(1.0f);

	// get information where it does work
	self->inRoom = imo->get_variables().get_value<SafePtr<Framework::Room>>(NAME(inRoom), SafePtr<Framework::Room>());
	if (!self->inRoom.get())
	{
		self->inRoom = imo->get_presence()->get_in_room();
	}

	if (auto* ai = imo->get_ai())
	{
		auto& arla = ai->access_rare_logic_advance();
		arla.reset_to_no_rare_advancement();
	}

	ai_log_colour(self, Colour::blue);
	ai_log(self, TXT("random \"%S\""), self->random.get_seed_string().to_char());
	ai_log_no_colour(self);

	if (self->inRoom.get())
	{
		{
			Framework::PointOfInterestInstance* poi;
			if (self->inRoom->find_any_point_of_interest(self->bouncingChargeManagerData->poiA, poi))
			{
				self->poiA = poi;
			}
		}
		{
			Framework::PointOfInterestInstance* poi;
			if (self->inRoom->find_any_point_of_interest(self->bouncingChargeManagerData->poiB, poi))
			{
				self->poiB = poi;
			}
		}
		if (self->poiA.get() &&
			self->poiB.get())
		{
			self->okToAdvance = true;
		}
	}

	while (true)
	{
		LATENT_WAIT_NO_RARE_ADVANCE(Random::get_float(0.1f, 0.5f));
	}

	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}

void BouncingChargeManager::advance(float _deltaTime)
{
	base::advance(_deltaTime);

	if (! okToAdvance)
	{
		return;
	}
	if (auto* imo = get_mind()->get_owner_as_modules_owner())
	{
		if (!charge.get() &&
			! chargeDOC.get() &&
			poiA.get() &&
			poiB.get())
		{
			Framework::DelayedObjectCreation* doc = new Framework::DelayedObjectCreation();
			doc->activateImmediatelyEvenIfRoomVisible = true;
			doc->wmpOwnerObject = imo;
			doc->inRoom = inRoom.get();
			doc->name = TXT("bouncing charge");
			doc->objectType = bouncingChargeManagerData->chargeActorType.get();
			Transform targetPlacement = (lastSafeT? poiA : poiB)->calculate_placement();
			Transform otherPlacement = (lastSafeT? poiB : poiA)->calculate_placement();
			targetPlacement = look_at_matrix(targetPlacement.get_translation(), otherPlacement.get_translation(), targetPlacement.get_axis(Axis::Up)).to_transform();
			doc->placement = targetPlacement;
			doc->randomGenerator = random.spawn();
			doc->priority = 1000;
			doc->checkSpaceBlockers = false;

			inRoom->collect_variables(doc->variables);

			chargeDOC = doc;

			Game::get()->queue_delayed_object_creation(doc);
		}

		if (auto* doc = chargeDOC.get())
		{
			if (doc->is_done())
			{
				charge = doc->createdObject.get();
				charge->set_instigator(imo);
				chargeDOC.clear();
				timeAtStop = bouncingChargeManagerData->timeAtStop - 0.5f;
				isMoving = false;

				if (auto* chargeIMO = charge.get())
				{
					if (auto* mp = fast_cast<Framework::ModuleMovementPath>(chargeIMO->get_movement()))
					{
						mp->use_linear_path(poiA->calculate_placement(), poiB->calculate_placement(), lastSafeT);
					}
				}
			}
		}
	}

	if (auto* chargeIMO = charge.get())
	{
		if (!isMoving)
		{
			timeAtStop += _deltaTime;
			if (! GameDirector::is_violence_disallowed())
			{
				if (timeAtStop >= bouncingChargeManagerData->timeAtStop)
				{
					timeAtStop = 0.0f;
					isMoving = true;
					if (auto* mp = fast_cast<Framework::ModuleMovementPath>(chargeIMO->get_movement()))
					{
						mp->set_target_t(1.0f - lastSafeT);
						mp->set_acceleration(100.0f);
						float v = 2.0f;
						if (poiA.get() && poiB.get())
						{
							float length = (poiA->calculate_placement().get_translation() - poiB->calculate_placement().get_translation()).length();
							float timeWhole = length = bouncingChargeManagerData->velocityBase;
							float v = length / max(bouncingChargeManagerData->timeRunMin, timeWhole - bouncingChargeManagerData->timeAtStop);
							v = min(v, bouncingChargeManagerData->velocityMax);
						}
						v = v * (1.0f + 0.5f * clamp(GameSettings::get().difficulty.aiMean, 0.0f, 1.0f));
						mp->set_speed(v);
					}
				}
			}
		}
		else
		{
			if (auto* mp = fast_cast<Framework::ModuleMovementPath>(chargeIMO->get_movement()))
			{
				float t = mp->get_t();
				if (abs(t - (1.0f - lastSafeT)) < 0.001f)
				{
					lastSafeT = t;
					isMoving = false;
					timeAtStop = 0.0f;
				}
			}
		}

		if (isMoving)
		{
			bool explode = false;
			if (GameSettings::get().difficulty.npcs > GameSettings::NPCS::NonAggressive)
			{
				//FOR_EVERY_OBJECT_IN_ROOM(object, inRoom.get())
				for_every_ptr(object, inRoom->get_objects())
				{
					if (object != chargeIMO)
					{
						if (object->get_custom<CustomModules::Health>())
						{
							if (object->get_gameplay_as<ModulePilgrim>() ||
								!object->get_tags().get_tag(NAME(smallActor)))
							{
								Vector3 relLoc = chargeIMO->get_presence()->get_placement().location_to_local(object->get_presence()->get_centre_of_presence_WS());
								if (abs(relLoc.y) < 0.3f)
								{
									explode = true;
									break;
								}
							}
						}
					}
				}
			}
			if (explode)
			{
				isMoving = false;
				timeAtStop = bouncingChargeManagerData->timeAtStop; // immediately release another one
				if (auto* mp = fast_cast<Framework::ModuleMovementPath>(chargeIMO->get_movement()))
				{
					float newT = lastSafeT;
					//float farT = 1.0f - round(mp->get_t());

					// if we're close to the starting point, give more time, if we're far, launch immediately
					timeAtStop = bouncingChargeManagerData->timeAtStop * (1.0f - 0.7f * (1.0f - abs(newT - mp->get_t())));

					mp->set_t(newT);
				}

				if (auto* h = chargeIMO->get_custom<CustomModules::Health>())
				{
					h->perform_death_without_reward();
				}
				charge.clear();
			}
		}
	}
}

void BouncingChargeManager::debug_draw(Framework::Room* _room) const
{
/*
#ifdef AN_DEBUG_RENDERER
	debug_context(_room);

	debug_no_context();
#endif
*/
}

//

REGISTER_FOR_FAST_CAST(BouncingChargeManagerData);

BouncingChargeManagerData::BouncingChargeManagerData()
{
}

BouncingChargeManagerData::~BouncingChargeManagerData()
{
}

bool BouncingChargeManagerData::load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	chargeActorType.load_from_xml(_node, TXT("chargeActorType"), _lc);

	XML_LOAD_NAME_ATTR(_node, poiA);
	XML_LOAD_NAME_ATTR(_node, poiB);
	
	XML_LOAD_FLOAT_ATTR(_node, timeAtStop);
	XML_LOAD_FLOAT_ATTR(_node, timeRunMin);
	XML_LOAD_FLOAT_ATTR(_node, velocityBase);
	XML_LOAD_FLOAT_ATTR(_node, velocityMax);

	return result;
}

bool BouncingChargeManagerData::prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	result &= chargeActorType.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);

	return result;
}

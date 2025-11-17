#include "aiManagerLogic_airshipsManager.h"

#include "..\..\..\game\game.h"
#include "..\..\..\game\gameDirector.h"
#include "..\..\..\modules\moduleMovementAirship.h"

#include "..\..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\..\framework\library\library.h"
#include "..\..\..\..\framework\module\moduleAI.h"
#include "..\..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\..\framework\modulesOwner\modulesOwner.h"
#include "..\..\..\..\framework\object\actor.h"
#include "..\..\..\..\framework\world\room.h"

#include "..\..\..\..\core\debug\\debugRenderer.h"

#ifdef AN_ALLOW_BULLSHOTS
#include "..\..\..\..\framework\game\bullshotSystem.h"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

#define AVOID_AIRSHIP 1
#define AVOID_OBSTACLE 2
#define AVOID_GAME_DIRECTOR_AIR_PROHIBITED_PLACE 3

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;
using namespace Managers;

//

DEFINE_STATIC_NAME(inRoom);

// bullshot system allowances
DEFINE_STATIC_NAME_STR(bsAirships, TXT("airships"));

//

REGISTER_FOR_FAST_CAST(AirshipsManager);

AirshipsManager::AirshipsManager(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
, airshipsManagerData(fast_cast<AirshipsManagerData>(_logicData))
{
	random = _mind->get_owner_as_modules_owner()->get_individual_random_generator();
	random.advance_seed(2397, 9752904);
}

AirshipsManager::~AirshipsManager()
{
}

int AirshipsManager::get_amount() const
{
	Concurrency::ScopedSpinLock lock(airshipsLock);
	return airships.get_size();
}

Framework::IModulesOwner* AirshipsManager::get_airship(int _idx) const
{
	scoped_call_stack_info_str_printf(TXT("get airship %i"), _idx);
	Concurrency::ScopedSpinLock lock(airshipsLock);
	if (airships.is_index_valid(_idx))
	{
		scoped_call_stack_info(TXT("index valid"));
		auto& airship = airships[_idx].airship;
		scoped_call_stack_info(TXT("got airship"));
		return airship.get();
	}
	else
	{
		scoped_call_stack_info(TXT("no airship"));
		return nullptr;
	}
}

LATENT_FUNCTION(AirshipsManager::execute_logic)
{
	LATENT_SCOPE();

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::Logic*, logic); // auto

	LATENT_END_PARAMS();

	LATENT_VAR(Array<Framework::LibraryStored*>, airshipTypes);

	auto * self = fast_cast<AirshipsManager>(logic);
	auto * imo = mind->get_owner_as_modules_owner();

	LATENT_BEGIN_CODE();

	self->started = false;

	// wait a bit to make sure POIs are there
	LATENT_WAIT(1.0f);

	// get information where it does work
	self->inRoom = imo->get_variables().get_value<SafePtr<Framework::Room>>(NAME(inRoom), SafePtr<Framework::Room>());

	ai_log_colour(self, Colour::blue);
	ai_log(self, TXT("random \"%S\""), self->random.get_seed_string().to_char());
	ai_log_no_colour(self);

	self->started = true;

	{
		Framework::WorldSettingConditionContext wscc(self->inRoom.get());
		wscc.get(Framework::Library::get_current(), Framework::SceneryType::library_type(), airshipTypes, self->airshipsManagerData->airshipType);
	}

	if (auto* ai = imo->get_ai())
	{
		auto& arla = ai->access_rare_logic_advance();
		arla.reset_to_no_rare_advancement();
	}

	while (true)
	{
		if (auto* doc = self->spawningAirshipDOC.get())
		{
			if (doc->is_done())
			{
				Airship airship;
				airship.airship = doc->createdObject.get();
				if (airship.airship.is_set())
				{
					Concurrency::ScopedSpinLock lock(self->airshipsLock);
					self->airships.push_back(airship);
				}
				self->spawningAirshipDOC.clear();
			}
		}
		if (!self->spawningAirshipDOC.is_set())
		{
#ifdef AN_ALLOW_BULLSHOTS
			if (!Framework::BullshotSystem::is_active() ||
				Framework::BullshotSystem::is_setting_active(NAME(bsAirships)))
#endif
			{
				if (self->get_amount() < self->airshipsManagerData->amount)
				{
					Framework::DelayedObjectCreation* doc = new Framework::DelayedObjectCreation();
					doc->activateImmediatelyEvenIfRoomVisible = true;
					doc->wmpOwnerObject = imo;
					doc->inRoom = self->inRoom.get();
					doc->name = TXT("airship");
					doc->objectType = fast_cast<Framework::ObjectType>(airshipTypes[self->random.get_int(airshipTypes.get_size())]);
					doc->placement = Transform(Vector3(0.0f, 0.0f, -1000.0f), Quat::identity); // well below the ground, will be teleported to appropriate location
					an_assert(!self->random.is_zero_seed());
					doc->randomGenerator = self->random.spawn();
					an_assert(!doc->randomGenerator.is_zero_seed());
					doc->priority = 100;
					doc->checkSpaceBlockers = false;

					self->inRoom->collect_variables(doc->variables);

					self->spawningAirshipDOC = doc;

					Game::get()->queue_delayed_object_creation(doc);
				}
				else
				{
					// wait rarer
					LATENT_WAIT_NO_RARE_ADVANCE(Random::get_float(1.0f, 5.0f));
				}
			}
		}
		LATENT_WAIT_NO_RARE_ADVANCE(Random::get_float(0.1f, 0.5f));
	}

	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}

void AirshipsManager::advance(float _deltaTime)
{
	base::advance(_deltaTime);

#ifdef AN_ALLOW_BULLSHOTS
	if (Framework::BullshotSystem::is_active() &&
		! Framework::BullshotSystem::is_setting_active(NAME(bsAirships)))
	{
		return;
	}
#endif

	Concurrency::ScopedSpinLock lock(airshipsLock);

	{
		todo_multiplayer_issue(TXT("we just get player here"));
		if (auto* g = Game::get_as<Game>())
		{
			if (auto* pa = g->access_player().get_actor())
			{
				if (auto* p = pa->get_presence())
				{
					auto* inRoom = p->get_in_room();
					if (mapCellDoneForPlayerInRoom != inRoom)
					{
						mapCellDoneForPlayerInRoom = inRoom;
						if (auto* piow = PilgrimageInstanceOpenWorld::get())
						{
							auto currentMapCell = piow->find_cell_at(inRoom);
							if (currentMapCell.is_set() && currentMapCell.get() != mapCell)
							{
								VectorInt2 moveByCells = mapCell - currentMapCell.get();
								Vector3 moveLocBy = moveByCells.to_vector2().to_vector3() * piow->get_cell_size();
								for_every(airship, airships)
								{
									if (!airship->zone.is_valid())
									{
										airship->location += moveLocBy;
									}
								}
								mapCell = currentMapCell.get();

								SafePtr<Framework::IModulesOwner> safeIMO(get_imo());
								obstaclesValid = false;
								g->add_async_world_job_top_priority(TXT("get airship obstacles"), [safeIMO, this]()
								{
									if (!safeIMO.is_set())
									{
										return;
									}

									if (auto* piow = PilgrimageInstanceOpenWorld::get())
									{
										piow->async_get_airship_obstacles(mapCell, hardcoded 500.0f, obstacles);
										obstaclesValid = true;
									}
								});
							}
						}
					}
				}
			}
		}
	}

	auto* gd = GameDirector::get_active();
	ARRAY_STACK(GameDirector::AirProhibitedPlace, gdAirProhibitedPlaces, max(1, gd ? gd->get_air_prohibited_places() : 0));
	if (gd)
	{
		gd->get_air_prohibited_places(&gdAirProhibitedPlaces);
		for_every(ao, gdAirProhibitedPlaces)
		{
			ao->place = Segment(ao->place.get_start() * Vector3(1.0f, 1.0f, 0.0f), ao->place.get_end() * Vector3(1.0f, 1.0f, 0.0f));
		}
	}

	for_every(airship, airships)
	{
		if (airship->requiresSetup)
		{
			airship->requiresSetup = false;
			AirshipsManagerData::AirshipZone const * inZone = nullptr;
			if (airshipsManagerData)
			{
				int idx = for_everys_index(airship);
				for_every(zone, airshipsManagerData->zones)
				{
					if (idx < zone->amount)
					{
						inZone = zone;
						break;
					}
					idx -= zone->amount;
				}
			}
			Random::Generator rg;
			airship->dir = rg.get_float(0.0f, 360.0f);
			float locationOff = (float)(for_everys_index(airship)) * 70.0f + rg.get_float(-40.0f, 40.0f);
			if (inZone)
			{
				airship->zone = inZone->zone;
				if (inZone->circleInRadius.is_set())
				{
					airship->circleInRadius = rg.get(inZone->circleInRadius.get());
				}
				if (inZone->stayBeyondRadius.is_set())
				{
					airship->stayBeyondRadius = rg.get(inZone->stayBeyondRadius.get());
				}
				if (inZone->stayInRadius.is_set())
				{
					airship->stayInRadius = rg.get(inZone->stayInRadius.get());
				}
				airship->speed = rg.get(inZone->speed);
				if (airship->circleInRadius.is_set())
				{
					airship->speed *= rg.get_bool() ? 1.0f : -1.0f;
				}
				airship->maxDirSpeed = rg.get(inZone->dirSpeed);
				airship->dirSpeedBlendTime = max(0.1f, rg.get(inZone->dirSpeedBlendTime));
				airship->maxDirSpeedBank = rg.get(inZone->dirSpeedBank);

				float altitude = rg.get_float(5.0f, 20.0f);
				if (inZone->altitude.is_set())
				{
					altitude = rg.get(inZone->altitude.get());
				}

				Vector3 loc = Vector3::zero;
				if (airship->circleInRadius.is_set())
				{
					loc = Vector3::yAxis * airship->circleInRadius.get();
					loc.rotate_by_yaw(locationOff);
				}
				else if (airship->stayBeyondRadius.is_set() || airship->stayInRadius.is_set())
				{
					Range distanceAllowed(airship->stayBeyondRadius.get(0.0f), airship->stayInRadius.get(10000.0f));
					loc = Vector3::yAxis * rg.get(distanceAllowed);
					loc.rotate_by_yaw(locationOff);
				}
				else
				{
					loc = Vector3::yAxis * rg.get_float(300.0f, 500.0f);
					loc.rotate_by_yaw(locationOff);
				}
				airship->location = loc;
				airship->location.z = altitude;
			}
			else
			{
				airship->location = Vector3::yAxis * rg.get_float(300.0f, 500.0f);
				airship->location.rotate_by_yaw(locationOff);
				airship->location.z = rg.get_float(30.0f, 100.0f);
				airship->speed = rg.get_float(5.0f, 20.0f);
			}
		}

		// move airship
		if (airship->circleInRadius.is_set())
		{
			Vector3 dir = airship->location.normal_2d().rotated_right();
			float alt = airship->location.z;
			airship->location.z = 0.0f;
			airship->location += dir * airship->speed * _deltaTime;
			airship->location = airship->location.normal() * airship->circleInRadius.get();
			airship->location.z = alt;
			airship->dir = Rotator3::get_yaw(dir) + (airship->speed >= 0.0f? 0.0f : 180.0f);
		}
		else
		{
			Vector3 dir = Vector3::yAxis.rotated_by_yaw(airship->dir);
			airship->location += dir * airship->speed * _deltaTime;
			airship->dir = mod(airship->dir + airship->dirSpeed * _deltaTime, 360.0f);
			float targetDirSpeed = 0.0f;
			float acceleration = max(0.01f, airship->maxDirSpeed / airship->dirSpeedBlendTime);

			if (airship->avoidingObstaclePriority < AVOID_AIRSHIP)
			{
				for_every(a, airships)
				{
					if (a != airship && !a->circleInRadius.is_set() &&
						a->zone == airship->zone)
					{
						float vertDistance = abs(a->location.z - airship->location.z);
						if (vertDistance < hardcoded magic_number 40.0f)
						{
							Vector3 toA = a->location - airship->location;
							toA.z = 0.0f;
							float distance = toA.length();
							float const distToDivert = hardcoded magic_number 250.0f;
							if (distance < distToDivert)
							{
								float pushOut = clamp((1.0f - distance / distToDivert) * 3.0f, 0.0f, 1.0f);
								airship->location -= airship->speed * 0.01f * pushOut * toA.normal();
								{
									// turn away
									float dirToA = toA.to_rotator().yaw;
									float dirToALocal = Rotator3::normalise_axis(dirToA - airship->dir);

									airship->targetDir = airship->dir + Random::get_float(40.0f, 90.0f) * (dirToALocal < 0.0f ? 1.0f : -1.0f);
									airship->avoidingObstaclePriority = AVOID_AIRSHIP;
								}
							}
						}
					}
				}
			}

			if (obstaclesValid && airship->avoidingObstaclePriority < AVOID_OBSTACLE)
			{
				Vector3 inDir = Rotator3(0.0f, airship->targetDir.get(airship->dir), 0.0f).get_forward();

				for_every(o, obstacles)
				{
					if (airship->location.z < o->height + hardcoded magic_number 40.0f)
					{
						Vector3 toOfwd = o->location - (airship->location + hardcoded magic_number inDir * 150.0f);
						Vector3 toO = o->location - airship->location;
						toOfwd.z = 0.0f;
						toO.z = 0.0f;
						float distance = toOfwd.length();
						float const distToDivert = hardcoded magic_number 60.0f;
						if (distance < distToDivert)
						{
							float pushOut = clamp((1.0f - distance / distToDivert) * 3.0f, 0.0f, 1.0f);
							airship->location -= airship->speed * 0.01f * pushOut * toO.normal();
							{
								// turn away
								float dirToO = toO.to_rotator().yaw;
								float dirToOLocal = Rotator3::normalise_axis(dirToO - airship->dir);

								airship->targetDir = Rotator3::get_yaw(inDir) + Random::get_float(20.0f, 30.0f) * (dirToOLocal < 0.0f ? 1.0f : -1.0f);
								airship->avoidingObstaclePriority = AVOID_OBSTACLE;
							}
						}
					}
				}
			}

			if (!gdAirProhibitedPlaces.is_empty() && airship->avoidingObstaclePriority < AVOID_GAME_DIRECTOR_AIR_PROHIBITED_PLACE)
			{
				Vector3 inDir = Rotator3(0.0f, airship->targetDir.get(airship->dir), 0.0f).get_forward();

				Vector3 willBeAt = airship->location + inDir * hardcoded magic_number 20.0f;
				willBeAt.z = 0.0f;
				for_every(ao, gdAirProhibitedPlaces)
				{
					an_assert(ao->place.get_start().z == 0.0f);
					an_assert(ao->place.get_end().z == 0.0f);
					float atT;
					float distance = ao->place.get_distance(willBeAt, OUT_ atT);
					float const distToDivert = hardcoded magic_number 100.0f;
					if (distance < ao->radius + distToDivert)
					{
						Vector3 oLoc = ao->place.get_at_t(atT);
						Vector3 toO = oLoc - airship->location;
						toO.z = 0.0f;
						{
							float pushOut = clamp((1.0f - distance / distToDivert) * 3.0f, 0.0f, 1.0f);
							airship->location -= airship->speed * 0.01f * pushOut * toO.normal();
							{
								// move away
								float dirToO = toO.to_rotator().yaw;

								airship->targetDir = -dirToO + Random::get_float(-20.0f, 20.0f);
								airship->avoidingObstaclePriority = AVOID_GAME_DIRECTOR_AIR_PROHIBITED_PLACE;
							}
						}
					}
				}
			}

			if (airship->targetDir.is_set())
			{
				float yawDeltaRequired = Rotator3::normalise_axis(airship->targetDir.get() - airship->dir);

				float timeRequiredToStop = abs(airship->dirSpeed / acceleration);
				float willGoTillStops = abs(airship->dirSpeed) * timeRequiredToStop - acceleration * sqr(timeRequiredToStop) * 0.5f;
				if (abs(yawDeltaRequired) - willGoTillStops < 5.0f)
				{
					airship->targetDir.clear();
					airship->avoidingObstaclePriority = 0;
				}
				else
				{
					targetDirSpeed = clamp(yawDeltaRequired, -airship->maxDirSpeed, airship->maxDirSpeed);
				}
			}
			else
			{
				if (airship->stayBeyondRadius.is_set())
				{
					if (airship->location.length() < airship->stayBeyondRadius.get())
					{
						airship->targetDir = (airship->location).to_rotator().yaw;
						if (abs(Rotator3::normalise_axis(airship->targetDir.get() - airship->dir)) < 90.0f)
						{
							airship->targetDir.clear();
						}
						else
						{
							float maxOff = 50.0f;
							if (airship->location.z < hardcoded magic_number 300.0f)
							{
								maxOff = 25.0f;
							}
							airship->targetDir = airship->targetDir.get() + Random::get_float(-maxOff, maxOff);
						}
					}
				}
				if (airship->stayInRadius.is_set())
				{
					if (airship->location.length() > airship->stayInRadius.get())
					{
						airship->targetDir = (-airship->location).to_rotator().yaw;
						if (abs(Rotator3::normalise_axis(airship->targetDir.get() - airship->dir)) < 90.0f)
						{
							airship->targetDir.clear();
						}
						else
						{
							float maxOff = 50.0f;
							if (airship->location.z < hardcoded magic_number 300.0f)
							{
								maxOff = 25.0f;
							}
							airship->targetDir = airship->targetDir.get() + Random::get_float(-maxOff, maxOff);
						}
					}
				}
			}

			if (airship->dirSpeed <= targetDirSpeed)
			{
				airship->dirSpeed = min(targetDirSpeed, airship->dirSpeed + acceleration * _deltaTime);
			}
			else
			{
				airship->dirSpeed = max(targetDirSpeed, airship->dirSpeed - acceleration * _deltaTime);
			}
		}

		// store its location
		if (auto* imo = airship->airship.get())
		{
			if (auto* m = fast_cast<ModuleMovementAirship>(imo->get_movement()))
			{
				float roll = 0.0f;
				if (airship->maxDirSpeed != 0.0f)
				{
					roll = (airship->dirSpeed / airship->maxDirSpeed) * airship->maxDirSpeedBank;
				}
				m->be_at(airship->zone, mapCell, Transform(airship->location, Rotator3(0.0f, airship->dir, roll).to_quat()));
			}
		}
	}
}

void AirshipsManager::debug_draw(Framework::Room* _room) const
{
#ifdef AN_DEBUG_RENDERER
	debug_context(_room);

	{
		Concurrency::ScopedSpinLock lock(airshipsLock);

		float _scale = 0.1f;

		for_every(airship, airships)
		{
			Vector3 loc = airship->location * _scale;
			Vector3 undLoc = loc * Vector3::xy;
			Colour colour = Colour::green;
			if (airship->location.z > 300.0f)
			{
				colour = Colour::orange;
			}
			if (airship->location.z > 1000.0f)
			{
				colour = Colour::blue;
			}
			debug_draw_line(true, colour.with_alpha(0.5f), Vector3::zero, undLoc);
			debug_draw_line(true, colour.with_alpha(0.5f), undLoc, loc);
			debug_draw_sphere(true, true, colour, 0.5f, Sphere(loc, 0.2f));
			debug_draw_arrow(true, colour, loc, loc + Rotator3(0.0f, airship->dir, 0.0f).get_forward());
			if (airship->targetDir.is_set())
			{
				debug_draw_arrow(true, Colour::red, loc, loc + Rotator3(0.0f, airship->targetDir.get(), 0.0f).get_forward());
			}
		}

		{
			auto* gd = GameDirector::get_active();
			ARRAY_STACK(GameDirector::AirProhibitedPlace, gdAirProhibitedPlaces, max(1, gd ? gd->get_air_prohibited_places() : 0));
			if (gd)
			{
				gd->get_air_prohibited_places(&gdAirProhibitedPlaces);
				for_every(ao, gdAirProhibitedPlaces)
				{
					ao->place = Segment(ao->place.get_start() * Vector3(1.0f, 1.0f, 0.0f) * _scale, ao->place.get_end() * Vector3(1.0f, 1.0f, 0.0f) * _scale);

					Vector3 s = ao->place.get_start();
					Vector3 e = ao->place.get_end();
					Vector3 s2e = ao->place.get_start_to_end_dir();
					Vector3 r = s2e.rotated_by_yaw(90.0f);

					r *= ao->radius * _scale;
					s2e *= ao->radius * _scale;
					debug_draw_line(true, Colour::red, s, e);
					Vector3 side = ao->place.get_start_to_end_dir().rotated_by_yaw(90.0f);
					debug_draw_line(true, Colour::red.with_alpha(0.6f), s + r, e + r);
					debug_draw_line(true, Colour::red.with_alpha(0.6f), s - r, e - r);
					debug_draw_line(true, Colour::red.with_alpha(0.6f), s - r, s - s2e);
					debug_draw_line(true, Colour::red.with_alpha(0.6f), s + r, s - s2e);
					debug_draw_line(true, Colour::red.with_alpha(0.6f), e - r, e + s2e);
					debug_draw_line(true, Colour::red.with_alpha(0.6f), e + r, e + s2e);
				}
			}
		}
	}

	debug_no_context();
#endif
}

//

REGISTER_FOR_FAST_CAST(AirshipsManagerData);

AirshipsManagerData::AirshipsManagerData()
{
}

AirshipsManagerData::~AirshipsManagerData()
{
}

bool AirshipsManagerData::load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	for_every(node, _node->children_named(TXT("airships")))
	{
		for_every(n, node->children_named(TXT("type")))
		{
			result &= airshipType.load_from_xml(n);
		}
	}

	amount = 0;
	for_every(node, _node->children_named(TXT("zones")))
	{
		for_every(n, node->children_named(TXT("zone")))
		{
			AirshipZone zone;
			zone.amount = n->get_int_attribute(TXT("amount"), zone.amount);
			zone.zone = n->get_name_attribute(TXT("zone"), zone.zone);
			zone.speed.load_from_xml(n, TXT("speed"));
			zone.dirSpeed.load_from_xml(n, TXT("dirSpeed"));
			zone.dirSpeedBank.load_from_xml(n, TXT("dirSpeedBank"));
			zone.dirSpeedBlendTime.load_from_xml(n, TXT("dirSpeedBlendTime"));
			zone.circleInRadius.load_from_xml(n, TXT("circleInRadius"));
			zone.stayBeyondRadius.load_from_xml(n, TXT("stayBeyondRadius"));
			zone.stayInRadius.load_from_xml(n, TXT("stayInRadius"));
			zone.altitude.load_from_xml(n, TXT("altitude"));

			amount += zone.amount;
			zones.push_back(zone);
		}
	}

	return result;
}

bool AirshipsManagerData::prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

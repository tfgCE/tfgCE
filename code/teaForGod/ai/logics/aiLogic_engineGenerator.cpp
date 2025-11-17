#include "aiLogic_engineGenerator.h"

#include "..\..\game\damage.h"
#include "..\..\game\game.h"
#include "..\..\library\library.h"
#include "..\..\utils\lightningDischarge.h"

#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\framework\module\modulePresence.h"
#include "..\..\..\framework\module\moduleSound.h"
#include "..\..\..\framework\module\custom\mc_lightningSpawner.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\framework\object\actor.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

// socket names
DEFINE_STATIC_NAME(lightning_socket_0); // left
DEFINE_STATIC_NAME(lightning_socket_1);
DEFINE_STATIC_NAME(lightning_socket_2); //u
DEFINE_STATIC_NAME(lightning_socket_3); //u
DEFINE_STATIC_NAME(lightning_socket_4);
DEFINE_STATIC_NAME(lightning_socket_5);
DEFINE_STATIC_NAME(lightning_socket_6); //u
DEFINE_STATIC_NAME(lightning_socket_7); //u
DEFINE_STATIC_NAME(lightning_socket_8); // right
DEFINE_STATIC_NAME(lightning_socket_9);
DEFINE_STATIC_NAME(lightning_socket_10); //u
DEFINE_STATIC_NAME(lightning_socket_11); //u
DEFINE_STATIC_NAME(lightning_socket_12);
DEFINE_STATIC_NAME(lightning_socket_13);
DEFINE_STATIC_NAME(lightning_socket_14); //u
DEFINE_STATIC_NAME(lightning_socket_15); //u

// params
DEFINE_STATIC_NAME(damage);
DEFINE_STATIC_NAME(generatorIdx);
DEFINE_STATIC_NAME(redoWallChance);
DEFINE_STATIC_NAME(platformWidth);
DEFINE_STATIC_NAME(platformTileWidth);

// lightnings
DEFINE_STATIC_NAME(idle);

// sounds
DEFINE_STATIC_NAME_STR(hum0, TXT("hum 0"));
DEFINE_STATIC_NAME_STR(hum1, TXT("hum 1"));

//

REGISTER_FOR_FAST_CAST(EngineGenerator);

EngineGenerator::EngineGenerator(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
{
	leftUpSockets.push_back(NAME(lightning_socket_2));
	leftUpSockets.push_back(NAME(lightning_socket_3));
	leftUpSockets.push_back(NAME(lightning_socket_6));
	leftUpSockets.push_back(NAME(lightning_socket_7));
	leftDownSockets.push_back(NAME(lightning_socket_0));
	leftDownSockets.push_back(NAME(lightning_socket_1));
	leftDownSockets.push_back(NAME(lightning_socket_4));
	leftDownSockets.push_back(NAME(lightning_socket_5));
	rightUpSockets.push_back(NAME(lightning_socket_10));
	rightUpSockets.push_back(NAME(lightning_socket_11));
	rightUpSockets.push_back(NAME(lightning_socket_14));
	rightUpSockets.push_back(NAME(lightning_socket_15));
	rightDownSockets.push_back(NAME(lightning_socket_8));
	rightDownSockets.push_back(NAME(lightning_socket_9));
	rightDownSockets.push_back(NAME(lightning_socket_12));
	rightDownSockets.push_back(NAME(lightning_socket_13));
}

EngineGenerator::~EngineGenerator()
{
}

void EngineGenerator::learn_from(SimpleVariableStorage& _parameters)
{
	generatorIdx = _parameters.get_value<int>(NAME(generatorIdx), generatorIdx);
	redoWallChance = _parameters.get_value<float>(NAME(redoWallChance), redoWallChance);
	platformWidth = _parameters.get_value<float>(NAME(platformWidth), platformWidth);
	platformTileWidth = clamp(_parameters.get_value<int>(NAME(platformTileWidth), platformTileWidth), 2, 3);

	mayDoWall = false;
	if (generatorIdx >= 2)
	{
		mayDoWall = mod(generatorIdx, 2) == 0;
	}
	if (generatorIdx >= 8)
	{
		mayDoWall = true;
	}

	damage = Energy::get_from_storage(_parameters, NAME(damage), damage);
}

LATENT_FUNCTION(EngineGenerator::execute_logic)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("execute logic engine generator"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(SafePtr<::Framework::IModulesOwner>, pilgrim);
	LATENT_VAR(Random::Generator, rg);

	auto* imo = mind->get_owner_as_modules_owner();
	auto* self = fast_cast<EngineGenerator>(logic);

	LATENT_BEGIN_CODE();

	rg = imo->get_individual_random_generator();

	if (auto* s = imo->get_sound())
	{
		s->play_sound((self->generatorIdx % 2) == 0 ? NAME(hum0) : NAME(hum1));
	}

	while (true)
	{
		LATENT_WAIT((self->state == State::Wall ? rg.get_float(0.03f, 0.07f) : rg.get_float(0.1f, 0.3f)));
		if (auto* pa = pilgrim.get())
		{
			if (pa->get_presence()->get_in_room() != imo->get_presence()->get_in_room())
			{
				pilgrim.clear();
			}
		}
		if (self->mayDoWall && !pilgrim.is_set())
		{
			todo_multiplayer_issue(TXT("we just get player here"));
			if (auto* g = Game::get_as<Game>())
			{
				if (auto* pa = g->access_player().get_actor())
				{
					if (pa->get_presence()->get_in_room() == imo->get_presence()->get_in_room())
					{
						pilgrim = pa;
					}
				}
			}
		}
		if (auto* pa = pilgrim.get())
		{
			State newState = self->state;
			float const distToOn = 4.0f;
			float const distToOff = 1.0f;
			Vector3 paLoc = pa->get_presence()->get_centre_of_presence_WS();
			Vector3 gnLoc = imo->get_presence()->get_centre_of_presence_WS();
			bool redoWall = false;
			// we rely on Engine Room layout
			if (self->state == State::Off ||
				self->state == State::InVisibleRoom)
			{
				newState = State::Idle;
			}
			if (GameSettings::get().difficulty.npcs > GameSettings::NPCS::NoHostiles)
			{
				if (self->state == State::Idle && paLoc.x < gnLoc.x + distToOn && paLoc.x > gnLoc.x - distToOff)
				{
					newState = State::Wall;
				}
			}
			if (self->state == State::Wall && paLoc.x < gnLoc.x - distToOff)
			{
				newState = State::IdlePostWall;
			}
			if (self->state == State::Wall)
			{
				if (self->redoWallAtDist.is_set())
				{
					if (paLoc.x < gnLoc.x + self->redoWallAtDist.get())
					{
						self->redoWallAtDist.clear();
						redoWall = true;
					}
				}
			}
			if (self->state != newState || redoWall)
			{
				if (self->state == State::Wall &&
					newState == State::IdlePostWall)
				{
					// give prize only if we didn't hit anything
					if (!self->hitAnything)
					{
						Energy experience = GameplayBalance::engine_generator__xp();
						if (experience.is_positive())
						{
							experience = experience.adjusted_plus_one(GameSettings::get().experienceModifier);
							PlayerSetup::access_current().stats__experience(experience);
							GameStats::get().add_experience(experience);
							Persistence::access_current().provide_experience(experience);
						}
					}
				}
				self->state = newState;
				if (self->state == State::Wall)
				{
					static int lastSafe[] = { 0, 0, 0 };
					static int lastWidth = 0;
					static int lastX = 0;
					static bool lastCrouch = false;

					int tryNo = 0;
					while (true)
					{
						++tryNo;
						int curSafe[] = { 0, 0, 0 };
						int width = self->platformTileWidth;
						bool allowCrouch = false;
						if (auto* ps = PlayerSetup::access_current_if_exists())
						{
							allowCrouch = (ps->get_preferences().allowCrouch != Option::Disallow);
						}
						bool crouched = allowCrouch && rg.get_chance(0.3f);

						if (width == 3)
						{
							if (crouched)
							{
								if (rg.get_chance(0.1f))
								{
									width = 1;
								}
								else if (rg.get_chance(0.5f))
								{
									width = 2;
								}
							}
							else
							{
								if (rg.get_chance(0.9f))
								{
									width = 1;
								}
								else
								{
									width = 2;
								}
							}
						}
						else
						{
							width = 2;
							if (crouched)
							{
								if (rg.get_chance(0.1f))
								{
									width = 1;
								}
							}
							else
							{
								if (rg.get_chance(0.9f))
								{
									width = 1;
								}
							}
						}
						int x = 0;
						if (width != self->platformTileWidth)
						{
							x = rg.get_int(self->platformTileWidth - width + 1);
							self->leftUp = x;
							self->leftDown = x;
							self->rightUp = (self->platformTileWidth - width) - x;
							self->rightDown = (self->platformTileWidth - width) - x;
						}
						else
						{
							self->leftUp = 0;
							self->leftDown = 0;
							self->rightUp = 0;
							self->rightDown = 0;
						}

						if (crouched)
						{
							self->leftUp = self->platformTileWidth;
							self->rightUp = self->platformTileWidth;
						}

						for (int i = x; i < x + width; ++i)
						{
							curSafe[i] = 1;
						}

						if (tryNo < 32 && crouched == lastCrouch)
						{
							// avoid having cases where you can just stand still
							if ((curSafe[0] && lastSafe[0]) ||
								(curSafe[1] && lastSafe[1]) ||
								(curSafe[2] && lastSafe[2]))
							{
								continue;
							}
						}

						// we should not have two at the same time
						// the code here is to avoid repeating same setup
						if (x == lastX &&
							width == lastWidth &&
							crouched == lastCrouch)
						{
							continue;
						}
						// no consecutive crouches
						if (crouched && crouched == lastCrouch)
						{
							continue;
						}
						// store
						{
							lastX = x;
							lastWidth = width;
							lastCrouch = crouched;
							for_count(int, i, 3) { lastSafe[i] = curSafe[i]; }
							break;
						}
					}

					self->lastHitSocket.clear();

					if (!redoWall)
					{
						if (rg.get_chance(self->redoWallChance))
						{
							self->redoWallAtDist = clamp(rg.get_float(1.0f, 2.5f), 1.25f, 2.0f);
						}
					}
				}
				if (self->state == State::Idle ||
					self->state == State::IdlePostWall)
				{
					self->set_idle_lightning(true);
				}
				else
				{
					self->set_idle_lightning(false);
				}
			}
			if (self->state == State::Wall)
			{
				int lightningsLeft = rg.get_int_from_range(2, 4);
				while (lightningsLeft > 0)
				{
					--lightningsLeft;
					Name trySocket;
					if (self->lastHitSocket.is_set())
					{
						trySocket = self->lastHitSocket.get();
					}
					else if (self->leftDown > 0 || self->leftUp > 0 || self->rightDown > 0 || self->rightUp > 0)
					{
						int triesLeft = 20;
						while (triesLeft > 0)
						{
							--triesLeft;
							int tryIdx = rg.get_int(4);
							if (tryIdx == 0 && self->leftDown > 0)  { trySocket = self->leftDownSockets[rg.get_int(self->leftDownSockets.get_size())]; break; }
							if (tryIdx == 1 && self->leftUp > 0)    { trySocket = self->leftUpSockets[rg.get_int(self->leftUpSockets.get_size())]; break; }
							if (tryIdx == 2 && self->rightDown > 0) { trySocket = self->rightDownSockets[rg.get_int(self->rightDownSockets.get_size())]; break; }
							if (tryIdx == 3 && self->rightUp > 0)   { trySocket = self->rightUpSockets[rg.get_int(self->rightUpSockets.get_size())]; break; }
						}
					}

					self->lastHitSocket.clear();

					if (trySocket.is_valid())
					{
						int dist = 0;
						if (self->leftDownSockets.does_contain(trySocket)) dist = self->leftDown;
						if (self->leftUpSockets.does_contain(trySocket)) dist = self->leftUp;
						if (self->rightDownSockets.does_contain(trySocket)) dist = self->rightDown;
						if (self->rightUpSockets.does_contain(trySocket)) dist = self->rightUp;

						if (dist > 0)
						{
							float useDist = self->platformWidth * ((float)dist - 0.1f) / ((float)self->platformTileWidth);

							LightningDischarge::Params params;

							params.for_imo(imo);
							params.with_start_placement_OS(imo->get_appearance()->calculate_socket_os(trySocket));
							params.with_max_dist(useDist);
							params.with_end_placement_offset(Vector3(rg.get_float(-0.15f, 0.15f), rg.get_float(-0.2f, 0.0f), rg.get_float(-0.1f, 0.1f)));
							params.with_setup_damage([self](Damage& dealDamage, DamageInfo& damageInfo)
								{
									dealDamage.damage = self->damage;
									dealDamage.cost = self->damage;
									damageInfo.requestCombatAuto = false;
								});
							params.with_ray_cast_search_for_objects(true);
							params.with_wide_search_for_objects(false);
							params.ignore_narrative();

							if (LightningDischarge::perform(params))
							{
								self->hitAnything = true;
								self->lastHitSocket = trySocket;
								lightningsLeft = 0;
							}
						}
					}
				}
			}
		}
		else
		{
			State newState = State::Off;
			if (auto* r = imo->get_presence()->get_in_room())
			{
				if (r->was_recently_seen_by_player())
				{
					newState = State::InVisibleRoom;
				}
			}
			if (self->state != newState)
			{
				self->state = newState;
				if (self->state == State::Off)
				{
					self->set_idle_lightning(false);
				}
				if (self->state == State::InVisibleRoom)
				{
					self->set_idle_lightning(true);
				}
			}
		}
	}

	LATENT_ON_BREAK();
	//
	
	LATENT_ON_END();

	LATENT_END_CODE();
	LATENT_RETURN();
}

void EngineGenerator::set_idle_lightning(bool _idleLightning)
{
	if (idleLightning != _idleLightning)
	{
		idleLightning = _idleLightning;
		if (auto* imo = get_mind()->get_owner_as_modules_owner())
		{
			if (auto* ls = imo->get_custom<Framework::CustomModules::LightningSpawner>())
			{
				if (idleLightning)
				{
					ls->start(NAME(idle));
				}
				else
				{
					ls->stop(NAME(idle));
				}
			}
		}
	}
}

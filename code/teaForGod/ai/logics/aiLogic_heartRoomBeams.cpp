#include "aiLogic_heartRoomBeams.h"

#include "..\..\game\energy.h"
#include "..\..\game\damage.h"
#include "..\..\game\game.h"
#include "..\..\game\gameDirector.h"

#include "..\..\modules\custom\mc_emissiveControl.h"
#include "..\..\modules\custom\health\mc_health.h"

#include "..\..\..\core\system\video\video3dPrimitives.h"

#include "..\..\..\framework\ai\aiMessageHandler.h"
#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\game\gameUtils.h"
#include "..\..\..\framework\module\moduleAI.h"
#include "..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\framework\module\moduleCollision.h"
#include "..\..\..\framework\module\modulePresence.h"
#include "..\..\..\framework\module\moduleSound.h"
#include "..\..\..\framework\module\moduleTemporaryObjects.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\framework\object\object.h"
#include "..\..\..\framework\world\room.h"
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

// parameters
DEFINE_STATIC_NAME(deviceSideCount);
DEFINE_STATIC_NAME(deviceBeamsPerSide);
DEFINE_STATIC_NAME(projectileSpeed);

// variables
DEFINE_STATIC_NAME_STR(spinSpeed, TXT("spin speed"));

// socket
DEFINE_STATIC_NAME(beamRest);

// emissives
DEFINE_STATIC_NAME(rotating);

// sounds
DEFINE_STATIC_NAME(spin);
DEFINE_STATIC_NAME(active);
DEFINE_STATIC_NAME_STR(beamHit, TXT("beam hit"));

// ai messages
DEFINE_STATIC_NAME_STR(aim_start, TXT("heart room beams; start"));
	DEFINE_STATIC_NAME(time);
	DEFINE_STATIC_NAME(speed);
	DEFINE_STATIC_NAME(acceleration);
	DEFINE_STATIC_NAME(accelerateInQuickly);
	DEFINE_STATIC_NAME(leavePlayerAlone);
	DEFINE_STATIC_NAME(noVerticalCheck);
	DEFINE_STATIC_NAME(swapDir);
DEFINE_STATIC_NAME_STR(aim_stop, TXT("heart room beams; stop"));
DEFINE_STATIC_NAME_STR(aim_startShooting, TXT("heart room beams; start shooting"));
	DEFINE_STATIC_NAME(interval);
DEFINE_STATIC_NAME_STR(aim_stopShooting, TXT("heart room beams; stop shooting"));

// temporary objects
DEFINE_STATIC_NAME(shoot);
DEFINE_STATIC_NAME(muzzleFlash);
DEFINE_STATIC_NAME(hit);

// game script traps
DEFINE_STATIC_NAME_STR(gst_stopped, TXT("heart room beams; stopped"));

//

REGISTER_FOR_FAST_CAST(HeartRoomBeams);

HeartRoomBeams::HeartRoomBeams(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const* _logicData)
: base(_mind, _logicData, execute_logic)
{
	heartRoomBeamsData = fast_cast<HeartRoomBeamsData>(_logicData);

	rotateDir = rg.get_bool() ? 1.0f : -1.0f;
}

HeartRoomBeams::~HeartRoomBeams()
{
	if (auto* mind = get_mind())
	{
		if (auto* imo = mind->get_owner_as_modules_owner())
		{
			if (auto* s = imo->get_sound())
			{
				s->stop_sound(NAME(active));
				s->stop_sound(NAME(spin));
			}
		}
	}
}

void HeartRoomBeams::learn_from(SimpleVariableStorage& _parameters)
{
	base::learn_from(_parameters);

	sideCount = _parameters.get_value<int>(NAME(deviceSideCount), sideCount);
	beamsPerSide = _parameters.get_value<int>(NAME(deviceBeamsPerSide), beamsPerSide);

	beams.make_space_for(sideCount * beamsPerSide);
	for_count(int, beamIdx, beamsPerSide)
	{
		for_count(int, sideIdx, sideCount)
		{
			beams.grow_size(1);
			auto& beam = beams.get_last();
			beam.sideIdx = sideIdx;
			beam.beamIdx = beamIdx;

			beam.boneStart = Name(String::printf(TXT("beam%i%i"), sideIdx, beamIdx));
			beam.boneEnd = Name(String::printf(TXT("beam%i%i_end"), sideIdx, beamIdx));

			beam.beamB.set_name(Name(String::printf(TXT("beamB%i"), sideIdx)));
			beam.beamT.set_name(Name(String::printf(TXT("beamT%i"), sideIdx)));
		}
	}

	beamRest.set_name(NAME(beamRest));

	sides.set_size(sideCount);
}

void HeartRoomBeams::advance(float _deltaTime)
{
	base::advance(_deltaTime);

	if (!readyAndRunning && ! beams.is_empty())
	{
		return;
	}

	bool prevActive = active;

	if (Framework::is_preview_game())
	{
		if (!active)
		{
			requestedActive = true;
			timeNextActive = 20.0f;
		}
		shoot = true;
		shootInterval = 0.1f;
	}

	if (active)
	{
		// use time only when beaming
	}
	else if (requestedActive) // next frame to reset
	{
		requestedActive = false;
		timeLeftActive = timeNextActive;
		active = timeLeftActive > 0.0f;
	}

	auto* imo = get_mind()->get_owner_as_modules_owner();

	if (active != prevActive)
	{
		if (auto* em = imo->get_custom<CustomModules::EmissiveControl>())
		{
			if (active)
			{ 
				em->emissive_activate(NAME(rotating));
			}
			else
			{
				em->emissive_deactivate(NAME(rotating));
			}
		}

		if (!active || GameSettings::get().difficulty.aiMean)
		{
			// make it easier to plan (or if mean, behave mean)
			rotateDir = rg.get_bool() ? 1.0f : -1.0f;
		}
		updateBeamTimeLeft = 0.0f;
	}

	auto* ma = imo->get_appearance();
	auto* mp = imo->get_presence();

	float targetSpeed = 0.0f;
	{
		targetSpeed = forceRotateSpeed.get(active ? heartRoomBeamsData->rotateSpeed : heartRoomBeamsData->rotateSpeedDown) * rotateDir;

		float acceleration = active ? heartRoomBeamsData->rotateAccelerateUp : heartRoomBeamsData->rotateAccelerateDown;
		if (forceAcceleration.is_set())
		{
			acceleration = forceAcceleration.get();
		}
		if (accelerateInQuickly)
		{
			acceleration = abs(targetSpeed) * 5.0f;
		}

		if (rotateSpeed <= targetSpeed)
		{
			rotateSpeed = min(targetSpeed, rotateSpeed + acceleration * _deltaTime);
		}
		else
		{
			rotateSpeed = max(targetSpeed, rotateSpeed - acceleration * _deltaTime);
		}
	}

	if (spinSpeedVar.is_valid())
	{
		spinSpeedVar.access<float>() = abs(rotateSpeed);
	}

	rotatingYaw += rotateSpeed * _deltaTime;
	rotatingYaw = Rotator3::normalise_axis(rotatingYaw);
	
	{
		float beamActive = heartRoomBeamsData->beamActive;
		for_every(beam, beams)
		{
			beam->timeSinceUpdate += _deltaTime;
			if (beamActive > 0.0f)
			{
				beam->scale = max(0.01f, 1.0f - beam->timeSinceUpdate / beamActive);
				if (beam->scale <= 0.01f)
				{
					beam->startWS = beam->restStartWS;
					beam->endWS = beam->restEndWS;
				}
			}
			else
			{
				if (active)
				{
					beam->scale = 1.0f;
				}
				else
				{
					beam->startWS = beam->restStartWS;
					beam->endWS = beam->restEndWS;
					beam->scale = 0.01f;
				}
			}
		}
	}
	bool activeBeaming = false;
	if (active && abs(rotateSpeed - targetSpeed) <= 0.05f)
	{
		{
			timeLeftActive -= _deltaTime;
			if (timeLeftActive <= 0.0f)
			{
				active = false;
				for_every(s, sides)
				{
					s->tracked.clear();
				}

				if (!requestedActive)
				{
					Framework::GameScript::ScriptExecution::trigger_execution_trap(NAME(gst_stopped));
				}
			}
		}
		if (active)
		{
			accelerateInQuickly = false; // no need to accelerate in
			activeBeaming = true;
			updateBeamTimeLeft -= _deltaTime;
			if (updateBeamTimeLeft <= 0.0f)
			{
				updateBeamTimeLeft = heartRoomBeamsData->beamInterval;
				if (!heartRoomBeamsData->beamCollisionFlags.is_empty())
				{
					auto& beam = beams[updateBeamIdx];
					beam.timeSinceUpdate = 0.0f;

					float startPt = rg.get_float(0.0f, 1.0f);
					float forwardPt = rg.get_float(0.0f, 1.0f);

					Transform beamBWS = mp->get_placement().to_world(ma->calculate_socket_os(beam.beamB.get_index()));
					Transform beamTWS = mp->get_placement().to_world(ma->calculate_socket_os(beam.beamT.get_index()));
					beam.startLocWS = lerp(startPt, beamBWS.get_translation(), beamTWS.get_translation());

					float beamLength = 100.0f;

					Vector3 dirWS = lerp(forwardPt, beamBWS.get_axis(Axis::Forward), beamTWS.get_axis(Axis::Forward));
					beam.endLocWS = beam.startLocWS + dirWS * beamLength;

					auto& side = sides[beam.sideIdx];

					ArrayStatic<Vector3, 4> endLocationsWS;
					ArrayStatic<Vector3, 4> forceHitLocationWS;
					ArrayStatic<CustomModules::Health*,4> forceHitHealth;
					// go through all health owners in the dir and try to force check them
					{
						Transform checkAt = look_matrix(beam.startLocWS, (beam.endLocWS - beam.startLocWS).normal_2d(), Vector3::zAxis).to_transform();

						for_every_ptr(targetImo, mp->get_in_room()->get_objects())
						{
							auto* h = targetImo->get_custom<CustomModules::Health>();
							if (! h ||
								! h->is_alive() ||
								! targetImo->get_as_actor()) // leave items alone
							{
								continue;
							}

							Optional<Vector3> prevRelLoc;
							Side::Tracked* tracked = nullptr;
							for_every(t, side.tracked)
							{
								if (t->imo == targetImo)
								{
									prevRelLoc = t->prevRelLoc;
									tracked = t;
								}
							}

							Vector3 relLoc = checkAt.location_to_local(targetImo->get_presence()->get_placement().get_translation());

							if (relLoc.y > 0.0f &&
								(noVerticalCheck || abs(relLoc.z) * 0.5f < relLoc.y))
							{
								if (abs(relLoc.x) < 0.3f ||
									(prevRelLoc.get(relLoc).x * relLoc.x) <= 0.0f)
								{
									{
										Vector3 targetLocOS = imo->get_ai()->get_target_placement_os_for(targetImo).get_translation();
										targetLocOS = lerp(0.5f, targetLocOS, imo->get_presence()->get_centre_of_presence_os().get_translation());
										Vector3 targetLocWS = targetImo->get_presence()->get_placement().location_to_world(targetLocOS);

										Vector3 endLocWS = beam.startLocWS + (targetLocWS - beam.startLocWS).normal() * beamLength;
										if (!Framework::GameUtils::is_controlled_by_player(targetImo))
										{
											if (forceHitLocationWS.has_place_left())
											{
												forceHitLocationWS.push_back(endLocWS);
												forceHitHealth.push_back(h);
											}
										}
										else if (! leavePlayerAlone)
										{
											if (endLocationsWS.has_place_left())
											{
												endLocationsWS.push_back(endLocWS);
											}
										}
									}
								}
							}

							if (tracked)
							{
								tracked->prevRelLoc = relLoc;
							}
							else
							{
								Side::Tracked tracked;
								tracked.prevRelLoc = relLoc;
								tracked.imo = targetImo;
								side.tracked.push_back(tracked);
							}
						}

						for_every(t, side.tracked)
						{
							if (! t->imo.get())
							{
								side.tracked.remove_fast_at(for_everys_index(t));
								break;
							}
						}
					}

					if (endLocationsWS.is_empty())
					{
						if (!forceHitLocationWS.is_empty())
						{
							endLocationsWS.push_back(forceHitLocationWS.get_first());
						}
						else
						{
							endLocationsWS.push_back(beam.endLocWS);
						}
					}

					for_count(int, forced, 2)
					{
						auto& endLocs = forced ? forceHitLocationWS : endLocationsWS;
						for_every(l, endLocs)
						{
							Vector3 startLocation = beam.startLocWS;
							Vector3 endLocation = *l;

							CustomModules::Health* hitHealth = nullptr;
							if (forced)
							{
								int i = for_everys_index(l);
								an_assert(forceHitHealth.is_index_valid(i));
								hitHealth = forceHitHealth[i];
							}
							else
							{
								beam.endLocWS = *l;

								Framework::CheckCollisionContext checkCollisionContext;
								checkCollisionContext.collision_info_needed();
								checkCollisionContext.avoid(imo);
								checkCollisionContext.use_collision_flags(heartRoomBeamsData->beamCollisionFlags);
								checkCollisionContext.ignore_temporary_objects();

								Framework::CollisionTrace collisionTrace(Framework::CollisionTraceInSpace::WS);
								collisionTrace.add_location(startLocation);
								collisionTrace.add_location(endLocation);
								Framework::CheckSegmentResult result;

								if (imo->get_presence()->trace_collision(AgainstCollision::Movement, collisionTrace, result, Framework::CollisionTraceFlag::ResultInPresenceRoom, checkCollisionContext))
								{
									endLocation = result.hitLocation;

									if (result.object)
									{
										auto* check = fast_cast<::Framework::IModulesOwner>(result.object);
										while (check)
										{
											if (auto* h = check->get_custom<CustomModules::Health>())
											{
												hitHealth = h;
												break;
											}
											check = check->get_presence()->get_attached_to();
										}
									}
								}

								beam.endLocWS = endLocation;
							}

							if (hitHealth)
							{
								if (leavePlayerAlone && Framework::GameUtils::is_controlled_by_player(hitHealth->get_owner()))
								{
									continue;
								}

								if (auto* tos = imo->get_temporary_objects())
								{
									tos->spawn_in_room(NAME(hit), mp->get_in_room(), Transform(endLocation, Quat::identity));
								}

								Damage dealDamage;

								dealDamage.damage = heartRoomBeamsData->damage;
								dealDamage.cost = Energy(1);

								dealDamage.damageType = DamageType::Gameplay;
								dealDamage.armourPiercing = EnergyCoef::one();

								auto* ep = hitHealth->get_owner()->get_presence();
								auto* presence = imo->get_presence();

								DamageInfo damageInfo;
								damageInfo.damager = imo;
								damageInfo.source = imo;
								damageInfo.instigator = imo;

								hitHealth->adjust_damage_on_hit_with_extra_effects(REF_ dealDamage);

								{
									if (ep->get_in_room() == presence->get_in_room())
									{
										damageInfo.hitRelativeLocation = ep->get_placement().location_to_local(startLocation);
									}
									else
									{
										Framework::RelativeToPresencePlacement rpp;
										rpp.be_temporary_snapshot();
										if (rpp.find_path(cast_to_nonconst(ep->get_owner()), presence->get_in_room(), Transform(startLocation, Quat::identity)))
										{
											damageInfo.hitRelativeLocation = ep->get_placement().location_to_local(rpp.location_from_target_to_owner(startLocation));
										}
									}
									hitHealth->deal_damage(dealDamage, dealDamage, damageInfo);
								}
							}
						}
					}

					if (auto* s = imo->get_sound())
					{
						s->play_sound_in_room(NAME(beamHit), imo->get_presence()->get_in_room(), Transform(beam.endLocWS, Quat::identity));
					}

					beam.update_transforms();
				}

				updateBeamIdx = (updateBeamIdx + 1) % beams.get_size();
			}
		}
	}

	if (shoot)
	{
		timeToNextShoot -= _deltaTime;
		if (timeToNextShoot < 0.0f)
		{
			timeToNextShoot = shootInterval;

			if (auto* tos = imo->get_temporary_objects())
			{
				auto* projectile = tos->spawn(NAME(shoot));
				if (projectile)
				{

					// just in any case if we would be shooting from inside of a capsule
					if (auto* collision = projectile->get_collision())
					{
						collision->dont_collide_with_up_to_top_instigator(imo, 0.5f);
					}
					float useProjectileSpeed = 20.0f;
					useProjectileSpeed = projectile->get_variables().get_value<float>(NAME(projectileSpeed), useProjectileSpeed);
					useProjectileSpeed = imo->get_variables().get_value<float>(NAME(projectileSpeed), useProjectileSpeed);

					Vector3 velocity = Vector3(0.0f, useProjectileSpeed, 0.0f);
					projectile->on_activate_set_relative_velocity(velocity);
				}
				tos->spawn_all(NAME(muzzleFlash));
				if (auto* s = imo->get_sound())
				{
					s->play_sound(NAME(shoot));
				}
			}
		}
	}

	if (auto* s = imo->get_sound())
	{
		if (rotateSpeed != 0.0f || targetSpeed != 0.0f)
		{
			if (!spinSound)
			{
				spinSound = true;
				s->play_sound(NAME(spin));
			}
		}
		else
		{
			if (spinSound)
			{
				spinSound = false;
				s->stop_sound(NAME(spin));
			}
		}
		if (activeBeaming)
		{
			if (!activeSound)
			{
				activeSound = true;
				s->play_sound(NAME(active));
			}
		}
		else
		{
			if (activeSound)
			{
				activeSound = false;
				s->stop_sound(NAME(active));
			}
		}
	}

}

LATENT_FUNCTION(HeartRoomBeams::execute_logic)
{
	PERFORMANCE_GUARD_LIMIT(0.005f, TXT("[heart room beams] execute logic"));

	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("execute logic"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(int, beamIdx);

	LATENT_VAR(::Framework::AI::MessageHandler, messageHandler);

	auto * imo = mind->get_owner_as_modules_owner();
	auto * self = fast_cast<HeartRoomBeams>(logic);

	LATENT_BEGIN_CODE();

	ai_log(self, TXT("heart room beams, hello!"));

	{
		messageHandler.use_with(mind);
		{
			messageHandler.set(NAME(aim_start), [self](Framework::AI::Message const& _message)
				{
					self->requestedActive = true;
					self->timeLeftActive = 0.0f; // restart
					self->timeNextActive = 20.0f;
					self->forceRotateSpeed.clear();
					self->forceAcceleration.clear();
					self->accelerateInQuickly = false;
					self->leavePlayerAlone = false;
					self->noVerticalCheck = false;
					if (auto* p = _message.get_param(NAME(time)))
					{
						self->timeNextActive = p->get_as<float>();
					}
					if (auto* p = _message.get_param(NAME(speed)))
					{
						self->forceRotateSpeed = p->get_as<float>();
					}
					if (auto* p = _message.get_param(NAME(acceleration)))
					{
						self->forceAcceleration = p->get_as<float>();
					}
					if (auto* p = _message.get_param(NAME(accelerateInQuickly)))
					{
						self->accelerateInQuickly = p->get_as<bool>();
					}
					if (auto* p = _message.get_param(NAME(leavePlayerAlone)))
					{
						self->leavePlayerAlone = p->get_as<bool>();
					}
					if (auto* p = _message.get_param(NAME(noVerticalCheck)))
					{
						self->noVerticalCheck = p->get_as<bool>();
					}
					if (auto* p = _message.get_param(NAME(swapDir)))
					{
						if (p->get_as<bool>())
						{
							self->rotateDir = -self->rotateDir;
						}
					}
				}
			);
			messageHandler.set(NAME(aim_stop), [self](Framework::AI::Message const& _message)
				{
					self->timeLeftActive = 0.0f;
					self->timeNextActive = 0.0f;
					self->forceRotateSpeed.clear();
					self->forceAcceleration.clear();
					if (auto* p = _message.get_param(NAME(speed)))
					{
						self->forceRotateSpeed = p->get_as<float>();
					}
					if (auto* p = _message.get_param(NAME(acceleration)))
					{
						self->forceAcceleration = p->get_as<float>();
					}
				}
			);
			messageHandler.set(NAME(aim_startShooting), [self](Framework::AI::Message const& _message)
				{
					self->shoot = true;
					self->timeToNextShoot = 0.0f;
					self->shootInterval = 0.1f;

					if (auto* p = _message.get_param(NAME(interval)))
					{
						self->shootInterval = p->get_as<float>();
					}
				}
			);
			messageHandler.set(NAME(aim_stopShooting), [self](Framework::AI::Message const& _message)
				{
					self->shoot = false;
				}
			);
		}
	}

	{
		self->spinSpeedVar.set_name(NAME(spinSpeed));
		self->spinSpeedVar.look_up<float>(imo->access_variables());
	}

	LATENT_WAIT(0.1f);

	beamIdx = 0;
	while (beamIdx < self->beams.get_size())
	{
		{
			auto& beam = self->beams[beamIdx];
			if (auto* a = imo->get_appearance())
			{
				if (auto* s = a->get_skeleton())
				{
					if (auto* as = s->get_skeleton())
					{
						beam.boneStart.look_up(as);
						beam.boneEnd.look_up(as);
					}
				}
				if (auto* m = a->get_mesh())
				{
					beam.beamB.look_up(m);
					beam.beamT.look_up(m);
				}
			}
		}
		++beamIdx;
		LATENT_YIELD();
	}

	{
		if (auto* a = imo->get_appearance())
		{
			if (auto* m = a->get_mesh())
			{
				self->beamRest.look_up(m);

				Transform beamRestAtOS = a->calculate_socket_os(self->beamRest.get_index());
				Transform beamRestAtWS = imo->get_presence()->get_placement().to_world(beamRestAtOS);
				for_every(beam, self->beams)
				{
					beam->restStartWS = beamRestAtWS.to_world(Transform(Vector3(0.0f, -0.05f, 0.0f), Quat::identity, 0.0f));
					beam->restEndWS = beamRestAtWS.to_world(Transform(Vector3(0.0f, 0.05f, 0.0f), Quat::identity, 0.0f));
				}
			}
		}
	}

	self->readyAndRunning = true;

	while (true)
	{
		LATENT_WAIT_NO_RARE_ADVANCE(self->rg.get_float(0.1f, 0.2f));
	}

	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}

void HeartRoomBeams::fill_beam_infos(REF_ Array<BeamInfo>& _beams) const
{
	_beams.clear();
	if (!readyAndRunning)
	{
		return;
	}

	_beams.set_size(beams.get_size());

	auto bdest = _beams.begin();
	for_every(bsrc, beams)
	{
		bdest->boneStart = bsrc->boneStart;
		bdest->boneEnd = bsrc->boneEnd;
		bdest->startWS = bsrc->startWS;
		bdest->endWS = bsrc->endWS;
		float scale = bsrc->scale;
		scale = 1.0f - scale;
		scale = scale * scale;
		scale = max(0.01f, 1.0f - scale);
		bdest->startWS.set_scale(scale);
		bdest->endWS.set_scale(scale);

		++bdest;
	}
}

//--

void HeartRoomBeams::Beam::update_transforms()
{
	startWS = look_at_matrix(startLocWS, endLocWS, Vector3::zAxis).to_transform();
	endWS = startWS;
	endWS.set_translation(endLocWS);
}

//--

REGISTER_FOR_FAST_CAST(HeartRoomBeamsData);

bool HeartRoomBeamsData::load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	beamInterval = _node->get_float_attribute_or_from_child(TXT("beamInterval"), beamInterval);
	beamActive = _node->get_float_attribute_or_from_child(TXT("beamActive"), beamActive);
	rotateSpeed = _node->get_float_attribute_or_from_child(TXT("rotateSpeed"), rotateSpeed);
	rotateSpeedDown = _node->get_float_attribute_or_from_child(TXT("rotateSpeedDown"), rotateSpeedDown);
	rotateAccelerateUp = _node->get_float_attribute_or_from_child(TXT("rotateAccelerateUp"), rotateAccelerateUp);
	rotateAccelerateDown = _node->get_float_attribute_or_from_child(TXT("rotateAccelerateDown"), rotateAccelerateDown);

	{
		String flags = _node->get_string_attribute_or_from_child(TXT("beamCollisionFlags"), String::empty());
		if (!flags.is_empty())
		{
			beamCollisionFlags = Collision::Flags::none();
			beamCollisionFlags.apply(flags);
		}
	}

	damage.load_from_attribute_or_from_child(_node, TXT("damage"));

	return result;
}

bool HeartRoomBeamsData::prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

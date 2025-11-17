#include "aiLogicComponent_airfighterArms.h"

#include "..\utils\aiLogicUtil_shootingAccuracy.h"

#include "..\..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\..\framework\module\moduleCollision.h"
#include "..\..\..\..\framework\module\moduleSound.h"
#include "..\..\..\..\framework\module\moduleTemporaryObjects.h"
#include "..\..\..\..\framework\object\temporaryObject.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

// ai messages
DEFINE_STATIC_NAME_STR(aim_aim, TXT("airfighter arm; aim"));
DEFINE_STATIC_NAME_STR(aim_arm, TXT("airfighter arm; arm"));
DEFINE_STATIC_NAME_STR(aim_idle, TXT("airfighter arm; idle"));
DEFINE_STATIC_NAME_STR(aim_allowShooting, TXT("airfighter arm; allow shooting"));
DEFINE_STATIC_NAME_STR(aim_disallowShooting, TXT("airfighter arm; disallow shooting"));
DEFINE_STATIC_NAME_STR(aim_setup, TXT("airfighter arm; setup"));

// ai messages params
DEFINE_STATIC_NAME(armId);
DEFINE_STATIC_NAME(atIMO);
DEFINE_STATIC_NAME(addOwnersVelocityForProjectiles);
DEFINE_STATIC_NAME(reset);

// temporary objects
DEFINE_STATIC_NAME(shoot);
DEFINE_STATIC_NAME(muzzleFlash);

// variables/params
DEFINE_STATIC_NAME(projectileSpeed);

//

AirfighterArms::AirfighterArms()
{
}

AirfighterArms::~AirfighterArms()
{
}

void AirfighterArms::setup(Framework::AI::MindInstance* mind, AirfighterArmsData const& _airfighterData)
{
	if (auto* imo = mind->get_owner_as_modules_owner())
	{
		rg = imo->get_individual_random_generator();
	}

	for_every(sarm, _airfighterData.arms)
	{
		Arm arm;
		arm.id = sarm->id;
		arm.topBone = sarm->topBone;
		arm.gunBone = sarm->gunBone;
		arm.muzzleSocket = sarm->muzzleSocket;
		arm.provideGunPlacementMSVar = sarm->provideGunPlacementMSVar;
		arm.provideGunStraightVar = sarm->provideGunStraightVar;

		arms.push_back(arm);
	}

	messageHandler.use_with(mind);
	{
		messageHandler.set(NAME(aim_aim), [this](Framework::AI::Message const& _message)
			{
				Name armId;
				if (auto* ptr = _message.get_param(NAME(armId)))
				{
					armId = ptr->get_as<Name>();
				}

				Framework::IModulesOwner* aimAtIMO = nullptr;
				if (auto* ptr = _message.get_param(NAME(atIMO)))
				{
					aimAtIMO = ptr->get_as< SafePtr<Framework::IModulesOwner> >().get();
				}

				for_every(arm, arms)
				{
					if (!armId.is_valid() || arm->id == armId)
					{
						arm->aimAtIMO = aimAtIMO;
						arm->targetPlacementSocket.clear();
						arm->targetOffsetTS = Vector3::zero;
						arm->state = ArmState::Target;
					}
				}
			});
		messageHandler.set(NAME(aim_arm), [this](Framework::AI::Message const& _message)
			{
				Name armId;
				if (auto* ptr = _message.get_param(NAME(armId)))
				{
					armId = ptr->get_as<Name>();
				}

				for_every(arm, arms)
				{
					if (!armId.is_valid() || arm->id == armId)
					{
						arm->state = ArmState::AttackPosition;
					}
				}
			});
		messageHandler.set(NAME(aim_idle), [this](Framework::AI::Message const& _message)
			{
				Name armId;
				if (auto* ptr = _message.get_param(NAME(armId)))
				{
					armId = ptr->get_as<Name>();
				}

				for_every(arm, arms)
				{
					if (!armId.is_valid() || arm->id == armId)
					{
						arm->aimAtIMO.clear();
						arm->targetPlacementSocket.clear();
						arm->targetOffsetTS = Vector3::zero;
						arm->state = ArmState::Idle;
					}
				}
			});
		messageHandler.set(NAME(aim_allowShooting), [this](Framework::AI::Message const& _message)
			{
				Name armId;
				if (auto* ptr = _message.get_param(NAME(armId)))
				{
					armId = ptr->get_as<Name>();
				}

				for_every(arm, arms)
				{
					if (!armId.is_valid() || arm->id == armId)
					{
						arm->allowShooting = true;
					}
				}
			});
		messageHandler.set(NAME(aim_disallowShooting), [this](Framework::AI::Message const& _message)
			{
				Name armId;
				if (auto* ptr = _message.get_param(NAME(armId)))
				{
					armId = ptr->get_as<Name>();
				}

				for_every(arm, arms)
				{
					if (!armId.is_valid() || arm->id == armId)
					{
						arm->allowShooting = false;
					}
				}
			});
		messageHandler.set(NAME(aim_setup), [this](Framework::AI::Message const& _message)
			{
				if (auto* ptr = _message.get_param(NAME(addOwnersVelocityForProjectiles)))
				{
					addOwnersVelocityForProjectiles = ptr->get_as<bool>();
				}
				if (auto* ptr = _message.get_param(NAME(reset)))
				{
					if (ptr->get_as<bool>())
					{
						addOwnersVelocityForProjectiles = false;
					}
				}
			});
	}

	shootingInterval = (60.0f / (max(1.0f, _airfighterData.shootingRPM))) / max(1.0f, (float)arms.get_size());
}

void AirfighterArms::fold_arms(float _maxDelay)
{
	for_every(arm, arms)
	{
		arm->timeToFold = rg.get_float(0.0f, _maxDelay);
		arm->timeToArm.clear();
	}

	if (!arms.is_empty())
	{
		// one should fold immediately
		arms[rg.get_int(arms.get_size())].timeToFold = 0.0f;
	}
}

void AirfighterArms::be_armed(Optional<float> const & _maxDelay, Optional<bool> const & _targetAfterwards)
{
	for_every(arm, arms)
	{
		arm->timeToArm = rg.get_float(0.0f, _maxDelay.get(0.9f));
		arm->timeToFold.clear();
		arm->timeToArmState = _targetAfterwards.get(false) ? ArmState::Target : ArmState::AttackPosition;
	}

	if (!arms.is_empty())
	{
		// one should open immediately
		arms[rg.get_int(arms.get_size())].timeToArm = 0.0f;
	}
}

void AirfighterArms::target(Framework::IModulesOwner* imo, Name const& _targetPlacementSocket, Vector3 const & _offsetTS)
{
	for_every(arm, arms)
	{
		if (arm->timeToArm.is_set())
		{
			arm->timeToArmState = ArmState::Target;
		}
		else
		{
			arm->state = ArmState::Target;
		}
		arm->timeToFold.clear();
		arm->aimAtIMO = imo;
		if (arm->targetPlacementSocket.get_name() != _targetPlacementSocket)
		{
			arm->targetPlacementSocket = _targetPlacementSocket;
			if (imo && arm->targetPlacementSocket.is_name_valid())
			{
				if (auto* a = imo->get_appearance())
				{
					arm->targetPlacementSocket.look_up(a->get_mesh());
				}
			}
		}
		arm->targetOffsetTS = _offsetTS;
	}
}

void AirfighterArms::shooting(bool _shooting)
{
	for_every(arm, arms)
	{
		arm->shootingRequested = _shooting;
	}

	shootingIntervalTimeLeft = 0.0f;
}

void AirfighterArms::advance(AdvanceParams const& _params)
{
	Transform placementWS = _params.placementWS;
	Transform inertiaPlacementWS = placementWS;
	{
		if (prevPlacementWS.is_set())
		{
			Transform shouldBeWS = prevPlacementWS.get();
			shouldBeWS.set_translation(shouldBeWS.get_translation() + (prevVelocityLinear - _params.roomVelocity) * _params.deltaTime);
			//shouldBeWS.set_orientation(shouldBeWS.get_orientation().to_world((prevVelocityRotation * _params.deltaTime).to_quat()));
			inertiaPlacementWS = shouldBeWS;
		}
		prevVelocityLinear = _params.velocityLinear;
		prevVelocityRotation = _params.velocityRotation;
		prevPlacementWS = placementWS;
	}

	bool shootNow = true;

	for_every(arm, arms)
	{
		// setup (if required)
		{
			if (!arm->provideGunPlacementMSVar.is_valid() ||
				!arm->provideGunStraightVar.is_valid())
			{
				if (auto* imo = _params.owner)
				{
					if (auto* ap = imo->get_appearance())
					{
						auto& varStorage = ap->access_controllers_variable_storage();
						arm->provideGunPlacementMSVar.look_up<Transform>(varStorage);
						arm->provideGunStraightVar.look_up<float>(varStorage);
					}
				}
			}
			if (arm->gunToTopMaxLength == 0.0f)
			{
				if (auto* imo = _params.owner)
				{
					if (auto* ap = imo->get_appearance())
					{
						if (auto* skel = ap->get_skeleton())
						{
							if (auto* coreSkel = skel->get_skeleton())
							{
								arm->topBone.look_up(coreSkel);
								arm->gunBone.look_up(coreSkel);
								arm->muzzleSocket.look_up(ap->get_mesh());
								if (arm->gunBone.is_valid() &&
									arm->topBone.is_valid())
								{
									Transform defaultGunMS = coreSkel->get_bones_default_placement_MS()[arm->gunBone.get_index()];
									Transform defaultTopMS = coreSkel->get_bones_default_placement_MS()[arm->topBone.get_index()];

									arm->topMS = defaultTopMS;

									arm->gunToTopMaxLength = (defaultTopMS.get_translation() - defaultGunMS.get_translation()).length();

									{
										Vector3 locMS = defaultTopMS.get_translation();
										locMS.x *= 0.5f;
										locMS.y -= arm->gunToTopMaxLength * 0.7f;
										locMS.z -= arm->gunToTopMaxLength * 0.1f;
										arm->idleMS = look_matrix(locMS, Vector3(-0.2f * sign(locMS.x), -1.1f, -0.3f).normal(), Vector3::zAxis).to_transform();
									}

									{
										Vector3 locMS = defaultTopMS.get_translation();
										locMS.x = sign(locMS.x) * (abs(locMS.x) * 0.9f + 0.1f);
										locMS.y -= 0.3f;
										locMS.z -= arm->gunToTopMaxLength * 0.55f;
										arm->attackMS = look_matrix(locMS, Vector3::yAxis, -Vector3::zAxis).to_transform();
										arm->axisMS = look_matrix(locMS * Vector3(0.0f, 1.0f, 1.0f), Vector3::yAxis, Vector3::zAxis).to_transform();
									}
								}
							}
						}
					}
				}
			}
		}

		if (arm->timeToFold.is_set())
		{
			if (arm->state != ArmState::Idle)
			{
				arm->timeToFold = arm->timeToFold.get() - _params.deltaTime;
				if (arm->timeToFold.get() <= 0.0f)
				{
					arm->state = ArmState::Idle;
					arm->timeToFold.clear();
				}
			}
			else
			{
				arm->timeToFold.clear();
			}
		}

		if (arm->timeToArm.is_set())
		{
			if (arm->state == ArmState::Idle)
			{
				arm->timeToArm = arm->timeToArm.get() - _params.deltaTime;
				if (arm->timeToArm.get() <= 0.0f)
				{
					arm->state = arm->timeToArmState;
					arm->timeToArm.clear();
				}
			}
			else
			{
				arm->timeToArm.clear();
			}
		}

		Vector3 requestedTargetDir = arm->targetDir;
		if (arm->state == ArmState::AttackPosition)
		{
			requestedTargetDir = Vector3(0.0f, 1.0f, -0.5f).normal();
		}
		if (arm->state == ArmState::Target)
		{
			requestedTargetDir = Vector3::yAxis; // default
			if (auto* aimAtIMO = arm->aimAtIMO.get())
			{
				if (auto* imo = _params.owner)
				{
					if (auto* ap = imo->get_appearance())
					{
						auto& poseMatMS = ap->get_final_pose_mat_MS();
						if (arm->gunBone.is_valid() &&
							poseMatMS.is_index_valid(arm->gunBone.get_index()))
						{
							Vector3 locMS = poseMatMS[arm->gunBone.get_index()].get_translation();
							Vector3 locWS = ap->get_ms_to_ws_transform().location_to_world(locMS);
							if (auto* p = aimAtIMO->get_presence())
							{
								if (p->get_in_room() == imo->get_presence()->get_in_room())
								{
									Transform targetPlacementWS = p->get_centre_of_presence_transform_WS();
									Vector3 toTargetWS = targetPlacementWS.get_translation() - _params.owner->get_presence()->get_placement().get_translation();
									Vector3 toTargetDirWS = toTargetWS.normal();
									projectileSpeed.set_if_not_set(70.0f);
									if (projectileSpeed.is_set())
									{
										// predict the target's placement

										Vector3 projVelocity = toTargetDirWS * projectileSpeed.get();
										if (addOwnersVelocityForProjectiles)
										{
											if (auto* imo = _params.owner)
											{
												projVelocity += imo->get_presence()->get_velocity_linear();
											}
										}

										float dist = toTargetWS.length();
										float distVel = Vector3::dot(p->get_velocity_linear(), toTargetDirWS);
										
										// distHit = dist + distVel * t
										// distHit = 0 + projSpeed * t
										// dist + distVel * t = projSpeed * t
										// (projSpeed - distVel) * t = dist
										// t = dist / (projSpeed - distVel)

										float t = dist / max(0.01f, (projectileSpeed.get() - distVel));
										t = clamp(t, 0.0f, 20.0f);

										targetPlacementWS.set_translation(targetPlacementWS.get_translation() + p->get_velocity_linear() * t);

									}
									if (arm->targetPlacementSocket.is_valid())
									{
										if (auto* a = aimAtIMO->get_appearance())
										{
											targetPlacementWS = p->get_placement().to_world(a->calculate_socket_os(arm->targetPlacementSocket.get_index()));
										}
									}
									Vector3 targetLocWS = targetPlacementWS.location_to_world(arm->targetOffsetTS);
									requestedTargetDir = ap->get_ms_to_ws_transform().vector_to_local((targetLocWS - locWS)).normal();
								}
							}
						}
					}
				}
			}
		}

		{
			if (arm->atAttackWeight == 0.0f)
			{
				arm->targetDir = requestedTargetDir;
			}
			else
			{
				arm->targetDir = blend_to_using_time(arm->targetDir, requestedTargetDir, 0.2f, _params.deltaTime).normal();
			}
		}

		{
			float targetAttackWeight = arm->state == ArmState::Idle ? 0.0f : 1.0f;
			arm->atAttackWeight = blend_to_using_speed_based_on_time(arm->atAttackWeight, targetAttackWeight, 1.0f, _params.deltaTime);
		}

		float useAtAttackWeight = BlendCurve::cubic(arm->atAttackWeight);

		if (arm->provideGunPlacementMSVar.is_valid())
		{
			Transform& providePlacementMS = arm->provideGunPlacementMSVar.access<Transform>();

			Vector3 offsetFromAxis = Vector3::zero;
			if (arm->axisMS.is_set())
			{
				if (auto* imo = _params.owner)
				{
					if (auto* ap = imo->get_appearance())
					{
						Transform axisMS = arm->axisMS.get();
						Transform axisRotMS = look_matrix(axisMS.get_translation(), (axisMS.get_axis(Axis::Forward) * 2.0f + arm->targetDir).normal(), axisMS.get_axis(Axis::Up)).to_transform();
						offsetFromAxis = axisRotMS.location_to_world(axisMS.location_to_local(arm->attackMS.get_translation())) - arm->attackMS.get_translation();
					}
				}
			}
			Transform atIdleMS = arm->idleMS;
			Transform atAttackMS = look_matrix(arm->attackMS.get_translation() + offsetFromAxis + arm->targetDir * Vector3(-0.1f, 0.1f, 0.05f), arm->targetDir.normal(), (arm->targetDir - Vector3::yAxis - Vector3::zAxis).normal()).to_transform();

			Vector3 atLocMS = atIdleMS.get_translation() * (1.0f - useAtAttackWeight) + atAttackMS.get_translation() * (useAtAttackWeight);
			{
				float atLocDistance = (atIdleMS.get_translation() - arm->topMS.get_translation()).length() * (1.0f - useAtAttackWeight) + (atAttackMS.get_translation() - arm->topMS.get_translation()).length() * (useAtAttackWeight);
				atLocMS = arm->topMS.get_translation() + (atLocMS - arm->topMS.get_translation()).normal() * atLocDistance;
			}

			Vector3 targetInertiaOffsetWS = inertiaPlacementWS.location_to_world(providePlacementMS.get_translation()) - placementWS.location_to_world(providePlacementMS.get_translation());
			if (_params.deltaTime != 0.0f)
			{
				targetInertiaOffsetWS /= _params.deltaTime;
				targetInertiaOffsetWS *= 0.2f;
				arm->inertiaOffsetWS = blend_to_using_time(arm->inertiaOffsetWS, targetInertiaOffsetWS, 0.1f, _params.deltaTime);
			}
			Vector3 intertiaOffsetMS = placementWS.vector_to_local(arm->inertiaOffsetWS);

			Transform placement(atLocMS + intertiaOffsetMS, Quat::slerp(useAtAttackWeight, atIdleMS.get_orientation(), atAttackMS.get_orientation()));

			debug_filter(ai_draw);
			debug_context(_params.owner->get_presence()->get_in_room());
			debug_draw_arrow(true, Colour::green, placementWS.location_to_world(placement.get_translation()), placementWS.location_to_world(placement.location_to_world(Vector3(0.0f, 10.0f, 0.0f))));
			debug_no_context();
			debug_no_filter();

			providePlacementMS = placement;
		}

		if (arm->provideGunStraightVar.is_valid())
		{
			arm->provideGunStraightVar.access<float>() = clamp(0.8f - 0.5f * useAtAttackWeight, 0.0f, 1.0f);
		}

		if (arm->state == ArmState::Target)
		{
			if (arm->allowShooting)
			{
				todo_implement;
			}
			if (arm->shootingRequested)
			{
				shootNow = true;
			}
		}
	}

	if (shootNow && ! arms.is_empty())
	{
		shootingIntervalTimeLeft -= _params.deltaTime;
		if (shootingIntervalTimeLeft < 0.0f)
		{
			shootingIntervalTimeLeft = shootingInterval;

			if (arms.is_index_valid(shootingArmIdx))
			{
				auto& arm = arms[shootingArmIdx];
				if (arm.allowShooting || arm.shootingRequested)
				{
					bool shot = false;
					if (auto* tos = _params.owner->get_temporary_objects())
					{
						auto* projectile = tos->spawn(NAME(shoot), Framework::ModuleTemporaryObjects::SpawnParams().at_socket(arm.muzzleSocket.get_name()));
						if (projectile)
						{
							// just in any case if we would be shooting from inside of a capsule
							if (auto* collision = projectile->get_collision())
							{
								collision->dont_collide_with_up_to_top_instigator(_params.owner, 0.05f);
							}
							{
								float useProjectileSpeed = projectile->get_variables().get_value<float>(NAME(projectileSpeed), 10.0f);
								useProjectileSpeed = _params.owner->get_variables().get_value<float>(NAME(projectileSpeed), useProjectileSpeed);
								projectileSpeed = useProjectileSpeed;
								Vector3 velocity = Vector3(0.0f, useProjectileSpeed, 0.0f);
								velocity = Utils::apply_shooting_accuracy(velocity, _params.owner);
								if (addOwnersVelocityForProjectiles)
								{
									if (auto* imo = _params.owner)
									{
										velocity += imo->get_presence()->get_velocity_linear();
									}
								}
								projectile->on_activate_set_relative_velocity(velocity);
							}
							shot = true;
						}
						if (shot)
						{
							// depends on muzzle being symmetrical/round
							Transform relativePlacement(Vector3::zero, Rotator3(0.0f, 0.0f, 90.0f * rg.get_int_from_range(-2, 2)).to_quat());
							tos->spawn(NAME(muzzleFlash), Framework::ModuleTemporaryObjects::SpawnParams().at_socket(arm.muzzleSocket.get_name()).at_relative_placement(relativePlacement));

							if (auto* s = _params.owner->get_sound())
							{
								s->play_sound(NAME(shoot), arm.muzzleSocket.get_name());
							}
						}
					}
				}
			}
			shootingArmIdx = mod(shootingArmIdx + 1, arms.get_size());
		}
	}
}

//

bool AirfighterArmsData::load_from_xml(IO::XML::Node const* _node, ::Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	shootingRPM = _node->get_float_attribute_or_from_child(TXT("rpm"), shootingRPM);

	for_every(node, _node->children_named(TXT("arm")))
	{
		Arm arm;
		arm.id= node->get_name_attribute(TXT("id"), arm.id);
		arm.gunBone = node->get_name_attribute(TXT("gunBone"), arm.gunBone);
		arm.topBone = node->get_name_attribute(TXT("topBone"), arm.topBone);
		arm.muzzleSocket = node->get_name_attribute(TXT("muzzleSocket"), arm.muzzleSocket);
		arm.provideGunPlacementMSVar = node->get_name_attribute(TXT("provideGunPlacementMSVarID"), arm.provideGunPlacementMSVar);
		arm.provideGunStraightVar = node->get_name_attribute(TXT("provideGunStraightVarID"), arm.provideGunStraightVar);
		error_loading_xml_on_assert(arm.gunBone.is_valid(), result, node, TXT("gunBone not provided"));
		error_loading_xml_on_assert(arm.topBone.is_valid(), result, node, TXT("topBone not provided"));
		error_loading_xml_on_assert(arm.muzzleSocket.is_valid(), result, node, TXT("muzzleSocket not provided"));
		error_loading_xml_on_assert(arm.provideGunPlacementMSVar.is_valid(), result, node, TXT("provideGunPlacementMSVarID not provided"));
		error_loading_xml_on_assert(arm.provideGunStraightVar.is_valid(), result, node, TXT("provideGunStraightVarID not provided"));
		arms.push_back(arm);
	}

	return result;
}

bool AirfighterArmsData::prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	return result;
}


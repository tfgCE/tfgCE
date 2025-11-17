#include "gameSceneLayer_World.h"

#include "..\game.h"
#include "..\gameConfig.h"
#include "..\gameDirector.h"

#include "..\environmentPlayer.h"

#include "..\..\modules\gameplay\moduleControllableTurret.h"
#include "..\..\modules\gameplay\modulePilgrim.h"

#include "..\..\music\musicPlayer.h"

#include "..\..\overlayInfo\overlayInfoSystem.h"
#include "..\..\pilgrim\pilgrimOverlayInfo.h"

#include "..\..\sound\voiceoverSystem.h"

#include "..\..\..\core\types\hand.h"
#include "..\..\..\core\types\names.h"

#include "..\..\..\framework\ai\movementParameters.h"
#include "..\..\..\framework\game\player.h"
#include "..\..\..\framework\jobSystem\jobSystem.h"
#include "..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\framework\module\moduleController.h"
#include "..\..\..\framework\module\moduleMovement.h"
#include "..\..\..\framework\object\actor.h"
#include "..\..\..\framework\sound\soundCamera.h"
#include "..\..\..\framework\sound\soundScene.h"

#ifdef AN_DEVELOPMENT_OR_PROFILER
#include "..\..\ai\logics\aiLogic_pilgrim.h"
#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\module\moduleAI.h"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#ifdef AN_DEVELOPMENT_OR_PROFILER
#define AUTO_PILOT
#endif

#define LIMIT_FLAT_MOVEMENT_DISTANCE 0.10f
#define STAND_STILL_HEAD_TO_BODY_YAW 1.0f

//

using namespace TeaForGodEmperor;
using namespace GameSceneLayers;

// gait
DEFINE_STATIC_NAME(normal);

// controls
DEFINE_STATIC_NAME(movement);
DEFINE_STATIC_NAME(rotation);
DEFINE_STATIC_NAME(pretendVRMovement);
DEFINE_STATIC_NAME(pretendVRRotation);
DEFINE_STATIC_NAME(immobileVRMovement);
DEFINE_STATIC_NAME(immobileVRRotation);
DEFINE_STATIC_NAME(immobileVRRun);
DEFINE_STATIC_NAME(requestInGameMenuHold);
DEFINE_STATIC_NAME(requestInGameMenu);

//

REGISTER_FOR_FAST_CAST(World);

World::World()
: player(nullptr)
, playerTakenControl(nullptr)
{
}

World::World(Player* _player, Player* _playerTakenControl)
: player(_player)
, playerTakenControl(_playerTakenControl)
{
}

World::World(Framework::Render::Camera const & _camera)
: player(nullptr)
, playerTakenControl(nullptr)
, camera(_camera)
{
}

void World::use(Player* _player, Player* _playerTakenControl)
{
	player = _player;
	playerTakenControl = _playerTakenControl;
	camera.clear();
	playerControlModeTime[0] = 0.0f;
	playerControlModeTime[1] = 0.0f;
}

void World::use(Framework::Render::Camera const & _camera)
{
	player = nullptr;
	playerTakenControl = nullptr;
	camera = _camera;
	playerControlModeTime[0] = 0.0f;
	playerControlModeTime[1] = 0.0f;
}

void World::on_start()
{
	base::on_start();

	an_assert(count_layers<World>() == 1, TXT("there should be just one world on the stack! if there are more, sound pausing/unpausing may break!"));
}

void World::pre_advance(Framework::GameAdvanceContext const & _advanceContext)
{
	base::pre_advance(_advanceContext);
}

void World::advance(Framework::GameAdvanceContext const & _advanceContext)
{
	base::advance(_advanceContext);
	game->advance_world(_advanceContext.get_world_delta_time());
	advancedAtLeastOnce = true;
}

Framework::Actor* World::get_player_in_control_actor_for_perception() const
{
	if (playerTakenControl && playerTakenControl->should_see_through_my_eyes())
	{
		return playerTakenControl->get_actor();
	}
	if (player && player->get_actor())
	{
		return player->get_actor();
	}
	return nullptr;
}

void World::process_controls(Framework::GameAdvanceContext const & _advanceContext)
{
#ifdef AN_DEVELOPMENT_OR_PROFILER
	Game* useGame = fast_cast<Game>(game);
#else
#ifdef RENDER_SCENE_EXTERNAL_VIEW_SUPPORT
	Game* useGame = fast_cast<Game>(game);
#endif
#endif
	base::process_controls(_advanceContext);
	if (player)
	{
		{
			bool shouldUseTakenControl = playerTakenControl && playerTakenControl->get_actor() && !playerTakenControl->may_control_both();
			if (usingTakenControl != shouldUseTakenControl)
			{
				usingTakenControl = shouldUseTakenControl;
				playerControlModeTime[0] = 0.0f;
				playerControlModeTime[1] = 0.0f;
			}
			{
				float const deltaTime = _advanceContext.get_other_delta_time();
				playerControlModeTime[0] += deltaTime;
				playerControlModeTime[1] += deltaTime;
			}
		}
		bool doGeneralControls = true;
		bool mainControl = true;
		for (int i = 0; i < 2; ++i)
		{
			if (i == 0 && playerTakenControl && playerTakenControl->get_actor() && !playerTakenControl->may_control_both())
			{
				// skip main player then
				continue;
			}
			if (Player* playerPtr = i == 0 ? this->player : playerTakenControl)
			{
				Player& player = *playerPtr;

				{
					DEFINE_STATIC_NAME(game);
					player.use_input_definition(NAME(game)); // we want to use this input definition!
				}

				ControlMode::Type newPlayerControlMode = player.get_control_mode();
				if (playerControlMode[i] != newPlayerControlMode)
				{
					playerControlMode[i] = newPlayerControlMode;
					playerControlModeTime[i] = 0.0f;
				}
				float const switchControlsBufferTime = 0.2f;

				if (
#ifdef AN_DEVELOPMENT_OR_PROFILER
					useGame->is_player_controls_allowed() &&
#endif
					player.get_control_mode() == ControlMode::Actor &&
					playerControlModeTime[i] > switchControlsBufferTime) // disallow all controls before "switch controls" buffer time
				{
					DEFINE_STATIC_NAME(autoAimNonVR);
					DEFINE_STATIC_NAME(autoAimNonVRActive);

					Name requestedGaitName = NAME(normal);

					bool movementRelativeToHeadYaw = true;

					// orientation
					if (Framework::Actor* actor = player.get_actor()) // only if bound to actor
					{
						if (actor->get_presence()->is_vr_movement())
						{
							// should be already handled in process_vr_controls
						}
						else
						{
							float yawControl = 0.0f;
							float pitchControl = 0.0f;
							float relativeYawControl = 0.0f;
							{
								Vector2 mouseRotation = player.get_input_non_vr().get_mouse(NAME(rotation));
								// this is to make default work fine
								yawControl += mouseRotation.x * 0.6f;
								pitchControl += mouseRotation.y * 0.6f;
							}
							{
								Vector2 stickRotation = player.get_input_non_vr().get_stick(NAME(rotation)) + player.get_input_vr_left().get_stick(NAME(rotation)) + player.get_input_vr_right().get_stick(NAME(rotation));
								if (Game::is_using_sliding_locomotion())
								{
									stickRotation += stickRotation += player.get_input_non_vr().get_stick(NAME(immobileVRRotation)) + player.get_input_vr_left().get_stick(NAME(immobileVRRotation)) + player.get_input_vr_right().get_stick(NAME(immobileVRRotation));
								}
								float gamepadCameraSpeed = 180.0f;
								if (GameConfig const * gc = GameConfig::get_as<GameConfig>())
								{
									gamepadCameraSpeed = 90.0f + gc->get_gamepad_camera_speed() * 180.0f;
								}
								yawControl += stickRotation.x * gamepadCameraSpeed * _advanceContext.get_world_delta_time();
								relativeYawControl += stickRotation.x;
								pitchControl += stickRotation.y * gamepadCameraSpeed * _advanceContext.get_world_delta_time();
							}

							// clamp, it should be relative, right?
							relativeYawControl = clamp(relativeYawControl, -1.0f, 1.0f);
							//
							{
								if (Game::get()->does_use_vr() && VR::IVR::can_be_used())
								{
									actor->get_controller()->set_requested_relative_velocity_orientation(Rotator3(0.0f, relativeYawControl, 0.0f));
								}
								if (!VR::IVR::can_be_used()) // checking actual VR to allow forced VR
								{
									Quat viewLocal = actor->get_controller()->get_relative_requested_look_orientation();
									{
										Rotator3 viewLocalRotator = viewLocal.to_rotator();
										viewLocalRotator.yaw += yawControl;
										viewLocalRotator.pitch = clamp(viewLocalRotator.pitch + pitchControl, -89.0f, 89.0f);
										viewLocalRotator.roll = 0.0f;
										viewLocal = viewLocalRotator.to_quat();
									}

									actor->get_controller()->set_relative_requested_look_orientation(viewLocal);
									actor->get_controller()->clear_snap_yaw_to_look_orientation();
									actor->get_controller()->set_follow_yaw_to_look_orientation(true);
									if (!actor->get_movement() || actor->get_movement()->is_immovable())
									{
										actor->get_controller()->set_maintain_relative_requested_look_orientation(true); // won't spin and we want to maintain the dir
									}
									else
									{
										actor->get_controller()->clear_maintain_relative_requested_look_orientation(); // otherwise just spins
									}
								}
							}
				
#ifdef AUTO_PILOT
							if (useGame->is_autopilot_on())
							{
								if (auto* pa = player.get_actor())
								{
									if (auto* ai = pa->get_ai())
									{
										if (auto* mind = ai->get_mind())
										{
											if (auto* logic = fast_cast<AI::Logics::Pilgrim>(mind->get_logic()))
											{
												static float timeNoInput = 0.0f;
												if (yawControl != 0.0f || pitchControl != 0.0f)
												{
													timeNoInput = 0.0f;
												}
												timeNoInput += _advanceContext.get_world_delta_time();
												if (! logic->get_direction_os().is_zero())
												{
													if (timeNoInput > 0.5f)
													{
														float requestedYaw = Rotator3::normalise_axis(logic->get_direction_os().to_rotator().yaw);
														requestedYaw = requestedYaw * clamp(_advanceContext.get_world_delta_time() / 0.35f, 0.0f, 1.0f);
														float requestedPitch = -30.0f;
														{
															float useNewPitch = clamp(_advanceContext.get_world_delta_time() / 0.5f, 0.0f, 1.0f);
															requestedPitch = requestedPitch * useNewPitch + (1.0f - useNewPitch) * actor->get_controller()->get_requested_look_orientation().to_rotator().pitch;
														}
														actor->get_controller()->set_relative_requested_look_orientation(Rotator3(requestedPitch, requestedYaw, 0.0f).to_quat());
														actor->get_controller()->set_snap_yaw_to_look_orientation(true);
													}
												}
											}
										}
									}
								}
							}
#endif
						}
					}
					// movement
					if (Framework::Actor* actor = player.get_actor()) // only if bound to actor
					{
						if (actor->get_presence()->is_vr_movement())
						{
							// should be already handled in process_vr_controls
						}
						else
						{
							Vector3 movementDirectionLocal = Vector3::zero;
							{
								Vector2 movementStick = player.get_input_non_vr().get_stick(NAME(movement)) + player.get_input_vr_left().get_stick(NAME(movement)) + player.get_input_vr_right().get_stick(NAME(movement));
								if (Game::is_using_sliding_locomotion())
								{
									movementStick += player.get_input_non_vr().get_stick(NAME(immobileVRMovement)) + player.get_input_vr_left().get_stick(NAME(immobileVRMovement)) + player.get_input_vr_right().get_stick(NAME(immobileVRMovement));
								}
								// clamp to sane values
								movementStick.x = clamp(movementStick.x, -1.0f, 1.0f);
								movementStick.y = clamp(movementStick.y, -1.0f, 1.0f);
								movementDirectionLocal.x += movementStick.x;
								movementDirectionLocal.y += movementStick.y;
								movementDirectionLocal = movementDirectionLocal.normal() * min(movementDirectionLocal.length(), 1.0f);
							}
#ifdef AUTO_PILOT
							if (useGame->is_autopilot_on())
							{
								if (auto* pa = player.get_actor())
								{
									if (auto* ai = pa->get_ai())
									{
										if (auto* mind = ai->get_mind())
										{
											if (auto* logic = fast_cast<AI::Logics::Pilgrim>(mind->get_logic()))
											{
												movementDirectionLocal += logic->get_direction_os();
											}
										}
									}
								}
							}
#endif
							Rotator3 viewLocal = actor->get_controller()->get_requested_relative_look().get_orientation().to_rotator();
#ifdef RENDER_SCENE_EXTERNAL_VIEW_SUPPORT
							static Vector3 turnInDirAS = Vector3::zero;
							bool useExternalViewControls = useGame->should_use_external_controls() && useGame->get_external_camera_vr_anchor_offset().is_set();
							if (!useExternalViewControls)
							{
								turnInDirAS = Vector3::zero;
							}
							if (useExternalViewControls)
							{
								Vector3 forwardAS = useGame->get_external_camera_vr_anchor_offset().get().get_axis(Axis::Forward);
								forwardAS.z = 0.0f;
								forwardAS.normalise();
								Vector3 rightAS = forwardAS.rotated_by_yaw(90.0f);

								// align to vr anchor
								forwardAS = Vector3::yAxis;
								rightAS = Vector3::xAxis;

								Transform vrAnchor = actor->get_presence()->get_vr_anchor();
								Vector3 forwardRS = vrAnchor.vector_to_world(forwardAS);
								Vector3 rightRS = vrAnchor.vector_to_world(rightAS);
								Transform placement = actor->get_presence()->get_placement();
								Vector3 forwardOS = placement.vector_to_local(forwardRS);
								Vector3 rightOS = placement.vector_to_local(rightRS);
								movementDirectionLocal = rightOS * movementDirectionLocal.x + forwardOS * movementDirectionLocal.y;
								movementDirectionLocal.z = 0.0f;
								movementDirectionLocal = movementDirectionLocal.normal() * min(movementDirectionLocal.length(), 1.0f);
								if (!movementDirectionLocal.is_zero())
								{
									turnInDirAS = vrAnchor.vector_to_local(placement.vector_to_world(movementDirectionLocal));
								}
#ifdef AN_STANDARD_INPUT
								todo_hack();
								if (::System::Input::get()->is_key_pressed(::System::Key::X))
								{
									turnInDirAS = Vector3(1.0f, -1.0f, 0.0f);
								}
#endif
								if (! turnInDirAS.is_zero())
								{
									Vector3 turnInDirLocal = placement.vector_to_local(vrAnchor.vector_to_world(turnInDirAS));
									Quat viewLocal = actor->get_controller()->get_requested_relative_look().get_orientation();
									{
										float requestedYawLocal = Rotator3::get_yaw(turnInDirLocal);
										float blendTime = 0.05f;
										Rotator3 viewLocalRotator = viewLocal.to_rotator();
										viewLocalRotator.yaw = blend_to_using_time(viewLocalRotator.yaw, requestedYawLocal, blendTime, _advanceContext.get_world_delta_time());;
										viewLocalRotator.pitch = blend_to_using_time(viewLocalRotator.pitch, 0.0f, blendTime, _advanceContext.get_world_delta_time());
										viewLocalRotator.roll = 0.0f;
										viewLocal = viewLocalRotator.to_quat();
									}

									actor->get_controller()->set_relative_requested_look_orientation(viewLocal);
									actor->get_controller()->clear_snap_yaw_to_look_orientation();
									actor->get_controller()->set_follow_yaw_to_look_orientation(true);
									if (!actor->get_movement() || actor->get_movement()->is_immovable())
									{
										actor->get_controller()->set_maintain_relative_requested_look_orientation(true); // won't spin and we want to maintain the dir
									}
									else
									{
										actor->get_controller()->clear_maintain_relative_requested_look_orientation(); // otherwise just spins
									}
								}
							}
							else
#endif
							{
								// we want our velocity be in relation to where we look
								if (movementRelativeToHeadYaw)
								{
									movementDirectionLocal.rotate_by_yaw((abs(viewLocal.roll) > 90.0f ? 180.0f : 0.0f) + viewLocal.yaw);
								}
							}
							actor->get_controller()->set_relative_requested_movement_direction(movementDirectionLocal);
							actor->get_controller()->set_requested_movement_parameters((::Framework::MovementParameters()).relative_speed(movementDirectionLocal.length()).gait(requestedGaitName));
						}
					}
					// other controls
					if (Framework::Actor* actor = player.get_actor()) // only if bound to actor
					{
						if (actor->get_presence()->is_vr_movement())
						{
							// should be already handled in process_vr_controls
						}
						else
						{
							if (auto * ic = actor->get_gameplay_as<IControllable>())
							{
								process_controllable_controls(ic, player, false, _advanceContext.get_other_delta_time());
							}
							if (doGeneralControls)
							{
								process_general_controls(player, false, _advanceContext.get_other_delta_time());
								doGeneralControls = false;
							}
						}
					}
				}
				else
				{
					// doesn't control actor, request standing still
					if (auto* actor = player.get_actor())
					{
						actor->get_controller()->clear_requested_movement_direction();
						actor->get_controller()->clear_requested_velocity_orientation();
						actor->get_controller()->set_requested_movement_parameters((::Framework::MovementParameters()).stop());
					}
				}
				mainControl = false;
			}
		}
	}
}

void World::process_vr_controls(Framework::GameAdvanceContext const & _advanceContext)
{
	base::process_vr_controls(_advanceContext);
	if (Game::get()->does_use_vr() && VR::IVR::can_be_used() &&
		VR::IVR::get()->is_controls_view_valid() &&
		VR::IVR::get()->is_controls_base_valid())
	{
		if (player)
		{
			bool doGeneralControls = true;
			bool mainControl = true;
			for(int i = 0; i < 2; ++ i)
			{
				if (i == 0 && playerTakenControl && playerTakenControl->get_actor() && !playerTakenControl->may_control_both())
				{
					// skip main player then
					continue;
				}
				if (Player* playerPtr = i == 0 ? this->player : playerTakenControl)
				{
					Player& player = *playerPtr;
					if (auto * playerActor = player.get_actor())
					{
#ifdef AN_DEVELOPMENT_OR_PROFILER
						if (Game::get_as<Game>()->is_player_controls_allowed() &&
							! Game::is_using_sliding_locomotion())
						{
							Vector2 movementStick = player.get_input_non_vr().get_stick(NAME(pretendVRMovement)) + player.get_input_vr_left().get_stick(NAME(pretendVRMovement)) + player.get_input_vr_right().get_stick(NAME(pretendVRMovement));
							// clamp to sane values
							movementStick.x = clamp(movementStick.x, -1.0f, 1.0f);
							movementStick.y = clamp(movementStick.y, -1.0f, 1.0f);
							Vector3 moveBy = Vector3::zero;
							float movementSpeed = 1.0f * _advanceContext.get_world_delta_time();
							moveBy.x = movementStick.x * movementSpeed;
							moveBy.y = movementStick.y * movementSpeed;
							//
							Vector2 rotationStick = player.get_input_non_vr().get_stick(NAME(pretendVRRotation)) + player.get_input_vr_left().get_stick(NAME(pretendVRRotation)) + player.get_input_vr_right().get_stick(NAME(pretendVRRotation));
							// clamp to sane values
							rotationStick.x = clamp(rotationStick.x, -1.0f, 1.0f);
							rotationStick.y = clamp(rotationStick.y, -1.0f, 1.0f);
							Rotator3 rotateBy = Rotator3::zero;
							float rotationSpeed = 180.0f * _advanceContext.get_world_delta_time();
							rotateBy.yaw = rotationStick.x * rotationSpeed;
							VR::IVR::get()->move_relative_dev_in_vr_space(moveBy, rotateBy);
						}
#endif
						todo_note(TXT("we should store in what mode we were and what should we clear"));

						Optional<Transform> lookLocal;
						// read values for looking around
						{
							Optional<Transform> handsLocal[2];
							bool handsLocalAccurate[2];
							handsLocalAccurate[0] = handsLocalAccurate[1] = false;
							// this is for standing using presence vr value (should at least)
							if (
#ifdef AN_DEVELOPMENT_OR_PROFILER
								Game::get_as<Game>()->is_player_controls_allowed() &&
#endif
								VR::IVR::get()->is_render_view_valid())
							{
								if (playerActor->get_presence()->is_vr_movement())
								{
									Transform lookVR = VR::IVR::get()->get_render_view();
									Transform placementInVR = playerActor->get_presence()->get_placement_in_vr_space();
									if (playerActor->get_gameplay_as<ModuleControllableTurret>())
									{
										if (timeSinceControllingTurret.get_time_since() > 5.0f)
										{
											controllingTurretVRBase.clear();
										}
										timeSinceControllingTurret.reset();
										if (!controllingTurretVRBase.is_set())
										{
											controllingTurretVRBase = look_matrix(lookVR.get_translation(), lookVR.get_axis(Axis::Forward).normal_2d(), Vector3::zAxis).to_transform();
										}
										placementInVR = controllingTurretVRBase.get();
									}
									lookLocal = placementInVR.to_local(lookVR);
									for_count(int, i, 2)
									{
										if (VR::IVR::get()->is_hand_available((Hand::Type)i))
										{
											int handIdx = VR::IVR::get()->get_hand((Hand::Type)i);
											if (handIdx != NONE)
											{
												auto const& hand = VR::IVR::get()->get_render_pose_set().hands[handIdx];
												if (hand.placement.is_set())
												{
													handsLocal[i] = placementInVR.to_local(hand.placement.get());
													handsLocalAccurate[i] = true;
												}
											}
										}
										else
										{
											Vector3 handLoc = lookLocal.get().get_translation();
											handLoc.z = handLoc.z * 0.7f - 0.25f;
											handLoc += Vector3(i == Hand::Left ? -0.2f : 0.2f, 0.05f, 0.0f);
											handsLocal[i] = look_matrix(handLoc, Vector3(0.0f, 1.0f, -1.0f).normal(), Vector3::zAxis).to_transform();
											if (playerActor->get_controller()->is_requested_hand_set(i))
											{
												float blendAmount = clamp(_advanceContext.get_world_delta_time() / 0.2f, 0.0f, 1.0f);
												handsLocal[i] = Transform::lerp(blendAmount, handsLocal[i].get(), playerActor->get_controller()->get_requested_hand(i));
											}
										}
									}
								}
								else
								{
									// this is for seated experience
									lookLocal = VR::IVR::get()->get_render_view_to_base_full();
								}
							}

							todo_note(TXT("if outside camera is used, do not set relative requested look orientation"));
							if (lookLocal.is_set())
							{
								playerActor->get_controller()->set_requested_relative_look(lookLocal.get());
							}
							else
							{
								playerActor->get_controller()->clear_requested_relative_look();
							}
							playerActor->get_controller()->clear_snap_yaw_to_look_orientation();
							for_count(int, i, 2)
							{
								if (handsLocal[i].is_set())
								{
									playerActor->get_controller()->set_requested_hand(i, handsLocal[i].get(), handsLocalAccurate[i]);
								}
								else
								{
									playerActor->get_controller()->clear_requested_hand(i);
								}
							}
						}

						/**
						 *	remember to zero_vr_anchor before advancing vr system, as then we will always try to move relatively to current location
						 *	this really helps with hitting walls and moving back
						 */
						if (Game::is_using_sliding_locomotion())
						{
#ifdef AN_DEVELOPMENT_OR_PROFILER
							if (Game::get_as<Game>()->is_player_controls_allowed())
#endif
							{
								bool doActualMovementViaController = false;
								if (auto* m = playerActor->get_movement())
								{
									doActualMovementViaController = m->does_use_controller_for_movement();
								}
								if (doActualMovementViaController)
								{
									Vector3 movementControl = Vector3::zero; // in world!
									float yawControl = 0.0f;
									{
										// additional offset from stick movement
										Vector2 stickMovement = player.get_input_non_vr().get_stick(NAME(movement)) + player.get_input_vr_left().get_stick(NAME(movement)) + player.get_input_vr_right().get_stick(NAME(movement));
										stickMovement += player.get_input_non_vr().get_stick(NAME(immobileVRMovement)) + player.get_input_vr_left().get_stick(NAME(immobileVRMovement)) + player.get_input_vr_right().get_stick(NAME(immobileVRMovement));
										float movementSpeed = 1.5f;
										auto* psp = PlayerSetup::get_current_if_exists();
										if (psp)
										{
											auto& pref = psp->get_preferences();
											movementSpeed = pref.immobileVRSpeed;
										}
										if (player.get_input_non_vr().is_button_pressed(NAME(immobileVRRun)) || player.get_input_vr_left().is_button_pressed(NAME(immobileVRRun)) || player.get_input_vr_right().is_button_pressed(NAME(immobileVRRun)))
										{
											movementSpeed *= 2.0f;
										}
										Vector3 movementControlLocal = stickMovement.to_vector3() * movementSpeed * _advanceContext.get_world_delta_time();

										Transform moveUsingDirVR = VR::IVR::get()->get_controls_movement_centre();
										if (psp && psp->get_preferences().immobileVRMovementRelativeTo != PlayerVRMovementRelativeTo::Head)
										{
											auto& controlsPoseSet = VR::IVR::get()->get_controls_pose_set();
											auto& handPose = controlsPoseSet.get_hand(psp->get_preferences().immobileVRMovementRelativeTo == PlayerVRMovementRelativeTo::LeftHand? Hand::Left : Hand::Right);
											if (handPose.placement.is_set())
											{
												moveUsingDirVR = handPose.placement.get();
											}
										}
										Transform moveUsingDirWS;
										{
											Vector3 forwardVR = calculate_flat_forward_from(moveUsingDirVR);

											Transform placementVR = look_matrix(moveUsingDirVR.get_translation() * Vector3::xy, forwardVR, Vector3::zAxis).to_transform();

											moveUsingDirWS = playerActor->get_presence()->get_vr_anchor().to_world(placementVR);
										}
										movementControl = moveUsingDirWS.vector_to_world(movementControlLocal);
									}
									{
										// additional offset from stick rotation
										Vector2 stickRotation = player.get_input_non_vr().get_stick(NAME(rotation)) + player.get_input_vr_left().get_stick(NAME(rotation)) + player.get_input_vr_right().get_stick(NAME(rotation));
										stickRotation += player.get_input_non_vr().get_stick(NAME(immobileVRRotation)) + player.get_input_vr_left().get_stick(NAME(immobileVRRotation)) + player.get_input_vr_right().get_stick(NAME(immobileVRRotation));
										float turnSpeed = 180.0f;
										float snapTurnAngle = 0.0f;
										if (auto* psp = PlayerSetup::get_current_if_exists())
										{
											auto& pref = psp->get_preferences();
											if (pref.immobileVRSnapTurn)
											{
												turnSpeed = 0.0f;
												snapTurnAngle = max(5.0f, pref.immobileVRSnapTurnAngle);
											}
											else
											{
												turnSpeed = max(10.0f, pref.immobileVRContinuousTurnSpeed);
												snapTurnAngle = 0.0f;
											}
										}
										if (snapTurnAngle > 0.0f)
										{
											float threshold = 0.7f;
											if (abs(slidingLocmotionLastStickRotationX) < threshold &&
												abs(stickRotation.x) >= threshold)
											{
												yawControl = snapTurnAngle * sign(stickRotation.x);
											}
											else
											{
												yawControl = 0.0f;
											}
										}
										else
										{
											yawControl = stickRotation.x * turnSpeed * _advanceContext.get_world_delta_time();
										}
										slidingLocmotionLastStickRotationX = stickRotation.x;
									}

									Vector3 movementVRAnchor = movementControl;
									float yawVRAnchor = yawControl;

									// movement from vr view
									if (playerActor->get_presence()->is_vr_movement() &&
										VR::IVR::get() && VR::IVR::get()->is_controls_view_valid())
									{
										Transform movementCentreVR = VR::IVR::get()->get_controls_movement_centre();
										Vector3 lookForwardVR = calculate_flat_forward_from(movementCentreVR);
										Transform placementVR = look_matrix(movementCentreVR.get_translation() * Vector3::xy, lookForwardVR, Vector3::zAxis).to_transform();
										Transform placementWS = playerActor->get_presence()->get_vr_anchor().to_world(placementVR);

										Vector3 vrAnchorUp = playerActor->get_presence()->get_vr_anchor().get_axis(Axis::Up);

										// linear movement
										{
											Vector3 inRoomCurLoc = playerActor->get_presence()->get_placement().get_translation();
											Vector3 inRoomNewLoc = placementWS.get_translation();
											inRoomNewLoc -= vrAnchorUp * Vector3::dot(vrAnchorUp, inRoomNewLoc - inRoomCurLoc); // place on same altitude
											Vector3 preciseMovement = inRoomNewLoc - inRoomCurLoc;

											// disallow for too large movements, when using sliding locomotion, to avoid going through walls
											{
												Vector3 flatPreciseMovement = preciseMovement.drop_using(vrAnchorUp);
												float const maxFlatPreciseMovement = LIMIT_FLAT_MOVEMENT_DISTANCE;
												if (flatPreciseMovement.length() >= maxFlatPreciseMovement)
												{
													float alongUp = Vector3::dot(vrAnchorUp, preciseMovement);
													preciseMovement = flatPreciseMovement.normal() * maxFlatPreciseMovement + vrAnchorUp * alongUp;
												}
											}

											movementControl += preciseMovement;
										}

										// rotation
										{
											Vector3 requestedDirWS = placementWS.get_axis(Axis::Forward);
											Vector3 requestedDirOS = playerActor->get_presence()->get_placement().vector_to_local(requestedDirWS);
											float localYaw = Rotator3::get_yaw(requestedDirOS);
										
											yawControl += localYaw;
										}
									}

									// move vr anchor but only by what we moved via sticks
									if (! movementVRAnchor.is_zero() || yawVRAnchor != 0.0f)
									{
										Vector3 preciseMovement = movementVRAnchor;
										Quat preciseRotation = Rotator3(0.0f, yawVRAnchor, 0.0f).to_quat();

										Transform perceivedPlacement = playerActor->get_presence()->get_placement();
										Transform vrAnchor = playerActor->get_presence()->get_vr_anchor();

										// try to pivot around player's actual location, that is perceived location - from view
										if (auto* vr = VR::IVR::get())
										{
											if (vr->is_controls_view_valid())
											{
												Transform movementCentreVR = vr->get_controls_movement_centre();
												Transform flatHead = look_matrix(movementCentreVR.get_translation() * Vector3::xy, calculate_flat_forward_from(movementCentreVR), Vector3::zAxis).to_transform();
												Transform flatHeadInWorld = vrAnchor.to_world(flatHead);
												perceivedPlacement = flatHeadInWorld;
											}
										}

										{
											Transform newPerceivedPlacement = perceivedPlacement;
											Transform newVRAnchor;

											newPerceivedPlacement.set_translation(perceivedPlacement.get_translation() + preciseMovement);
											newPerceivedPlacement.set_orientation(perceivedPlacement.get_orientation().to_world(preciseRotation));
											Transform relVRAnchor = perceivedPlacement.to_local(vrAnchor);
											newVRAnchor = newPerceivedPlacement.to_world(relVRAnchor);

											Transform transformVRAnchor = vrAnchor.to_local(newVRAnchor);
											playerActor->get_presence()->transform_vr_anchor(transformVRAnchor);
											if (auto* mp = playerActor->get_gameplay_as<ModulePilgrim>())
											{
												auto& poi = mp->access_overlay_info();
												poi.on_transform_vr_anchor(transformVRAnchor);
											}
											if (auto* ois = OverlayInfo::System::get())
											{
												ois->on_transform_vr_anchor(transformVRAnchor);
											}
										}
									}

									// apply movement (linear and rotation)
									{
										playerActor->get_controller()->set_requested_precise_movement(movementControl);
										playerActor->get_controller()->set_requested_movement_direction(movementControl.normal());
										playerActor->get_controller()->set_requested_precise_rotation(Rotator3(0.0f, yawControl, 0.0f).to_quat());
										DEFINE_STATIC_NAME(normal);
										playerActor->get_controller()->set_requested_movement_parameters((::Framework::MovementParameters()).gait(NAME(normal)));
										todo_note(TXT("or maybe we should allow gravity to let us fall?"));
										playerActor->get_controller()->set_allow_gravity(false);
									}
								}
								else
								{
									{
										playerActor->get_controller()->clear_requested_precise_movement();
										playerActor->get_controller()->clear_requested_movement_direction();
										playerActor->get_controller()->clear_requested_precise_rotation();
										todo_note(TXT("or maybe we should allow gravity to let us fall?"));
										playerActor->get_controller()->set_allow_gravity(false);
									}
								}
							}
						}
						else // 
						{
							an_assert(!Game::is_using_sliding_locomotion());
							// reset player movement from view
#ifdef USE_PLAYER_MOVEMENT_FROM_HEAD
							{
								auto* inRoom = playerActor->get_presence()->get_in_room();
								if (playerMovementFromHead.inControlOf != playerActor ||
									playerMovementFromHead.inRoom != playerActor->get_presence()->get_in_room())
								{
									// reset
									playerMovementFromHead = PlayerMovementFromHead();
								}
								playerMovementFromHead.inControlOf = playerActor;
								playerMovementFromHead.inRoom = inRoom;
							}
#endif

							// movement
							if (playerActor->get_presence()->is_vr_movement())
							{
								if (
#ifdef AN_DEVELOPMENT_OR_PROFILER
									Game::get_as<Game>()->is_player_controls_allowed() &&
#endif
									VR::IVR::get() && VR::IVR::get()->is_controls_view_valid())
								{
									Vector3 newVRLoc = VR::IVR::get()->get_controls_movement_centre().get_translation();
									Vector3 inRoomNewLoc = playerActor->get_presence()->get_vr_anchor().location_to_world(newVRLoc);
									Transform const& currentPlacement = playerActor->get_presence()->get_placement();
									Vector3 inRoomCurLoc = currentPlacement.get_translation();
									Vector3 vrAnchorUp = playerActor->get_presence()->get_vr_anchor().get_axis(Axis::Up);
									inRoomNewLoc -= vrAnchorUp * Vector3::dot(vrAnchorUp, inRoomNewLoc - inRoomCurLoc); // place on same altitude
									Vector3 preciseMovement = inRoomNewLoc - inRoomCurLoc;
#ifdef USE_PLAYER_MOVEMENT_FROM_HEAD
									{
										// raw delta time as we're dealign with real world movement
										playerMovementFromHead.disallowStandStillForTime = max(0.0f, playerMovementFromHead.disallowStandStillForTime - ::System::Core::get_raw_delta_time());
										if (!playerMovementFromHead.standStillLoc.is_set())
										{
											playerMovementFromHead.standStillLoc = inRoomNewLoc;
										}
										float distance = (inRoomNewLoc - playerMovementFromHead.standStillLoc.get()).length();
										if (distance > 0.10f)
										{
											playerMovementFromHead.standStillLoc = inRoomNewLoc;
											playerMovementFromHead.mark_moving();
										}
										if (playerMovementFromHead.should_act_as_standing())
										{
											// no movement, wait till we move more
											playerMovementFromHead.usePreciseMovement = 0.0f;
										}
										else
										{
											playerMovementFromHead.usePreciseMovement = blend_to_using_time(playerMovementFromHead.usePreciseMovement, 1.0f, 0.6f, ::System::Core::get_raw_delta_time());
										}
										preciseMovement *= sqr(playerMovementFromHead.usePreciseMovement);
									}
#endif
									// Z should be placed exactly where vr anchor is, so we won't have the body floating way up or below the floor
									{
										float vraAlongUp = Vector3::dot(vrAnchorUp, playerActor->get_presence()->get_vr_anchor().get_translation());
										float curAlongUp = Vector3::dot(vrAnchorUp, playerActor->get_presence()->get_placement().get_translation());
										preciseMovement += vrAnchorUp * (vraAlongUp - curAlongUp);
									}

									playerActor->get_controller()->set_requested_precise_movement(preciseMovement);
									playerActor->get_controller()->set_requested_movement_direction(preciseMovement.normal());
									DEFINE_STATIC_NAME(normal);
									playerActor->get_controller()->set_requested_movement_parameters((::Framework::MovementParameters()).gait(NAME(normal)));
									todo_note(TXT("or maybe we should allow gravity to let us fall?"));
									playerActor->get_controller()->set_allow_gravity(false);
								}
								else
								{
									playerActor->get_controller()->clear_requested_precise_movement();
									playerActor->get_controller()->clear_requested_movement_direction();
									playerActor->get_controller()->set_requested_movement_parameters((::Framework::MovementParameters()));
									playerActor->get_controller()->clear_allow_gravity();
								}
							}
							
							DEFINE_STATIC_NAME(followHeadLookVR);
							Optional<Vector3> lookForward;
							if (lookLocal.is_set())
							{
								lookForward = calculate_flat_forward_from(lookLocal.get());
							}

							{
								if (lookForward.is_set() &&
#ifdef AN_DEVELOPMENT_OR_PROFILER
									Game::get_as<Game>()->is_player_controls_allowed() &&
#endif
									player.get_control_mode() == ControlMode::Actor &&
									(player.get_input_non_vr().is_button_pressed(NAME(followHeadLookVR)) || playerActor->get_presence()->is_vr_movement()))
								{
									// will emulate behavior of non vr - turn crawler towards where we're looking
									// or do this for vr movement - face where we look
									playerActor->get_controller()->set_follow_yaw_to_look_orientation(false);
									bool requestOrientation = true;
#ifdef USE_PLAYER_MOVEMENT_FROM_HEAD
									{
										float lookYaw = Rotator3::get_yaw(lookForward.get());
										if (abs(lookYaw) < STAND_STILL_HEAD_TO_BODY_YAW)
										{
											if (playerMovementFromHead.should_act_as_standing())
											{
												requestOrientation = false;
											}
										}
										else
										{
											playerMovementFromHead.mark_moving();
										}
									}
#endif
									if (requestOrientation)
									{
										playerActor->get_controller()->set_requested_orientation(playerActor->get_presence()->get_placement().to_world(look_matrix(Vector3::zero, lookForward.get(), Vector3::zAxis).to_quat()));
									}
									else
									{
										playerActor->get_controller()->clear_requested_orientation();
									}
									// but still maintain relative requested look orientation - we want that still to be active to keep forward as forward
									playerActor->get_controller()->set_maintain_relative_requested_look_orientation(true);
								}
								else
								{
									// stop rotation
									playerActor->get_controller()->set_follow_yaw_to_look_orientation(false);
									playerActor->get_controller()->clear_requested_orientation();
									playerActor->get_controller()->set_maintain_relative_requested_look_orientation(true);
								}
							}
						}

						//

						// advance other controls

#ifdef AN_DEVELOPMENT_OR_PROFILER
						if (Game::get_as<Game>()->is_player_controls_allowed())
#endif
						{
							if (auto * ic = playerActor->get_gameplay_as<IControllable>())
							{
								process_controllable_controls(ic, player, true, _advanceContext.get_world_delta_time());
							}
							if (doGeneralControls)
							{
								process_general_controls(player, true, _advanceContext.get_other_delta_time());
								doGeneralControls = false;
							}
						}

						//
				
						if (advancedAtLeastOnce) // we need at least one valid frame to allow vr controls to use stuff from that
						{
							playerActor->advance_vr_important(_advanceContext.get_world_delta_time());
						}

						mainControl = false;
					}
				}
			}
		}
#ifdef INVESTIGATE_MISSING_INPUT
		else
		{
			REMOVE_AS_SOON_AS_POSSIBLE_ output(TXT("!@# can't process_vr_controls (no player)"));
		}
#endif
	}
#ifdef INVESTIGATE_MISSING_INPUT
	else
	{
		REMOVE_AS_SOON_AS_POSSIBLE_ output(TXT("!@# can't process_vr_controls (vr not ok)"));
	}
#endif
}

void World::process_controllable_controls(IControllable* _ic, Player & _player, bool _vr, float _deltaTime)
{
	if (!System::Core::is_paused_at_all())
	{
		_ic->pre_process_controls();
		_ic->process_controls(Hand::Left, _vr ? _player.get_input_vr_left() : _player.get_input_non_vr(), _deltaTime);
		_ic->process_controls(Hand::Right, _vr ? _player.get_input_vr_right() : _player.get_input_non_vr(), _deltaTime);
	}
#ifdef INVESTIGATE_MISSING_INPUT
	else
	{
		REMOVE_AS_SOON_AS_POSSIBLE_ output(TXT("!@# can't process_controllable_controls"));
	}
#endif
}

void World::process_general_controls(Player & _player, bool _vr, float _deltaTime)
{
	if (!System::Core::is_paused_at_all())
	{
		bool inputHeadMenuRequested[2];
		bool inputHeadMenuRequestedHold[2];
		for_count(int, hIdx, Hand::MAX)
		{
			auto& controls = _vr ? (hIdx == Hand::Left? _player.get_input_vr_left() : _player.get_input_vr_right()) : _player.get_input_non_vr();
			inputHeadMenuRequested[hIdx] = controls.get_button_state(NAME(requestInGameMenu)) > CONTROLS_THRESHOLD;
			inputHeadMenuRequestedHold[hIdx] = controls.get_button_state(NAME(requestInGameMenuHold)) > CONTROLS_THRESHOLD;
		}

		bool headMenuRequested = inputHeadMenuRequested[0] || inputHeadMenuRequested[1];
		bool headMenuRequestedHold = inputHeadMenuRequestedHold[0] && inputHeadMenuRequestedHold[1]; // hold head with both hands!

		if (auto* gd = GameDirector::get())
		{
			if (gd->is_game_menu_blocked())
			{
				headMenuRequested = false;
				headMenuRequestedHold = false;
			}
		}

		fast_cast<Game>(game)->request_in_game_menu(headMenuRequested, headMenuRequestedHold);
	}
}

void World::prepare_to_sound(Framework::GameAdvanceContext const & _advanceContext)
{
	base::prepare_to_sound(_advanceContext);
	if (auto* playerActor = get_player_in_control_actor_for_perception())
	{
		fast_cast<Game>(game)->prepare_world_to_sound(_advanceContext.get_world_delta_time(), playerActor);
	}
	else if (camera.is_set())
	{
		::Framework::Sound::Camera soundCamera;
		soundCamera.base_on(camera.get());
		fast_cast<Game>(game)->prepare_world_to_sound(_advanceContext.get_world_delta_time(), soundCamera);
	}
}

void World::prepare_to_render(Framework::CustomRenderContext * _customRC)
{
	base::prepare_to_render(_customRC);
	if (auto* playerActor = get_player_in_control_actor_for_perception())
	{
		fast_cast<Game>(game)->prepare_world_to_render(playerActor, _customRC);
	}
	else if (camera.is_set())
	{
		fast_cast<Game>(game)->prepare_world_to_render(camera.get(), _customRC);
	}
}

void World::render_on_render(Framework::CustomRenderContext * _customRC)
{
	base::render_on_render(_customRC);
	fast_cast<Game>(game)->render_prepared_world_on_render(_customRC);
}

void World::on_paused()
{
	base::on_paused();

	Game* useGame = fast_cast<Game>(game);
	useGame->access_sound_scene()->pause();
	useGame->access_music_player()->pause();
	useGame->access_voiceover_system()->pause();
	useGame->access_environment_player()->pause();
}

void World::on_resumed()
{
	Game* useGame = fast_cast<Game>(game);
	useGame->access_sound_scene()->resume();
	useGame->access_music_player()->resume();
	useGame->access_voiceover_system()->resume();
	useGame->access_environment_player()->resume();

	playerControlModeTime[0] = 0.0f;
	playerControlModeTime[1] = 0.0f;

	base::on_resumed();
}


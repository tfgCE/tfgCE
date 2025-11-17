#include "gse_presenceForceDisplacementVRAnchor.h"

#include "..\..\module\modulePresence.h"
#include "..\..\world\room.h"

#include "..\..\..\core\io\xml.h"
#include "..\..\..\core\system\core.h"

#include "..\..\..\core\debug\debugRenderer.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//#define DEBUG_DRAW_PRESENCE_FORCE_DISPLACEMENT_VR_ANCHOR

//

using namespace Framework;
using namespace GameScript;
using namespace Elements;

//

DEFINE_STATIC_NAME(self);

// execution variables
DEFINE_STATIC_NAME(presenceForceDisplacementVRAnchor_room);
DEFINE_STATIC_NAME(presenceForceDisplacementVRAnchor_time);
DEFINE_STATIC_NAME(presenceForceDisplacementVRAnchor_rotVel);

//

bool PresenceForceDisplacementVRAnchor::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	objectVar = _node->get_name_attribute(TXT("objectVar"), objectVar);

	if (!objectVar.is_valid())
	{
		objectVar = NAME(self);
	}

	useObjectVariableForLinearVelocity = _node->get_name_attribute(TXT("useObjectVariableForLinearVelocity"), useObjectVariableForLinearVelocity);
	continueUsingObjectVariableForLinearVelocity = _node->get_bool_attribute_or_from_child_presence(TXT("continueUsingObjectVariableForLinearVelocity"), continueUsingObjectVariableForLinearVelocity);
	for_every(n, _node->children_named(TXT("objectVariableForLinearVelocity")))
	{
		useObjectVariableForLinearVelocity = n->get_name_attribute(TXT("var"), useObjectVariableForLinearVelocity);
		continueUsingObjectVariableForLinearVelocity = n->get_bool_attribute_or_from_child_presence(TXT("continue"), continueUsingObjectVariableForLinearVelocity);
		continueUsingObjectVariableForLinearVelocity = n->get_bool_attribute_or_from_child_presence(TXT("continuation"), continueUsingObjectVariableForLinearVelocity);
	}

	for_every(n, _node->children_named(TXT("forceLoc")))
	{
		forceLocX.load_from_xml(n, TXT("x"));
		forceLocY.load_from_xml(n, TXT("y"));
		forceLocZ.load_from_xml(n, TXT("z"));
	}
	forceForwardDir.load_from_xml(_node, TXT("forceForwardDir"));
	forceUpDir.load_from_xml(_node, TXT("forceUpDir"));

	for_every(node, _node->children_named(TXT("timeBasedDisplacement")))
	{
		timeBasedDisplacement.defined = true;
		for_every(n, node->children_named(TXT("startingVelocity")))
		{
			timeBasedDisplacement.startingVelocityXY.load_from_xml(n, TXT("horizontal"));
			timeBasedDisplacement.startingVelocityZ.load_from_xml(n, TXT("vertical"));
		}
		timeBasedDisplacement.velocity.load_from_xml(node, TXT("velocity"));
		timeBasedDisplacement.velocityFromObjectVar.load_from_xml(node, TXT("velocityFromObjectVar"));
		timeBasedDisplacement.maintainHorizontalVelocity.load_from_xml(node, TXT("maintainHorizontalVelocity"));
		timeBasedDisplacement.linearAcceleration.load_from_xml(node, TXT("linearAcceleration"));
		for_every(n, node->children_named(TXT("velocity")))
		{
			timeBasedDisplacement.linearAcceleration.load_from_xml(n, TXT("acceleration"));
		}
		for_every(n, node->children_named(TXT("pullHorizontally")))
		{
			timeBasedDisplacement.pullHorizontallyTowardsPOI.load_from_xml(n, TXT("towardsPOI"));
			timeBasedDisplacement.pullHorizontallySpeed.load_from_xml(n, TXT("speed"));
			timeBasedDisplacement.pullHorizontallyBlendTime.load_from_xml(n, TXT("blendTime"));
			timeBasedDisplacement.pullHorizontallyCoef.load_from_xml(n, TXT("coef"));
		}

		timeBasedDisplacement.rotate.load_from_xml(node, TXT("rotate"));
		timeBasedDisplacement.orientationAcceleration.load_from_xml(node, TXT("orientationAcceleration"));
		timeBasedDisplacement.ownOrientationAcceleration = node->get_bool_attribute_or_from_child_presence(TXT("ownOrientationAcceleration"), timeBasedDisplacement.ownOrientationAcceleration);
		timeBasedDisplacement.ownOrientationAccelerationContinue = node->get_bool_attribute_or_from_child_presence(TXT("ownOrientationAccelerationContinue"), timeBasedDisplacement.ownOrientationAccelerationContinue);
		for_every(n, node->children_named(TXT("rotate")))
		{
			timeBasedDisplacement.orientationAcceleration.load_from_xml(n, TXT("acceleration"));
			timeBasedDisplacement.ownOrientationAcceleration = n->get_bool_attribute_or_from_child_presence(TXT("ownAcceleration"), timeBasedDisplacement.ownOrientationAcceleration);
			timeBasedDisplacement.ownOrientationAccelerationContinue = n->get_bool_attribute_or_from_child_presence(TXT("ownAccelerationContinue"), timeBasedDisplacement.ownOrientationAccelerationContinue);
		}
	}

	if (timeBasedDisplacement.ownOrientationAccelerationContinue)
	{
		timeBasedDisplacement.ownOrientationAcceleration = true;
	}

	for_every(node, _node->children_named(TXT("followPath")))
	{
		followPath.defined = true;
		followPath.poi = node->get_name_attribute(TXT("poi"), followPath.poi);

		for_every(n, node->children_named(TXT("location")))
		{
			followPath.location.speed.load_from_xml(n, TXT("speed"));
			followPath.location.acceleration.load_from_xml(n, TXT("acceleration"));
			followPath.location.accelerationAlong.load_from_xml(n, TXT("accelerationAlong"));
			for_every(subn, n->children_named(TXT("towards")))
			{
				result &= followPath.location.towards.load_from_xml(subn);
			}
		}
		for_every(n, node->children_named(TXT("orientation")))
		{
			followPath.orientation.speed.load_from_xml(n, TXT("speed"));
			followPath.orientation.speedCoef.load_from_xml(n, TXT("speedCoef"));
			followPath.orientation.finalDiff.load_from_xml(n, TXT("finalDiff"));
			for_every(subn, n->children_named(TXT("relativeTo")))
			{
				result &= followPath.orientation.relativeTo.load_from_xml(subn);
			}
			for_every(subn, n->children_named(TXT("upTowards")))
			{
				result &= followPath.orientation.upTowards.load_from_xml(subn);
			}
			for_every(subn, n->children_named(TXT("forwardTowards")))
			{
				result &= followPath.orientation.forwardTowards.load_from_xml(subn);
			}
		}
	}

	for_every(node, _node->children_named(TXT("endCondition")))
	{
		endCondition.leavesRoom = node->get_bool_attribute_or_from_child_presence(TXT("leavesRoom"), endCondition.leavesRoom);
		endCondition.afterTime.load_from_xml(node, TXT("afterTime"));
		endCondition.atZBelow.load_from_xml(node, TXT("atZBelow"));
	}

	return result;
}

bool PresenceForceDisplacementVRAnchor::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	return result;
}

ScriptExecutionResult::Type PresenceForceDisplacementVRAnchor::execute(ScriptExecution& _execution, ScriptExecution::Flags _flags) const
{
	bool repeatThis = false;

	if (objectVar.is_valid())
	{
		if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::IModulesOwner>>(objectVar))
		{
			if (auto* imo = exPtr->get())
			{
				if (auto* p = imo->get_presence())
				{
					float deltaTime = ::System::Core::get_delta_time();

					repeatThis = true;
					if (endCondition.leavesRoom)
					{
						SafePtr<Framework::Room>& inRoomVar = _execution.access_variables().access<SafePtr<Framework::Room>>(NAME(presenceForceDisplacementVRAnchor_room));
						if (is_flag_set(_flags, ScriptExecution::Flag::Entered))
						{
							inRoomVar = p->get_in_room();
						}
						else
						{
							if (inRoomVar != p->get_in_room())
							{
								repeatThis = false;
							}
						}
					}
					if (endCondition.afterTime.is_set())
					{
						float& timeHere = _execution.access_variables().access<float>(NAME(presenceForceDisplacementVRAnchor_time));
						if (is_flag_set(_flags, ScriptExecution::Flag::Entered))
						{
							timeHere = 0.0f;
						}
						else
						{
							timeHere += deltaTime;
						}
						if (timeHere >= endCondition.afterTime.get())
						{
							repeatThis = false;
						}
					}
					if (endCondition.atZBelow.is_set())
					{
						if (p->get_placement().get_translation().z < endCondition.atZBelow.get())
						{
							repeatThis = false;
						}
					}

					Transform placementWS = p->get_placement();
					Transform prevPlacementWS = p->get_placement().to_world(p->get_prev_placement_offset());

					Vector3 currentLinearVelocityWS;
					Rotator3 currentOrientationVelocity;
					{
						float prevDeltaTime = p->get_prev_placement_offset_delta_time();
						float invPrevDeltaTime = (prevDeltaTime > 0.0f ? 1.0f / prevDeltaTime : 0.0f);
						currentLinearVelocityWS = (placementWS.get_translation() - prevPlacementWS.get_translation()) * invPrevDeltaTime;
						currentOrientationVelocity = Quat::slerp(invPrevDeltaTime, Quat::identity, prevPlacementWS.to_local(placementWS).get_orientation()).to_rotator();
					}
					if (useObjectVariableForLinearVelocity.is_valid())
					{
						if (! is_flag_set(_flags, ScriptExecution::Flag::Entered) || // if we have variable set, then unless we entered
							continueUsingObjectVariableForLinearVelocity) // or if we're continuing, then always
						{
							currentLinearVelocityWS = imo->get_variables().get_value<Vector3>(useObjectVariableForLinearVelocity, currentLinearVelocityWS);
						}
					}

					Optional<Vector3> locationDisplacement;
					Optional<Quat> orientationDisplacement;

#ifdef DEBUG_DRAW_PRESENCE_FORCE_DISPLACEMENT_VR_ANCHOR
					debug_context(p->get_in_room());

					debug_draw_transform(true, placementWS);
#endif

					Optional<Vector3> outputVelocityWS;

					if (timeBasedDisplacement.defined)
					{
						Optional<Vector3> velocity = timeBasedDisplacement.velocity;
						if (timeBasedDisplacement.velocityFromObjectVar.is_set())
						{
							if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::IModulesOwner>>(timeBasedDisplacement.velocityFromObjectVar.get()))
							{
								if (auto* imo = exPtr->get())
								{
									if (auto* p = imo->get_presence())
									{
										velocity = p->get_velocity_linear();
									}
								}
							}
						}
						if (velocity.is_set())
						{
							Vector3 resultVelocityWS = velocity.get();
							if (timeBasedDisplacement.linearAcceleration.is_set())
							{
								Vector3 targetVelocityWS = resultVelocityWS;

								Vector3 useLinearVelocityWS = currentLinearVelocityWS;
								if (timeBasedDisplacement.maintainHorizontalVelocity.is_set())
								{
									useLinearVelocityWS.x = 0.0f;
									useLinearVelocityWS.y = 0.0f;
								}
								if (is_flag_set(_flags, ScriptExecution::Flag::Entered))
								{
									if (timeBasedDisplacement.startingVelocityXY.is_set())
									{
										useLinearVelocityWS.x = timeBasedDisplacement.startingVelocityXY.get().x;
										useLinearVelocityWS.y = timeBasedDisplacement.startingVelocityXY.get().y;
									}
									if (timeBasedDisplacement.startingVelocityZ.is_set())
									{
										useLinearVelocityWS.z = timeBasedDisplacement.startingVelocityZ.get();
									}
								}

								Vector3 diff = targetVelocityWS - useLinearVelocityWS;
								resultVelocityWS = useLinearVelocityWS + diff.normal() * min(diff.length(), timeBasedDisplacement.linearAcceleration.get() * deltaTime);
							}
							if (timeBasedDisplacement.maintainHorizontalVelocity.is_set())
							{
								resultVelocityWS += timeBasedDisplacement.maintainHorizontalVelocity.get();
							}
							locationDisplacement = resultVelocityWS * deltaTime;

							if (timeBasedDisplacement.pullHorizontallyTowardsPOI.is_set())
							{
								Framework::PointOfInterestInstance * foundPOI = nullptr;
								if (p->get_in_room()->find_any_point_of_interest(timeBasedDisplacement.pullHorizontallyTowardsPOI, OUT_ foundPOI))
								{
									Transform poiPlacement = foundPOI->calculate_placement();
									Vector3 diff = poiPlacement.get_translation() - placementWS.get_translation();
									diff.z = 0.0f;
									Vector3 applyHorizontalDisplacement = Vector3::zero;
									if (timeBasedDisplacement.pullHorizontallySpeed.is_set())
									{
										applyHorizontalDisplacement = diff.normal() * min(diff.length(), timeBasedDisplacement.pullHorizontallySpeed.get() * deltaTime);
									}
									else
									{
										applyHorizontalDisplacement = diff * clamp(deltaTime / timeBasedDisplacement.pullHorizontallyBlendTime.get(0.3f), 0.0f, 1.0f);
									}
									applyHorizontalDisplacement *= timeBasedDisplacement.pullHorizontallyCoef.get(1.0f);
									locationDisplacement = locationDisplacement.get(Vector3::zero) + applyHorizontalDisplacement;
								}
							}

							outputVelocityWS = resultVelocityWS;
						}

						if (timeBasedDisplacement.rotate.is_set())
						{
							Rotator3 resultVelocity = timeBasedDisplacement.rotate.get();
							if (timeBasedDisplacement.orientationAcceleration.is_set())
							{
								Rotator3 targetVelocity = resultVelocity;

								if (timeBasedDisplacement.ownOrientationAcceleration)
								{
									Rotator3& curOriVel = _execution.access_variables().access<Rotator3>(NAME(presenceForceDisplacementVRAnchor_rotVel));
									if (!timeBasedDisplacement.ownOrientationAccelerationContinue)
									{
										if (is_flag_set(_flags, ScriptExecution::Flag::Entered))
										{
											curOriVel = currentOrientationVelocity;
										}
									}
									Rotator3 diff = targetVelocity - curOriVel;
									resultVelocity = curOriVel + diff.normal() * min(diff.length(), timeBasedDisplacement.orientationAcceleration.get() * deltaTime);
									curOriVel = resultVelocity;
								}
								else
								{
									Rotator3 diff = targetVelocity - currentOrientationVelocity;
									resultVelocity = currentOrientationVelocity + diff.normal() * min(diff.length(), timeBasedDisplacement.orientationAcceleration.get() * deltaTime);
								}
							}
							orientationDisplacement = (resultVelocity * deltaTime).to_quat();
						}
					}

					if (followPath.defined)
					{
						Range relativeDistance = Range::zero;
						followPath.location.towards.update_relative_distance_range(REF_ relativeDistance);
						followPath.orientation.relativeTo.update_relative_distance_range(REF_ relativeDistance);
						followPath.orientation.upTowards.update_relative_distance_range(REF_ relativeDistance);
						followPath.orientation.forwardTowards.update_relative_distance_range(REF_ relativeDistance);

						if (followPath.poi.is_valid())
						{
							Vector3 pAt = p->get_placement().get_translation();
							float maxDist = max(-relativeDistance.min, relativeDistance.max) + 5.0f;

							ArrayStatic<FollowPath::FoundPOI, 32> foundPOIs;
							p->get_in_room()->for_every_point_of_interest(followPath.poi,
								[maxDist, pAt, &foundPOIs](Framework::PointOfInterestInstance* _fpoi)
								{
									Transform poiPlacement = _fpoi->calculate_placement();
									float dist = (poiPlacement.get_translation() - pAt).length();
									if (dist < maxDist)
									{
										an_assert(foundPOIs.has_place_left());
										if (foundPOIs.has_place_left())
										{
											FollowPath::FoundPOI fp;
											fp.placement = poiPlacement;
											fp.relativePlacement = -poiPlacement.location_to_local(pAt).y;
											foundPOIs.push_back(fp);
										}
									}
								});
							if (!foundPOIs.is_empty())
							{
								sort(foundPOIs);

								if (followPath.location.speed.is_set())
								{
									Vector3 resultVelocityWS = Vector3::zero;
									if (followPath.location.towards.defined)
									{
										Vector3 towardsWS = followPath.location.towards.process(foundPOIs.get_data(), foundPOIs.get_size());
#ifdef DEBUG_DRAW_PRESENCE_FORCE_DISPLACEMENT_VR_ANCHOR
										debug_draw_arrow(true, Colour::magenta, pAt, towardsWS);
#endif
										resultVelocityWS = (towardsWS - pAt);
									}
									resultVelocityWS = resultVelocityWS.normal() * followPath.location.speed.get();
									if (followPath.location.acceleration.is_set())
									{
										Vector3 targetVelocityWS = resultVelocityWS;

										Vector3 diff = targetVelocityWS - currentLinearVelocityWS;
										resultVelocityWS = currentLinearVelocityWS + diff.normal() * min(diff.length(), followPath.location.acceleration.get() * deltaTime);
									}
									if (followPath.location.accelerationAlong.is_set())
									{
										Vector3 targetVelocityWS = resultVelocityWS;

										float speed = currentLinearVelocityWS.length();
										float diff = resultVelocityWS.length() - speed;
										resultVelocityWS = targetVelocityWS.normal() * (speed + sign(diff) * min(abs(diff), followPath.location.accelerationAlong.get() * deltaTime));
									}
									locationDisplacement = resultVelocityWS * deltaTime;

									outputVelocityWS = resultVelocityWS;
								}

								if (followPath.orientation.speed.is_set())
								{
									Vector3 upDirWS = p->get_placement().get_axis(Axis::Up);
									Vector3 forwardDirWS = p->get_placement().get_axis(Axis::Forward);
									Vector3 rightDirWS = p->get_placement().get_axis(Axis::Right);

									Vector3 relativeTo = pAt;
									if (followPath.orientation.relativeTo.defined)
									{
										relativeTo = followPath.orientation.relativeTo.process(foundPOIs.get_data(), foundPOIs.get_size());
									}
									if (followPath.orientation.upTowards.defined)
									{
										Vector3 locWS = followPath.orientation.upTowards.process(foundPOIs.get_data(), foundPOIs.get_size());
#ifdef DEBUG_DRAW_PRESENCE_FORCE_DISPLACEMENT_VR_ANCHOR
										debug_draw_arrow(true, Colour::blue, relativeTo, locWS);
#endif
										upDirWS = (locWS - relativeTo).normal();
									}
									if (followPath.orientation.forwardTowards.defined)
									{
										Vector3 locWS = followPath.orientation.forwardTowards.process(foundPOIs.get_data(), foundPOIs.get_size());
#ifdef DEBUG_DRAW_PRESENCE_FORCE_DISPLACEMENT_VR_ANCHOR
										debug_draw_arrow(true, Colour::green, relativeTo, locWS);
#endif
										forwardDirWS = (locWS - relativeTo).normal();
									}

									Transform targetPlacement;
									targetPlacement.build_from_axes(Axis::Up, Axis::Forward, forwardDirWS, rightDirWS, upDirWS, relativeTo);

#ifdef DEBUG_DRAW_PRESENCE_FORCE_DISPLACEMENT_VR_ANCHOR
									debug_draw_transform_coloured(true, targetPlacement, Colour::zxWhite, Colour::zxGreen, Colour::zxBlue);
#endif

									Quat orientationRequired = p->get_placement().to_local(targetPlacement).get_orientation();
									Rotator3 rotationStep = orientationRequired.to_rotator();
									Rotator3 maxRotationStep = followPath.orientation.speed.get() * deltaTime;
									Rotator3 useFully(1.0f, 1.0f, 1.0f);
									if (followPath.orientation.finalDiff.is_set())
									{
										if (followPath.orientation.finalDiff.get().yaw > 0.0f)
										{
											useFully.yaw = min(useFully.yaw, abs(rotationStep.yaw) / followPath.orientation.finalDiff.get().yaw);
										}
										if (followPath.orientation.finalDiff.get().pitch > 0.0f)
										{
											useFully.pitch = min(useFully.pitch, abs(rotationStep.pitch) / followPath.orientation.finalDiff.get().pitch);
										}
										if (followPath.orientation.finalDiff.get().roll > 0.0f)
										{
											useFully.roll = min(useFully.roll, abs(rotationStep.roll) / followPath.orientation.finalDiff.get().roll);
										}
									}
									if (maxRotationStep.pitch > 0.0f)
									{
										if (abs(rotationStep.pitch) > maxRotationStep.pitch)
										{
											rotationStep *= maxRotationStep.pitch / abs(rotationStep.pitch);
										}
									}
									if (maxRotationStep.yaw > 0.0f)
									{
										if (abs(rotationStep.yaw) > maxRotationStep.yaw)
										{
											rotationStep *= maxRotationStep.yaw / abs(rotationStep.yaw);
										}
									}
									if (maxRotationStep.roll > 0.0f)
									{
										if (abs(rotationStep.roll) > maxRotationStep.roll)
										{
											rotationStep *= maxRotationStep.roll / abs(rotationStep.roll);
										}
									}
									if (followPath.orientation.speedCoef.is_set())
									{
										if (followPath.orientation.speedCoef.get().yaw > 0.0f)
										{
											rotationStep.yaw *= followPath.orientation.speedCoef.get().yaw;
										}
										if (followPath.orientation.speedCoef.get().pitch > 0.0f)
										{
											rotationStep.pitch *= followPath.orientation.speedCoef.get().pitch;
										}
										if (followPath.orientation.speedCoef.get().roll > 0.0f)
										{
											rotationStep.roll *= followPath.orientation.speedCoef.get().roll;
										}
									}
									rotationStep *= useFully;
									orientationDisplacement = rotationStep.to_quat();
								}
							}
						}
					}

					if (forceLocX.is_set() || forceLocY.is_set() || forceLocZ.is_set())
					{
						Vector3 currLoc = placementWS.get_translation();
						Vector3 targetLoc = currLoc + locationDisplacement.get(Vector3::zero);
						targetLoc.x = forceLocX.get(targetLoc.x);
						targetLoc.y = forceLocY.get(targetLoc.y);
						targetLoc.z = forceLocZ.get(targetLoc.z);
						locationDisplacement = targetLoc - currLoc;
					}

					if (forceForwardDir.is_set() ||
						forceUpDir.is_set())
					{
						Quat currOrientation = placementWS.get_orientation();
						Quat targetOrientation = currOrientation.to_world(orientationDisplacement.get(Quat::identity));
						Quat forcedOrientation = look_matrix33(forceForwardDir.get(targetOrientation.get_axis(Axis::Forward)), forceUpDir.get(targetOrientation.get_axis(Axis::Up))).to_quat();
						orientationDisplacement = currOrientation.to_local(forcedOrientation);
					}

					if (outputVelocityWS.is_set())
					{
						if (useObjectVariableForLinearVelocity.is_valid())
						{
							imo->access_variables().access<Vector3>(useObjectVariableForLinearVelocity) = outputVelocityWS.get();
						}
					}

					if (locationDisplacement.is_set())
					{
						p->force_location_displacement_vr_anchor(locationDisplacement.get());
					}
					if (orientationDisplacement.is_set())
					{
						REMOVE_OR_CHANGE_BEFORE_COMMIT_ output(TXT("%p %S"), this, orientationDisplacement.get().to_rotator().to_string().to_char());
						p->force_orientation_displacement_vr_anchor(orientationDisplacement.get());
					}

#ifdef DEBUG_DRAW_PRESENCE_FORCE_DISPLACEMENT_VR_ANCHOR
					debug_no_context();
#endif
				}
			}
		}
	}

	return repeatThis? ScriptExecutionResult::Repeat : ScriptExecutionResult::Continue;
}

//

bool PresenceForceDisplacementVRAnchor::FollowPath::LocationDescription::load_from_xml(IO::XML::Node const* _node)
{
	bool result = true;
	if (_node)
	{
		poiPlacementRelativeDistance.load_from_xml(_node, TXT("poiPlacementRelativeDistance"));
		poiPlacementLocationLS.load_from_xml(_node, TXT("poiPlacementLocationLS"));
		for_every(node, _node->children_named(TXT("poiPlacement")))
		{
			poiPlacementRelativeDistance.load_from_xml(node, TXT("relativeDistance"));
			poiPlacementLocationLS.load_from_xml(node, TXT("locationLS"));
		}
		addWS.load_from_xml(_node, TXT("addWS"));
		defined = true;
	}
	return result;
}

void PresenceForceDisplacementVRAnchor::FollowPath::LocationDescription::update_relative_distance_range(REF_ Range& _relativeDistance) const
{
	if (poiPlacementRelativeDistance.is_set())
	{
		_relativeDistance.include(poiPlacementRelativeDistance.get());
	}
}

Vector3 PresenceForceDisplacementVRAnchor::FollowPath::LocationDescription::process(FoundPOI const* _pois, int _poiCount) const
{
	Transform placementWS = _pois->placement;

	{
		FoundPOI const* poi = _pois;
		FoundPOI const* poiNext = poi;
		++poiNext;

		float useRelativeDistance = poiPlacementRelativeDistance.get(0.0f);

		bool found = false;
		for_count(int, i, _poiCount - 1)
		{
			float poiR = poi->relativePlacement - useRelativeDistance;
			float poiNR = poiNext->relativePlacement - useRelativeDistance;
			if (poiR <= 0.0f && poiNR >= 0.0f)
			{
				float pt = clamp(poiNR != poiR? (0.0f - poiR) / (poiNR - poiR) : 0.5f, 0.0f, 1.0f);
				placementWS = Transform::lerp(pt, poi->placement, poiNext->placement);
				found = true;
				break;
			}
			++poi;
			++poiNext;
		}

		if (!found)
		{
			an_assert(false, TXT("use larger range"));
			if (useRelativeDistance < _pois->relativePlacement)
			{
				placementWS = _pois->placement;
			}
			else
			{
				placementWS = _pois[_poiCount - 1].placement;
			}
		}
	}

	Vector3 locWS = placementWS.get_translation();

	if (poiPlacementLocationLS.is_set())
	{
		locWS = placementWS.location_to_world(poiPlacementLocationLS.get());
	}
	if (addWS.is_set())
	{
		locWS += addWS.get();
	}

	return locWS;
}

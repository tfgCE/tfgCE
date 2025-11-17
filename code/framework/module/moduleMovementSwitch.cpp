#include "moduleMovementSwitch.h"

#include "registeredModule.h"
#include "modules.h"
#include "moduleDataImpl.inl"

#include "..\advance\advanceContext.h"

#include "..\..\core\performance\performanceUtils.h"

using namespace Framework;

//

// module parameters
DEFINE_STATIC_NAME(initialPt);
DEFINE_STATIC_NAME(restAt);
DEFINE_STATIC_NAME(lockedAt);
DEFINE_STATIC_NAME(placementCurveAlongAxis);
DEFINE_STATIC_NAME(parentRadialCentreSocket);
DEFINE_STATIC_NAME(radialToCentreAxis);
DEFINE_STATIC_NAME(radialRotationAxis);
DEFINE_STATIC_NAME(radialRadius);
DEFINE_STATIC_NAME(blendPlacementPtTime);
DEFINE_STATIC_NAME(allowRestDistPt);
DEFINE_STATIC_NAME(canStopAnywhere);
DEFINE_STATIC_NAME(activeDist);
DEFINE_STATIC_NAME(inertia);
DEFINE_STATIC_NAME(placementSocket0);
DEFINE_STATIC_NAME(placementSocket1);
DEFINE_STATIC_NAME(placementSocket2);
DEFINE_STATIC_NAME(placementSocket3);
DEFINE_STATIC_NAME(placementRelative0);
DEFINE_STATIC_NAME(placementRelative1);
DEFINE_STATIC_NAME(placementRelative2);
DEFINE_STATIC_NAME(placementRelative3);
DEFINE_STATIC_NAME(placementParentSocket0);
DEFINE_STATIC_NAME(placementParentSocket1);
DEFINE_STATIC_NAME(placementParentSocket2);
DEFINE_STATIC_NAME(placementParentSocket3);
DEFINE_STATIC_NAME(placementAllowRest0);
DEFINE_STATIC_NAME(placementAllowRest1);
DEFINE_STATIC_NAME(placementAllowRest2);
DEFINE_STATIC_NAME(placementAllowRest3);
DEFINE_STATIC_NAME(placementSoundIn0);
DEFINE_STATIC_NAME(placementSoundIn1);
DEFINE_STATIC_NAME(placementSoundIn2);
DEFINE_STATIC_NAME(placementSoundIn3);
DEFINE_STATIC_NAME(placementSoundOut0);
DEFINE_STATIC_NAME(placementSoundOut1);
DEFINE_STATIC_NAME(placementSoundOut2);
DEFINE_STATIC_NAME(placementSoundOut3);
DEFINE_STATIC_NAME(placementOutput0);
DEFINE_STATIC_NAME(placementOutput1);
DEFINE_STATIC_NAME(placementOutput2);
DEFINE_STATIC_NAME(placementOutput3);

//

bool ModuleMovementSwitchSetup::Placement::load_from_xml(IO::XML::Node const * _node)
{
	bool result = true;

	output.load_from_xml(_node, TXT("output"));
	placement.load_from_xml(_node, TXT("placement"));
	socket = _node->get_name_attribute_or_from_child(TXT("socket"), socket);
	parentSocket = _node->get_name_attribute_or_from_child(TXT("parentSocket"), parentSocket);
	allowRest = _node->get_bool_attribute_or_from_child_presence(TXT("allowRest"), allowRest);
	soundIn = _node->get_name_attribute_or_from_child(TXT("soundIn"), soundIn);
	soundOut = _node->get_name_attribute_or_from_child(TXT("soundOut"), soundOut);

	return result;
}

//

REGISTER_FOR_FAST_CAST(ModuleMovementSwitch);

static ModuleMovement* create_module(IModulesOwner* _owner)
{
	return new ModuleMovementSwitch(_owner);
}

static ModuleData* create_module_data(::Framework::LibraryStored* _inLibraryStored)
{
	return new ModuleMovementSwitchData(_inLibraryStored);
}

RegisteredModule<ModuleMovement> & ModuleMovementSwitch::register_itself()
{
	return Modules::movement.register_module(String(TXT("switch")), create_module, create_module_data);
}

ModuleMovementSwitch::ModuleMovementSwitch(IModulesOwner* _owner)
: base(_owner)
{
	requiresPrepareMove = false;
}

void ModuleMovementSwitch::setup_with(ModuleData const * _moduleData)
{
	base::setup_with(_moduleData);
	if (_moduleData)
	{
		moduleMovementSwitchData = fast_cast<ModuleMovementSwitchData>(_moduleData);

		setup = moduleMovementSwitchData->setup;

		placementPt = _moduleData->get_parameter<float>(this, NAME(initialPt), placementPt);
		if (_moduleData->has_parameter<float>(this, NAME(restAt)))
		{
			restAt = _moduleData->get_parameter<float>(this, NAME(restAt), placementPt);
		}
		if (_moduleData->has_parameter<float>(this, NAME(lockedAt)))
		{
			lockedAt = _moduleData->get_parameter<float>(this, NAME(lockedAt), placementPt);
		}
		if (_moduleData->has_parameter<Name>(this, NAME(placementCurveAlongAxis)))
		{
			setup.placementCurveAlongAxis = Axis::parse_from(_moduleData->get_parameter<Name>(this, NAME(placementCurveAlongAxis), Name::invalid()).to_string());
		}
		setup.parentRadialCentreSocket = _moduleData->get_parameter<Name>(this, NAME(parentRadialCentreSocket), setup.parentRadialCentreSocket);
		if (_moduleData->has_parameter<Name>(this, NAME(radialToCentreAxis)))
		{
			setup.radialToCentreAxis = Axis::parse_from(_moduleData->get_parameter<Name>(this, NAME(radialToCentreAxis), Name::invalid()).to_string());
		}
		if (_moduleData->has_parameter<Name>(this, NAME(radialRotationAxis)))
		{
			setup.radialRotationAxis = Axis::parse_from(_moduleData->get_parameter<Name>(this, NAME(radialRotationAxis), Name::invalid()).to_string());
		}
		setup.radialRadius = _moduleData->get_parameter<float>(this, NAME(radialRadius), setup.radialRadius);
		setup.blendPlacementPtTime = _moduleData->get_parameter<float>(this, NAME(blendPlacementPtTime), setup.blendPlacementPtTime);
		setup.allowRestDistPt = _moduleData->get_parameter<float>(this, NAME(allowRestDistPt), setup.allowRestDistPt);
		setup.canStopAnywhere = _moduleData->get_parameter<bool>(this, NAME(canStopAnywhere), setup.canStopAnywhere);
		setup.activeDist = _moduleData->get_parameter<float>(this, NAME(activeDist), setup.activeDist);
		setup.inertia = _moduleData->get_parameter<float>(this, NAME(inertia), setup.inertia);

		Name placementSocketNames[] = { NAME(placementSocket0),
										NAME(placementSocket1),
										NAME(placementSocket2),
										NAME(placementSocket3) };
		Name placementRelativeNames[] = { NAME(placementRelative0),
										  NAME(placementRelative1),
										  NAME(placementRelative2),
										  NAME(placementRelative3) };
		Name placementParentSocketNames[] = { NAME(placementParentSocket0),
											  NAME(placementParentSocket1),
											  NAME(placementParentSocket2),
											  NAME(placementParentSocket3) };
		Name placementAllowRestNames[] = { NAME(placementAllowRest0),
										   NAME(placementAllowRest1),
										   NAME(placementAllowRest2),
										   NAME(placementAllowRest3) };
		Name placementSoundInNames[] = { NAME(placementSoundIn0),
										 NAME(placementSoundIn1),
										 NAME(placementSoundIn2),
										 NAME(placementSoundIn3) };
		Name placementSoundOutNames[] = { NAME(placementSoundOut0),
										  NAME(placementSoundOut1),
										  NAME(placementSoundOut2),
										  NAME(placementSoundOut3) };
		Name placementOutputNames[] = { NAME(placementOutput0),
										NAME(placementOutput1),
										NAME(placementOutput2),
										NAME(placementOutput3) };
		for_count(int, i, 4)
		{
			if (_moduleData->has_parameter<Transform>(this, placementRelativeNames[i]))
			{
				setup.placements.set_size(max(i + 1, setup.placements.get_size()));
				setup.placements[i].relativePlacement = _moduleData->get_parameter<Transform>(this, placementRelativeNames[i], Transform::identity);
			}
			if (_moduleData->has_parameter<Vector3>(this, placementRelativeNames[i]))
			{
				setup.placements.set_size(max(i + 1, setup.placements.get_size()));
				setup.placements[i].relativePlacement = Transform(_moduleData->get_parameter<Vector3>(this, placementRelativeNames[i], Vector3::zero), Quat::identity);
			}
			if (_moduleData->has_parameter<Name>(this, placementSocketNames[i]))
			{
				setup.placements.set_size(max(i + 1, setup.placements.get_size()));
				setup.placements[i].socket = _moduleData->get_parameter<Name>(this, placementSocketNames[i], setup.placements[i].socket);
			}
			if (_moduleData->has_parameter<Name>(this, placementParentSocketNames[i]))
			{
				setup.placements.set_size(max(i + 1, setup.placements.get_size()));
				setup.placements[i].parentSocket = _moduleData->get_parameter<Name>(this, placementParentSocketNames[i], setup.placements[i].parentSocket);
			}
			if (_moduleData->has_parameter<bool>(this, placementAllowRestNames[i]))
			{
				setup.placements.set_size(max(i + 1, setup.placements.get_size()));
				setup.placements[i].allowRest = _moduleData->get_parameter<bool>(this, placementAllowRestNames[i], setup.placements[i].allowRest);
			}
			if (_moduleData->has_parameter<Name>(this, placementSoundInNames[i]))
			{
				setup.placements.set_size(max(i + 1, setup.placements.get_size()));
				setup.placements[i].soundIn = _moduleData->get_parameter<Name>(this, placementSoundInNames[i], setup.placements[i].soundIn);
			}
			if (_moduleData->has_parameter<Name>(this, placementSoundOutNames[i]))
			{
				setup.placements.set_size(max(i + 1, setup.placements.get_size()));
				setup.placements[i].soundOut = _moduleData->get_parameter<Name>(this, placementSoundOutNames[i], setup.placements[i].soundOut);
			}
			if (_moduleData->has_parameter<float>(this, placementOutputNames[i]))
			{
				setup.placements.set_size(max(i + 1, setup.placements.get_size()));
				setup.placements[i].output = _moduleData->get_parameter<float>(this, placementOutputNames[i], setup.placements[i].output.get((float)i));
			}
		}
	}
}

void ModuleMovementSwitch::activate()
{
	base::activate();

	if (!baseObject.is_set())
	{
		placementType = InWorldSpace;
	}

	// this has to be done on activation when other objects are already initialised
	if (!setup.placements.is_empty())
	{
		Optional<PlacementType> newPlacementType;
		placements.clear();
		for_every(pSrc, setup.placements)
		{
			Placement p;
			if (pSrc->placement.is_set())
			{
				an_assert(!newPlacementType.is_set() || newPlacementType.get() == InWorldSpace, TXT("CAN'T MIX!"));
				newPlacementType = InWorldSpace;
				p.placement = pSrc->placement.get();
			}
			else if (pSrc->relativePlacement.is_set())
			{
				if (baseObject.is_set())
				{
					an_assert(!newPlacementType.is_set() || newPlacementType.get() == RelativeToAttachmentPlacement, TXT("CAN'T MIX!"));
					newPlacementType = RelativeToAttachmentPlacement;
					p.placement = pSrc->relativePlacement.get();
				}
				else
				{
					an_assert(!newPlacementType.is_set() || newPlacementType.get() == RelativeToInitialPlacementWS, TXT("CAN'T MIX!"));
					newPlacementType = RelativeToInitialPlacementWS;
					p.placement = pSrc->relativePlacement.get();
				}
			}
			else if (pSrc->parentSocket.is_valid() &&
				baseObject.is_set() &&
				baseObject->get_appearance() &&
				baseObject->get_appearance()->find_socket_index(pSrc->parentSocket) != NONE)
			{
				an_assert(!newPlacementType.is_set() || newPlacementType.get() == RelativeToBaseObjectUsingParentSocket, TXT("CAN'T MIX!"));
				newPlacementType = RelativeToBaseObjectUsingParentSocket;
				p.parentSocket = pSrc->parentSocket;
			}
			else if (pSrc->socket.is_valid() &&
				get_owner()->get_appearance() &&
				get_owner()->get_appearance()->find_socket_index(pSrc->socket) != NONE)
			{
				if (baseObject.is_set())
				{
					an_assert(!newPlacementType.is_set() || newPlacementType.get() == RelativeToBaseObjectUsingSocket, TXT("CAN'T MIX!"));
					newPlacementType = RelativeToBaseObjectUsingSocket;
				}
				else
				{
					an_assert(!newPlacementType.is_set() || newPlacementType.get() == UsingOwnSocketRelativeToInitialPlacementWS, TXT("CAN'T MIX!"));
					newPlacementType = UsingOwnSocketRelativeToInitialPlacementWS;
				}
				p.socket = pSrc->socket;
			}
			else
			{
				an_assert(false, TXT("no valid placement provided for switch"));
			}
			p.allowRest = pSrc->allowRest;
			p.soundIn = pSrc->soundIn;
			p.soundOut = pSrc->soundOut;
			p.output = pSrc->output;
			placements.push_back(p);
		}
		placementType = newPlacementType.get();
		if (placementType == RelativeToAttachmentPlacement &&
			relativeInitialPlacementAttachedToBoneIdx == NONE &&
			relativeInitialPlacementAttachedToSocketIdx == NONE)
		{
			placementType = RelativeToBaseObject;
		}
	}
	
	update_placements(true);

	lastRestPt = prevPlacementPt = placementPt;

	updateFrames = 5;
}

void ModuleMovementSwitch::update_placements(bool _forced)
{
	placementsUpdateRequired = true;
	placementsUpdateRequiredForced |= _forced;
}

void ModuleMovementSwitch::update_placements_if_required()
{
	if (!placementsUpdateRequired)
	{
		return;
	}

	if (!basePlacementValid)
	{
		return;
	}

	bool updatedSomething = placementsUpdateRequiredForced;

	auto * presence = get_owner()->get_presence();

	{
		for_every(p, placements)
		{
			if (placementType == RelativeToBaseObjectUsingSocket ||
				placementType == UsingOwnSocketRelativeToInitialPlacementWS)
			{
				an_assert(p->socket.is_valid());
				if (auto * a = get_owner()->get_appearance())
				{
					p->placement = a->calculate_socket_os(p->socket);
					updatedSomething = true;
				}
			}
			else if (placementType == RelativeToBaseObjectUsingParentSocket)
			{
				an_assert(p->parentSocket.is_valid());
				IModulesOwner* baseObject = nullptr;
				if (auto * a = presence->get_attached_to()) { baseObject = a; }
				if (auto * a = presence->get_based_on()) { baseObject = a; }
				if (baseObject)
				{
					if (auto * a = baseObject->get_appearance())
					{
						p->placement = a->calculate_socket_os(p->parentSocket);
						updatedSomething = true;
					}
				}
			}
		}
	}

	if (updatedSomething)
	{
		update_curves();
	}

	placementsUpdateRequired = false;
	placementsUpdateRequiredForced = false;
}

void ModuleMovementSwitch::update_curves()
{
	for_count(int, i, placements.get_size() - 1)
	{
		auto & p = placements[i];
		auto & n = placements[i + 1];
		p.curve.p0 = p.placement.get_translation();
		p.curve.p3 = n.placement.get_translation();
		p.curve.make_linear();
		if (setup.placementCurveAlongAxis != Axis::None)
		{
			{
				Vector3 pAxis = p.placement.get_axis(setup.placementCurveAlongAxis);
				float sign = Vector3::dot(pAxis, n.placement.get_translation() - p.placement.get_translation()) >= 0.0f ? 1.0f : -1.0f;
				p.curve.p1 = p.curve.p0 + pAxis * sign;
			}
			{
				Vector3 nAxis = n.placement.get_axis(setup.placementCurveAlongAxis);
				float sign = Vector3::dot(nAxis, p.placement.get_translation() - n.placement.get_translation()) >= 0.0f ? 1.0f : -1.0f;
				p.curve.p2 = p.curve.p3 + nAxis * sign;
			}
			p.curve.make_roundly_separated();
		}
	}
}

void ModuleMovementSwitch::apply_changes_to_velocity_and_displacement(float _deltaTime, VelocityAndDisplacementContext & _context)
{
	prevPlacementPt = placementPt;

	// use inertia, collision will be applied in base class
	_context.currentDisplacementLinear *= setup.inertia;
	_context.currentDisplacementRotation = Quat::slerp(clamp(setup.inertia, 0.0f, 1.0f),  Quat::identity, _context.currentDisplacementRotation);
	an_assert(_context.currentDisplacementRotation.is_normalised(0.1f));
	_context.velocityLinear *= setup.inertia;
	_context.velocityRotation *= setup.inertia;
	update_base_object();
	base::apply_changes_to_velocity_and_displacement(_deltaTime, _context);
	auto * presence = get_owner()->get_presence();

	bool forceMovement = false;
	if (auto * controller = get_owner()->get_controller())
	{
		if (controller->is_requested_placement_set())
		{
			// ignore collisions and other input
			Transform requestedPlacement = controller->get_requested_placement();
			Transform currentPlacement = presence->get_placement();
			_context.currentDisplacementLinear = requestedPlacement.get_translation() - currentPlacement.get_translation();
			_context.velocityLinear = _deltaTime != 0.0f ? _context.currentDisplacementLinear / _deltaTime : Vector3::zero;
			todo_note(TXT("handle twisting/rotating?"));
			_context.currentDisplacementRotation = Quat::identity;
			an_assert(_context.currentDisplacementRotation.is_normalised(0.1f));
			_context.velocityRotation = Rotator3::zero;
			// disallow locking and resting
			forceMovement = true;
		}
	}

#ifdef AN_DEBUG_RENDERER
	debug_filter(moduleMovementSwitch);
	if (debug_is_filter_allowed())
	{
		if (presence->is_attached())
		{
			debug_push_transform(presence->get_attached_to()->get_presence()->get_placement());
		}
		else if (presence->get_based_on())
		{
			debug_push_transform(presence->get_based_on()->get_presence()->get_placement());
		}
	}
#endif

	if (_context.currentDisplacementLinear.is_zero())
	{
		timeSinceNonZeroDisplacement += _deltaTime;
	}
	else
	{
		timeSinceNonZeroDisplacement = 0.0f;
	}

	if (goTo.is_set())
	{
		goToFor = max(0.0f, goToFor - _deltaTime);
		if (goToFor <= 0.0f)
		{
			goTo.clear();
		}
	}

	update_base_placement();
	update_placements_if_required();

	float moveDueToRestLock = 0.0f;
	if (forceMovement)
	{
		lockedAt.clear();
		restAt.clear();
	}
	else
	{
		Optional<float> restPt;
		{
			Optional<float> closestRestPt;
			if (setup.allowRestDistPt > 0.0f)
			{
				float closestRestDist = 0.0f;
				for_every(p, placements)
				{
					if (p->allowRest)
					{
						float dist = abs(placementPt - (float)(for_everys_index(p)));
						if (dist <= setup.allowRestDistPt &&
							(!closestRestPt.is_set() || closestRestDist > dist))
						{
							closestRestPt = (float)for_everys_index(p);
							closestRestDist = dist;
						}
					}
				}
				if (closestRestPt.is_set())
				{
					lastRestPt = closestRestPt.get();
				}
			}
			if (lockedAt.is_set())
			{
				restPt = lockedAt;
			}
			else if (goTo.is_set())
			{
				restPt = goTo.get();
			}
			else
			{
				if (restAt.is_set())
				{
					restPt = restAt;
				}
				else
				{
					restPt = closestRestPt;
				}
			}
			if (! setup.canStopAnywhere && !restPt.is_set())
			{
				restPt = lastRestPt;
			}
		}
		if (restPt.is_set())
		{
			float movePlacementPt = placementPt;
			float diff = abs(restPt.get() - placementPt);
			if (diff > 0.001f)
			{
				movePlacementPt = blend_to_using_time(placementPt, restPt.get(),
					max(max(0.0f, 5.0f * (0.1f - timeSinceNonZeroDisplacement)), setup.blendPlacementPtTime), // slow down if being pushed
					_deltaTime);
			}

			if (placementPt != prevPlacementPt)
			{
				updateFrames = 1;
			}
			moveDueToRestLock = movePlacementPt - placementPt;
		}
	}

	if (placementsUpdateRequired || !basePlacementValid)
	{
		basePlacementValid = false; // check below
		// stop any further update for now
		return;
	}

	float moveDueToForces = 0.0f;
	{	// get how displacement linear affects movement
		if (!_context.currentDisplacementLinear.is_zero() && !placements.is_empty() && !lockedAt.is_set())
		{
			float newPlacementPt = placementPt;

			Vector3 currentDisplacementLinear = _context.currentDisplacementLinear;
			if (basePlacement.is_set())
			{
				currentDisplacementLinear = basePlacement.get().vector_to_local(currentDisplacementLinear);
			}

			Vector3 requestedLoc = calculate_relative_placement().get_translation() + currentDisplacementLinear;

			{
				int bestI = NONE;
				float bestIt = 0.0f;
				float bestDist = 0.0f;
				for_count(int, i, placements.get_size() - 1)
				{
					auto const & p = placements[i];
					float dist;
					float t = p.curve.find_t_at(requestedLoc, &dist);
					if (bestI == NONE || bestDist > dist)
					{
						bestI = i;
						bestIt = t;
						bestDist = dist;
					}
				}

				if (bestI == NONE)
				{
					bestI = 0;
				}

				newPlacementPt = (float)bestI + bestIt;
			}
			moveDueToForces = newPlacementPt - placementPt;
			if (forceMovement)
			{
				lastMoveDueToForces = 0.0f;
				lastMoveDueToForcesTimeLeft = 0.0f;
			}
			else
			{
				if (lastMoveDueToForces * moveDueToForces < 0.0f)
				{
#ifdef AN_DEBUG_RENDERER
					if (debug_is_filter_allowed())
					{
						Vector3 at = calculate_relative_placement().get_translation();
						debug_draw_text(true, Colour::red, at + Vector3(0.0f, 0.0f, 0.1f), NP, NP, 0.1f, NP, TXT("op dir m:%.3f v l:%.3f"), moveDueToForces, lastMoveDueToForces);
					}
#endif
					moveDueToForces = 0.0f;
				}

				if (moveDueToForces * moveDueToRestLock < 0.0f)
				{
					moveDueToRestLock = 0.0f;
				}
			}
		}
	}

#ifdef AN_DEBUG_RENDERER
	if (debug_is_filter_allowed())
	{
		for_count(int, i, placements.get_size() - 1)
		{
			auto & p = placements[i];
			Vector3 prev = Vector3::zero;
			for (float t = 0.0f; t <= 1.0f; t += 0.1f)
			{
				Vector3 curr = p.curve.calculate_at(t);
				if (t > 0.0f)
				{
					debug_draw_line(true, Colour::red, prev, curr);
				}
				prev = curr;
			}
		}
		{
			Vector3 at = calculate_relative_placement().get_translation();
			debug_draw_arrow(true, Colour::yellow, at, at + _context.currentDisplacementLinear * 10.0f);
			if (lastChangePt != 0.0f)
			{
				Vector3 atPlus = calculate_relative_placement(lastChangePt).get_translation();
				Vector3 atMinus = calculate_relative_placement(-lastChangePt).get_translation();
				Vector3 dir = (atPlus - atMinus).normal();
				debug_draw_arrow(true, Colour::blue, at, at + dir * 0.1f);
			}
			debug_draw_text(true, Colour::yellow, at, NP, NP, 0.1f, NP, TXT("f:%.3f(l:%.3f) rl:%.3f"), moveDueToForces, lastMoveDueToForces, moveDueToRestLock);
		}
	}
#endif

	placementPt += moveDueToForces + moveDueToRestLock;
	placementPt = clamp(placementPt, 0.0f, (float)(placements.get_size()) - 1);

	if (_context.currentDisplacementLinear.is_zero())
	{
		lastMoveDueToForcesTimeLeft = max(0.0f, lastMoveDueToForcesTimeLeft - _deltaTime);
		if (lastMoveDueToForcesTimeLeft <= 0.0f)
		{
			lastMoveDueToForces = 0.0f;
		}
	}
	else if (moveDueToForces != 0.0f)
	{
		lastMoveDueToForcesTimeLeft = 0.1f;
		if (!restAt.is_set() ||
			(moveDueToForces * (restAt.get() - placementPt) < 0.0f)) // we push in opposite direction to rest
		{
			an_assert(_deltaTime != 0.0f);
			if (abs(moveDueToForces / _deltaTime) > 0.5f)
			{
				lastMoveDueToForces = moveDueToForces;
			}
		}
	}

	if (placementPt != prevPlacementPt || updateFrames)
	{
		updateFrames = max(0, updateFrames - 1);

		Transform shouldBeAt = calculate_relative_placement();

		an_assert(abs(shouldBeAt.get_orientation().length() - 1.0f) < 0.1f);

		if (presence->is_attached())
		{
			presence->update_attachment_relative_placement(relativeInitialPlacement.to_world(shouldBeAt));
		}

		if (basePlacement.is_set())
		{
			shouldBeAt = basePlacement.get().to_world(shouldBeAt);
		}

		an_assert(abs(shouldBeAt.get_orientation().length() - 1.0f) < 0.1f);

		Transform const isAt = get_owner()->get_presence()->get_placement();
		_context.currentDisplacementLinear = shouldBeAt.get_translation() - isAt.get_translation();
		_context.currentDisplacementRotation = isAt.get_orientation().to_local(shouldBeAt.get_orientation());
		an_assert(abs(isAt.get_orientation().length() - 1.0f) < 0.1f);
		an_assert(_context.currentDisplacementRotation.is_normalised(0.1f));
	}
	else
	{
		_context.currentDisplacementLinear = Vector3::zero;
		_context.currentDisplacementRotation = Quat::identity;
	}

	if (_deltaTime != 0.0f)
	{
		// note that due to implementation (that I am not willing to change ATM, attachments have their velocities updated automatically basing on difference in placement
		_context.velocityLinear = _context.currentDisplacementLinear / _deltaTime;
		_context.velocityRotation = _context.currentDisplacementRotation.to_rotator() / _deltaTime;
	}
	else
	{
		_context.velocityLinear = Vector3::zero;
		_context.velocityRotation = Rotator3::zero;
	}

	// sounds
	if (!placements.is_empty() &&
		placementPt != prevPlacementPt)
	{
		if (auto * s = get_owner()->get_sound())
		{
			{
				int atIdx = get_closest();
				if (placements[atIdx].soundIn.is_valid() &&
					has_been_actived_at(atIdx))
				{
					s->play_sound(placements[atIdx].soundIn);
				}
			}
			{
				int prevAtIdx = get_closest(prevPlacementPt);
				if (placements[prevAtIdx].soundOut.is_valid() &&
					has_been_deactived_at(prevAtIdx))
				{
					s->play_sound(placements[prevAtIdx].soundOut);
				}
			}
		}
	}

#ifdef AN_DEBUG_RENDERER
	if (debug_is_filter_allowed())
	{
		if (presence->is_attached())
		{
			debug_pop_transform();
		}
		else if (presence->get_based_on())
		{
			debug_pop_transform();
		}
	}
	debug_no_filter();
#endif

	basePlacementValid = false; // invalidate, we don't know what will happen next
}

void ModuleMovementSwitch::break_pt(float _pt, OUT_ int & _i, OUT_ float & _t) const
{
	_t = mod(_pt, 1.0f);
	_i = (int)(_pt - _t + 0.5f); // round to make sure
}

Transform ModuleMovementSwitch::calculate_relative_placement(float _addPt) const
{
	an_assert(! placementsUpdateRequired);
	if (placements.get_size() == 0)
	{
		return Transform::identity;
	}

	float t;
	int i;

	float atPt = placementPt + _addPt;

	break_pt(atPt, OUT_ i, OUT_ t);
	if (i < 0)
	{
		i = 0;
		t = 0.0f;
	}
	if (i >= placements.get_size() - 1)
	{
		i = placements.get_size() - 2;
		t = 1.0f;
	}
	Vector3 shouldBeAtLoc = placements[i].curve.calculate_at(t);
	Quat shouldBeAtRot = Quat::slerp(t, placements[i].placement.get_orientation(), placements[i + 1].placement.get_orientation());
	Transform shouldBeAt(shouldBeAtLoc, shouldBeAtRot);
	if (setup.radialToCentreAxis != Axis::None &&
		setup.radialRotationAxis != Axis::None &&
		setup.parentRadialCentreSocket.is_valid())
	{
		an_assert(placementType == RelativeToBaseObjectUsingParentSocket);
		if (baseObject.is_set())
		{
			if (auto * a = baseObject->get_appearance())
			{
				Vector3 loc = shouldBeAt.get_translation();
				Vector3 const centreLoc = a->calculate_socket_os(setup.parentRadialCentreSocket).get_translation();
				Vector3 const towardsCentre = (centreLoc - shouldBeAt.get_translation()).normal();
				Vector3 axes[4] = { Vector3::zero,
									shouldBeAt.get_axis(Axis::X),
									shouldBeAt.get_axis(Axis::Y),
									shouldBeAt.get_axis(Axis::Z) };
				an_assert(Axis::X == 1);
				an_assert(Axis::Y == 2);
				an_assert(Axis::Z == 3);
				axes[setup.radialToCentreAxis] = towardsCentre * sign(Vector3::dot(towardsCentre, axes[setup.radialToCentreAxis]));
				if (setup.radialRadius != 0.0f)
				{
					loc = centreLoc - towardsCentre * setup.radialRadius;
				}
				shouldBeAt.build_from_axes(setup.radialToCentreAxis, setup.radialRotationAxis, axes[Axis::Forward], axes[Axis::Right], axes[Axis::Up], loc);
			}
		}
	}
	return shouldBeAt;
}

void ModuleMovementSwitch::go_to(int _idx, float _tryFor)
{
	goTo = (float)_idx;
	goToFor = _tryFor;
	lockedAt.clear();
}

void ModuleMovementSwitch::rest_at(int _idx)
{
	restAt = (float)_idx;
	lockedAt.clear();
}

void ModuleMovementSwitch::lock_at(int _idx)
{
	restAt.clear();
	lockedAt = (float)_idx;
}

void ModuleMovementSwitch::resume()
{
	restAt.clear();
	lockedAt.clear();
}

int ModuleMovementSwitch::get_closest(Optional<float> const & _pt) const
{
	if (placements.is_empty())
	{
		return 0;
	}
	return clamp((int)round(_pt.get(placementPt)), 0, placements.get_size() - 1);
}

float ModuleMovementSwitch::get_output() const
{
	if (placements.is_empty())
	{
		return 0.0f;
	}
	int i = (int)placementPt;
	int n = i + 1;
	float t = clamp(placementPt - (float)i, 0.0f, 1.0f);

	i = clamp(i, 0, placements.get_size() - 1);
	n = clamp(n, 0, placements.get_size() - 1);

	auto const & pi = placements[i];
	auto const & pn = placements[n];

	return pi.output.get((float)i) * (1.0f - t) + t * pn.output.get((float)n);
}

bool ModuleMovementSwitch::is_active_at(int _idx) const
{
	float pt = (float)_idx;
	return abs(placementPt - pt) <= setup.activeDist + 0.000001f;
}

bool ModuleMovementSwitch::has_been_actived_at(int _idx) const
{
	float pt = (float)_idx;
	return abs(placementPt - pt) <= max(setup.activeDist, 0.001f) &&
		   abs(prevPlacementPt - pt) > max(setup.activeDist, 0.001f);
}

bool ModuleMovementSwitch::has_been_deactived_at(int _idx) const
{
	float pt = (float)_idx;
	return abs(placementPt - pt) > max(setup.activeDist, 0.001f) &&
		   abs(prevPlacementPt - pt) <= max(setup.activeDist, 0.001f);
}

//

REGISTER_FOR_FAST_CAST(ModuleMovementSwitchData);

ModuleMovementSwitchData::ModuleMovementSwitchData(LibraryStored* _inLibraryStored)
: base(_inLibraryStored)
{
}

bool ModuleMovementSwitchData::read_parameter_from(IO::XML::Attribute const * _attr, LibraryLoadingContext & _lc)
{
	if (_attr->get_name() == TXT("placementCurveAlongAxis"))
	{
		setup.placementCurveAlongAxis = Axis::parse_from(_attr->get_as_string(), setup.placementCurveAlongAxis);
		return true;
	}
	else if (_attr->get_name() == TXT("parentRadialCentreSocket"))
	{
		setup.parentRadialCentreSocket = _attr->get_as_name();
		return true;
	}
	else if (_attr->get_name() == TXT("radialToCentreAxis"))
	{
		setup.radialToCentreAxis = Axis::parse_from(_attr->get_as_string(), setup.radialToCentreAxis);
		return true;
	}
	else if (_attr->get_name() == TXT("radialRotationAxis"))
	{
		setup.radialRotationAxis = Axis::parse_from(_attr->get_as_string(), setup.radialRotationAxis);
		return true;
	}	
	else
	{
		return base::read_parameter_from(_attr, _lc);
	}
}

bool ModuleMovementSwitchData::read_parameter_from(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	if (_node->get_name() == TXT("placementCurveAlongAxis"))
	{
		setup.placementCurveAlongAxis = Axis::parse_from(_node->get_text(), setup.placementCurveAlongAxis);
		return true;
	}
	else if (_node->get_name() == TXT("parentRadialCentreSocket"))
	{
		setup.parentRadialCentreSocket = Name(_node->get_text());
		return true;
	}
	else if (_node->get_name() == TXT("radialToCentreAxis"))
	{
		setup.radialToCentreAxis = Axis::parse_from(_node->get_text(), setup.radialToCentreAxis);
		return true;
	}
	else if (_node->get_name() == TXT("radialRotationAxis"))
	{
		setup.radialRotationAxis = Axis::parse_from(_node->get_text(), setup.radialRotationAxis);
		return true;
	}
	else if (_node->get_name() == TXT("placement"))
	{
		ModuleMovementSwitchSetup::Placement p;
		if (p.load_from_xml(_node))
		{
			setup.placements.push_back(p);
			return true;
		}
		else
		{
			error_loading_xml(_node, TXT("couldn't load placement"));
			return false;
		}
	}
	else
	{
		return base::read_parameter_from(_node, _lc);
	}
}

#include "mc_lightningSpawner.h"

#include "mc_lightSources.h"

#include "..\modules.h"
#include "..\moduleDataImpl.inl"

#include "..\..\appearance\controllers\ac_particlesUtils.h"
#include "..\..\collision\checkCollisionContext.h"
#include "..\..\collision\checkSegmentResult.h"
#include "..\..\game\gameUtils.h"
#include "..\..\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\object\temporaryObject.h"
#include "..\..\sound\soundSample.h"
#include "..\..\world\room.h"

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

using namespace Framework;
using namespace CustomModules;

//

bool CustomModules::LightningData::load_from_xml(IO::XML::Node const * _node)
{
	bool result = true;

	id = _node->get_name_attribute_or_from_child(TXT("id"), id);
	error_loading_xml_on_assert(id.is_valid(), result, _node, TXT("no id"));

	count = _node->get_int_attribute(TXT("count"), count);

	if (auto* attr = _node->get_attribute(TXT("type")))
	{
		type = Lightning::parse(attr->get_as_string());
	}

	arcVarID = _node->get_name_attribute_or_from_child(TXT("arcVarID"), arcVarID);
	lengthVarID = _node->get_name_attribute_or_from_child(TXT("lengthVarID"), lengthVarID);
	radiusVarID = _node->get_name_attribute_or_from_child(TXT("radiusVarID"), radiusVarID);

	if (auto* attr = _node->get_attribute(TXT("temporaryObjectID")))
	{
		temporaryObjects.push_back(attr->get_as_name());
	}
	
	if (auto* attr = _node->get_attribute(TXT("lightSourceID")))
	{
		lightSources.push_back(attr->get_as_name());
	}
	
	if (auto* attr = _node->get_attribute(TXT("soundID")))
	{
		sounds.push_back(attr->get_as_name());
	}

	for_every(node, _node->children_named(TXT("temporaryObject")))
	{
		Name id = node->get_name_attribute_or_from_child(TXT("id"), Name::invalid());
		if (id.is_valid())
		{
			temporaryObjects.push_back(id);
		}
		else
		{
			error_loading_xml(node, TXT("no id"));
			result = false;
		}
	}

	defaultOn = _node->get_bool_attribute_or_from_child_presence(TXT("defaultOn"), defaultOn);
	autoStop = _node->get_bool_attribute_or_from_child_presence(TXT("autoStop"), autoStop);
	autoStop = ! _node->get_bool_attribute_or_from_child_presence(TXT("dontStop"), ! autoStop);
	startImmediately = _node->get_bool_attribute_or_from_child_presence(TXT("startImmediately"), startImmediately);
	allowThroughDoors = _node->get_bool_attribute_or_from_child_presence(TXT("allowThroughDoors"), allowThroughDoors);
	allowThroughDoors = ! _node->get_bool_attribute_or_from_child_presence(TXT("disallowThroughDoors"), ! allowThroughDoors);
	allowMiss = _node->get_bool_attribute_or_from_child_presence(TXT("allowMiss"), allowMiss);
	allowMiss = ! _node->get_bool_attribute_or_from_child_presence(TXT("disallowMiss"), !allowMiss);
	result &= interval.load_from_xml(_node, TXT("interval"));
	result &= timeToLive.load_from_xml(_node, TXT("timeToLive"));
	result &= timeToLiveMiss.load_from_xml(_node, TXT("timeToLiveMiss"));	

	// strike
	result &= length.load_from_xml(_node, TXT("length"));
	result &= arc.load_from_xml(_node, TXT("arc"));
	result &= arcSpeed.load_from_xml(_node, TXT("arcSpeed"));
	result &= hitFlags.load_from_xml(_node, TXT("hitCollisionFlags"));
	dirAngle.load_from_xml(_node, TXT("dirAngle"));
	continuousPlacementUpdate = _node->get_bool_attribute_or_from_child_presence(TXT("continuousPlacementUpdate"), continuousPlacementUpdate);
	continuousPlacementUpdate = ! _node->get_bool_attribute_or_from_child_presence(TXT("noContinuousPlacementUpdate"), ! continuousPlacementUpdate);
	impulseOnHit = _node->get_float_attribute_or_from_child(TXT("impulseOnHit"), impulseOnHit);
	impulesOnHitObjectsOnly = _node->get_bool_attribute_or_from_child_presence(TXT("impulesOnHitObjectsOnly"), impulesOnHitObjectsOnly);
	impulseNominalDistance = _node->get_float_attribute_or_from_child(TXT("impulseNominalDistance"), impulseNominalDistance);
	useTarget = _node->get_bool_attribute_or_from_child_presence(TXT("useTarget"), useTarget);
	useTargetRadius = _node->get_float_attribute_or_from_child(TXT("useTargetRadius"), useTargetRadius);
	extraHitPenetration.load_from_xml(_node, TXT("extraHitPenetration"));
	extraHitRadius = _node->get_float_attribute_or_from_child(TXT("extraHitRadius"), extraHitRadius);

	// presence
	result &= radiusRelativeToPresence.load_from_xml(_node, TXT("radiusRelativeToPresence"));

	vrPulse.load_from_xml(_node, TXT("vrPulse"));

	// common
	exclusiveSockets = _node->get_bool_attribute_or_from_child_presence(TXT("exclusiveSockets"), exclusiveSockets);
	exclusiveSocketPairs = _node->get_bool_attribute_or_from_child_presence(TXT("exclusiveSocketPairs"), exclusiveSocketPairs);
	crawlSocketPairs = _node->get_bool_attribute_or_from_child_presence(TXT("crawlSocketPairs"), crawlSocketPairs);
	if (auto* attr = _node->get_attribute(TXT("atSocket")))
	{
		AtSocket as;
		as.socket = attr->get_as_name();
		if (auto* attrUp = _node->get_attribute(TXT("socketUpAxis")))
		{
			as.upAxis = Axis::parse_from(attr->get_as_string(), as.upAxis.get(Axis::Z));
		}
		as.upAxisDir = _node->get_float_attribute(TXT("socketUpAxisDir"), as.upAxisDir);
		atSocket.push_back(as);
	}
	for_every(node, _node->children_named(TXT("at")))
	{
		if (auto* attr = node->get_attribute(TXT("socket")))
		{
			AtSocket as;
			as.socket = attr->get_as_name();
			if (auto* attrUp = node->get_attribute(TXT("upAxis")))
			{
				as.upAxis = Axis::parse_from(attr->get_as_string(), as.upAxis.get(Axis::Z));
			}
			as.upAxisDir = node->get_float_attribute(TXT("upAxisDir"), as.upAxisDir);
			if (auto* attr = node->get_attribute(TXT("vrPulseHand")))
			{
				as.vrPulseHand = Hand::parse(attr->get_as_string());
			}
			atSocket.push_back(as);
		}
	}
	for_every(node, _node->children_named(TXT("sockets")))
	{
		auto* attrA = node->get_attribute(TXT("a"));
		auto* attrB = node->get_attribute(TXT("b"));
		if (attrA && attrB)
		{
			SocketPair socketPair;
			socketPair.a = attrA->get_as_name();
			socketPair.b = attrB->get_as_name();
			socketPair.altB = node->get_name_attribute(TXT("altB"));
			socketPairs.push_back(socketPair);
		}
	}
	placement.load_from_xml_child_node(_node, TXT("placement"));

	return result;
};

//

REGISTER_FOR_FAST_CAST(CustomModules::LightningSpawner);

static Framework::ModuleCustom* create_module(Framework::IModulesOwner* _owner)
{
	return new CustomModules::LightningSpawner(_owner);
}

static Framework::ModuleData* create_module_data(::Framework::LibraryStored* _inLibraryStored)
{
	return new LightningSpawnerData(_inLibraryStored);
}

Framework::RegisteredModule<Framework::ModuleCustom> & CustomModules::LightningSpawner::register_itself()
{
	return Framework::Modules::custom.register_module(String(TXT("lightningSpawner")), create_module, create_module_data);
}

CustomModules::LightningSpawner::LightningSpawner(Framework::IModulesOwner* _owner)
: base(_owner)
{
}

CustomModules::LightningSpawner::~LightningSpawner()
{
}

void CustomModules::LightningSpawner::reset()
{
	base::reset();

	lightnings.clear();
	requests.requests.clear();
}

void CustomModules::LightningSpawner::initialise()
{
	base::initialise();
}

void CustomModules::LightningSpawner::activate()
{
	base::activate();

	rg = get_owner()->get_individual_random_generator();

	lightnings.clear();
	requests.requests.clear();
	defaultsStarted = false;
	if (get_owner()->get_presence()->get_in_room())
	{
		start_defaults();
	}
}

void CustomModules::LightningSpawner::start_defaults()
{
	if (defaultsStarted)
	{
		return;
	}
	for_every(ld, lightningSpawnerData->lightnings)
	{
		if (ld->defaultOn)
		{
			start(ld->id);
		}
	}
	defaultsStarted = true;
}

void CustomModules::LightningSpawner::on_owner_destroy()
{
	for_every(l, lightnings)
	{
		if (auto* to = fast_cast<TemporaryObject>(l->to.get()))
		{
			ParticlesUtils::desire_to_deactivate(to);
			to->mark_to_deactivate(false);
		}
		clean_up_lightning(*l);
	}
	lightnings.clear();
}

void CustomModules::LightningSpawner::setup_with(Framework::ModuleData const * _moduleData)
{
	base::setup_with(_moduleData);
	lightningSpawnerData = fast_cast<LightningSpawnerData>(_moduleData);
	collectSocketsInUse = false;
	collectSocketPairsInUse = false;
	if (lightningSpawnerData)
	{
		for_every(l, lightningSpawnerData->lightnings)
		{
			collectSocketsInUse |= l->exclusiveSockets;
			collectSocketPairsInUse |= l->exclusiveSocketPairs;
		}
	}
}

void CustomModules::LightningSpawner::introduce_lightning(Lightning & l)
{
	if (l.introduced)
	{
		return;
	}

	l.introduced = true;

	if (collectSocketsInUse)
	{
		if (l.socketAIdx != NONE)
		{
			socketsInUse.push_back(l.socketAIdx);
		}
		if (l.socketBIdx != NONE)
		{
			socketsInUse.push_back(l.socketBIdx);
		}
	}

	if (collectSocketPairsInUse)
	{
		if (l.socketAIdx != NONE && l.socketBIdx != NONE)
		{
			socketPairsInUse.push_back(ExSocketPair(l.socketAIdx, l.socketBIdx));
		}
	}
}

void CustomModules::LightningSpawner::clean_up_lightning(Lightning & l)
{
	if (! l.introduced)
	{
		return;
	}

	l.introduced = false;

	if (l.sound.is_set() &&
		l.sound->get_sample_instance().is_set() &&
		l.sound->get_sample_instance().get_sound_sample() &&
		l.sound->get_sample_instance().get_sound_sample()->is_looped())
	{
		l.sound->stop();
		l.sound.clear();
	}

	l.hitPlacement.reset();
	l.target.reset();

	if (collectSocketsInUse)
	{
		if (l.socketAIdx != NONE)
		{
			socketsInUse.remove_fast(l.socketAIdx);
		}
		if (l.socketBIdx != NONE)
		{
			socketsInUse.remove_fast(l.socketBIdx);
		}
	}

	if (collectSocketPairsInUse)
	{
		socketPairsInUse.remove_fast(ExSocketPair(l.socketAIdx, l.socketBIdx));
	}
}

void CustomModules::LightningSpawner::calculate_socket_arc_placement(Lightning const & l, OUT_ float & length, OUT_ Transform & placement) const
{
	Transform socketAPlacementOS = get_owner()->get_appearance()->calculate_socket_os(l.socketAIdx);
	Transform socketBPlacementOS = get_owner()->get_appearance()->calculate_socket_os(l.socketBIdx);
	Transform socketBPlacement_sAS = socketAPlacementOS.to_local(socketBPlacementOS);
	Vector3 socketAtoB_sAS = socketBPlacement_sAS.get_translation();
	length = socketAtoB_sAS.length();
	// we assume sockets use forward direction as "out"
	placement = get_owner()->get_presence()->get_placement().to_world(look_at_matrix(socketAPlacementOS.get_translation(), socketBPlacementOS.get_translation(), (socketAPlacementOS.get_axis(Axis::Y) + socketBPlacementOS.get_axis(Axis::Y)).normal()).to_transform());
	an_assert_immediate(placement.is_ok());
}

void CustomModules::LightningSpawner::calculate_socket_arc_relative_placement(Lightning const & l, OUT_ float & length, OUT_ Transform & relativePlacement) const
{
	Transform socketAPlacementOS = get_owner()->get_appearance()->calculate_socket_os(l.socketAIdx);
	Transform socketBPlacementOS = get_owner()->get_appearance()->calculate_socket_os(l.socketBIdx);
	Transform socketBPlacement_sAS = socketAPlacementOS.to_local(socketBPlacementOS);
	Vector3 socketAtoB_sAS = socketBPlacement_sAS.get_translation();
	length = socketAtoB_sAS.length();
	// we assume sockets use forward direction as "out"
	// we assume both have normalised axes
	an_assert(socketBPlacement_sAS.get_axis(Axis::Y).is_normalised());
	relativePlacement = look_at_matrix(Vector3::zero, socketAtoB_sAS, (Vector3::yAxis + socketBPlacement_sAS.get_axis(Axis::Y)).normal()).to_transform();
}

void CustomModules::LightningSpawner::on_placed_in_world()
{
	base::on_placed_in_world();

	if (!defaultsStarted)
	{
		start_defaults();
	}
}

void CustomModules::LightningSpawner::advance_post(float _deltaTime)
{
	base::advance_post(_deltaTime);

	if (! get_owner()->get_presence()->get_in_room())
	{
		return;
	}

	rarer_post_advance_if_not_visible();

	if (!defaultsStarted)
	{
		start_defaults();
	}

	{
		Concurrency::ScopedSpinLock lock(requests.lock);
		for_every(request, requests.requests)
		{
			if (request->op == Requests::Request::Op::Add) internal_add(request->id, request->params);
			if (request->op == Requests::Request::Op::Stop) internal_stop(request->id);
			if (request->op == Requests::Request::Op::Single) internal_single(request->id, request->params);
		}
		requests.requests.clear();
	}

	{
		inactive = false;

		ARRAY_STACK(Name, requiredLightSources, lightnings.get_size());
			
		for (int i = 0; i < lightnings.get_size(); ++i)
		{
			auto & l = lightnings[i];
			if (! l.single || l.data->crawlSocketPairs) // crawlSocketPairs with single means a single pass
			{
				if (!l.data->interval.is_empty())
				{
					l.timeToSpawn -= _deltaTime;
					if (l.timeToSpawn < 0.0f)
					{
						l.spawnPending = true;
						l.timeToSpawn = rg.get(l.data->interval);
					}
				}
				else
				{
					l.timeToSpawn = 0.0f;
					l.spawnPending = ! l.to.is_set(); // spawn only if nothing exists
				}
			}
			if (!l.to.is_set())
			{
				l.to.clear();
			}
			if (l.to.is_set() &&
				l.timeToLive.is_set())
			{
				l.timeToLive = l.timeToLive.get() - _deltaTime;
				if (l.timeToLive.get() <= 0.0f)
				{
					l.timeToLive.clear();
					l.shouldKill = true;
				}
			}
			if ((l.shouldStop || l.spawnPending) && l.data->autoStop)
			{
				l.shouldKill = true;
			}
			if (l.shouldKill)
			{
				if (l.to.is_set())
				{
					clean_up_lightning(l);
					ParticlesUtils::desire_to_deactivate(l.to.get());
					l.to.clear();
				}
				l.shouldKill = false;
			}
			if (l.shouldStop)
			{
				clean_up_lightning(l);
				lightnings.remove_fast_at(i);
				--i;
				continue;
			}
			if (l.spawnPending)
			{
				l.to.clear();
				l.vrPulseHand.clear();
				if (auto* to = get_owner()->get_temporary_objects())
				{
					auto * ownerPresence = get_owner()->get_presence();
					Transform basePlacement = ownerPresence->get_placement();
					Transform placement = basePlacement.to_world(l.startPlacementOS.get(Transform::identity));
					an_assert_immediate(placement.is_ok());
					if (l.startPlacementWS.is_set())
					{
						placement = l.startPlacementWS.get();
						an_assert_immediate(placement.is_ok());
					}
					Name atSocketA;
					Name atSocketB;
					bool spawn = true;
					if (!l.data->timeToLive.is_empty())
					{
						l.timeToLive = rg.get(l.data->timeToLive);
					}
					l.arc = rg.get(l.data->arc);
					l.arcSpeed = rg.get(l.data->arcSpeed);
					if (l.data->type == CustomModules::Lightning::SocketArc &&
						! l.data->socketPairs.is_empty())
					{
						spawn = false;
						int triesLeft = 100;
						while (triesLeft > 0)
						{
							--triesLeft;
							int chosenSocketPair;
							if (l.data->crawlSocketPairs)
							{
								++l.crawlSocketPairIdx;
								if (l.data->socketPairs.is_index_valid(l.crawlSocketPairIdx))
								{
									chosenSocketPair = l.crawlSocketPairIdx;
								}
								else
								{
									// we reached the end
									break;
								}
							}
							else
							{
								chosenSocketPair = rg.get_int(l.data->socketPairs.get_size());
							}
							auto const & chosen = l.data->socketPairs[chosenSocketPair];
							atSocketA = chosen.a;
							atSocketB = chosen.b;
							l.socketAIdx = get_owner()->get_appearance()->find_socket_index(atSocketA);
							l.socketBIdx = get_owner()->get_appearance()->find_socket_index(atSocketB);
							if (l.socketBIdx == NONE && chosen.altB.is_valid())
							{
								atSocketB = chosen.altB;
								l.socketBIdx = get_owner()->get_appearance()->find_socket_index(atSocketB);
							}
							if (l.socketAIdx != NONE &&
								l.socketBIdx != NONE)
							{
								if ((l.data->exclusiveSockets &&
									 ((socketsInUse.does_contain(l.socketAIdx)) ||
									  (socketsInUse.does_contain(l.socketBIdx)))) ||
									(l.data->exclusiveSocketPairs &&
									 (socketPairsInUse.does_contain(ExSocketPair(l.socketAIdx, l.socketBIdx)))))
								{
									l.socketAIdx = NONE;
									l.socketBIdx = NONE;
									continue;
								}
								l.socketAUpAxis.clear();
								spawn = true;
								break;
							}
						}
						if (spawn)
						{
							basePlacement = basePlacement.to_world(get_owner()->get_appearance()->calculate_socket_os(l.socketAIdx));
							placement = basePlacement;
							an_assert_immediate(placement.is_ok());
						}
					}
					else if (! l.data->atSocket.is_empty())
					{
						spawn = false;
						int triesLeft = 100;
						while (triesLeft > 0)
						{
							--triesLeft;
							int chosenSocket = rg.get_int(l.data->atSocket.get_size());
							auto const & chosen = l.data->atSocket[chosenSocket];
							atSocketA = chosen.socket;
							l.socketAIdx = get_owner()->get_appearance()->find_socket_index(atSocketA);
							if (l.socketAIdx != NONE)
							{
								if (l.data->exclusiveSockets &&
									(socketsInUse.does_contain(l.socketAIdx)))
								{
									l.socketAIdx = NONE;
									l.socketBIdx = NONE;
									continue;
								}
								spawn = true;
								l.socketAUpAxis = chosen.upAxis;
								l.socketAUpAxisDir = chosen.upAxisDir;
								l.vrPulseHand = chosen.vrPulseHand;
								break;
							}
						}
						basePlacement = basePlacement.to_world(get_owner()->get_appearance()->calculate_socket_os(l.socketAIdx));
						if (l.socketAUpAxis.is_set())
						{
							if (l.socketAUpAxis.get() != Axis::Y)
							{
								basePlacement = basePlacement.to_world(look_matrix(Vector3::zero, Vector3::yAxis, Vector3::axis(l.socketAUpAxis.get()) * l.socketAUpAxisDir).to_transform());
							}
							else
							{
								basePlacement = basePlacement.to_world(look_matrix(Vector3::zero, Vector3::xAxis, Vector3::axis(l.socketAUpAxis.get()) * l.socketAUpAxisDir).to_transform());
							}
						}
						placement = basePlacement;
						an_assert_immediate(placement.is_ok());
					}
					else
					{
						l.socketAIdx = NONE;
						l.socketBIdx = NONE;
					}
					if (spawn)
					{
						// find placement
						float length = rg.get(l.data->length);
						{
							if (l.data->useTarget && l.target.is_active())
							{
								// we use target to turn in proper direction, then we will use standard hit to check whether we hit or not
								Transform targetPlacementInOwnerRoom = l.target.get_placement_in_owner_room();
								Vector3 targetLocationInOwnerRoom = targetPlacementInOwnerRoom.get_translation();
								auto* actualTarget = l.target.get_target();
								auto* ai = get_owner()->get_ai();
								if (actualTarget && ai && !l.isDirectTarget)
								{
									targetLocationInOwnerRoom = targetPlacementInOwnerRoom.location_to_world(ai->get_target_placement_os_for(actualTarget).get_translation());
								}
								if (l.data->useTargetRadius > 0.0f)
								{
									Vector3 r = Vector3(rg.get_float(-1.0f, 1.0f), rg.get_float(-1.0f, 1.0f), rg.get_float(-1.0f, 1.0f)).normal();
									targetLocationInOwnerRoom += r * rg.get_float(0.0f, l.data->useTargetRadius);
								}
								placement = look_at_matrix(placement.get_translation(), targetLocationInOwnerRoom, placement.get_axis(Axis::Z)).to_transform();
								an_assert_immediate(placement.is_ok());
								placement.normalise_orientation();
								an_assert_immediate(placement.is_ok());
							}
							if (! l.data->dirAngle.is_empty())
							{
								placement = offset_transform_by_forward_angle(placement, REF_ rg, l.data->dirAngle);
								an_assert_immediate(placement.is_ok());
							}
							if (l.data->type == CustomModules::Lightning::Strike)
							{
								if (l.isForced)
								{
									if (l.forcedLength.is_set())
									{
										length = l.forcedLength.get();
										Vector3 startLocation = placement.get_translation();
										Vector3 endLocation = placement.location_to_world(Vector3::yAxis * (length * ownerPresence->get_placement().get_scale()));
										l.hitPlacement.clear_target(); // will reset final room to owner's room
										l.hitPlacement.set_placement_in_final_room(Transform(endLocation, Quat::identity));
									}
									else if (l.target.is_active())
									{
										Vector3 startLocation = placement.get_translation();
										l.hitPlacement = l.target;
										Vector3 diff = l.hitPlacement.get_placement_in_owner_room().get_translation() - startLocation;
										length = diff.length() / ownerPresence->get_placement().get_scale();
										Vector3 dir = diff.normal();
										placement = look_matrix_no_up(startLocation, dir).to_transform();
										an_assert_immediate(placement.is_ok());

									}
									else
									{
										error(TXT("requires target or forcedLength to be provided if forced"));
									}
								}
								else if (!l.data->hitFlags.is_empty())
								{
									Vector3 startLocation = placement.get_translation();
									Vector3 endLocation = placement.location_to_world(Vector3::yAxis * (length * ownerPresence->get_placement().get_scale()));

									CheckCollisionContext checkCollisionContext;
									checkCollisionContext.collision_info_needed();
									checkCollisionContext.use_collision_flags(l.data->hitFlags);
									checkCollisionContext.ignore_temporary_objects();
									checkCollisionContext.avoid_up_to_top_instigator(get_owner());
									CollisionTrace collisionTrace(CollisionTraceInSpace::WS);
									collisionTrace.add_location(startLocation);
									collisionTrace.add_location(endLocation);
									CheckSegmentResult result;
									l.hitPlacement.set_owner(get_owner());
									bool hit = false;
									bool forceSkipOnMiss = false;
									int colFlags = CollisionTraceFlag::ResultInPresenceRoom;
									if (ownerPresence->trace_collision(AgainstCollision::Precise, collisionTrace, result, colFlags, checkCollisionContext, &l.hitPlacement))
									{
										if (l.data->allowThroughDoors || ! result.changedRoom)
										{
											hit = true;
											Vector3 totalOffset = Vector3::zero;
											if (l.data->extraHitRadius > 0.0f)
											{
												Vector3 r = Vector3(rg.get_float(-1.0f, 1.0f), rg.get_float(-1.0f, 1.0f), rg.get_float(-1.0f, 1.0f)).normal();
												Vector3 offset = r * rg.get_float(0.0f, l.data->extraHitRadius);
												result.hitLocation += offset;
												totalOffset += offset;
											}
											{	// apply that offset
												Transform p = l.hitPlacement.get_placement_in_final_room();
												p.set_translation(p.get_translation() + l.hitPlacement.vector_from_owner_to_target(totalOffset));
												l.hitPlacement.set_placement_in_final_room(p);
											}
											Vector3 diff = result.hitLocation - startLocation;
											if (diff.is_almost_zero())
											{
												diff = Vector3::yAxis * 0.1f; // something! maybe he is stepping over us?
											}
											length = diff.length() / ownerPresence->get_placement().get_scale();
											if (!l.data->extraHitPenetration.is_empty())
											{
												l.extraHitPenetration = rg.get(l.data->extraHitPenetration);
												length += l.extraHitPenetration;
											}
											Vector3 dir = diff.normal();
											placement = look_matrix_no_up(startLocation, dir).to_transform();
											an_assert_immediate(placement.is_ok());
											if (l.data->impulseOnHit != 0.0f &&
												(! l.data->impulesOnHitObjectsOnly || fast_cast<Object>(result.object)))
											{
												if (auto* p = get_owner()->get_presence())
												{
													float apply = 1.0f;
													if (l.data->impulseNominalDistance > 0.0f)
													{
														apply = clamp(1.0f - length / l.data->impulseNominalDistance, 0.0f, 1.0f);
													}
													p->add_velocity_impulse(l.data->impulseOnHit * dir * apply);
												}
											}
										}
										else
										{
											// we hit door and we didn't want to, we should not spawn anything, we may have incorrect result
											forceSkipOnMiss = true;
										}
									}
									if (! hit)
									{
										l.hitPlacement.clear_target(); // will reset final room to owner's room
										l.hitPlacement.set_placement_in_final_room(Transform(endLocation, Quat::identity));
										if (!l.data->timeToLiveMiss.is_empty())
										{
											l.timeToLive = rg.get(l.data->timeToLiveMiss);
										}
										if (!l.data->allowMiss || forceSkipOnMiss)
										{
											spawn = false;
										}
									}
								}
								else if (l.data->useTarget && l.target.is_active())
								{
									l.hitPlacement = l.target;
									if (l.data->extraHitRadius > 0.0f)
									{
										Vector3 totalOffset = Vector3::zero;
										{	// calculate offset
											Vector3 r = Vector3(rg.get_float(-1.0f, 1.0f), rg.get_float(-1.0f, 1.0f), rg.get_float(-1.0f, 1.0f)).normal();
											Vector3 offset = r * rg.get_float(0.0f, l.data->extraHitRadius);
											totalOffset = offset;
										}

										{	// apply that offset
											Transform p = l.hitPlacement.get_placement_in_final_room();
											p.set_translation(p.get_translation() + l.hitPlacement.vector_from_owner_to_target(totalOffset));
											l.hitPlacement.set_placement_in_final_room(p);
										}
									}
									length = (l.hitPlacement.get_placement_in_owner_room().get_translation() - placement.get_translation()).length();
								}
							}

							if (l.data->type == CustomModules::Lightning::SocketArc &&
								l.socketAIdx != NONE && l.socketBIdx != NONE)
							{
								calculate_socket_arc_placement(l, OUT_ length, OUT_ placement);
							}
							if (l.data->type == CustomModules::Lightning::Presence)
							{
								placement = placement.to_world(ownerPresence->get_centre_of_presence_os());
								an_assert_immediate(placement.is_ok());
								an_assert(ownerPresence->get_presence_centre_distance().is_zero(), TXT("implement_!"));
							}
							placement.normalise_orientation(); // just in any case
							an_assert_immediate(placement.is_ok());
						}
						// do the actual spawn
						if (spawn)
						{
							l.to = to->spawn(l.data->temporaryObjects[rg.get_int(l.data->temporaryObjects.get_size())], ModuleTemporaryObjects::SpawnParams().at_socket(atSocketA).at_relative_placement(basePlacement.to_local(placement)));
							if (l.to.is_set())
							{
								if (!l.data->sounds.is_empty())
								{
									if (auto* s = get_owner()->get_sound())
									{
										Transform relativePlacement = placement;
										if (atSocketB.is_valid())
										{
											relativePlacement = relativePlacement.to_world(Transform(Vector3::yAxis * length * 0.5f, Quat::identity));
										}
										relativePlacement = basePlacement.to_local(relativePlacement);
										l.sound = s->play_sound(l.data->sounds[rg.get_int(l.data->sounds.get_size())], atSocketA, relativePlacement);
									}
								}
								introduce_lightning(l);
								if (l.data->type == CustomModules::Lightning::Strike ||
									l.data->type == CustomModules::Lightning::SocketArc)
								{
									if (l.data->lengthVarID.is_valid())
									{
										l.to->access_variables().access<float>(l.data->lengthVarID) = length * (ownerPresence->get_placement().get_scale() / l.to->get_presence()->get_placement().get_scale());
									}
									if (l.data->arcVarID.is_valid())
									{
										l.to->access_variables().access<float>(l.data->arcVarID) = l.arc;
									}
								}
								if (l.data->type == CustomModules::Lightning::Presence &&
									l.data->radiusVarID.is_valid())
								{
									l.to->access_variables().access<Range>(l.data->radiusVarID) = l.data->radiusRelativeToPresence * (ownerPresence->get_presence_radius() * (ownerPresence->get_placement().get_scale() / l.to->get_presence()->get_placement().get_scale()));
								}
							}
							else if (l.single)
							{
								// we may safely ignore it
								clean_up_lightning(l);
								lightnings.remove_fast_at(i);
								--i;
								continue;
							}
						}
					}
					else
					{
						l.vrPulseHand.clear();
					}
				}
				if (l.vrPulseHand.is_set() &&
					l.data->vrPulse.is_set())
				{
					if (Framework::GameUtils::is_controlled_by_local_player(get_owner()))
					{
						if (auto* vr = VR::IVR::get())
						{
							vr->trigger_pulse(l.vrPulseHand.get(), l.data->vrPulse);
						}
					}
				}
				l.spawnPending = false;
			}
			if (auto* to = l.to.get())
			{
				if (auto* toInRoom = to->get_presence()->get_in_room())
				{
					l.arc += l.arcSpeed * _deltaTime;
					if (l.hitPlacement.is_active())
					{
						if (l.hitPlacement.get_owner() &&
							l.hitPlacement.get_owner() != to)
						{
							if (!l.hitPlacement.find_path(to, l.hitPlacement.get_in_final_room(), l.hitPlacement.get_placement_in_final_room()))
							{
								l.hitPlacement.reset();
							}
						}
					}
					if ((l.data->type == CustomModules::Lightning::Strike && l.hitPlacement.is_active() && l.hitPlacement.get_owner() == to) ||
						(l.data->type == CustomModules::Lightning::SocketArc))
					{
						auto * ownerPresence = get_owner()->get_presence();
						float length = 0.0f;
						Transform currPlacement = to->get_presence()->get_placement();
						Optional<Transform> newPlacement = currPlacement;
						Optional<Transform> newRelativeToSocketPlacement;
						if (l.data->type == CustomModules::Lightning::Strike)
						{
							length = l.hitPlacement.get_straight_length() / ownerPresence->get_placement().get_scale();
							length += l.extraHitPenetration;
							Vector3 endLoc = l.hitPlacement.get_placement_in_owner_room().get_translation();
							Vector3 dir = (endLoc - currPlacement.get_translation()).normal();
							Vector3 up = currPlacement.get_axis(Axis::Z).normal();
							todo_note(TXT("for time being ignore l.socketAUpAxiss, keeping same up dir should be enough"));
							newPlacement = look_matrix(currPlacement.get_translation(), dir, up).to_transform();
						}
						if (l.data->type == CustomModules::Lightning::SocketArc &&
							l.socketAIdx != NONE && l.socketBIdx != NONE)
						{
							newRelativeToSocketPlacement.set_if_not_set();
							calculate_socket_arc_relative_placement(l, OUT_ length, OUT_ newRelativeToSocketPlacement.access());
						}
						an_assert(newPlacement.is_set() || newRelativeToSocketPlacement.is_set());
						if (l.data->continuousPlacementUpdate)
						{
							todo_note(TXT("if sockets are attached to same bone it should be fine, otherwise we should calculate new UP dir"));
							if (to->get_presence()->is_attached())
							{
								if (newRelativeToSocketPlacement.is_set())
								{
									to->get_presence()->update_attachment_relative_placement(newRelativeToSocketPlacement.get());
								}
								else
								{
									Transform relativePlacement;
									if (to->get_presence()->get_attached_relative_placement_info(OUT_ relativePlacement))
									{
										// reset to socket placement by using reverse relative placement
										float scale = relativePlacement.get_scale();
										relativePlacement.set_scale(1.0f);
										Transform baseCurrPlacement = currPlacement.to_world(relativePlacement.inverted());
										Transform newRelativePlacement = baseCurrPlacement.to_local(newPlacement.get());
										newRelativePlacement.normalise_orientation();
										newRelativePlacement.set_scale(scale);
										to->get_presence()->update_attachment_relative_placement(newRelativePlacement);
									}
								}
							}
							else
							{
								an_assert(newPlacement.is_set());
								to->get_presence()->set_next_orientation_displacement(currPlacement.to_local(newPlacement.get()).get_orientation());
							}
						}
						if (l.data->lengthVarID.is_valid())
						{
							to->access_variables().access<float>(l.data->lengthVarID) = length * (ownerPresence->get_placement().get_scale() / l.to->get_presence()->get_placement().get_scale());
						}
						if (l.data->arcVarID.is_valid())
						{
							to->access_variables().access<float>(l.data->arcVarID) = l.arc;
						}
					}
					if (l.data->type == CustomModules::Lightning::Presence &&
						l.data->radiusVarID.is_valid())
					{
						auto * ownerPresence = get_owner()->get_presence();
						l.to->access_variables().access<Range>(l.data->radiusVarID) = l.data->radiusRelativeToPresence * (ownerPresence->get_presence_radius() * (ownerPresence->get_placement().get_scale() / l.to->get_presence()->get_placement().get_scale()));
					}
				}
			}
			if (!l.to.is_set() && 
				(l.single &&
				 (! l.data->crawlSocketPairs ||									// for single we either are not crawling socket pairs
				  l.crawlSocketPairIdx >= l.data->socketPairs.get_size())))		// or we have reached the end
			{
				clean_up_lightning(l);
				lightnings.remove_fast_at(i);
				--i;
				continue;
			}
			if (l.to.is_set() && ! l.data->lightSources.is_empty())
			{
				for_every(ls, l.data->lightSources)
				{
					requiredLightSources.push_back_unique(*ls);
				}
			}
		}
		if (lightnings.is_empty() && !requiredLightSources.is_empty())
		{
			mark_no_longer_requires(all_customs__advance_post);
			inactive = true;
		}
		if (auto* ls = get_owner()->get_custom<CustomModules::LightSources>())
		{
			bool changedActiveLightSources = false;
			for_every(als, activeLightSources)
			{
				if (!requiredLightSources.does_contain(*als))
				{
					changedActiveLightSources = true;
					ls->remove(*als);
				}
			}
			for_every(rls, requiredLightSources)
			{
				if (!activeLightSources.does_contain(*rls))
				{
					changedActiveLightSources = true;
					ls->add(*rls, true);
				}
			}
			if (changedActiveLightSources)
			{
				activeLightSources.clear();
				activeLightSources.make_space_for(requiredLightSources.get_size());
				for_every(rls, requiredLightSources)
				{
					activeLightSources.push_back(*rls);
				}
			}
		}
	}

}

void CustomModules::LightningSpawner::start(Name const & _id, LightningParams const& _params)
{
	stop(_id); // stop all existing (with this id) and start again

	add(_id, _params);
}

void CustomModules::LightningSpawner::add(Name const& _id, LightningParams const& _params)
{
	if (inactive)
	{
		mark_requires(all_customs__advance_post);
	}

	an_assert_immediate(!_params.startPlacementOS.is_set() || _params.startPlacementOS.get().is_ok());
	an_assert_immediate(!_params.startPlacementWS.is_set() || _params.startPlacementWS.get().is_ok());

	Concurrency::ScopedSpinLock lock(requests.lock);
	requests.requests.push_back(Requests::Request(Requests::Request::Op::Add, _id, _params));
}

void CustomModules::LightningSpawner::internal_add(Name const & _id, LightningParams const& _params)
{
	for_every(ld, lightningSpawnerData->lightnings)
	{
		if (ld->id == _id)
		{
			int useCount = _params.count.get(ld->count);
			for_count(int, i, useCount)
			{
				Lightning l;
				l.data = ld;
				l.id = _id;
				l.crawlSocketPairIdx = NONE;
				if (ld->interval.is_empty() || ld->startImmediately)
				{
					l.spawnPending = true;
				}
				else
				{
					l.timeToSpawn = rg.get(ld->interval);
				}
				l.target = _params.target;
				l.isDirectTarget = _params.isDirectTarget;
				l.isForced = _params.isForced;
				l.forcedLength = _params.length;
				l.startPlacementOS = _params.startPlacementOS;
				l.startPlacementWS = _params.startPlacementWS;
				lightnings.push_back(l);
			}
		}
	}
}

void CustomModules::LightningSpawner::stop(Name const & _id)
{
	Concurrency::ScopedSpinLock lock(requests.lock);
	requests.requests.push_back(Requests::Request(Requests::Request::Op::Stop, _id));
}

void CustomModules::LightningSpawner::internal_stop(Name const& _id)
{
	for_every(l, lightnings)
	{
		if (l->id == _id)
		{
			l->shouldStop = true;
		}
	}
}

void CustomModules::LightningSpawner::single(Name const & _id, LightningParams const& _params)
{
	if (inactive)
	{
		mark_requires(all_customs__advance_post);
	}

	an_assert_immediate(!_params.startPlacementOS.is_set() || _params.startPlacementOS.get().is_ok());
	an_assert_immediate(!_params.startPlacementWS.is_set() || _params.startPlacementWS.get().is_ok());

	Concurrency::ScopedSpinLock lock(requests.lock);
	requests.requests.push_back(Requests::Request(Requests::Request::Single, _id, _params));
}

void CustomModules::LightningSpawner::internal_single(Name const& _id, LightningParams const & _params)
{
	for_every(ld, lightningSpawnerData->lightnings)
	{
		if (ld->id == _id)
		{
			int useCount = _params.count.get(ld->count);
			for_count(int, i, useCount)
			{
				Lightning l;
				l.single = true;
				l.data = ld;
				l.id = _id;
				l.crawlSocketPairIdx = NONE;
				l.spawnPending = true;
				l.target = _params.target;
				l.isDirectTarget = _params.isDirectTarget;
				l.isForced = _params.isForced;
				l.forcedLength = _params.length;
				l.startPlacementOS = _params.startPlacementOS;
				l.startPlacementWS = _params.startPlacementWS;
				lightnings.push_back(l);
			}
		}
	}
}

//

REGISTER_FOR_FAST_CAST(CustomModules::LightningSpawnerData);

LightningSpawnerData::LightningSpawnerData(LibraryStored* _inLibraryStored)
: base(_inLibraryStored)
{
}

bool LightningSpawnerData::read_parameter_from(IO::XML::Attribute const * _attr, LibraryLoadingContext & _lc)
{
	return base::read_parameter_from(_attr, _lc);
}

bool LightningSpawnerData::read_parameter_from(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	if (_node->get_name() == TXT("lightning"))
	{
		LightningData l;
		if (l.load_from_xml(_node))
		{
			lightnings.push_back(l);
		}
		return true;
	}
	return base::read_parameter_from(_node, _lc);
}

bool LightningSpawnerData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	return result;
}

bool LightningSpawnerData::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

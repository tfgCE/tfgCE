#include "ac_particlesMeshParticles.h"

#include "..\appearanceControllerPoseContext.h"

#include "..\meshParticles.h"

#include "..\..\library\library.h"
#include "..\..\meshGen\meshGenGenerationContext.h"
#include "..\..\meshGen\meshGenParamImpl.inl"
#include "..\..\module\moduleAppearance.h"
#include "..\..\object\temporaryObject.h"
#include "..\..\world\room.h"

#include "..\..\..\core\mesh\pose.h"
#include "..\..\..\core\other\parserUtils.h"
#include "..\..\..\core\performance\performanceUtils.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//#define DEBUG_MESH_PARTICLES

//

using namespace Framework;
using namespace AppearanceControllersLib;

//

DEFINE_STATIC_NAME(particleInfos);

//

REGISTER_FOR_FAST_CAST(ParticlesMeshParticles);

ParticlesMeshParticles::ParticlesMeshParticles(ParticlesMeshParticlesData const * _data)
: base(_data)
, particlesMeshParticlesData(_data)
{
}

ParticlesMeshParticles::~ParticlesMeshParticles()
{
	stop_observing_presence();
}

void ParticlesMeshParticles::on_presence_changed_room(ModulePresence* _presence, Room * _intoRoom, Transform const & _intoRoomTransform, DoorInRoom* _exitThrough, DoorInRoom* _enterThrough, DoorInRoomArrayPtr const * _throughDoors)
{
	if (particlesMeshParticlesData->is_in_local_space() ||
		particlesMeshParticlesData->is_in_vr_space())
	{
		return;
	}
	for_every(particle, particles)
	{
		particle->placement = _intoRoomTransform.to_local(particle->placement);
		particle->velocityLinear = _intoRoomTransform.vector_to_local(particle->velocityLinear);
		// velocity orientation stays
	}

}

void ParticlesMeshParticles::on_presence_added_to_room(ModulePresence* _presence, Room* _room, DoorInRoom * _enteredThroughDoor)
{

}

void ParticlesMeshParticles::on_presence_removed_from_room(ModulePresence* _presence, Room* _room)
{

}

void ParticlesMeshParticles::on_presence_destroy(ModulePresence* _presence)
{
	an_assert(observingPresence == _presence);
	stop_observing_presence();
}

void ParticlesMeshParticles::stop_observing_presence()
{
	if (observingPresence)
	{
		observingPresence->remove_presence_observer(this);
		observingPresence = nullptr;
	}
}

void ParticlesMeshParticles::initialise(ModuleAppearance* _owner)
{
	base::initialise(_owner);

	if (isValid)
	{
		// already initialised
#ifdef AN_DEVELOPMENT
		IModulesOwner* imo = get_owner()->get_owner();
		an_assert(observingPresence == imo->get_presence());
#endif
		return;
	}

	isValid = true;

	if (auto* meshParticles = fast_cast<MeshParticles>(get_owner()->get_mesh()))
	{
		Array<int> particleBoneIndices;
		meshParticles->get_particle_indices(particlesMeshParticlesData->get_bone_particle_type(), particleBoneIndices);
		particles.set_size(particleBoneIndices.get_size());
		particleInfos.set_size(particleBoneIndices.get_size());
		int const * particleBoneIndex = particleBoneIndices.begin_const();
		for_every(particle, particles)
		{
			particle->boneIndex = *particleBoneIndex;
			++particleBoneIndex;
		}
	}
	else
	{
		error(TXT("particles AC can be used only with mesh particles"));
		isValid = false;
	}

	IModulesOwner* imo = get_owner()->get_owner();

	an_assert(!observingPresence);
	imo->get_presence()->add_presence_observer(this);
	observingPresence = imo->get_presence();
}

void ParticlesMeshParticles::activate()
{
	base::activate();

	// setup before activating particles
	
	//
	placementOffRadiusCoef = particlesMeshParticlesData->get_placement_off_radius_coef();
	placementOffAlongRadiusCoef = particlesMeshParticlesData->get_placement_off_along_radius_coef();
	minAmountSeparation = particlesMeshParticlesData->get_min_amount_separation();

	//
	placementSeparation = particlesMeshParticlesData->get_placement_separation();
	{
		ParticlesPlacement::Type usePlacement = particlesMeshParticlesData->get_initial_placement();
		float sepRef = particlesMeshParticlesData->get_placement_separation_ref();
		if (sepRef != 0.0f)
		{
			if (usePlacement == ParticlesPlacement::CreatorPresence)
			{
				if (auto* owner = get_owner()->get_owner()->get_creator(true))
				{
					placementSeparation *= (owner->get_presence()->get_presence_size() / sepRef);
				}
			}
			else if (usePlacement == ParticlesPlacement::InstigatorPresence)
			{
				if (auto* instigator = get_owner()->get_owner()->get_instigator(true))
				{
					placementSeparation *= (instigator->get_presence()->get_presence_size() / sepRef);
				}
			}
			else
			{
				error(TXT("separation ref not supported, implement"));
			}
		}
		if (placementSeparation != 0.0f)
		{
			if (usePlacement != ParticlesPlacement::CreatorPresence &&
				usePlacement != ParticlesPlacement::InstigatorPresence)
			{
				error(TXT("placement separation available only for presence"));
			}
		}
	}

	//
	particlesScale = particlesMeshParticlesData->get_particles_scale();
	{
		ParticlesScale::Type useParticlesScaleType = particlesMeshParticlesData->get_particles_scale_type();

		float scaleRef = particlesMeshParticlesData->get_particles_scale_ref();
		if (scaleRef != 0.0f)
		{
			if (useParticlesScaleType == ParticlesScale::InstigatorPresenceSize)
			{
				if (auto* instigator = get_owner()->get_owner()->get_instigator(true))
				{
					particlesScale *= (instigator->get_presence()->get_presence_size() / scaleRef);
				}
			}
			else if (useParticlesScaleType == ParticlesScale::CreatorPresenceSize)
			{
				if (auto* owner = get_owner()->get_owner()->get_creator(true))
				{
					particlesScale *= (owner->get_presence()->get_presence_size() / scaleRef);
				}
			}
		}
	}

	activate_particle(particles.begin(), particles.end());

	useRoomVelocity = particlesMeshParticlesData->get_use_room_velocity();
	useRoomVelocityInTime = particlesMeshParticlesData->get_use_room_velocity_in_time();
	useOwnerMovement = particlesMeshParticlesData->get_use_owner_movement();
	useOwnerMovementForTime = particlesMeshParticlesData->get_use_owner_movement_for_time();

}

void ParticlesMeshParticles::activate_particle(Particle * firstParticle, Particle* endParticle)
{
	if (!are_particles_active())
	{
		return;
	}

	an_assert(firstParticle, TXT("why have particles if there are no particles?"));

	if (!firstParticle)
	{
		return;
	}

	if (!endParticle)
	{
		endParticle = firstParticle + 1;
	}

	internal_activate_particle_in_ws(firstParticle, endParticle);

	move_from_ws_into_proper_space(firstParticle, endParticle);
}

void ParticlesMeshParticles::internal_activate_particle_in_ws(Particle * firstParticle, Particle* endParticle)
{
	an_assert(endParticle);
	for (Particle* particle = firstParticle; particle != endParticle; ++particle)
	{
		particle->timeBeingAlive = 0.0f;
		particle->activationFlags = 0;
		particle->randomNumber = rg.get_float(0.0f, 1.0f);
		particle->individualParticleScale = rg.get(particlesMeshParticlesData->get_individual_particle_scale());
	}

	// by default assume WS, change to local space at the end

	bool placed = false;

	ParticlesPlacement::Type useInitialPlacement = particlesMeshParticlesData->get_initial_placement();

	if (!placed && (useInitialPlacement == ParticlesPlacement::InstigatorBones ||
					useInitialPlacement == ParticlesPlacement::CreatorBones))
	{
		IModulesOwner* at = nullptr;
		if (useInitialPlacement == ParticlesPlacement::InstigatorBones)
		{
			at = get_owner()->get_owner()->get_instigator(true);
		}
		else
		{
			at = get_owner()->get_owner()->get_creator(true);
		}
		if (at)
		{
			if (auto* ma = at->get_appearance())
			{
				if (auto* skeleton = ma->get_skeleton())
				{
					auto* pose = ma->get_final_pose_MS();
					if (pose->get_num_bones() > 0)
					{
						for (Particle* particle = firstParticle; particle != endParticle; ++particle)
						{
							int atIdx = rg.get_int(pose->get_num_bones());
							Transform boneMS = pose->get_bone(atIdx);
							Transform boneWS = at->get_appearance()->get_ms_to_ws_transform().to_world(boneMS);

							particle->placement = boneWS;
						}
						placed = true;
					}
				}
			}
		}
		if (!placed)
		{
			useInitialPlacement = useInitialPlacement == ParticlesPlacement::InstigatorBones? ParticlesPlacement::InstigatorSkeleton : ParticlesPlacement::CreatorSkeleton; // fallback
		}
	}

	if (!placed && (useInitialPlacement == ParticlesPlacement::InstigatorSkeleton ||
					useInitialPlacement == ParticlesPlacement::CreatorSkeleton))
	{
		IModulesOwner* at = nullptr;
		if (useInitialPlacement == ParticlesPlacement::InstigatorSkeleton)
		{
			at = get_owner()->get_owner()->get_instigator(true);
		}
		else
		{
			at = get_owner()->get_owner()->get_creator(true);
		}
		if (at)
		{
			if (auto* ma = at->get_appearance())
			{
				if (auto * skeleton = ma->get_skeleton())
				{
					todo_note(TXT("implement_"));
					//ma->get_final_pose_MS();
					placed = false;
				}
			}
		}
		if (!placed)
		{
			useInitialPlacement = useInitialPlacement == ParticlesPlacement::InstigatorSkeleton? ParticlesPlacement::InstigatorPresence : ParticlesPlacement::CreatorPresence; // fallback
		}
	}

	if (!placed && (useInitialPlacement == ParticlesPlacement::InstigatorPresence ||
					useInitialPlacement == ParticlesPlacement::CreatorPresence))
	{
		useInitialPlacement = ParticlesPlacement::OwnerPlacement;
		IModulesOwner* at = nullptr;
		if (useInitialPlacement == ParticlesPlacement::InstigatorSkeleton)
		{
			at = get_owner()->get_owner()->get_instigator(true);
		}
		else
		{
			at = get_owner()->get_owner()->get_creator(true);
		}
		if (at)
		{
			if (auto* mp = at->get_presence())
			{
				Transform atPlacement = mp->get_placement();

				Vector3 locA, locB;
				float radius;
				if (mp->get_presence_primitive_info(locA, locB, radius))
				{
					float placementOffRadius = radius * placementOffRadiusCoef;
					float placementOffAlongRadius = radius * placementOffAlongRadiusCoef.get(placementOffRadiusCoef);
					if (placementSeparation != 0.0f)
					{
						if (locA != locB)
						{
							Vector3 a2b = locB - locA;
							float a2bLen = a2b.length();

							{
								Vector3 a2bDir = a2b.normal();
								locA -= a2bDir * placementOffAlongRadius;
								locB += a2bDir * placementOffAlongRadius;

								a2b = locB - locA;
								a2bLen = a2b.length();
							}

							float rawAmount = a2bLen / placementSeparation;
							if (rawAmount < 1.0f)
							{
								// prefer two for smaller objects
								rawAmount += 0.5f;
							}
							else if (rawAmount < 2.0f)
							{
								// prefer three for not so small objects
								rawAmount += 0.25f;
							}
							int amount = min((int)(endParticle - firstParticle), max(minAmountSeparation, TypeConversions::Normal::f_i_closest(rawAmount)));

							int idx = 0;
							for (Particle* particle = firstParticle; particle != endParticle; ++particle, ++ idx)
							{
								float at = rg.get_float(0.25f, 0.75f);
								if (amount > 1)
								{
									at = ((float)idx + rg.get_float(-0.25f, 0.25f)) / (float)(amount - 1);
									at = clamp(at, 0.0f, 1.0f);
								}
								Vector3 off = Vector3(rg.get_float(-1.0f, 1.0f), rg.get_float(-1.0f, 1.0f), rg.get_float(-1.0f, 1.0f)).normal() * rg.get_float(0.0f, placementOffRadius);
								particle->placement = Transform(locA + off + a2b * at, atPlacement.get_orientation());
								if (idx >= amount)
								{
									// make them dead
									particle->timeBeingAlive = 100.0f;
									particle->activationFlags = 0; // clear activation flags
								}
							}
							placed = true;
						}
						else
						{
							// point based
							int amount = min((int)(endParticle - firstParticle), max(0, minAmountSeparation));

							int idx = 0;
							for (Particle* particle = firstParticle; particle != endParticle; ++particle, ++ idx)
							{
								Vector3 off = Vector3(rg.get_float(-1.0f, 1.0f), rg.get_float(-1.0f, 1.0f), rg.get_float(-1.0f, 1.0f)).normal() * rg.get_float(0.0f, placementOffRadius);
								particle->placement = Transform(locA + off, atPlacement.get_orientation());
								if (idx >= amount)
								{
									// make them dead
									particle->timeBeingAlive = 100.0f;
									particle->activationFlags = 0; // clear activation flags
								}
							}
							placed = true;
						}
					}
					else
					{
						if (locA != locB)
						{
							// segment based
							Vector3 a2b = locB - locA;
							a2b.normalise();
							locA -= a2b * placementOffAlongRadius;
							locB += a2b * placementOffAlongRadius;

							// whole distance
							a2b = locB - locA;

							for (Particle* particle = firstParticle; particle != endParticle; ++particle)
							{
								particle->placement = Transform(locA + a2b * rg.get_float(0.0f, 1.0f), atPlacement.get_orientation());
							}
							placed = true;
						}
						else
						{
							// point based
							for (Particle* particle = firstParticle; particle != endParticle; ++particle)
							{
								particle->placement = Transform(locA + Vector3(rg.get_float(-1.0f, 1.0f), rg.get_float(-1.0f, 1.0f), rg.get_float(-1.0f, 1.0f)).normal() * rg.get_float(0.0f, placementOffRadius * 0.5f), atPlacement.get_orientation());
							}
							placed = true;
						}
					}
				}
			}
		}
	}

	if (!placed)
	{
		// 
		IModulesOwner* imo = get_owner()->get_owner();
		Transform ownerPlacement = imo->get_presence()->get_placement();

		for (Particle* particle = firstParticle; particle != endParticle; ++particle)
		{
			particle->placement = ownerPlacement;
		}
	}
}

void ParticlesMeshParticles::move_from_ws_into_proper_space(Particle * firstParticle, Particle* endParticle)
{
	if (particlesMeshParticlesData->is_in_local_space())
	{
		IModulesOwner* imo = get_owner()->get_owner();
		Transform ownerPlacement = imo->get_presence()->get_placement();

		for (Particle* particle = firstParticle; particle != endParticle; ++particle)
		{
			particle->placement = ownerPlacement.to_local(particle->placement);
			particle->velocityLinear = ownerPlacement.vector_to_local(particle->velocityLinear);
		}
	}
	else if (particlesMeshParticlesData->is_in_vr_space())
	{
		IModulesOwner* imo = get_owner()->get_owner();
		Transform ownerPlacement = imo->get_presence()->get_placement();
		Transform ownerVRPlacement = imo->get_presence()->get_placement_in_vr_space();
		Transform toVRTransform = ownerVRPlacement.to_world(ownerPlacement.inverted());

		for (Particle* particle = firstParticle; particle != endParticle; ++particle)
		{
			particle->placement = toVRTransform.to_world(particle->placement);
			particle->velocityLinear = toVRTransform.vector_to_world(particle->velocityLinear);
		}
	}
}

void ParticlesMeshParticles::advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::advance_and_adjust_preliminary_pose(_context);

	MEASURE_PERFORMANCE(particlesMeshParticles_preliminaryPose);

	Vector3 roomVelocity = Vector3::zero;
	Transform ownerPlacement = Transform::identity;
	Vector3 ownerLinearDisplacement = Vector3::zero;
	Vector3 initialAdditionalAbsoluteVelocity = Vector3::zero;
	Quat ownerRotateDisplacement = Quat::identity;
	IModulesOwner* imo = get_owner()->get_owner();
	bool inWS = false;
	bool alignToVelocity = particlesMeshParticlesData->should_align_to_velocity();
	if (imo)
	{
		if (auto* r = imo->get_presence()->get_in_room())
		{
			ownerPlacement = imo->get_presence()->get_placement();
			roomVelocity = r->get_room_velocity();
			if (!roomVelocity.is_zero())
			{
				if (particlesMeshParticlesData->is_in_local_space())
				{
					roomVelocity = ownerPlacement.vector_to_local(roomVelocity);
				}
				else if (particlesMeshParticlesData->is_in_vr_space())
				{
					IModulesOwner* imo = get_owner()->get_owner();
					Transform ownerVRPlacement = imo->get_presence()->get_placement_in_vr_space();
					Transform toVRTransform = ownerVRPlacement.to_world(ownerPlacement.inverted());
					roomVelocity = toVRTransform.vector_to_local(roomVelocity);
				}
			}
			
			if (!particlesMeshParticlesData->is_in_local_space() &&
				!particlesMeshParticlesData->is_in_vr_space())
			{
				inWS = true;
				ownerLinearDisplacement = ownerPlacement.vector_to_local(imo->get_presence()->get_velocity_linear()) * _context.get_delta_time();
				ownerRotateDisplacement = (imo->get_presence()->get_velocity_rotation() * _context.get_delta_time()).to_quat();
			}
		}
		if (alignToVelocity)
		{
			if (auto* to = imo->get_as_temporary_object())
			{
				initialAdditionalAbsoluteVelocity = to->get_initial_additional_absolute_velocity().get(Vector3::zero);
			}
		}
	}
	Vector3 roomVelocityDisplacement = -roomVelocity * _context.get_delta_time();
	for_every(particle, particles)
	{
		particle->timeBeingAlive += _context.get_delta_time();
		{
			Vector3 newLoc = particle->placement.get_translation() + particle->velocityLinear * _context.get_delta_time();
			Quat newOri = particle->placement.get_orientation().rotate_by((particle->velocityRotation * _context.get_delta_time()).to_quat());

			newLoc += roomVelocityDisplacement * useRoomVelocity * (useRoomVelocityInTime <= 0.0f ? 1.0f : clamp(particle->timeBeingAlive / useRoomVelocityInTime, 0.0f, 1.0f));

			if (useOwnerMovement != 0.0f && imo && inWS)
			{
				float useOwnerMovementNow = useOwnerMovement * (useOwnerMovementForTime <= 0.0f ? 1.0f : clamp(1.0f - particle->timeBeingAlive / useOwnerMovementForTime, 0.0f, 1.0f));
				if (useOwnerMovementNow > 0.0f)
				{
					Vector3 displaceL = ownerLinearDisplacement * useOwnerMovementNow;
					Quat displaceR = Quat::slerp(useOwnerMovementNow, Quat::identity, ownerRotateDisplacement);

					newLoc += displaceL;
					newOri = newOri.rotate_by(displaceR);

					// if owner rotates, we should rotate with the owner, even if we're far away
					if (! displaceR.is_identity())
					{	
						Vector3 relLoc = ownerPlacement.location_to_local(newLoc);
						Transform rotatedPlacement = ownerPlacement;
						rotatedPlacement.set_orientation(ownerPlacement.get_orientation().rotate_by(displaceR));
						newLoc = rotatedPlacement.location_to_world(relLoc);
					}
				}
			}

			particle->placement.set_translation(newLoc);
			particle->placement.set_orientation(newOri);
		}
		if (alignToVelocity)
		{
			Vector3 dir = particle->velocityLinear - initialAdditionalAbsoluteVelocity;

			if (!dir.is_zero())
			{
				particle->placement.set_orientation(look_matrix33(dir.normal(), particle->placement.get_orientation().get_z_axis()).to_quat());
			}
		}
		if (particlesMeshParticlesData->should_scale_with_velocity())
		{
			float velocity = particle->velocityLinear.length();
			float scale = velocity / particlesMeshParticlesData->get_scale_with_velocity();
			scale = particlesMeshParticlesData->get_scale_with_velocity_range().clamp(scale);
			particle->placement.set_scale(particle->placement.get_scale() * scale);
		}
	}
}

void ParticlesMeshParticles::calculate_final_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::calculate_final_pose(_context);

	if (! isValid)
	{
		return;
	}
	
#ifdef MEASURE_PERFORMANCE_APPEARANCE_CONTROLLERS_DETAILED
	MEASURE_PERFORMANCE(particlesMeshParticles_finalPose);
#endif

	Meshes::Pose * poseLS = _context.access_pose_LS();

	if (ModuleAppearance * appearance = get_owner())
	{
		if (IModulesOwner const * owner = appearance->get_owner())
		{
			if (ModulePresence const * presence = owner->get_presence())
			{
				Transform transform; // inverse
				if (particlesMeshParticlesData->is_in_local_space())
				{
					// os space -> ms space
					transform = appearance->get_ms_to_os_transform();
				}
				else if (particlesMeshParticlesData->is_in_vr_space())
				{
					// vr space -> os space -> ms space
					transform = presence->get_placement_in_vr_space().to_world(appearance->get_ms_to_os_transform());
				}
				else
				{
					// ws space -> ms space
					transform = appearance->get_ms_to_ws_transform();
				}

				// should match bone count
				particleInfos.set_size(poseLS->get_num_bones());

				// read values as they may be shared and when we set it back, we want to set it with existing values
				if (auto* spi = appearance->access_mesh_instance().access_material_instance(0)->get_shader_program_instance())
				{
					if (auto* sp = spi->get_shader_param(NAME(particleInfos)))
					{
						if (sp->is_set())
						{
							sp->get_as_array_vector4(particleInfos, false);
						}
					}
				}

#ifdef DEBUG_MESH_PARTICLES
				LogInfoContext l;

				l.log(TXT("MESH PARTICLES ap%p msh%p rmshinst%p"), appearance, appearance->get_mesh(), &appearance->access_mesh_instance());
				{
					l.log(TXT("read"));
					LOG_INDENT(l);
					for_every(pd, particleInfos)
					{
						int i = for_everys_index(pd);
						l.log(TXT("%2i : %.6f"), i, pd->y);
					}
				}
#endif

#ifdef AN_DEVELOPMENT
				bool anyParticleAlphaed = false;
				bool anyParticleDissolved = false;
#endif
				for_every(particle, particles)
				{
					int idx = particle->boneIndex;
					if (idx >= 0 && idx < poseLS->get_num_bones())
					{
						Transform particlePlacement = particle->placement;
						float fullScale = particlesScale * particle->individualParticleScale;
						float scale = fullScale;
						if (particlesMeshParticlesData->get_particles_scale_pulse_period() != 0.0f)
						{
							scale += particlesMeshParticlesData->get_particles_scale_pulse() * sin_deg(particle->timeBeingAlive * 360.0f / particlesMeshParticlesData->get_particles_scale_pulse_period());
						}
						scale *= particlePlacement.get_scale();
						if (scale <= 0.0f)
						{
							poseLS->set_bone(idx, Transform(Vector3::zero, Quat::identity, 0.0f)); // let it be at the origin
							particleInfos[idx].x = 0.0f; // alpha
							particleInfos[idx].y = 1.0f; // dissolve
						}
						else
						{
							particlePlacement.set_scale(scale);
							an_assert(particlePlacement.get_orientation().is_normalised());
							an_assert(transform.get_orientation().is_normalised());
							an_assert(transform.to_local(particlePlacement).get_orientation().is_normalised());
							poseLS->set_bone(idx, transform.to_local(particlePlacement));
							particleInfos[idx].x = particle->alpha;
							particleInfos[idx].y = particle->dissolve;
						}
						particleInfos[idx].z = fullScale; // full scale
						particleInfos[idx].w = particle->randomNumber;
#ifdef AN_DEVELOPMENT
						anyParticleAlphaed |= particle->alpha < 1.0f;
						anyParticleDissolved |= particle->dissolve > 0.0f;
#endif
#ifdef DEBUG_MESH_PARTICLES
						l.log(TXT("set %2i : %2i : dsslv:%.6f (scale:%.6f) %S %S"),
							for_everys_index(particle), idx,
							particleInfos[idx].y,
							scale,
							scale != 0.0f? TXT("<<<<<<") : TXT(""),
							particlePlacement.get_translation().to_string().to_char());
#endif
					}
				}

#ifndef AN_CLANG
				an_assert(poseLS->is_ok());
#endif

#ifdef AN_DEVELOPMENT
				if (anyParticleAlphaed || anyParticleDissolved)
				{
					if (appearance->access_mesh_instance().get_material_instance_count() > 0 &&
						appearance->access_mesh_instance().get_material_instance(0)->get_material() &&
						!appearance->access_mesh_instance().get_material_instance(0)->get_material()->are_individual_instances_allowed())
					{
						error(TXT("individual instances not allowed for material of \"%S\" set material to allow individual instances"), get_owner()->get_owner()->get_object_type_name().to_char());
					}
				}
#endif

#ifdef DEBUG_MESH_PARTICLES
				{
					l.log(TXT("to send ap%p msh%p smshinst%p"), appearance, appearance->get_mesh(), &appearance->access_mesh_instance());
					LOG_INDENT(l);
					for_every(pd, particleInfos)
					{
						int i = for_everys_index(pd);
						l.log(TXT("%2i : %.6f"), i, pd->y);
					}
				}
#endif

#ifdef AN_DEVELOPMENT
				an_assert(Framework::Library::may_ignore_missing_references() || (appearance->access_mesh_instance().get_material_instance_count() > 0 && appearance->access_mesh_instance().get_material_instance(0)->get_material()), TXT("non material appearance permitted only if may ignore missing references"));
				if (appearance->access_mesh_instance().get_material_instance_count() > 0)
#endif
				for_count(int, miIdx, appearance->access_mesh_instance().get_material_instance_count())
				{
					appearance->access_mesh_instance().access_material_instance(miIdx)->set_uniform(NAME(particleInfos), particleInfos);
				}

#ifdef DEBUG_MESH_PARTICLES
				l.output_to_output();
#endif
			}
		}
	}
}

Transform ParticlesMeshParticles::get_owner_placement_in_proper_space() const
{
	if (particlesMeshParticlesData->is_in_local_space())
	{
		return Transform::identity;
	}

	IModulesOwner* imo = get_owner()->get_owner();
	if (particlesMeshParticlesData->is_in_vr_space())
	{
		return imo->get_presence()->get_placement_in_vr_space();
	}

	return imo->get_presence()->get_placement();
}

//

REGISTER_FOR_FAST_CAST(ParticlesMeshParticlesData);

AppearanceControllerData* ParticlesMeshParticlesData::create_data()
{
	return new ParticlesMeshParticlesData();
}

bool ParticlesMeshParticlesData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	inLocalSpace = _node->get_bool_attribute_or_from_child_presence(TXT("inLocalSpace"), inLocalSpace);
	inVRSpace = _node->get_bool_attribute_or_from_child_presence(TXT("inVRSpace"), inVRSpace);
	alignToVelocity = _node->get_bool_attribute_or_from_child_presence(TXT("alignToVelocity"), alignToVelocity);
	scaleWithVelocity = _node->get_float_attribute_or_from_child(TXT("scaleWithVelocity"), scaleWithVelocity);
	scaleWithVelocityRange.load_from_xml(_node, TXT("scaleWithVelocityRange"));
	boneParticleType = _node->get_name_attribute_or_from_child(TXT("boneParticleType"), boneParticleType);

	initialPlacement = ParticlesPlacement::parse(_node->get_string_attribute_or_from_child(TXT("initialPlacement")), initialPlacement);
	result &= minAmountSeparation.load_from_xml(_node, TXT("minAmountSeparation"));
	result &= placementSeparation.load_from_xml(_node, TXT("placementSeparation"));
	result &= placementSeparationRef.load_from_xml(_node, TXT("placementSeparationRef"));
	if (placementSeparation.is_set())
	{
		placementOffRadiusCoef = 0.5f;
	}
	result &= placementOffRadiusCoef.load_from_xml(_node, TXT("placementOffRadiusCoef"));
	result &= placementOffAlongRadiusCoef.load_from_xml(_node, TXT("placementOffAlongRadiusCoef"));
	result &= individualParticleScale.load_from_xml(_node, TXT("individualParticleScale"));
	particlesScaleType = ParticlesScale::parse(_node->get_string_attribute_or_from_child(TXT("scaleType")), particlesScaleType);
	particlesScaleType = ParticlesScale::parse(_node->get_string_attribute_or_from_child(TXT("particlesScaleType")), particlesScaleType);
	result &= particlesScaleRef.load_from_xml(_node, TXT("scaleRef"));
	result &= particlesScaleRef.load_from_xml(_node, TXT("particlesScaleRef"));
	result &= particlesScale.load_from_xml(_node, TXT("scale"));
	result &= particlesScale.load_from_xml(_node, TXT("particlesScale"));
	result &= particlesScaleMul.load_from_xml(_node, TXT("scaleMul"));
	result &= particlesScaleMul.load_from_xml(_node, TXT("particlesScaleMul"));
	result &= particlesScalePulse.load_from_xml(_node, TXT("particlesScalePulse"));
	result &= particlesScalePulsePeriod.load_from_xml(_node, TXT("particlesScalePulsePeriod"));
	result &= useRoomVelocity.load_from_xml(_node, TXT("useRoomVelocity"));
	result &= useRoomVelocityInTime.load_from_xml(_node, TXT("useRoomVelocityInTime"));
	result &= useOwnerMovement.load_from_xml(_node, TXT("useOwnerMovement"));
	result &= useOwnerMovementForTime.load_from_xml(_node, TXT("useOwnerMovementForTime"));

	return result;
}

bool ParticlesMeshParticlesData::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

void ParticlesMeshParticlesData::apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context)
{
	base::apply_mesh_gen_params(_context);

	placementOffRadiusCoef.fill_value_with(_context);
	placementOffAlongRadiusCoef.fill_value_with(_context);
	minAmountSeparation.fill_value_with(_context);
	placementSeparation.fill_value_with(_context);
	placementSeparationRef.fill_value_with(_context);
	individualParticleScale.fill_value_with(_context);
	particlesScale.fill_value_with(_context);
	particlesScaleMul.fill_value_with(_context);
	particlesScaleRef.fill_value_with(_context);
	particlesScalePulse.fill_value_with(_context);
	particlesScalePulsePeriod.fill_value_with(_context);
	useRoomVelocity.fill_value_with(_context);
	useRoomVelocityInTime.fill_value_with(_context);
	useOwnerMovement.fill_value_with(_context);
	useOwnerMovementForTime.fill_value_with(_context);
}


#include "aiLogic_pilgrimFake.h"

#include "..\..\library\library.h"
#include "..\..\game\game.h"

#include "..\..\modules\gameplay\equipment\me_gun.h"

#include "..\..\..\core\debug\debugRenderer.h"

#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\ai\aiMessageHandler.h"
#include "..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\framework\module\modulePresence.h"
#include "..\..\..\framework\module\custom\mc_vrPlacementsProvider.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\framework\nav\navPath.h"
#include "..\..\..\framework\nav\navTask.h"
#include "..\..\..\framework\nav\tasks\navTask_FindPath.h"
#include "..\..\..\framework\presence\relativeToPresencePlacement.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

// bones
DEFINE_STATIC_NAME(head);
DEFINE_STATIC_NAME(l_hand);
DEFINE_STATIC_NAME(r_hand);

// tags 
DEFINE_STATIC_NAME(leftWeapon);
DEFINE_STATIC_NAME(rightWeapon);

// variables
DEFINE_STATIC_NAME(l_hand_ai);
DEFINE_STATIC_NAME(r_hand_ai);

// poses (variables)
DEFINE_STATIC_NAME(pose_attack_0_0);
DEFINE_STATIC_NAME(pose_attack_l45_0);
DEFINE_STATIC_NAME(pose_attack_r45_0);
DEFINE_STATIC_NAME(pose_attack_0_30);
DEFINE_STATIC_NAME(pose_attack_l45_30);
DEFINE_STATIC_NAME(pose_attack_r45_30);

// ai messages
DEFINE_STATIC_NAME_STR(aim_lookAt, TXT("pilgrim fake; look at"));
	DEFINE_STATIC_NAME(target);
	DEFINE_STATIC_NAME(targetLocWS);
	DEFINE_STATIC_NAME(offset);
	DEFINE_STATIC_NAME(power);
	DEFINE_STATIC_NAME(socket);
DEFINE_STATIC_NAME_STR(aim_noLookAt, TXT("pilgrim fake; no look at"));
DEFINE_STATIC_NAME_STR(aim_move, TXT("pilgrim fake; move"));
	DEFINE_STATIC_NAME(gait);
	DEFINE_STATIC_NAME(speed);
	DEFINE_STATIC_NAME(toPOI);
DEFINE_STATIC_NAME_STR(aim_stop, TXT("pilgrim fake; stop"));
DEFINE_STATIC_NAME_STR(aim_hands, TXT("pilgrim fake; hands"));
	DEFINE_STATIC_NAME(left);
	DEFINE_STATIC_NAME(right);
DEFINE_STATIC_NAME_STR(aim_aim, TXT("pilgrim fake; aim"));
	//DEFINE_STATIC_NAME(target);
	//DEFINE_STATIC_NAME(targetLocWS);
	//DEFINE_STATIC_NAME(offset);
	DEFINE_STATIC_NAME(shoot);
	DEFINE_STATIC_NAME(shootLength);
	DEFINE_STATIC_NAME(shootBreakLength);

//

REGISTER_FOR_FAST_CAST(PilgrimFake);

PilgrimFake::PilgrimFake(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
, pilgrimFakeData(fast_cast<PilgrimFakeData>(_logicData))
{
}

PilgrimFake::~PilgrimFake()
{
}

void PilgrimFake::advance(float _deltaTime)
{
	base::advance(_deltaTime);

	if (!started)
	{
		return;
	}

	Transform headTarget = defaultHead;
	Transform handLeftTarget = handLeft;
	Transform handRightTarget = handRight;

	read_animated();

	auto* imo = get_mind()->get_owner_as_modules_owner();

	//

	if (lookAt.at.is_active())
	{
		Vector3 fromLocOS = head.get_translation();
		Vector3 fromLocWS = imo->get_presence()->get_placement().location_to_world(fromLocOS);
		Transform atWS = lookAt.at.get_placement_in_owner_room();
		if (lookAt.atSocket.is_name_valid())
		{
			if (auto* imo = lookAt.at.get_target())
			{
				if (auto* a = imo->get_appearance())
				{
					if (!lookAt.atSocket.is_valid())
					{
						lookAt.atSocket.look_up(a->get_mesh());
					}
					atWS = atWS.to_world(a->calculate_socket_os(lookAt.atSocket.get_index()));
				}
			}
		}
		atWS.set_translation(atWS.location_to_world(lookAt.offset));

		Vector3 dirWS = (atWS.get_translation() - fromLocWS).normal();

		Vector3 dirOS = imo->get_presence()->get_placement().vector_to_local(dirWS);
		dirOS = lerp(lookAt.power, Vector3::yAxis, dirOS).normal();
		{
			Rotator3 rotOS = dirOS.to_rotator();
			rotOS.pitch = lookAt.limits.y.clamp(rotOS.pitch);
			rotOS.yaw = lookAt.limits.x.clamp(rotOS.yaw);
			dirOS = rotOS.get_forward();
		}

		headTarget = look_matrix(fromLocOS, dirOS, Vector3::zAxis).to_transform();
	}

	if (move.newOrder)
	{
		move.newOrder = false;
		auto& locomotion = get_mind()->access_locomotion();
		if (move.move)
		{
			if (move.toPOI.is_set())
			{
				Vector3 loc = imo->get_presence()->get_placement().get_translation();

				if (auto* r = imo->get_presence()->get_in_room())
				{
					Framework::PointOfInterestInstance* foundPOI = nullptr;
					if (r->find_any_point_of_interest(move.toPOI.get(), foundPOI))
					{
						loc = foundPOI->calculate_placement().get_translation();
					}
				}
				Framework::MovementParameters params;
				if (move.gait.is_set())
				{
					params.gait(move.gait.get());
				}
				if (move.speed.is_set())
				{
					params.absolute_speed(move.speed.get());
				}
				locomotion.move_to_2d(loc, 0.1f, params);
				locomotion.turn_in_movement_direction_2d();
			}
		}
		else
		{
			locomotion.stop();
		}
	}

	//

	float blendLeftHand = 0.5f;
	if (forceHandLeft.is_set())
	{
		handLeftTarget = forceHandLeft.get();
		if (leftHandAIVar.access<float>() == 1.0f)
		{
			blendLeftHand = 0.0f;
		}
	}
	leftHandAIVar.access<float>() = blend_to_using_time(leftHandAIVar.access<float>(), forceHandLeft.is_set()? 0.0f : 1.0f, 0.2f, _deltaTime);

	float blendRightHand = 0.5f;
	if (forceHandRight.is_set())
	{
		handRightTarget = forceHandRight.get();
		if (rightHandAIVar.access<float>() == 1.0f)
		{
			blendRightHand = 0.0f;
		}
	}
	rightHandAIVar.access<float>() = blend_to_using_time(rightHandAIVar.access<float>(), forceHandRight.is_set()? 0.0f : 1.0f, 0.2f, _deltaTime);

	//

	head = blend_to_using_time(head, headTarget, 0.2f, _deltaTime);
	handLeft = blend_to_using_time(handLeft, handLeftTarget, blendLeftHand, _deltaTime);
	handRight = blend_to_using_time(handRight, handRightTarget, blendRightHand, _deltaTime);

	if (imo)
	{
		if (auto* vrpp = imo->get_custom<Framework::CustomModules::VRPlacementsProvider>())
		{
			vrpp->set_head(head);
			vrpp->set_hand_left(handLeft);
			vrpp->set_hand_right(handRight);
		}
	}

	{
		float useAimTarget = 0.0f;
		Rotator3 aimAnglesTarget = aim.aimAngles;

		Optional<Vector3> targetWS;
		if (aim.at.is_active())
		{
			useAimTarget = 1.0f;

			targetWS = aim.at.get_placement_in_owner_room().get_translation() + aim.offset;
			Vector3 fromWS = imo->get_presence()->get_placement().location_to_world(Vector3(0.0f, 0.0f, 1.3f));
			Vector3 dirWS = targetWS.get() - fromWS;
			Vector3 dirOS = imo->get_presence()->get_placement().vector_to_local(dirWS);
			Rotator3 rotOS = dirOS.to_rotator();
			aimAnglesTarget = rotOS;
		}

		aim.aimAngles = blend_to_using_time(aim.aimAngles, aimAnglesTarget, 0.4f, _deltaTime);

		aim.use = blend_to_using_time(aim.use, useAimTarget, 0.55f, _deltaTime);

		aim.aimAngles.yaw = clamp(aim.aimAngles.yaw, -45.0f, 45.0f);
		aim.aimAngles.pitch = clamp(aim.aimAngles.pitch, 0.0f, 30.0f);
		
		float useLeft45 = clamp(aim.aimAngles.yaw / -45.0f, 0.0f, 1.0f);
		float useRight45 = clamp(aim.aimAngles.yaw / 45.0f, 0.0f, 1.0f);
		float useUp30 = clamp(aim.aimAngles.pitch / 30.0f, 0.0f, 1.0f);

		aim.aim_0_0_var.access<float>() = aim.use;
		aim.aim_L45_0_var.access<float>() = aim.use * useLeft45;
		aim.aim_R45_0_var.access<float>() = aim.use * useRight45;
		aim.aim_0_30_var.access<float>() = aim.use * useUp30;
		aim.aim_L45_30_var.access<float>() = aim.use * useUp30 * useLeft45;
		aim.aim_R45_30_var.access<float>() = aim.use * useUp30 * useRight45;

		if (aim.shoot)
		{
			if (aim.shootBreakLengthTimeLeft <= 0.0f)
			{
				if (!aim.shootLength.is_empty())
				{
					aim.shootLengthTimeLeft -= _deltaTime;
					if (aim.shootLengthTimeLeft <= 0.0f)
					{
						aim.shootLengthTimeLeft = 0.0f;
						aim.shootBreakLengthTimeLeft = aim.shootBreakLength.is_empty() ? 0.001f : rg.get(aim.shootBreakLength);
					}
				}

				for_count(int, i, 2)
				{
					if (auto* gunIMO = i == 0 ? leftWeapon.get() : rightWeapon.get())
					{
						float& shootTimeLeft = i == 0 ? aim.shootLeftTimeLeft : aim.shootRightTimeLeft;
						shootTimeLeft -= _deltaTime;
						if (shootTimeLeft <= 0.0f)
						{
							shootTimeLeft = i == 0 ? leftWeaponShootInterval : rightWeaponShootInterval;
							shootTimeLeft += rg.get_float(0.0f, 0.5f);

							if (auto* g = gunIMO->get_gameplay_as<ModuleEquipments::Gun>())
							{
								ModuleEquipments::Gun::FireParams params;
								params.at_location_WS(targetWS);
								g->fire(params);
							}
						}
					}
				}
			}
			else
			{
				aim.shootBreakLengthTimeLeft -= _deltaTime;
				if (aim.shootBreakLengthTimeLeft <= 0.0f)
				{
					aim.shootLengthTimeLeft = aim.shootLength.is_empty() ? 0.0f : rg.get(aim.shootLength);
					aim.shootBreakLengthTimeLeft = 0.0f;

					aim.shootLeftTimeLeft = 0.0f;
					aim.shootRightTimeLeft = 0.0f;
				}
			}
		}
		else
		{
			aim.shootLengthTimeLeft = 0.0f;
			aim.shootBreakLengthTimeLeft = 0.001f; // to start shooting next
		}
	}
}

void PilgrimFake::read_animated()
{
	if (auto* imo = get_mind()->get_owner_as_modules_owner())
	{
		if (auto* a = imo->get_appearance())
		{
			if (auto* fs = a->get_skeleton())
			{
				if (auto* s = fs->get_skeleton())
				{
					int headIdx = s->find_bone_index(NAME(head));
					int handLeftIdx = s->find_bone_index(NAME(l_hand));
					int handRightIdx = s->find_bone_index(NAME(r_hand));

					auto* pose = a->get_final_pose_MS();

					if (headIdx != NONE)
					{
						animatedHead = a->get_ms_to_os_transform().to_world(pose->get_bone(headIdx));
						animatedHandLeft = a->get_ms_to_os_transform().to_world(pose->get_bone(handLeftIdx));
						animatedHandRight = a->get_ms_to_os_transform().to_world(pose->get_bone(handRightIdx));
					}
				}
			}
		}
	}
}

LATENT_FUNCTION(PilgrimFake::execute_logic)
{
	LATENT_SCOPE();

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(::Framework::AI::MessageHandler, messageHandler);

	auto* imo = mind->get_owner_as_modules_owner();
	auto* self = fast_cast<PilgrimFake>(logic);

	LATENT_BEGIN_CODE();

	{
		messageHandler.use_with(mind);
		{
			messageHandler.set(NAME(aim_lookAt), [self](Framework::AI::Message const& _message)
				{
					Framework::IModulesOwner* target = nullptr;
					Optional<Vector3> targetLocWS;
					if (auto* p = _message.get_param(NAME(target)))
					{
						if (auto* who = p->get_as<SafePtr<Framework::IModulesOwner>>().get())
						{
							target = who;
						}
					}
					if (auto* p = _message.get_param(NAME(targetLocWS)))
					{
						targetLocWS = p->get_as<Vector3>();
					}
					auto* imo = self->get_mind()->get_owner_as_modules_owner();
					if (targetLocWS.is_set())
					{
						self->lookAt.at.find_path(imo, imo->get_presence()->get_in_room(), Transform(targetLocWS.get(), Quat::identity));
					}
					else if (target)
					{
						self->lookAt.at.find_path(imo, target);
					}
					else
					{
						self->lookAt.at.reset();
					}

					self->lookAt.power = 0.8f;
					if (auto* p = _message.get_param(NAME(power)))
					{
						self->lookAt.power = p->get_as<float>();
					}

					self->lookAt.atSocket.clear();
					if (auto* p = _message.get_param(NAME(socket)))
					{
						self->lookAt.atSocket.set_name(p->get_as<Name>());
					}

					self->lookAt.offset = Vector3::zero;
					if (auto* p = _message.get_param(NAME(offset)))
					{
						self->lookAt.offset = p->get_as<Vector3>();
					}
				}
			);
			messageHandler.set(NAME(aim_noLookAt), [self](Framework::AI::Message const& _message)
				{
					self->lookAt.at.reset();
				}
			);
			messageHandler.set(NAME(aim_move), [self](Framework::AI::Message const& _message)
				{
					self->move.newOrder = true;
					self->move.move = true;

					self->move.speed.clear();
					if (auto* p = _message.get_param(NAME(speed)))
					{
						self->move.speed = p->get_as<float>();
					}

					self->move.gait.clear();
					if (auto* p = _message.get_param(NAME(gait)))
					{
						self->move.gait = p->get_as<Name>();
					}

					self->move.toPOI.clear();
					if (auto* p = _message.get_param(NAME(toPOI)))
					{
						self->move.toPOI = p->get_as<Name>();
					}
				}
			);
			messageHandler.set(NAME(aim_stop), [self](Framework::AI::Message const& _message)
				{
					self->move.newOrder = true;
					self->move.move = false;
				}
			);
			messageHandler.set(NAME(aim_hands), [self](Framework::AI::Message const& _message)
				{
					self->forceHandLeft.clear();
					self->forceHandRight.clear();
					if (auto* p = _message.get_param(NAME(left)))
					{
						self->forceHandLeft = p->get_as<Transform>();
					}
					if (auto* p = _message.get_param(NAME(right)))
					{
						self->forceHandRight = p->get_as<Transform>();
					}
				}
			);
			messageHandler.set(NAME(aim_aim), [self](Framework::AI::Message const& _message)
				{
					Framework::IModulesOwner* target = nullptr;
					Optional<Vector3> targetLocWS;
					if (auto* p = _message.get_param(NAME(target)))
					{
						if (auto* who = p->get_as<SafePtr<Framework::IModulesOwner>>().get())
						{
							target = who;
						}
					}
					if (auto* p = _message.get_param(NAME(targetLocWS)))
					{
						targetLocWS = p->get_as<Vector3>();
					}
					auto* imo = self->get_mind()->get_owner_as_modules_owner();
					if (targetLocWS.is_set())
					{
						self->aim.at.find_path(imo, imo->get_presence()->get_in_room(), Transform(targetLocWS.get(), Quat::identity));
					}
					else if (target)
					{
						if (self->aim.at.get_target() != target)
						{
							self->aim.at.find_path(self->get_mind()->get_owner_as_modules_owner(), target);
						}
					}
					else
					{
						self->aim.at.reset();
					}

					self->aim.offset = Vector3::zero;
					if (auto* p = _message.get_param(NAME(offset)))
					{
						self->aim.offset = p->get_as<Vector3>();
					}

					self->aim.shoot = false;
					if (auto* p = _message.get_param(NAME(shoot)))
					{
						self->aim.shoot = p->get_as<bool>();
					}

					self->aim.shootLength = Range::empty;
					if (auto* p = _message.get_param(NAME(shootLength)))
					{
						self->aim.shootLength = p->get_as<Range>();
					}

					self->aim.shootBreakLength = Range::empty;
					if (auto* p = _message.get_param(NAME(shootBreakLength)))
					{
						self->aim.shootBreakLength = p->get_as<Range>();
					}
				}
			);
		}
	}

	LATENT_WAIT(self->rg.get_float(0.05f, 0.1f));

	self->read_animated();

	self->defaultHead = self->animatedHead;
	self->defaultHandLeft = self->animatedHandLeft;
	self->defaultHandRight = self->animatedHandRight;

	if (auto* vrpp = imo->get_custom<Framework::CustomModules::VRPlacementsProvider>())
	{
		if (vrpp->get_head().is_set())
		{
			self->defaultHead = vrpp->get_head().get();
		}
		if (vrpp->get_hand_left().is_set())
		{
			self->defaultHandLeft = vrpp->get_hand_left().get();
		}
		if (vrpp->get_hand_right().is_set())
		{
			self->defaultHandRight = vrpp->get_hand_right().get();
		}
	}

	self->head = self->defaultHead;
	self->handLeft = self->defaultHandLeft;
	self->handRight = self->defaultHandRight;

	self->leftHandAIVar = NAME(l_hand_ai);
	self->rightHandAIVar = NAME(r_hand_ai);

	self->aim.aim_0_0_var = NAME(pose_attack_0_0);
	self->aim.aim_L45_0_var = NAME(pose_attack_l45_0);
	self->aim.aim_R45_0_var = NAME(pose_attack_r45_0);
	self->aim.aim_0_30_var = NAME(pose_attack_0_30);
	self->aim.aim_L45_30_var = NAME(pose_attack_l45_30);
	self->aim.aim_R45_30_var = NAME(pose_attack_r45_30);
	{
		auto& vars = imo->access_variables();
		self->leftHandAIVar.look_up<float>(vars);
		self->rightHandAIVar.look_up<float>(vars);
		self->aim.aim_0_0_var.look_up<float>(vars);
		self->aim.aim_L45_0_var.look_up<float>(vars);
		self->aim.aim_R45_0_var.look_up<float>(vars);
		self->aim.aim_0_30_var.look_up<float>(vars);
		self->aim.aim_L45_30_var.look_up<float>(vars);
		self->aim.aim_R45_30_var.look_up<float>(vars);
	}

	{
		imo->get_presence()->for_every_attached(
			[self](Framework::IModulesOwner* _imo)
			{
				if (auto* o = _imo->get_as_object())
				{
					if (o->get_tags().get_tag(NAME(leftWeapon)))
					{
						self->leftWeapon = _imo;
					}
					if (o->get_tags().get_tag(NAME(rightWeapon)))
					{
						self->rightWeapon = _imo;
					}
				}
			});
		
		for_count(int, i, 2)
		{
			if (auto* gunIMO = i == 0 ? self->leftWeapon.get() : self->rightWeapon.get())
			{
				if (auto* g = gunIMO->get_gameplay_as<ModuleEquipments::Gun>())
				{
					g->ex__update_stats();
					float shootInterval = g->get_chamber().as_float() / max(0.1f, g->get_storage_output_speed().as_float());
					float& storeInterval = i == 0 ? self->leftWeaponShootInterval : self->rightWeaponShootInterval;
					storeInterval = max(0.4f, shootInterval);
				}
			}
		}
	}

	self->started = true;

	while (true)
	{
		// hang in here
		LATENT_YIELD(); // unless we can wait lazily
	}

	LATENT_ON_BREAK();

	LATENT_END_CODE();
	LATENT_RETURN();
}

//

REGISTER_FOR_FAST_CAST(PilgrimFakeData);

bool PilgrimFakeData::load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	return result;
}
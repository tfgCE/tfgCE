#include "aiLogic_factoryArm.h"

#include "..\..\..\framework\ai\aiLatentTask.h"
#include "..\..\..\framework\ai\aiLogicWithLatentTask.h"
#include "..\..\..\framework\ai\aiMessageHandler.h"
#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\module\moduleAI.h"
#include "..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\framework\world\world.h"

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

// ai messages
DEFINE_STATIC_NAME_STR(aim_idle, TXT("factory arm; idle"));
DEFINE_STATIC_NAME_STR(aim_connect, TXT("factory arm; connect"));
	DEFINE_STATIC_NAME(toIMO);
	DEFINE_STATIC_NAME(toSocket); // socket or prefix
DEFINE_STATIC_NAME_STR(aim_operate, TXT("factory arm; operate"));
	DEFINE_STATIC_NAME(operateRandom);
DEFINE_STATIC_NAME_STR(aim_operatePilgrimTank, TXT("factory arm; operate pilgrim tank"));
	DEFINE_STATIC_NAME(pilgrimTank);

// ai messages out
DEFINE_STATIC_NAME_STR(aimIdle, TXT("pilgrim tank; idle"));
DEFINE_STATIC_NAME_STR(aimRunSync, TXT("pilgrim tank; run sync"));
DEFINE_STATIC_NAME_STR(aimRandomise, TXT("pilgrim tank; randomise"));
DEFINE_STATIC_NAME_STR(aimChangeBlinking, TXT("pilgrim tank; change blinking"));
DEFINE_STATIC_NAME_STR(aimPauseBlinking, TXT("pilgrim tank; pause blinking"));

//

REGISTER_FOR_FAST_CAST(FactoryArm);

FactoryArm::FactoryArm(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
{
	mark_requires_ready_to_activate();

	factoryArmData = fast_cast<FactoryArmData>(_logicData);
	an_assert(factoryArmData);
	handTipPlacementMSVar = factoryArmData->handTipPlacementMSVar;
	handTipIdlePlacementMSVar = factoryArmData->handTipIdlePlacementMSVar;
	handTipRotateSpeedVar = factoryArmData->handTipRotateSpeedVar;
	handTipRotateSpeedLengthVar = factoryArmData->handTipRotateSpeedLengthVar;
	handTipAtTargetVar = factoryArmData->handTipAtTargetVar;
}

FactoryArm::~FactoryArm()
{
}

void FactoryArm::ready_to_activate()
{
	base::ready_to_activate();

	if (auto* imo = get_mind()->get_owner_as_modules_owner())
	{
		handTipPlacementMSVar.look_up<Transform>(imo->access_variables());
		handTipIdlePlacementMSVar.look_up<Transform>(imo->access_variables());
		handTipRotateSpeedVar.look_up<Rotator3>(imo->access_variables());
		handTipRotateSpeedLengthVar.look_up<float>(imo->access_variables());
		handTipAtTargetVar.look_up<bool>(imo->access_variables());
	}
}

void FactoryArm::advance(float _deltaTime)
{
	set_auto_rare_advance_if_not_visible(Range(0.1f, 0.5f));

	rotateSpeed = blend_to_using_time(rotateSpeed, rotateSpeedTarget, factoryArmData->handTipRotateSpeedBlendTime, _deltaTime);
	if (handTipRotateSpeedVar.is_valid())
	{
		handTipRotateSpeedVar.access<Rotator3>() = Rotator3(0.0f, 0.0, rotateSpeed);
	}
	if (handTipRotateSpeedLengthVar.is_valid())
	{
		handTipRotateSpeedLengthVar.access<float>() = abs(rotateSpeed);
	}

	base::advance(_deltaTime);
}

bool FactoryArm::is_rotate_speed_at_target() const
{
	return abs(rotateSpeed - rotateSpeedTarget) <= 1.0f;
}

bool FactoryArm::is_hand_tip_at_target() const
{
	if (handTipAtTargetVar.is_valid())
	{
		return handTipAtTargetVar.get<bool>();
	}
	return true;
}

void FactoryArm::start_movement(Transform const & _to)
{
	targetPlacement = _to;
	handTipPlacementMSVar.access<Transform>() = targetPlacement;
	if (handTipAtTargetVar.is_valid())
	{
		handTipAtTargetVar.access<bool>() = false; // appearance controller will update
	}
}

LATENT_FUNCTION(FactoryArm::execute_logic)
{
	LATENT_SCOPE();

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();
	
	LATENT_VAR(Random::Generator, rg);
	
	LATENT_VAR(int, rotationsLeft);
	LATENT_VAR(float, rotationDir);
	LATENT_VAR(Transform, targetPlacement);
	LATENT_VAR(Transform, idlePlacement);
	LATENT_VAR(Transform, connectPlacement);
	LATENT_VAR(Transform, offsetTransform);
	LATENT_VAR(SafePtr<Framework::IModulesOwner>, connectToIMO);
	LATENT_VAR(Name, connectToSocket);
	LATENT_VAR(SafePtr<Framework::IModulesOwner>, operateIMO);
	LATENT_VAR(bool, operateRandom);
	LATENT_VAR(bool, operatePilgrimTank);
	LATENT_VAR(SafePtr<Framework::IModulesOwner>, pilgrimTank);
	LATENT_VAR(Name, operateSocket);
	LATENT_VAR(Array<Name>, operateSockets);

	LATENT_VAR(Name, operation);

	LATENT_VAR(bool, requiresMoveBack);

	LATENT_VAR(::Framework::AI::MessageHandler, messageHandler);

	auto* self = fast_cast<FactoryArm>(logic);
	auto* imo = mind->get_owner_as_modules_owner();

	LATENT_BEGIN_CODE();

	self->targetPlacement = self->handTipPlacementMSVar.get<Transform>();
	targetPlacement = self->targetPlacement;

	if (self->handTipIdlePlacementMSVar.is_valid())
	{
		idlePlacement = self->handTipIdlePlacementMSVar.get<Transform>();
	}
	else
	{
		idlePlacement = targetPlacement;
	}
	
	offsetTransform = Transform(Vector3(0.0f, -self->factoryArmData->handTipTargetOffset, 0.0f), Quat::identity);

	rotationsLeft = 0;
	operation = NAME(aim_idle);

	messageHandler.use_with(mind);
	{
		auto* framePtr = &_frame;
		messageHandler.set(NAME(aim_idle), [framePtr, mind, &operation](Framework::AI::Message const& _message)
		{
			operation = NAME(aim_idle);
			framePtr->end_waiting();
			AI_LATENT_STOP_LONG_RARE_ADVANCE();
		});
		messageHandler.set(NAME(aim_operatePilgrimTank), [framePtr, mind, &operatePilgrimTank, &pilgrimTank](Framework::AI::Message const& _message)
		{
			operatePilgrimTank = true;
			pilgrimTank.clear();
			if (auto* connectToIMOPtr = _message.get_param(NAME(pilgrimTank)))
			{
				auto* imo = connectToIMOPtr->get_as< SafePtr<Framework::IModulesOwner> >().get();
				if (imo)
				{
					pilgrimTank = imo;
				}
			}
			framePtr->end_waiting();
			AI_LATENT_STOP_LONG_RARE_ADVANCE();
		});
		messageHandler.set(NAME(aim_operate), [framePtr, mind, &operation, &operateIMO, &operateSocket, &operateSockets, &operateRandom](Framework::AI::Message const& _message)
		{
			operation = NAME(aim_operate);
			operateIMO.clear();
			operateSocket = Name::invalid();
			operateSockets.clear();
			if (auto* connectToIMOPtr = _message.get_param(NAME(toIMO)))
			{
				auto* imo = connectToIMOPtr->get_as< SafePtr<Framework::IModulesOwner> >().get();
				if (imo)
				{
					operateIMO = imo;
				}
			}
			if (auto* connectToSocketPtr = _message.get_param(NAME(toSocket)))
			{
				operateSocket = connectToSocketPtr->get_as<Name>();
			}
			operateRandom = false;
			if (auto* operateRandomPtr = _message.get_param(NAME(operateRandom)))
			{
				operateRandom = operateRandomPtr->get_as<bool>();
			}
			operateRandom = false;
			if (auto* operateRandomPtr = _message.get_param(NAME(operateRandom)))
			{
				operateRandom = operateRandomPtr->get_as<bool>();
			}
			framePtr->end_waiting();
			AI_LATENT_STOP_LONG_RARE_ADVANCE();
		});
		messageHandler.set(NAME(aim_connect), [framePtr, mind, &operation, &connectToIMO, &connectToSocket](Framework::AI::Message const& _message)
		{
			operation = NAME(aim_connect);
			connectToIMO.clear();
			connectToSocket = Name::invalid();
			if (auto* connectToIMOPtr = _message.get_param(NAME(toIMO)))
			{
				auto* imo = connectToIMOPtr->get_as< SafePtr<Framework::IModulesOwner> >().get();
				if (imo)
				{
					connectToIMO = imo;
				}
			}
			if (auto* connectToSocketPtr = _message.get_param(NAME(toSocket)))
			{
				connectToSocket = connectToSocketPtr->get_as<Name>();
			}
			framePtr->end_waiting();
			AI_LATENT_STOP_LONG_RARE_ADVANCE();
		});
	}
	
	while (true)
	{
		MAIN:
			if (operation == NAME(aim_idle))
			{
				rotationsLeft = 0;
				goto MOVE_ARM_TO_IDLE;
			}
			if (operation == NAME(aim_connect))
			{
				rotationsLeft = 0;
				goto MOVE_ARM_TO_CONNECT;
			}
			if (operation == NAME(aim_operate))
			{
				if (operateSocket.is_valid() &&
					operateSockets.is_empty())
				{
					if (auto* oimo = operateIMO.get())
					{
						if (auto* ap = oimo->get_appearance())
						{
							ap->for_every_socket_starting_with(operateSocket, [&operateSockets](Name const& _socket) {operateSockets.push_back(_socket); });
						}
					}
				}
				if (rotationsLeft > 0)
				{
					goto ROTATE;
				}
				goto MOVE_ARM;
			}
			LATENT_YIELD();
			goto MAIN;

		ROTATE:
			--rotationsLeft;
			rotationDir = -rotationDir;
			self->rotateSpeedTarget = rotationDir * rg.get(self->factoryArmData->handTipRotateSpeed);

			while (!self->is_rotate_speed_at_target())
			{
				LATENT_WAIT(rg.get_float(0.1f, 0.3f));
			}

			if (operatePilgrimTank && pilgrimTank.get())
			{
				imo->get_in_world()->create_ai_message(NAME(aimChangeBlinking))->to_ai_object(pilgrimTank.get());
			}

			if (operation == NAME(aim_operate))
			{
				LATENT_WAIT(rg.get(self->factoryArmData->rotateTime));
			}

			self->rotateSpeedTarget = 0.0f;

			while (!self->is_rotate_speed_at_target())
			{
				LATENT_WAIT(rg.get_float(0.1f, 0.3f));
			}
			
			self->rotateSpeed = 0.0f;

			goto MAIN;

		MOVE_ARM_TO_IDLE:
			if (operatePilgrimTank && pilgrimTank.get())
			{
				imo->get_in_world()->create_ai_message(NAME(aimIdle))->to_ai_object(pilgrimTank.get());
			}
			if (! Transform::same_with_orientation(targetPlacement, idlePlacement))
			{
				targetPlacement = idlePlacement;
				self->start_movement(targetPlacement);
			}
			LATENT_WAIT(rg.get_float(0.1f, 0.3f));
			goto MAIN;

		MOVE_ARM_TO_CONNECT:
			connectPlacement = idlePlacement;
			if (auto* cimo = connectToIMO.get())
			{
				Transform placementWS = cimo->get_presence()->get_placement();
				if (connectToSocket.is_valid())
				{
					if (auto* ap = cimo->get_appearance())
					{
						placementWS = placementWS.to_world(ap->calculate_socket_os(connectToSocket));
					}
				}
				if (auto* ap = imo->get_appearance())
				{
					connectPlacement = ap->get_ms_to_ws_transform().to_local(placementWS);
				}
			}
			{
				targetPlacement = connectPlacement.to_world(offsetTransform);
				self->start_movement(targetPlacement);
			}
			while (!self->is_hand_tip_at_target())
			{
				LATENT_WAIT(rg.get_float(0.1f, 0.3f));
				if (operation != NAME(aim_connect)) { goto MAIN; }
			}
			{
				targetPlacement = connectPlacement;
				self->start_movement(targetPlacement);
			}
			while (!self->is_hand_tip_at_target())
			{
				LATENT_WAIT(rg.get_float(0.1f, 0.3f));
			}
			while (operation == NAME(aim_connect))
			{
				LATENT_WAIT(rg.get_float(0.1f, 0.3f));
			}
			{
				targetPlacement = connectPlacement.to_world(offsetTransform);
				self->start_movement(targetPlacement);
			}
			while (!self->is_hand_tip_at_target())
			{
				LATENT_WAIT(rg.get_float(0.1f, 0.3f));
			}
			goto MAIN;

		MOVE_ARM:
			if (requiresMoveBack)
			{
				targetPlacement = targetPlacement.to_world(offsetTransform);
				self->start_movement(targetPlacement);
				if (operatePilgrimTank && pilgrimTank.get())
				{
					imo->get_in_world()->create_ai_message(NAME(aimPauseBlinking))->to_ai_object(pilgrimTank.get());
				}
				while (!self->is_hand_tip_at_target())
				{
					LATENT_WAIT(rg.get_float(0.1f, 0.3f));
					if (operation != NAME(aim_operate)) { goto MAIN; }
				}
				requiresMoveBack = false;
			}
			if (operateRandom)
			{
				Vector3 atOS = Vector3(rg.get_float(-0.5f, 0.5f), rg.get_float(0.3f, 1.0f), 0.0f);
				atOS = atOS.normal() * rg.get_float(1.0f, 2.0f);
				atOS.x = clamp(atOS.x, -2.0f, 2.0f);
				atOS.y = 2.0f;
				atOS.z = rg.get_float(0.5f, 1.5f);
				Vector3 dir = Vector3::yAxis;
				dir.x += rg.get_float(-0.3f, 0.3f);
				dir.y += rg.get_float(-0.3f, 0.3f);
				dir.z += rg.get_float(-0.3f, 0.3f);
				dir = dir.normal();
				targetPlacement = look_matrix(atOS, dir.normal(), Vector3::zAxis).to_transform();
			}
			else
			{
				if (auto* oimo = operateIMO.get())
				{
					Transform placementWS = oimo->get_presence()->get_placement();
					if (!operateSockets.is_empty())
					{
						if (auto* ap = oimo->get_appearance())
						{
							placementWS = placementWS.to_world(ap->calculate_socket_os(operateSockets[rg.get_int(operateSockets.get_size())]));

							placementWS = look_matrix(placementWS.get_translation(), placementWS.vector_to_world(-Vector3::zAxis), Vector3::zAxis).to_transform();
						}
					}
					if (auto* ap = imo->get_appearance())
					{
						targetPlacement = ap->get_ms_to_ws_transform().to_local(placementWS);
					}
				}
				else
				{
					targetPlacement = idlePlacement;
				}
			}
			self->start_movement(targetPlacement.to_world(offsetTransform));
			while (!self->is_hand_tip_at_target())
			{
				LATENT_WAIT(rg.get_float(0.1f, 0.3f));
				if (operation != NAME(aim_operate)) { goto MAIN; }
			}
			self->start_movement(targetPlacement);
			while (!self->is_hand_tip_at_target())
			{
				LATENT_WAIT(rg.get_float(0.1f, 0.3f));
				if (operation != NAME(aim_operate)) { goto MAIN; }
			}
			requiresMoveBack = self->is_hand_tip_at_target();

			rotationsLeft = self->factoryArmData->rotateCount.get(rg);
			rotationDir = rg.get_bool() ? -1.0f : 1.0f;
			if (operatePilgrimTank && pilgrimTank.get())
			{
				imo->get_in_world()->create_ai_message(NAME(aimRunSync))->to_ai_object(pilgrimTank.get());
				imo->get_in_world()->create_ai_message(NAME(aimRandomise))->to_ai_object(pilgrimTank.get());
			}
			goto MAIN;
	}

	LATENT_ON_BREAK();
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}

//

REGISTER_FOR_FAST_CAST(FactoryArmData);

bool FactoryArmData::load_from_xml(IO::XML::Node const* _node, ::Framework::LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	handTipPlacementMSVar = _node->get_name_attribute_or_from_child(TXT("handTipPlacementMSVarID"), handTipPlacementMSVar);
	handTipIdlePlacementMSVar = _node->get_name_attribute_or_from_child(TXT("handTipIdlePlacementMSVarID"), handTipIdlePlacementMSVar);
	handTipRotateSpeedVar = _node->get_name_attribute_or_from_child(TXT("handTipRotateSpeedVarID"), handTipRotateSpeedVar);
	handTipRotateSpeedLengthVar = _node->get_name_attribute_or_from_child(TXT("handTipRotateSpeedLengthVarID"), handTipRotateSpeedLengthVar);
	handTipAtTargetVar = _node->get_name_attribute_or_from_child(TXT("handTipAtTargetVarID"), handTipAtTargetVar);
	result &= rotateCount.load_from_xml(_node, TXT("rotateCount"));
	result &= rotateTime.load_from_xml(_node, TXT("rotateTime"));
	result &= handTipRotateSpeed.load_from_xml(_node, TXT("handTipRotateSpeed"));
	handTipRotateSpeedBlendTime = _node->get_float_attribute_or_from_child(TXT("handTipRotateSpeedBlendTime"), handTipRotateSpeedBlendTime);

	handTipTargetOffset = _node->get_float_attribute_or_from_child(TXT("handTipTargetOffset"), handTipTargetOffset);

	error_loading_xml_on_assert(handTipPlacementMSVar.is_valid(), result, _node, TXT("missing \"handTipPlacementMSVarID\""));
	
	return result;
}

bool FactoryArmData::prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}
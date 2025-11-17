#include "aiLogic_doorDevice.h"

#include "..\..\utils.h"

#include "..\..\game\game.h"
#include "..\..\game\modes\gameMode_Pilgrimage.h"
#include "..\..\modules\custom\mc_emissiveControl.h"
#include "..\..\modules\custom\mc_powerSource.h"
#include "..\..\pilgrimage\pilgrimageInstanceLinear.h"

#include "..\..\..\core\system\video\video3dPrimitives.h"

#include "..\..\..\framework\ai\aiMessageHandler.h"
#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\framework\module\moduleMovementSwitch.h"
#include "..\..\..\framework\module\moduleTemporaryObjects.h"
#include "..\..\..\framework\object\actor.h"
#include "..\..\..\framework\world\door.h"
#include "..\..\..\framework\world\doorInRoom.h"
#include "..\..\..\framework\world\environment.h"
#include "..\..\..\framework\world\room.h"

#ifdef AN_CLANG
#include "..\..\..\framework\library\usedLibraryStored.inl"
#endif

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
DEFINE_STATIC_NAME_STR(aim_scripted, TXT("scripted"));
	DEFINE_STATIC_NAME(action);
		DEFINE_STATIC_NAME_STR(releaseSteam, TXT("release steam"));
			DEFINE_STATIC_NAME(time);
		DEFINE_STATIC_NAME(open);

// parameters
DEFINE_STATIC_NAME(controlDeviceId);

DEFINE_STATIC_NAME(inOpen);
DEFINE_STATIC_NAME(outOpen);
DEFINE_STATIC_NAME(testDeviceEnable);
DEFINE_STATIC_NAME(testDeviceDisable);

// temporary objects
DEFINE_STATIC_NAME(steam);

//

REGISTER_FOR_FAST_CAST(DoorDevice);

DoorDevice::DoorDevice(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
, doorDeviceData(fast_cast<DoorDeviceData>(_logicData))
{
	openVarID = NAME(inOpen);
	actualOpenVarID = NAME(outOpen);

	if (doorDeviceData && doorDeviceData->openVarID.is_valid())
	{
		openVarID = doorDeviceData->openVarID;
	}
	if (doorDeviceData && doorDeviceData->actualOpenVarID.is_valid())
	{
		actualOpenVarID = doorDeviceData->actualOpenVarID;
	}
}

DoorDevice::~DoorDevice()
{
}

void DoorDevice::advance(float _deltaTime)
{
	timeSinceDoorClosed += _deltaTime;
	base::advance(_deltaTime);
}

void DoorDevice::learn_from(SimpleVariableStorage & _parameters)
{
	base::learn_from(_parameters);

	isValid = true;
}

void DoorDevice::set_operation(DoorDeviceOperation::Type _operation)
{
#ifdef AN_USE_AI_LOG
	if (operation != _operation)
	{
		ai_log(this, TXT("set operation %S -> %S"), DoorDeviceOperation::to_char(operation), DoorDeviceOperation::to_char(_operation));
	}
#endif
	operation = _operation;
}

LATENT_FUNCTION(DoorDevice::do_steam)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("do steam"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto
	LATENT_PARAM(::Framework::AI::Logic*, logic);
	LATENT_END_PARAMS();

	auto * self = fast_cast<DoorDevice>(logic);

	LATENT_BEGIN_CODE();

	if (auto* object = mind->get_owner_as_modules_owner()->get_as_object())
	{
		if (auto* to = object->get_temporary_objects())
		{
			to->spawn_all(NAME(steam), NP, &self->steamObjects);
		}
	}

	LATENT_WAIT(self->releaseSteamTime);
	{
		for_every_ref(so, self->steamObjects)
		{
			if (auto* to = fast_cast<Framework::TemporaryObject>(so))
			{
				to->desire_to_deactivate();
			}
		}
		self->steamObjects.clear();
	}
	LATENT_WAIT(0.5f);

	LATENT_ON_BREAK();
	{
		// deactivate if broken
		for_every_ref(so, self->steamObjects)
		{
			if (auto* to = fast_cast<Framework::TemporaryObject>(so))
			{
				to->desire_to_deactivate();
			}
		}
		self->steamObjects.clear();
	}

	//

	LATENT_END_CODE();
	LATENT_RETURN();
}

LATENT_FUNCTION(DoorDevice::do_door_op)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("do door op"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto
	LATENT_PARAM(::Framework::AI::Logic*, logic);
	LATENT_PARAM(DoorDeviceOperation::Type, currentOperation);
	LATENT_END_PARAMS();

	LATENT_VAR(float, waitToOpen);
	LATENT_VAR(bool, firstCall);

	auto * self = fast_cast<DoorDevice>(logic);

	waitToOpen += LATENT_DELTA_TIME;

	LATENT_BEGIN_CODE();

	ai_log_colour(self, Colour::blue);
	ai_log(self, TXT("start door op %S"), DoorDeviceOperation::to_char(currentOperation));
	ai_log_no_colour(self);

	if (currentOperation == DoorDeviceOperation::ReadyToOpen)
	{
		LATENT_WAIT(1.0f);
		// launch steam task separately, so opening doors won't stop it
		{
			ai_log(self, TXT("do steam"));
			::Framework::AI::LatentTaskInfoWithParams steamTaskInfo;
			steamTaskInfo.clear();
			steamTaskInfo.propose(AI_LATENT_TASK_FUNCTION(do_steam));
			ADD_LATENT_TASK_INFO_PARAM(steamTaskInfo, ::Framework::AI::Logic*, logic);
			self->steamTask.start_latent_task(mind, steamTaskInfo);
		}
		//
		ai_log(self, TXT("wait for steam task"));
		waitToOpen = 0.0f;
		while (waitToOpen < 1.5f || self->steamTask.is_running())
		{
			LATENT_YIELD();
		}
		ai_log(self, TXT("steam task done"));
	}
	else if (currentOperation == DoorDeviceOperation::Open)
	{
		if (currentOperation == DoorDeviceOperation::Open)
		{
			// open immediately
			ai_log(self, TXT("open immediately"));
			if (auto* door = self->door.get())
			{
				door->set_mute(false); // let it be heard
				door->set_operation(Framework::DoorOperation::StayOpen);
			}
			ai_log(self, TXT("wait for door"));
			while (self->door.get() && self->door.get()->get_abs_open_factor() < 1.0f)
			{
				LATENT_YIELD();
			}
			ai_log(self, TXT("door open"));
		}
		self->doorState = DoorDeviceDoorState::Opening;

		// mark it is open sooner 
		ai_log(self, TXT("open blinds and door"));
		firstCall = true;
		while ((self->door.get() && self->door.get()->get_abs_open_factor() < magic_number 0.7f) ||
			   (self->actualOpenVar.is_valid() && self->actualOpenVar.get<float>() < magic_number 0.7f) ||
				firstCall)
		{
			firstCall = false;
			// keep/force
			if (auto* door = self->door.get())
			{
				door->set_operation(Framework::DoorOperation::StayOpen);
			}
			if (self->openVar.is_valid())
			{
				self->openVar.access<float>() = 1.0f;
			}
			LATENT_YIELD();
		}
		self->doorState = DoorDeviceDoorState::Open;
		ai_log(self, TXT("open - done"));
	}
	else if (currentOperation == DoorDeviceOperation::Close)
	{
		{
			// close blinds first, then door
			ai_log(self, TXT("close blinds first,"));
			if (self->openVar.is_valid())
			{
				self->openVar.access<float>() = 0.0f;
			}
			//
			ai_log(self, TXT("wait for door to close"));
			while (self->actualOpenVar.is_valid() && self->actualOpenVar.get<float>() > 0.0f)
			{
				self->doorState = DoorDeviceDoorState::Closing;
				LATENT_YIELD();
			}
			if (auto* door = self->door.get())
			{
				door->set_mute(true); // behind blinds
			}
			ai_log(self, TXT("door close"));
		}

		// make sure it is closed
		ai_log(self, TXT("close blinds and door"));
		firstCall = true;
		while ((self->door.get() && self->door.get()->can_be_closed() && self->door.get()->get_abs_open_factor() > 0.0f) ||
			   (self->actualOpenVar.is_valid() && self->actualOpenVar.get<float>() > 0.0f) ||
				firstCall)
		{
			if (!firstCall)
			{
				self->doorState = DoorDeviceDoorState::Closing;
			}
			firstCall = false;
			// keep/force
			if (auto* door = self->door.get())
			{
				door->set_operation(Framework::DoorOperation::StayClosed);
			}
			if (self->openVar.is_valid())
			{
				self->openVar.access<float>() = 0.0f;
			}
			LATENT_YIELD();
		}
		self->doorState = DoorDeviceDoorState::Closed;
		self->timeSinceDoorClosed = 0.0f;
		ai_log(self, TXT("close - done"));
	}

	LATENT_ON_BREAK();
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}

LATENT_FUNCTION(DoorDevice::execute_logic)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("execute logic"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(::Framework::AI::MessageHandler, messageHandler);
	LATENT_VAR(::Framework::AI::LatentTaskHandle, operationTask);
	LATENT_VAR(DoorDeviceOperation::Type, currentOperation);

	auto * self = fast_cast<DoorDevice>(logic);

	LATENT_BEGIN_CODE();

	currentOperation = DoorDeviceOperation::None;
	self->operation = DoorDeviceOperation::None;

	messageHandler.use_with(mind);
	{
#ifdef AN_DEVELOPMENT_OR_PROFILER
		messageHandler.set(NAME(testDeviceEnable), [self](Framework::AI::Message const & _message)
		{
			self->test_mode();
			self->set_operation(DoorDeviceOperation::Open);
		}
		);
		messageHandler.set(NAME(testDeviceDisable), [self](Framework::AI::Message const & _message)
		{
			self->test_mode();
			self->set_operation(DoorDeviceOperation::Close);
		}
		);
#endif
		messageHandler.set(NAME(aim_scripted), [self](Framework::AI::Message const& _message)
			{
#ifdef AN_DEVELOPMENT_OR_PROFILER
				ai_log(self, TXT("scripted ai message received"));
#endif
				if (auto* ptr = _message.get_param(NAME(action)))
				{
					Name action = ptr->get_as<Name>();
#ifdef AN_DEVELOPMENT_OR_PROFILER
					ai_log(self, TXT("action: \"%S\""), action.to_char());
#endif

					if (action == NAME(releaseSteam))
					{
						self->set_operation(DoorDeviceOperation::ReadyToOpen);
						if (auto* p = _message.get_param(NAME(time)))
						{
							self->releaseSteamTime = max(1.0f, p->get_as<float>());
						}
					}

					if (action == NAME(open))
					{
						self->set_operation(DoorDeviceOperation::Open);
					}
				}
			}
		);
	}
	
	if (auto* doorDevice = self->get_mind()->get_owner_as_modules_owner())
	{
		auto & vars = doorDevice->access_variables();
		if (!self->openVar.is_valid())
		{
			self->openVar = vars.find<float>(self->openVarID);
		}
		if (!self->actualOpenVar.is_valid())
		{
			self->actualOpenVar = vars.find<float>(self->actualOpenVarID);
		}
	}

	LATENT_WAIT(Random::get_float(0.1f, 0.6f));

	while (true)
	{
		if (!self->door.is_set())
		{
			if (auto* imo = mind->get_owner_as_modules_owner())
			{
				if (auto* room = imo->get_presence()->get_in_room())
				{
					Transform doorDevicePlacement = imo->get_presence()->get_placement();
					for_every_ptr(door, room->get_all_doors())
					{
						// invisible doors are fine here
						if (Transform::same_with_orientation(doorDevicePlacement, door->get_placement()))
						{
							self->door = door->get_door();
							self->newDoor = true;
							{
								ai_log_colour(self, Colour::green);
								ai_log(self, TXT("start info"));
								ai_log_increase_indent(self);
								if (door->get_door()->get_linked_door_a() && door->get_door()->get_linked_door_a()->get_in_room())
								{
									ai_log(self, TXT("door a : %S (%i)"), door->get_door()->get_linked_door_a()->get_in_room()->get_name().to_char(), door->get_door()->get_linked_door_a()->get_in_room());
								}
								if (door->get_door()->get_linked_door_b() && door->get_door()->get_linked_door_b()->get_in_room())
								{
									ai_log(self, TXT("door b : %S (%i)"), door->get_door()->get_linked_door_b()->get_in_room()->get_name().to_char(), door->get_door()->get_linked_door_b()->get_in_room());
								}
								ai_log_decrease_indent(self);
								ai_log_no_colour(self);
							}
							break;
						}
					}
				}
			}
		}
		if (currentOperation != self->operation)
		{
			operationTask.stop();

			currentOperation = self->operation;

			if (currentOperation != DoorDeviceOperation::None)
			{
				ai_log_colour(self, Colour::red);
				ai_log(self, TXT("do door op %S"), DoorDeviceOperation::to_char(currentOperation));
				ai_log_no_colour(self);
				::Framework::AI::LatentTaskInfoWithParams operationTaskInfo;
				operationTaskInfo.clear();
				operationTaskInfo.propose(AI_LATENT_TASK_FUNCTION(do_door_op));
				ADD_LATENT_TASK_INFO_PARAM(operationTaskInfo, ::Framework::AI::Logic*, logic);
				ADD_LATENT_TASK_INFO_PARAM(operationTaskInfo, DoorDeviceOperation::Type, currentOperation);
				operationTask.start_latent_task(mind, operationTaskInfo);
			}
		}
		LATENT_YIELD();
	}

	LATENT_ON_BREAK();
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}

//

REGISTER_FOR_FAST_CAST(DoorDeviceData);

DoorDeviceData::DoorDeviceData()
{
	idVar = NAME(controlDeviceId);
}

DoorDeviceData::~DoorDeviceData()
{
}

bool DoorDeviceData::load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	idVar = _node->get_name_attribute_or_from_child(TXT("idVar"), idVar);

	openVarID = _node->get_name_attribute_or_from_child(TXT("openVarID"), openVarID);
	actualOpenVarID = _node->get_name_attribute_or_from_child(TXT("actualOpenVarID"), actualOpenVarID);

	return result;
}

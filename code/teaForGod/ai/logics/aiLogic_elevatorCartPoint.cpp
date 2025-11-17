#include "aiLogic_elevatorCartPoint.h"

#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\framework\object\actor.h"

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

DEFINE_STATIC_NAME(cart);
DEFINE_STATIC_NAME(platform);
DEFINE_STATIC_NAME(elevatorCartPoint);
DEFINE_STATIC_NAME(open);
DEFINE_STATIC_NAME(close);
DEFINE_STATIC_NAME(atCartPoint);
DEFINE_STATIC_NAME(noLongerAtCartPoint);
DEFINE_STATIC_NAME(doorOpen);
DEFINE_STATIC_NAME(doorOpenTime);
DEFINE_STATIC_NAME(doorMovementCollisionOpen);
DEFINE_STATIC_NAME(beAware);
DEFINE_STATIC_NAME(beLazy);

//

REGISTER_FOR_FAST_CAST(ElevatorCartPoint);

ElevatorCartPoint::ElevatorCartPoint(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData)
{
	doorOpenACVarID = NAME(doorOpen);
	doorMovementCollisionOpenACVarID = NAME(doorMovementCollisionOpen);

	_mind->access_execution().register_message_handler(NAME(open), this);
	_mind->access_execution().register_message_handler(NAME(close), this);
	_mind->access_execution().register_message_handler(NAME(beAware), this);
	_mind->access_execution().register_message_handler(NAME(beLazy), this);
	_mind->access_execution().register_message_handler(NAME(atCartPoint), this);
	_mind->access_execution().register_message_handler(NAME(noLongerAtCartPoint), this);
}

ElevatorCartPoint::~ElevatorCartPoint()
{
	get_mind()->access_execution().unregister_message_handler(NAME(open), this);
	get_mind()->access_execution().unregister_message_handler(NAME(close), this);
	get_mind()->access_execution().unregister_message_handler(NAME(beAware), this);
	get_mind()->access_execution().unregister_message_handler(NAME(beLazy), this);
	get_mind()->access_execution().unregister_message_handler(NAME(atCartPoint), this);
	get_mind()->access_execution().unregister_message_handler(NAME(noLongerAtCartPoint), this);
}

void ElevatorCartPoint::handle_message(::Framework::AI::Message const & _message)
{
	if (_message.get_name() == NAME(open) ||
		_message.get_name() == NAME(close))
	{
		if (_message.get_name() == NAME(open))
		{
			++openCount;
		}
		if (_message.get_name() == NAME(close))
		{
			--openCount;
		}
		if (auto doorOpenTimeParam = _message.get_param(NAME(doorOpenTime)))
		{
			doorOpenTime = doorOpenTimeParam->get_as<float>();
		}
	}

	if (_message.get_name() == NAME(atCartPoint))
	{
		++atCartPointCount;
	}
	if (_message.get_name() == NAME(noLongerAtCartPoint))
	{
		--atCartPointCount;
	}

	if (_message.get_name() == NAME(beAware))
	{
		beAware = true;
	}
	if (_message.get_name() == NAME(beLazy))
	{
		beAware = false;
	}

	// react!
	set_advance_logic_rarely(Range::empty);
}

void ElevatorCartPoint::advance(float _deltaTime)
{
	base::advance(_deltaTime);

	if (isValid)
	{
		float doorOpenTarget = openCount ? 1.0f : 0.0f;
		if (doorOpenTarget != doorOpen)
		{
			keepNonRareUpdateFor = 0.2f; // to update collisions
		}
		else if (keepNonRareUpdateFor.is_set())
		{
			keepNonRareUpdateFor = max(0.0f, keepNonRareUpdateFor.get() - _deltaTime);
			if (keepNonRareUpdateFor.get() <= 0.0f)
			{
				keepNonRareUpdateFor.clear();
			}
		}

		if (keepNonRareUpdateFor.is_set())
		{
			// every frame, both, to move doors
			set_advance_logic_rarely(Range::empty);
			set_advance_appearance_rarely(Range::empty);
		}
		else
		{
			// nothing changes, it can be at any rare rate
			if (beAware)
			{
				// a bit more often if in visible room
				set_advance_logic_rarely(Range(0.1f, 0.3f));
				set_advance_appearance_rarely(Range(0.1f, 0.3f));
			}
			else
			{
				set_advance_logic_rarely(NP); // as usual
				set_advance_appearance_rarely(Range(0.5f, 2.0f)); // nothing changes, no need to update more often
			}
		}

		if (auto* cartPoint = get_mind()->get_owner_as_modules_owner())
		{
			doorOpen = blend_to_using_speed_based_on_time(doorOpen, doorOpenTarget, doorOpenTime, _deltaTime);

			if (auto * appearance = cartPoint->get_appearance())
			{
				auto & acVars = appearance->access_controllers_variable_storage();
				if (!doorOpenACVar.is_valid())
				{
					doorOpenACVar = acVars.find<float>(doorOpenACVarID);
				}
				if (!doorMovementCollisionOpenACVar.is_valid())
				{
					doorMovementCollisionOpenACVar = acVars.find<float>(doorMovementCollisionOpenACVarID);
				}

				doorOpenACVar.access<float>() = doorOpen;
				doorMovementCollisionOpenACVar.access<float>() = (doorOpen > 0.99f || (doorOpen > 0.3f && doorOpenTarget > 0.5f)) && atCartPointCount ? 1.0f : 0.0f;
			}
		}
	}
}

void ElevatorCartPoint::learn_from(SimpleVariableStorage & _parameters)
{
	base::learn_from(_parameters);

	isValid = true;
}

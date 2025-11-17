#include "aiManagerLogic_doorFalling.h"

#include "..\..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\..\framework\ai\aiMessageHandler.h"
#include "..\..\..\..\framework\module\moduleAI.h"
#include "..\..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\..\framework\module\custom\mc_lcdLights.h"
#include "..\..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\..\framework\world\door.h"
#include "..\..\..\..\framework\world\doorInRoom.h"
#include "..\..\..\..\framework\world\room.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//#define AUTO_TEST

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;
using namespace Managers;

//

// ai messages
DEFINE_STATIC_NAME_STR(aim_start, TXT("door falling; start"));
DEFINE_STATIC_NAME(door);
DEFINE_STATIC_NAME(inRoom);
DEFINE_STATIC_NAME(gravity);
DEFINE_STATIC_NAME(maxVerticalVelocity);
DEFINE_STATIC_NAME(fallingOrientation);
DEFINE_STATIC_NAME(fallingOrientationAcceleration);
DEFINE_STATIC_NAME(fallingOrientationMaxVelocity);

//

REGISTER_FOR_FAST_CAST(DoorFallingManager);

DoorFallingManager::DoorFallingManager(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
{
}

DoorFallingManager::~DoorFallingManager()
{
}

void DoorFallingManager::learn_from(SimpleVariableStorage& _parameters)
{
	base::learn_from(_parameters);


}
	
void DoorFallingManager::advance(float _deltaTime)
{
	base::advance(_deltaTime);

	if (auto* d = fallingDoor.get())
	{
		if (!currentPlacement.is_set())
		{
			initialPlacement = d->get_placement();
			currentPlacement = initialPlacement;
			currentRelOrientation = Rotator3::zero;
			verticalVelocity = 0.0f;
			orientationVelocity = Rotator3::zero;
		}

		verticalVelocity -= gravity * _deltaTime;
		if (maxVerticalVelocity != 0.0f)
		{
			verticalVelocity = max(verticalVelocity, -maxVerticalVelocity);
		}

		struct AdvanceSpeed
		{
			static void advance(float const pos, float const target, float const maxDiff, REF_ float& speed, float const acceleration, float const maxSpeed, float const _deltaTime)
			{
				// s = v * t - a * t^2 / 2
				// v1 = v0 - a * t
				// v0 - v1 = a * t
				// t = (v0 - v1) / a

				if (acceleration == 0.0f)
				{
					return;
				}

				float timeRequiredToStop = sqr(speed) / acceleration + _deltaTime;
				float distRequiredToStop = speed * timeRequiredToStop - acceleration * sqr(timeRequiredToStop) / 2.0f;

				if (speed > 0.0f && pos + distRequiredToStop > target)
				{
					speed = max(0.0f, speed - acceleration * _deltaTime);
				}
				else if (speed < 0.0f && pos + distRequiredToStop < target)
				{
					speed = min(0.0f, speed + acceleration * _deltaTime);
				}
				else if (abs(pos - target) > maxDiff)
				{
					if (pos < target)
					{
						speed += acceleration * _deltaTime;
					}
					else
					{
						speed -= acceleration * _deltaTime;
					}
				}
				else
				{
					if (speed > 0.0f)
					{
						speed = max(0.0f, speed - acceleration * _deltaTime);
					}
					else
					{
						speed = min(0.0f, speed + acceleration * _deltaTime);
					}
				}

				if (maxSpeed != 0.0f)
				{
					speed = clamp(speed, -maxSpeed, maxSpeed);
				}
			}
		};

		AdvanceSpeed::advance(currentRelOrientation.pitch, fallingOrientation.pitch, 0.2f, REF_ orientationVelocity.pitch, fallingOrientationAcceleration.pitch, fallingOrientationMaxVelocity.pitch, _deltaTime);
		AdvanceSpeed::advance(currentRelOrientation.yaw, fallingOrientation.yaw, 0.2f, REF_ orientationVelocity.yaw, fallingOrientationAcceleration.yaw, fallingOrientationMaxVelocity.yaw, _deltaTime);
		AdvanceSpeed::advance(currentRelOrientation.roll, fallingOrientation.roll, 0.1f, REF_ orientationVelocity.roll, fallingOrientationAcceleration.roll, fallingOrientationMaxVelocity.roll, _deltaTime);

		Transform nextPlacement = currentPlacement.get();

		nextPlacement.set_translation(nextPlacement.get_translation() + Vector3::zAxis * verticalVelocity * _deltaTime);
		currentPlacement = nextPlacement;

		currentRelOrientation += orientationVelocity * _deltaTime;
		nextPlacement.set_orientation(initialPlacement.get_orientation().to_world(Rotator3(0.0f, 180.0f, 0.0f).to_quat().to_world(currentRelOrientation.to_quat().to_world(Rotator3(0.0f, 180.0f, 0.0f).to_quat()))));

		d->request_placement(nextPlacement);
	}
}

struct DoorFallingManager_Read
{
	template <typename Class>
	static void from_message(Framework::AI::Message const& _message, REF_ Class & _into, Name const& _what)
	{
		if (auto* ptr = _message.get_param(_what))
		{
			_into = ptr->get_as<Class>();
		}
	}
};

LATENT_FUNCTION(DoorFallingManager::execute_logic)
{
	PERFORMANCE_GUARD_LIMIT(0.005f, TXT("[ai door falling] execute logic"));

	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("execute logic"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(::Framework::AI::MessageHandler, messageHandler);
#ifdef AUTO_TEST
	LATENT_VAR(::System::TimeStamp, autoTestTS);
#endif

	auto * self = fast_cast<DoorFallingManager>(logic);
	auto* imo = mind->get_owner_as_modules_owner();

	LATENT_BEGIN_CODE();

	ai_log(self, TXT("door falling, hello!"));

	messageHandler.use_with(mind);
	{
		messageHandler.set(NAME(aim_start), [self](Framework::AI::Message const& _message)
			{
				auto* doorPtr = _message.get_param(NAME(door));
				auto* inRoomPtr = _message.get_param(NAME(inRoom));

				self->fallingDoor.clear();
				self->currentPlacement.clear();
				self->maxVerticalVelocity = 0.0f;

				if (doorPtr && inRoomPtr)
				{
					auto* d = doorPtr->get_as<SafePtr<Framework::Door>>().get();
					auto* r = inRoomPtr->get_as<SafePtr<Framework::Room>>().get();

					if (d && r)
					{
						if (auto* da = d->get_linked_door_a())
						{
							if (da->get_in_room() == r)
							{
								self->fallingDoor = da;
							}
						}
						if (auto* da = d->get_linked_door_b())
						{
							if (da->get_in_room() == r)
							{
								self->fallingDoor = da;
							}
						}
					}
				}

				DoorFallingManager_Read::from_message(_message, REF_ self->gravity, NAME(gravity));
				DoorFallingManager_Read::from_message(_message, REF_ self->maxVerticalVelocity, NAME(maxVerticalVelocity));
				DoorFallingManager_Read::from_message(_message, REF_ self->fallingOrientation, NAME(fallingOrientation));
				DoorFallingManager_Read::from_message(_message, REF_ self->fallingOrientationAcceleration, NAME(fallingOrientationAcceleration));
				DoorFallingManager_Read::from_message(_message, REF_ self->fallingOrientationMaxVelocity, NAME(fallingOrientationMaxVelocity));

				if (!self->fallingDoor.is_set())
				{
					error(TXT("could not find door for doorFalling"));
				}
			});
	}

	if (auto* ai = imo->get_ai())
	{
		auto& arla = ai->access_rare_logic_advance();
		arla.reset_to_no_rare_advancement();
	}

	while (true)
	{
		LATENT_WAIT_NO_RARE_ADVANCE(Random::get_float(0.1f, 0.5f));
	}

	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}

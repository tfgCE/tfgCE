#include "moduleMovementInRoomFollower.h"

#include "..\game\game.h"

#include "..\..\framework\advance\advanceContext.h"

#include "..\..\framework\module\modules.h"
#include "..\..\framework\module\registeredModule.h"
#include "..\..\framework\object\actor.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;

//

// module parameters
DEFINE_STATIC_NAME_AS(mp_followPlayer, followPlayer);
DEFINE_STATIC_NAME_AS(mp_followSocket, followSocket);
DEFINE_STATIC_NAME_AS(mp_axisAligned, axisAligned);

//

REGISTER_FOR_FAST_CAST(ModuleMovementInRoomFollower);

static Framework::ModuleMovement* create_module(Framework::IModulesOwner* _owner)
{
	return new ModuleMovementInRoomFollower(_owner);
}

Framework::RegisteredModule<Framework::ModuleMovement> & ModuleMovementInRoomFollower::register_itself()
{
	return Framework::Modules::movement.register_module(String(TXT("inRoomFollower")), create_module);
}

ModuleMovementInRoomFollower::ModuleMovementInRoomFollower(Framework::IModulesOwner* _owner)
: base(_owner)
{
	requiresPrepareMove = false;
}

ModuleMovementInRoomFollower::~ModuleMovementInRoomFollower()
{
}

void ModuleMovementInRoomFollower::setup_with(Framework::ModuleData const* _moduleData)
{
	base::setup_with(_moduleData);
	if (_moduleData)
	{
		followPlayer = _moduleData->get_parameter<bool>(this, NAME(mp_followPlayer), followPlayer);
		followSocket = _moduleData->get_parameter<Name>(this, NAME(mp_followSocket), followSocket);
		axisAligned = _moduleData->get_parameter<bool>(this, NAME(mp_axisAligned), axisAligned);
	}
}

void ModuleMovementInRoomFollower::apply_changes_to_velocity_and_displacement(float _deltaTime, VelocityAndDisplacementContext& _context)
{
	auto* ownPresence = get_owner()->get_presence();
	auto* inRoom = ownPresence->get_in_room();

	if (auto* imo = followObject.get())
	{
		if (imo->get_presence()->get_in_room() != inRoom)
		{
			followObject.clear();
		}
	}

	if (!followObject.get())
	{
		if (followPlayer)
		{
			if (auto* g = Game::get_as<Game>())
			{
				if (auto* pa = g->access_player().get_actor())
				{
					if (pa->get_presence()->get_in_room() == inRoom)
					{
						followObject = pa;
					}
				}
			}
		}
	}

	if (auto* imo = followObject.get())
	{
		Transform beAt;
		if (followSocket.is_valid())
		{
			beAt = imo->get_presence()->get_placement();
			if (auto* a = imo->get_appearance())
			{
				beAt = beAt.to_world(a->calculate_socket_os(followSocket));
			}
		}
		else
		{
			beAt = imo->get_presence()->get_centre_of_presence_transform_WS();
		}
		_context.currentDisplacementLinear = beAt.get_translation() - ownPresence->get_placement().get_translation();
		if (!axisAligned) // because we will redo it anyway
		{
			_context.currentDisplacementRotation = ownPresence->get_placement().get_orientation().to_local(beAt.get_orientation());
		}
	}
	else
	{
		_context.currentDisplacementLinear = Vector3::zero;
		_context.currentDisplacementRotation = Quat::identity;
	}

	if (axisAligned)
	{
		_context.currentDisplacementRotation = ownPresence->get_placement().get_orientation().to_local(Quat::identity);
	}

	// this is commented out for reason unknown
	// if there's collision, it may override displacement
	//_context.applyCollision = true;
	//base::apply_changes_to_velocity_and_displacement(_deltaTime, _context);
}

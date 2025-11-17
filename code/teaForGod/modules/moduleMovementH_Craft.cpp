#include "moduleMovementH_Craft.h"

#include "..\..\framework\ai\aiMindInstance.h"
#include "..\..\framework\module\moduleAI.h"
#include "..\..\framework\module\modulePresence.h"
#include "..\..\framework\module\modules.h"
#include "..\..\framework\modulesOwner\modulesOwner.h"
#include "..\..\framework\world\room.h"

#include "..\ai\logics\aiLogic_h_craft.h"
#include "..\ai\logics\aiLogic_h_craft_v2.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;

//

// parameters
DEFINE_STATIC_NAME(blendInTime);

//

REGISTER_FOR_FAST_CAST(ModuleMovementH_Craft);

static Framework::ModuleMovement* create_module(Framework::IModulesOwner* _owner)
{
	return new ModuleMovementH_Craft(_owner);
}

Framework::RegisteredModule<Framework::ModuleMovement> & ModuleMovementH_Craft::register_itself()
{
	return Framework::Modules::movement.register_module(String(TXT("h_craft")), create_module);
}

ModuleMovementH_Craft::ModuleMovementH_Craft(Framework::IModulesOwner* _owner)
: base(_owner)
{
	requiresPrepareMove = false;
}

void ModuleMovementH_Craft::setup_with(Framework::ModuleData const* _moduleData)
{
	base::setup_with(_moduleData);

	if (_moduleData)
	{
		blendInTime = _moduleData->get_parameter<float>(this, NAME(blendInTime), blendInTime);
	}
}

void ModuleMovementH_Craft::on_activate_movement(Framework::ModuleMovement * _prevMovement)
{
	base::on_activate_movement(_prevMovement);

	timeActive = 0.0f;
}

void ModuleMovementH_Craft::apply_changes_to_velocity_and_displacement(float _deltaTime, VelocityAndDisplacementContext & _context)
{
	timeActive += _deltaTime;

	float use = blendInTime > 0.0f && timeActive < blendInTime? timeActive / blendInTime : 1.0f;

	Transform const isAt = get_owner()->get_presence()->get_placement();
	Transform shouldBeAt = isAt;
	Vector3 velocityLinear = get_owner()->get_presence()->get_velocity_linear();
	Rotator3 velocityRotation = get_owner()->get_presence()->get_velocity_rotation();

	if (auto* ai = get_owner()->get_ai())
	{
		if (auto* m = ai->get_mind())
		{
			if (auto* l = fast_cast<AI::Logics::H_Craft>(m->get_logic()))
			{
				shouldBeAt = l->get_placement();
				velocityLinear = l->get_velocity_linear();
				velocityRotation = l->get_velocity_rotation();
			}
			if (auto* l = fast_cast<AI::Logics::H_Craft_V2>(m->get_logic()))
			{
				shouldBeAt = l->get_placement();
				velocityLinear = l->get_velocity_linear();
				velocityRotation = l->get_velocity_rotation();
			}
		}
	}

	// force displacements
	if (use < 1.0f)
	{
		Transform predicted = isAt;
		Vector3 orgVelocityLinear = get_owner()->get_presence()->get_velocity_linear();
		Rotator3 orgVelocityRotation = get_owner()->get_presence()->get_velocity_rotation();
		predicted.set_translation(predicted.get_translation() + orgVelocityLinear * _deltaTime);
		predicted.set_orientation(predicted.get_orientation().to_world((orgVelocityRotation * _deltaTime).to_quat()));

		predicted = isAt;
		predicted.set_translation(isAt.get_translation() + _context.currentDisplacementLinear);
		predicted.set_scale(isAt.get_scale());
		predicted.set_orientation(isAt.get_orientation().rotate_by(_context.currentDisplacementRotation));

		Transform localPred = isAt.to_local(predicted);
		Transform localShould = isAt.to_local(shouldBeAt);

		Vector3 forwardLocalPred = localPred.get_axis(Axis::Forward);
		Vector3 forwardLocalShould = localShould.get_axis(Axis::Forward);
		Vector3 upLocalPred = localPred.get_axis(Axis::Up);
		Vector3 upLocalShould = localShould.get_axis(Axis::Up);

		Vector3 forwardLocalMixed = lerp(use, forwardLocalPred, forwardLocalShould);
		Vector3 upLocalMixed = lerp(use, upLocalPred, upLocalShould);

		Quat atLocalMixed = look_matrix33(forwardLocalMixed.normal(), upLocalMixed.normal()).to_quat();

		_context.currentDisplacementLinear = lerp(use, predicted.get_translation() - isAt.get_translation(), shouldBeAt.get_translation() - isAt.get_translation());
		_context.currentDisplacementRotation = Quat::slerp(use, isAt.get_orientation().to_local(predicted.get_orientation()), isAt.get_orientation().to_local(shouldBeAt.get_orientation()));
		_context.currentDisplacementRotation = atLocalMixed;
		_context.velocityLinear = lerp(use, orgVelocityLinear, velocityLinear);
		_context.velocityRotation = lerp(use, orgVelocityRotation, velocityRotation);
		get_owner()->get_presence()->set_scale(lerp(use, get_owner()->get_presence()->get_placement().get_scale(), shouldBeAt.get_scale()));

		_context.currentDisplacementRotation.normalise();
	}
	else
	{
		_context.currentDisplacementLinear = shouldBeAt.get_translation() - isAt.get_translation();
		_context.currentDisplacementRotation = isAt.get_orientation().to_local(shouldBeAt.get_orientation());
		_context.currentDisplacementRotation.normalise();
		_context.velocityLinear = velocityLinear;
		_context.velocityRotation = velocityRotation;
		get_owner()->get_presence()->set_scale(shouldBeAt.get_scale());
	}

	// ignore base base::apply_changes_to_velocity_and_displacement(_deltaTime, _context);
}

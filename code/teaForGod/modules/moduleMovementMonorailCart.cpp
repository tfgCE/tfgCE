#include "moduleMovementMonorailCart.h"

#include "..\..\framework\module\moduleAppearance.h"
#include "..\..\framework\module\modules.h"
#include "..\..\framework\module\registeredModule.h"

#include "..\..\framework\advance\advanceContext.h"

#include "..\..\core\performance\performanceUtils.h"

using namespace TeaForGodEmperor;

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

DEFINE_STATIC_NAME(speed);

//

REGISTER_FOR_FAST_CAST(ModuleMovementMonorailCart);

static Framework::ModuleMovement* create_module(Framework::IModulesOwner* _owner)
{
	return new ModuleMovementMonorailCart(_owner);
}

Framework::RegisteredModule<Framework::ModuleMovement> & ModuleMovementMonorailCart::register_itself()
{
	return Framework::Modules::movement.register_module(String(TXT("monorail cart")), create_module);
}

ModuleMovementMonorailCart::ModuleMovementMonorailCart(Framework::IModulesOwner* _owner)
: base(_owner)
{
	requiresPrepareMove = false;
}

void ModuleMovementMonorailCart::setup_with(Framework::ModuleData const * _moduleData)
{
	base::setup_with(_moduleData);
	if (_moduleData)
	{
		speed = _moduleData->get_parameter<float>(this, NAME(speed), speed);
	}
}

void ModuleMovementMonorailCart::use_track(Vector3 const & _a, Vector3 const& _b)
{
	decouple_forward();
	decouple_back();
	trackLinear.a = _a;
	trackLinear.b = _b;
	trackLinear.track = Segment(_a, _b);
	trackType = TrackType::Linear;
}

void ModuleMovementMonorailCart::couple_forward_to(Framework::IModulesOwner* _to, Name const& _throughSocket, Name const& _toSocket)
{
	decouple_forward();
	auto* bmc = this;
	auto* fmc = fast_cast<ModuleMovementMonorailCart>(_to->get_movement());
	an_assert(fmc);
	fmc->decouple_back();

	if (_to &&
		get_owner()->get_appearance() &&
		_to->get_appearance())
	{
		fmc->coupledBack = get_owner();
		fmc->connectorBack.set_name(_toSocket);
		fmc->connectorBack.look_up(_to->get_appearance()->get_mesh());
		bmc->coupledForward = _to;
		bmc->connectorForward.set_name(_throughSocket);
		bmc->connectorForward.look_up(get_owner()->get_appearance()->get_mesh());
	}
}

void ModuleMovementMonorailCart::decouple_internal(Framework::IModulesOwner* _forward, Framework::IModulesOwner* _back)
{
	if (_forward && _back)
	{
		auto* fmc = fast_cast<ModuleMovementMonorailCart>(_forward->get_movement());
		auto* bmc = fast_cast<ModuleMovementMonorailCart>(_back->get_movement());
		an_assert(fmc && fmc->coupledBack.get() == _back);
		an_assert(bmc && bmc->coupledForward.get() == _forward);
		if (fmc)
		{
			fmc->coupledBack.clear();
		}
		if (bmc)
		{
			bmc->coupledForward.clear();
		}
	}
}

void ModuleMovementMonorailCart::decouple()
{
	decouple_forward();
	decouple_back();
}

void ModuleMovementMonorailCart::decouple_forward()
{
	decouple_internal(coupledForward.get(), get_owner());
	an_assert(! coupledForward.is_set());
}

void ModuleMovementMonorailCart::decouple_back()
{
	decouple_internal(get_owner(), coupledBack.get());
	an_assert(!coupledBack.is_set());
}

void ModuleMovementMonorailCart::apply_changes_to_velocity_and_displacement(float _deltaTime, VelocityAndDisplacementContext & _context)
{
	Transform const isAt = get_owner()->get_presence()->get_placement();
	Transform shouldBeAt = isAt;

	if (trackType == TrackType::Linear)
	{
		_context.velocityLinear = (trackLinear.b - trackLinear.a).normal() * speed;
		_context.currentDisplacementLinear = _context.velocityLinear * _deltaTime;

		Transform placement = get_owner()->get_presence()->get_placement();

		// put on track, now, as it is to match connectors properly
		{
			Transform tempPlace = placement;
			//
			float along = Vector3::dot(trackLinear.track.get_start_to_end_dir(), tempPlace.get_translation() - trackLinear.track.get_start());
			Vector3 shouldBeAt = trackLinear.track.get_start() + trackLinear.track.get_start_to_end_dir() * along;
			_context.currentDisplacementLinear += shouldBeAt - tempPlace.get_translation();
			tempPlace.set_translation(shouldBeAt);
			//
			Quat shouldBeOriented = look_matrix33(trackLinear.track.get_start_to_end_dir(), tempPlace.get_axis(Axis::Up)).to_quat();
			_context.currentDisplacementRotation = _context.currentDisplacementRotation.to_world(tempPlace.get_orientation().to_local(shouldBeOriented));
			tempPlace.set_orientation(shouldBeOriented);
			//
			placement = tempPlace;
		}

		// match connector of forward one
		if (auto* f = coupledForward.get())
		{
			auto* b = get_owner();
			if (auto * fmc = fast_cast<ModuleMovementMonorailCart>(f->get_movement()))
			{
				auto* bmc = this;
				auto* fap = f->get_appearance();
				auto* bap = b->get_appearance();
				if (fap && bap)
				{
					Vector3 fcOS = fap->calculate_socket_os(fmc->connectorBack.get_index()).get_translation();
					Vector3 bcOS = bap->calculate_socket_os(bmc->connectorForward.get_index()).get_translation();

					Vector3 fcWS = f->get_presence()->get_placement().location_to_world(fcOS);
					Vector3 bcWS = placement.location_to_world(bcOS);

					Vector3 diff = fcWS - bcWS;
					if (diff.length() > 0.1f)
					{
						_context.currentDisplacementLinear += diff;
					}
					else
					{
						_context.currentDisplacementLinear += diff * clamp(_deltaTime / 0.2f, 0.0f, 1.0f);
					}
				}
			}
		}

		// put on track (look forward!)
		{
			Transform tempPlace = placement;
			tempPlace.set_translation(tempPlace.get_translation() + _context.velocityLinear * _deltaTime);
			//
			float along = Vector3::dot(trackLinear.track.get_start_to_end_dir(), tempPlace.get_translation() - trackLinear.track.get_start());
			Vector3 shouldBeAt = trackLinear.track.get_start() + trackLinear.track.get_start_to_end_dir() * along;
			_context.currentDisplacementLinear += shouldBeAt - tempPlace.get_translation();
			tempPlace.set_translation(shouldBeAt);
			//
			Quat shouldBeOriented = look_matrix33(trackLinear.track.get_start_to_end_dir(), tempPlace.get_axis(Axis::Up)).to_quat();
			_context.currentDisplacementRotation = _context.currentDisplacementRotation.to_world(tempPlace.get_orientation().to_local(shouldBeOriented));
			tempPlace.set_orientation(shouldBeOriented);
			//
			placement = tempPlace;
		}
	}
	else if (trackType == TrackType::NoTrack)
	{
		_context.velocityLinear = Vector3::zero;
		_context.currentDisplacementLinear = Vector3::zero;
		_context.velocityRotation = Rotator3::zero;
		_context.currentDisplacementRotation = Quat::identity;
	}
	else
	{
		todo_important(TXT("implement other track type"));
	}
	// ignore base base::apply_changes_to_velocity_and_displacement(_deltaTime, _context);
}

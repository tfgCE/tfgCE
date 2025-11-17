#include "moduleMovementDial.h"

#include "registeredModule.h"
#include "modules.h"
#include "moduleDataImpl.inl"

#include "..\advance\advanceContext.h"

#include "..\..\core\other\parserUtils.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#ifdef AN_DEVELOPMENT
//#define DEBUG_DIAL
#endif

//

using namespace Framework;

//

// sounds
DEFINE_STATIC_NAME(step);

//

// module parameters

//

REGISTER_FOR_FAST_CAST(ModuleMovementDial);

static ModuleMovement* create_module(IModulesOwner* _owner)
{
	return new ModuleMovementDial(_owner);
}

static ModuleData* create_module_data(::Framework::LibraryStored* _inLibraryStored)
{
	return new ModuleMovementDialData(_inLibraryStored);
}

RegisteredModule<ModuleMovement> & ModuleMovementDial::register_itself()
{
	return Modules::movement.register_module(String(TXT("dial")), create_module, create_module_data);
}

ModuleMovementDial::ModuleMovementDial(IModulesOwner* _owner)
: base(_owner)
{
	requiresPrepareMove = false;
}

void ModuleMovementDial::setup_with(ModuleData const * _moduleData)
{
	base::setup_with(_moduleData);
	if (_moduleData)
	{
		moduleMovementDialData = fast_cast<ModuleMovementDialData>(_moduleData);

		step = _moduleData->get_parameter<float>(this, NAME(step), step);
	}
}

void ModuleMovementDial::activate()
{
	base::activate();
	relativeInitialPlacementAttachedToBoneIdx = NONE;
	relativeInitialPlacementAttachedToSocketIdx = NONE;
	relativeInitialPlacement = Transform::identity;
}

void ModuleMovementDial::apply_changes_to_velocity_and_displacement(float _deltaTime, VelocityAndDisplacementContext& _context)
{
	base::apply_changes_to_velocity_and_displacement(_deltaTime, _context);

	auto* presence = get_owner()->get_presence();

	// this thing does not move
	_context.currentDisplacementLinear = Vector3::zero;
	_context.velocityLinear = Vector3::zero;
	_context.velocityRotation = Rotator3::zero;

	// we're to calculate currentDisplacementRotation
	Rotator3 displacementRotation = Rotator3::zero;

	Optional<float> requestedYawChange;

	if (auto* controller = get_owner()->get_controller())
	{
		if (controller->is_requested_placement_set())
		{
			// ignore collisions and other input
			Transform requestedPlacement = controller->get_requested_placement();
			Transform currentPlacement = presence->get_placement();

			Vector3 requestedForwardLS = currentPlacement.vector_to_local(requestedPlacement.get_axis(hardcoded Axis::Forward));
			requestedYawChange = Rotator3::get_yaw(requestedForwardLS);
		}
	}

	if (requestedYawChange.is_set())
	{
		if (!turnStartedAt.is_set())
		{
			turnStartedAt = keepDeadZonedAt;
		}
		displacementRotation.yaw = requestedYawChange.get();
	}
	else
	{
		// this is incorrect, ignore this, we don't actually use turnstartedAt, as we use only get_absolute_dial
		/*
		int prevAbsoluteDial = get_absolute_dial();
		float prevTurn = turn;
		//turn = Rotator3::normalise_axis(turn);
		keepDeadZonedAt += turn - prevTurn; // to remain relative to turn
		int currAbsoluteDial = get_absolute_dial();
		absoluteDialOffset += prevAbsoluteDial - currAbsoluteDial;
		*/
		turnStartedAt.clear();
	}

	turn += displacementRotation.yaw;
	// update keep dead zone, so we jump between two
	{
		float useStep = step != 0.0f ? step : 1.0f;
		if (abs(Rotator3::normalise_axis(turn - keepDeadZonedAt)) >= 0.5f * useStep * (1.0 + deadZonePct))
		{
			int currentlyAt = TypeConversions::Normal::f_i_closest(turn / useStep);
			keepDeadZonedAt = useStep * (float)currentlyAt;
		}
	}
	useTurnForDial = keepDeadZonedAt; // will be at the centre of where we should remain

#ifdef AN_DEVELOPMENT
#ifdef DEBUG_DIAL
	initialPlacementForDebug.set_if_not_set(get_owner()->get_presence()->get_placement());
	debug_context(get_owner()->get_presence()->get_in_room());
	debug_push_transform(initialPlacementForDebug.get());
	debug_draw_arrow(true, Colour::orange, Vector3::zero, Rotator3(0.0f, keepDeadZonedAt, 0.0f).get_forward() * 0.1f);
	debug_draw_arrow(true, requestedYawChange.is_set()? Colour::green : Colour::c64LightBlue, Vector3::zero, Rotator3(0.0f, turn, 0.0f).get_forward() * 0.1f);
	debug_pop_transform();
	debug_no_context();
#endif
#endif

	{
		// we should update attachment
		if (presence->is_attached())
		{
			Transform shouldBeAt = calculate_relative_placement();

			an_assert(abs(shouldBeAt.get_orientation().length() - 1.0f) < 0.1f);

			presence->update_attachment_relative_placement(relativeInitialPlacement.to_world(shouldBeAt));
		}

		_context.currentDisplacementRotation = displacementRotation.to_quat();
	}

	if (step > 0.0f)
	{
		int curDial = get_absolute_dial();
		if (curDial != lastDial)
		{
			lastDial = curDial;

			if (auto* s = get_owner()->get_sound())
			{
				s->play_sound(NAME(step));
			}
		}
	}
}

Transform ModuleMovementDial::calculate_relative_placement() const
{
	return Transform(Vector3::zero, Rotator3(0.0f, turn, 0.0f).to_quat());
}

int ModuleMovementDial::get_dials(OUT_ int& _startedAt, OUT_ int& _currentlyAt) const
{
	float useStep = step != 0.0f ? step : 1.0f;

	_startedAt = 0;
	_currentlyAt = TypeConversions::Normal::f_i_closest(useTurnForDial / useStep);

	if (turnStartedAt.is_set())
	{
		_startedAt = TypeConversions::Normal::f_i_closest(turnStartedAt.get() / useStep);
		return _currentlyAt - _startedAt;
	}
	else
	{
		return 0;
	}
}

int ModuleMovementDial::get_dial() const
{
	int startedAt;
	int currentlyAt;
	get_dials(startedAt, currentlyAt);
	return currentlyAt - startedAt;
}

int ModuleMovementDial::get_absolute_dial() const
{
	int startedAt;
	int currentlyAt;
	get_dials(startedAt, currentlyAt);
	return currentlyAt + absoluteDialOffset;
}

//

REGISTER_FOR_FAST_CAST(ModuleMovementDialData);

ModuleMovementDialData::ModuleMovementDialData(LibraryStored* _inLibraryStored)
: base(_inLibraryStored)
{
}

bool ModuleMovementDialData::read_parameter_from(IO::XML::Attribute const * _attr, LibraryLoadingContext & _lc)
{
	if (_attr->get_name() == TXT("sound"))
	{
		sound = _attr->get_as_name();
		return true;
	}
	else
	{
		return base::read_parameter_from(_attr, _lc);
	}
}

bool ModuleMovementDialData::read_parameter_from(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	if (_node->get_name() == TXT("sound"))
	{
		sound = Name(_node->get_text());
		return true;
	}
	else
	{
		return base::read_parameter_from(_node, _lc);
	}
}

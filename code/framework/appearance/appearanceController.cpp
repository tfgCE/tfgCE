#include "appearanceController.h"

#include "appearanceControllerData.h"
#include "appearanceControllerPoseContext.h"

#include "..\module\moduleAppearance.h"

using namespace Framework;

REGISTER_FOR_FAST_CAST(AppearanceController);

AppearanceController::AppearanceController(AppearanceControllerData const * _data)
: data(cast_to_nonconst(_data))
, processingOrder(NONE)
{
	for_every(v, _data->activeVars)
	{
		activeVars.push_back(v->get());
	}
	for_every(v, _data->blockActiveVars)
	{
		blockActiveVars.push_back(v->get());
	}
	for_every(v, _data->holdVars)
	{
		holdVars.push_back(v->get());
	}
	processAtLevel = data->processAtLevel.get();
	activeBlendCurve = data->activeBlendCurve.get();
	active = data->activeInitial.get();
	activeBlendInTime = data->activeBlendInTime.get();
	activeBlendOutTime = data->activeBlendOutTime.get();
	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA_OPTIONAL(activeBlendTimeVar);
	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA_OPTIONAL(outActiveVar);
	activeMask = data->activeMask.get();
	prevActive = active;
}

AppearanceController::~AppearanceController()
{
}

void AppearanceController::initialise(ModuleAppearance* _owner)
{
	owner = _owner;
	initialised = true;
	firstUpdate = true;

	auto & varStorage = get_owner()->access_controllers_variable_storage();
	for_every(v, activeVars)
	{
		v->look_up<float>(varStorage);
	}
	for_every(v, blockActiveVars)
	{
		v->look_up<float>(varStorage);
	}
	for_every(v, holdVars)
	{
		v->look_up<float>(varStorage);
	}
	activeBlendTimeVar.look_up<float>(varStorage);
	outActiveVar.look_up<float>(varStorage);
}

void AppearanceController::activate()
{
	an_assert(initialised);
}

void AppearanceController::advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context)
{
	prevActive = firstUpdate? 0.0f : active;
	firstUpdate = false;

	Optional<float> targetActive;

	if (!activeVars.is_empty())
	{
		bool provided = false;
		float newTargetActive = 0.0f;
		for_every(v, activeVars)
		{
			if (v->is_valid())
			{
				provided = true;
				newTargetActive = max(newTargetActive, v->get<float>());
			}
		}
		if (provided)
		{
			targetActive = clamp(newTargetActive, 0.0f, 1.0f);
		}
	}
	if (! blockActiveVars.is_empty())
	{
		bool provided = false;
		float blockActive = 0.0f;
		for_every(v, blockActiveVars)
		{
			if (v->is_valid())
			{
				provided = true;
				blockActive = max(blockActive, v->get<float>());
			}
		}
		if (provided)
		{
			if (targetActive.is_set())
			{
				targetActive = targetActive.get() * (1.0f - clamp(blockActive, 0.0f, 1.0f));
			}
			else
			{
				targetActive = 1.0f - clamp(blockActive, 0.0f, 1.0f);
			}
		}
	}

	targetActive.set_if_not_set(active);
	activeTarget = targetActive.get();

	shouldHold = false;
	if (!holdVars.is_empty())
	{
		for_every(v, holdVars)
		{
			if (v->is_valid())
			{
				if (v->get<float>() > 0.5f)
				{
					shouldHold = true;
				}
			}
		}
	}

	justActivated = false;
	justDeactivated = false;
	{
		float useBlendTime = activeTarget > active ? activeBlendInTime : activeBlendOutTime;
		if (activeBlendTimeVar.is_valid())
		{
			useBlendTime = activeBlendTimeVar.get<float>();
		}
		active = blend_to_using_speed_based_on_time(active, activeTarget, useBlendTime, _context.get_delta_time());
	}

	if (outActiveVar.is_valid())
	{
		outActiveVar.access<float>() = active;
	}

	justActivated = prevActive == 0.0f && active > 0.0f;
	justDeactivated = prevActive > 0.0f && active == 0.0f;
}

void AppearanceController::calculate_final_pose(REF_ AppearanceControllerPoseContext & _context)
{
}

bool AppearanceController::check_processing_order(REF_ AppearanceControllerPoseContext & _context)
{
	an_assert(processingOrder >= 0);
	_context.require_processing_order(processingOrder);
	return _context.get_processing_order() == processingOrder;
}

float AppearanceController::get_active() const
{
	return BlendCurve::apply(activeBlendCurve, active) * activeMask;
}

void AppearanceController::get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
	OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const
{
	for_every(v, activeVars)
	{
		dependsOnVariables.push_back(v->get_name());
	}
	for_every(v, blockActiveVars)
	{
		dependsOnVariables.push_back(v->get_name());
	}
}

void AppearanceController::log(LogInfoContext & _log) const
{
#ifdef AN_DEVELOPMENT
	_log.log(TXT("+- [PAT:%05i;PO:%05i] [%03.0f] %S (%S) @%S"), processAtLevel, processingOrder, active * 100.0f, get_name_for_log(), data->get_desc(), data->get_location_info());
#else
	_log.log(TXT("+- [PAT:%05i;PO:%05i] [%03.0f] %S"), processAtLevel, processingOrder, active * 100.0f, get_name_for_log());
#endif
	LOG_INDENT(_log);
	internal_log(_log);
}

void AppearanceController::internal_log(LogInfoContext & _log) const
{
	if (!activeVars.is_empty())
	{
		_log.log(TXT("active vars"));
		LOG_INDENT(_log);
		for_every(v, activeVars)
		{
			_log.log(TXT("[%03.0f] %S"), v->get<float>() * 100.0f, v->get_name().to_char());
		}
	}
	if (!blockActiveVars.is_empty())
	{
		_log.log(TXT("block active vars"));
		LOG_INDENT(_log);
		for_every(v, blockActiveVars)
		{
			_log.log(TXT("[%03.0f] %S"), v->get<float>() * 100.0f, v->get_name().to_char());
		}
	}
}

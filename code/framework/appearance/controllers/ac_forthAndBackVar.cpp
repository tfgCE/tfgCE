#include "ac_forthAndBackVar.h"

#include "..\appearanceControllerPoseContext.h"
#include "..\appearanceControllers.h"

#include "..\..\meshGen\meshGenGenerationContext.h"
#include "..\..\meshGen\meshGenParamImpl.inl"
#include "..\..\module\moduleAppearance.h"
#include "..\..\module\moduleSound.h"
#include "..\..\world\room.h"

#include "..\..\..\core\mesh\pose.h"

#include "..\..\..\core\performance\performanceUtils.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace AppearanceControllersLib;

//

DEFINE_STATIC_NAME(forthAndBackVar);

//

REGISTER_FOR_FAST_CAST(ForthAndBackVar);

ForthAndBackVar::ForthAndBackVar(ForthAndBackVarData const* _data)
	: base(_data)
{
	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA(var);
	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA_OPTIONAL(keepAt0Var);
	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA_OPTIONAL(keepAt1Var);
	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA(value0);
	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA(value1);
	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA(to0Time);
	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA(to1Time);
	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA_OPTIONAL(playSoundAt0);
	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA_OPTIONAL(playSoundAt1);
	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA_OPTIONAL(playSoundMoving);

	atPt = rg.get_float(0.0f, 1.0f);
	targetPt = rg.get_bool() ? 1.0f : 0.0f;
}

ForthAndBackVar::~ForthAndBackVar()
{
}

void ForthAndBackVar::initialise(ModuleAppearance* _owner)
{
	base::initialise(_owner);

	auto& ownerStorage = _owner->get_owner()->access_variables();

	var.look_up<float>(ownerStorage);
	keepAt0Var.look_up<bool>(ownerStorage);
	keepAt1Var.look_up<bool>(ownerStorage);

	if (playSoundMoving.is_valid())
	{
		if (auto* s = get_owner()->get_owner()->get_sound())
		{
			s->stop_sound(playSoundMoving);
		}
	}
}

void ForthAndBackVar::advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::advance_and_adjust_preliminary_pose(_context);
}

void ForthAndBackVar::calculate_final_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::calculate_final_pose(_context);

	if (! get_active())
	{
		return;
	}

	{
		bool k0 = keepAt0Var.is_valid() && keepAt0Var.get<bool>();
		bool k1 = keepAt1Var.is_valid() && keepAt1Var.get<bool>();
		if (k0 || k1)
		{
			if (k0) targetPt = 0.0f;
			if (k1) targetPt = 1.0f;
			if (k0 && k1) targetPt = atPt >= 0.5f ? 1.0f : 0.0f;
			if (firstUpdate)
			{
				atPt = targetPt;
			}
		}
	}

	if (currentToTime == 0.0f)
	{
		if (atPt < targetPt)
		{
			currentToTime = max(0.001f, rg.get(to1Time));
		}
		else 
		{
			currentToTime = max(0.001f, rg.get(to0Time));
		}
	}

	float prevAtPt = atPt;

	float deltaTime = _context.get_delta_time();
	if (atPt <= targetPt)
	{
		atPt = min(targetPt, atPt + deltaTime / currentToTime);
	}
	else 
	{
		atPt = max(targetPt, atPt - deltaTime / currentToTime);
	}

	bool playSoundNow = false;

	if (atPt == targetPt)
	{
		targetPt = targetPt > 0.5f ? 0.0f : 1.0f;
		currentToTime = targetPt > 0.5f ? rg.get(to1Time) : rg.get(to0Time);

		if (prevAtPt != atPt)
		{
			playSoundNow = true;
		}
	}

	int prevMovingDir = movingDir;
	if (atPt == prevAtPt)
	{
		movingDir = 0;
	}
	else
	{
		if (movingDir == 0)
		{
			playSoundNow = true;
		}
		movingDir = atPt > prevAtPt ? 1 : -1;
	}
	if (playSoundMoving.is_valid())
	{
		if (movingDir != prevMovingDir)
		{
			if (auto* s = get_owner()->get_owner()->get_sound())
			{
				s->stop_sound(playSoundMoving);
				if (movingDir)
				{
					// play new one
					s->play_sound(playSoundMoving);
				}
			}
		}
	}

	if (playSoundNow)
	{
		Name sound = atPt > 0.5f ? playSoundAt1 : playSoundAt0;
		if (sound.is_valid())
		{
			if (auto* s = get_owner()->get_owner()->get_sound())
			{
				s->play_sound(sound);
			}
		}
	}
	
	if (var.is_valid())
	{
		var.access<float>() = atPt;
	}
}

void ForthAndBackVar::get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
	OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const
{
	base::get_info_for_auto_processing_order(OUT_ dependsOnBones, OUT_ dependsOnParentBones, OUT_ usesBones, OUT_ providesBones,
		OUT_ dependsOnVariables, OUT_ providesVariables, OUT_ dependsOnControllers, OUT_ providesControllers);

	providesVariables.push_back(var.get_name());
}

//

REGISTER_FOR_FAST_CAST(ForthAndBackVarData);

AppearanceControllerData* ForthAndBackVarData::create_data()
{
	return new ForthAndBackVarData();
}

void ForthAndBackVarData::register_itself()
{
	AppearanceControllers::register_appearance_controller(NAME(forthAndBackVar), create_data);
}

ForthAndBackVarData::ForthAndBackVarData()
{
}

bool ForthAndBackVarData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	var.load_from_xml(_node, TXT("varID"));
	keepAt0Var.load_from_xml(_node, TXT("keepAt0VarID"));
	keepAt1Var.load_from_xml(_node, TXT("keepAt1VarID"));

	value0.load_from_xml(_node, TXT("value0"));
	value1.load_from_xml(_node, TXT("value1"));

	to0Time.load_from_xml(_node, TXT("time"));
	to1Time.load_from_xml(_node, TXT("time"));
	to0Time.load_from_xml(_node, TXT("to0Time"));
	to1Time.load_from_xml(_node, TXT("to1Time"));

	playSoundAt0.load_from_xml(_node, TXT("playSound"));
	playSoundAt1.load_from_xml(_node, TXT("playSound"));
	playSoundAt0.load_from_xml(_node, TXT("playSoundAt0"));
	playSoundAt1.load_from_xml(_node, TXT("playSoundAt1"));
	
	playSoundMoving.load_from_xml(_node, TXT("playSoundMoving"));

	return result;
}

bool ForthAndBackVarData::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

AppearanceControllerData* ForthAndBackVarData::create_copy() const
{
	ForthAndBackVarData* copy = new ForthAndBackVarData();
	*copy = *this;
	return copy;
}

AppearanceController* ForthAndBackVarData::create_controller()
{
	return new ForthAndBackVar(this);
}

void ForthAndBackVarData::apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context)
{
	base::apply_mesh_gen_params(_context);

	var.fill_value_with(_context);
	keepAt0Var.fill_value_with(_context);
	keepAt1Var.fill_value_with(_context);
	value0.fill_value_with(_context);
	value1.fill_value_with(_context);
	to0Time.fill_value_with(_context);
	to1Time.fill_value_with(_context);
	playSoundAt0.fill_value_with(_context);
	playSoundAt1.fill_value_with(_context);
	playSoundMoving.fill_value_with(_context);
}

void ForthAndBackVarData::apply_transform(Matrix44 const & _transform)
{
	base::apply_transform(_transform);
}

void ForthAndBackVarData::reskin(Meshes::BoneRenameFunc _rename)
{
	base::reskin(_rename);
}

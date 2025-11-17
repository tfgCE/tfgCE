#include "ac_applyTransformToVar.h"

#include "..\appearanceControllers.h"

#include "..\..\meshGen\meshGenGenerationContext.h"
#include "..\..\meshGen\meshGenParamImpl.inl"
#include "..\..\module\moduleAppearance.h"

#include "..\..\..\core\performance\performanceUtils.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace AppearanceControllersLib;

//

DEFINE_STATIC_NAME(ApplyTransformToVar);

//

REGISTER_FOR_FAST_CAST(ApplyTransformToVar);

ApplyTransformToVar::ApplyTransformToVar(ApplyTransformToVarData const * _data)
: base(_data)
, applyTransformToVarData(_data)
{
	inVar = applyTransformToVarData->inVar.get();
	outVar = applyTransformToVarData->outVar.get();

	applyTransform = applyTransformToVarData->applyTransform.get(Transform::identity);
}

ApplyTransformToVar::~ApplyTransformToVar()
{
}

void ApplyTransformToVar::initialise(ModuleAppearance* _owner)
{
	base::initialise(_owner);

	auto & varStorage = get_owner()->access_controllers_variable_storage();
	inVar.look_up<Transform>(varStorage);
	outVar.look_up<Transform>(varStorage);
}

void ApplyTransformToVar::advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::advance_and_adjust_preliminary_pose(_context);
}

void ApplyTransformToVar::calculate_final_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::calculate_final_pose(_context);

#ifdef MEASURE_PERFORMANCE_APPEARANCE_CONTROLLERS_DETAILED
	MEASURE_PERFORMANCE(ApplyTransformToVar_finalPose);
#endif

	auto const * mesh = get_owner()->get_mesh();
	if (!mesh)
	{
		return;
	}

	Transform inValue = inVar.get<Transform>();
	Transform newValue = inValue.to_world(applyTransform);
	if (get_active() < 1.0f)
	{
		if (get_active() <= 0.0f)
		{
			newValue = inValue;
		}
		else
		{
			newValue = Transform::lerp(get_active(), inValue, newValue);
		}
	}
	outVar.access<Transform>() = newValue;
}

void ApplyTransformToVar::get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
	OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const
{
	base::get_info_for_auto_processing_order(OUT_ dependsOnBones, OUT_ dependsOnParentBones, OUT_ usesBones, OUT_ providesBones,
		OUT_ dependsOnVariables, OUT_ providesVariables, OUT_ dependsOnControllers, OUT_ providesControllers);

	dependsOnVariables.push_back(inVar.get_name());
	providesVariables.push_back(outVar.get_name());
}

//

REGISTER_FOR_FAST_CAST(ApplyTransformToVarData);

AppearanceControllerData* ApplyTransformToVarData::create_data()
{
	return new ApplyTransformToVarData();
}

void ApplyTransformToVarData::register_itself()
{
	AppearanceControllers::register_appearance_controller(NAME(ApplyTransformToVar), create_data);
}

ApplyTransformToVarData::ApplyTransformToVarData()
{
}

bool ApplyTransformToVarData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= inVar.load_from_xml(_node, TXT("varID"));
	result &= outVar.load_from_xml(_node, TXT("varID"));
	result &= inVar.load_from_xml(_node, TXT("inVarID"));
	result &= outVar.load_from_xml(_node, TXT("outVarID"));

	result &= applyTransform.load_from_xml(_node, TXT("applyTransform"));

	return result;
}

bool ApplyTransformToVarData::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

AppearanceControllerData* ApplyTransformToVarData::create_copy() const
{
	ApplyTransformToVarData* copy = new ApplyTransformToVarData();
	*copy = *this;
	return copy;
}

AppearanceController* ApplyTransformToVarData::create_controller()
{
	return new ApplyTransformToVar(this);
}

void ApplyTransformToVarData::reskin(Meshes::BoneRenameFunc _rename)
{
	base::reskin(_rename);

	inVar.if_value_set([this, _rename]() { inVar = _rename(inVar.get(), NP); });
	outVar.if_value_set([this, _rename]() { outVar = _rename(outVar.get(), NP); });
}

void ApplyTransformToVarData::apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context)
{
	base::apply_mesh_gen_params(_context);

	inVar.fill_value_with(_context);
	outVar.fill_value_with(_context);
	applyTransform.fill_value_with(_context);
}

void ApplyTransformToVarData::apply_transform(Matrix44 const & _transform)
{
	base::apply_transform(_transform);
}

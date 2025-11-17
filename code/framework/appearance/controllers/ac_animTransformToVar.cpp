#include "ac_animTransformToVar.h"

#include "..\appearanceControllerPoseContext.h"
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

DEFINE_STATIC_NAME(AnimTransformToVar);

//

REGISTER_FOR_FAST_CAST(AnimTransformToVar);

AnimTransformToVar::AnimTransformToVar(AnimTransformToVarData const * _data)
: base(_data)
, animTransformToVarData(_data)
{
	inVar = animTransformToVarData->inVar.get();
	outVar = animTransformToVarData->outVar.get();

	inMS = animTransformToVarData->inMS;

	for_every(atSrc, animTransformToVarData->animTransforms)
	{
		AnimTransform at;
		at.animTransform = atSrc->animTransform.get(Transform::identity);
		at.time = atSrc->time.get(0.5f);
		animTransforms.push_back(at);
	}
}

AnimTransformToVar::~AnimTransformToVar()
{
}

void AnimTransformToVar::initialise(ModuleAppearance* _owner)
{
	base::initialise(_owner);

	auto & varStorage = get_owner()->access_controllers_variable_storage();
	inVar.look_up<Transform>(varStorage);
	outVar.look_up<Transform>(varStorage);
}

void AnimTransformToVar::advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::advance_and_adjust_preliminary_pose(_context);
}

void AnimTransformToVar::calculate_final_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::calculate_final_pose(_context);

#ifdef MEASURE_PERFORMANCE_APPEARANCE_CONTROLLERS_DETAILED
	MEASURE_PERFORMANCE(AnimTransformToVar_finalPose);
#endif

	auto const * mesh = get_owner()->get_mesh();
	if (!mesh)
	{
		return;
	}

	float deltaTime = _context.get_delta_time();

	Transform animTransform = Transform::identity;

	if (!animTransforms.is_empty())
	{
		animTransformTimeLeft -= deltaTime;
		while (animTransformTimeLeft < 0.0f)
		{
			animTransformPrevIdx = animTransformIdx;
			animTransformIdx = mod(animTransformIdx + 1, animTransforms.get_size());

			animTransformTimeLeft += animTransforms[animTransformIdx].time;
		}

		auto& at = animTransforms[animTransformIdx];
		auto& pt = animTransforms[animTransformPrevIdx];
		animTransform = Transform::lerp(1.0f - animTransformTimeLeft / at.time, pt.animTransform, at.animTransform);
	}

	Transform inValue = inVar.get<Transform>();
	Transform newValue = inMS? animTransform : inValue.to_world(animTransform);
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

void AnimTransformToVar::get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
	OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const
{
	base::get_info_for_auto_processing_order(OUT_ dependsOnBones, OUT_ dependsOnParentBones, OUT_ usesBones, OUT_ providesBones,
		OUT_ dependsOnVariables, OUT_ providesVariables, OUT_ dependsOnControllers, OUT_ providesControllers);

	dependsOnVariables.push_back(inVar.get_name());
	providesVariables.push_back(outVar.get_name());
}

//

REGISTER_FOR_FAST_CAST(AnimTransformToVarData);

AppearanceControllerData* AnimTransformToVarData::create_data()
{
	return new AnimTransformToVarData();
}

void AnimTransformToVarData::register_itself()
{
	AppearanceControllers::register_appearance_controller(NAME(AnimTransformToVar), create_data);
}

AnimTransformToVarData::AnimTransformToVarData()
{
}

bool AnimTransformToVarData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= inVar.load_from_xml(_node, TXT("varID"));
	result &= outVar.load_from_xml(_node, TXT("varID"));
	result &= inVar.load_from_xml(_node, TXT("inVarID"));
	result &= outVar.load_from_xml(_node, TXT("outVarID"));

	inMS = _node->get_bool_attribute_or_from_child_presence(TXT("inMS"), inMS);

	for_every(node, _node->children_named(TXT("anim")))
	{
		AnimTransform at;
		result &= at.animTransform.load_from_xml(node, TXT("transform"));
		at.time.load_from_xml(node, TXT("time"));
		animTransforms.push_back(at);
	}

	return result;
}

bool AnimTransformToVarData::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

AppearanceControllerData* AnimTransformToVarData::create_copy() const
{
	AnimTransformToVarData* copy = new AnimTransformToVarData();
	*copy = *this;
	return copy;
}

AppearanceController* AnimTransformToVarData::create_controller()
{
	return new AnimTransformToVar(this);
}

void AnimTransformToVarData::reskin(Meshes::BoneRenameFunc _rename)
{
	base::reskin(_rename);

	inVar.if_value_set([this, _rename]() { inVar = _rename(inVar.get(), NP); });
	outVar.if_value_set([this, _rename]() { outVar = _rename(outVar.get(), NP); });
}

void AnimTransformToVarData::apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context)
{
	base::apply_mesh_gen_params(_context);

	inVar.fill_value_with(_context);
	outVar.fill_value_with(_context);

	for_every(at, animTransforms)
	{
		at->animTransform.fill_value_with(_context);
		at->time.fill_value_with(_context);
	}
}

void AnimTransformToVarData::apply_transform(Matrix44 const & _transform)
{
	base::apply_transform(_transform);
}

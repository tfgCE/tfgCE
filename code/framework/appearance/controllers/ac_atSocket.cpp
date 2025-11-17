#include "ac_atSocket.h"

#include "..\appearanceControllerPoseContext.h"
#include "..\appearanceControllers.h"

#include "..\..\meshGen\meshGenGenerationContext.h"
#include "..\..\meshGen\meshGenParamImpl.inl"
#include "..\..\module\moduleAppearance.h"
#include "..\..\module\moduleSound.h"

#include "..\..\..\core\mesh\pose.h"

#include "..\..\..\core\performance\performanceUtils.h"

//

using namespace Framework;
using namespace AppearanceControllersLib;

//

DEFINE_STATIC_NAME(atSocket);

//

REGISTER_FOR_FAST_CAST(AtSocket);

AtSocket::AtSocket(AtSocketData const * _data)
: base(_data)
, atSocketData(_data)
{
	bone = atSocketData->bone.is_set()? atSocketData->bone.get() : bone;
	placementVar = atSocketData->placementVar.is_set()? atSocketData->placementVar.get() : placementVar;
	variable = atSocketData->variable.is_set() ? atSocketData->variable.get() : variable;
	socket0.set_name(atSocketData->socket0.get());
	socket1.set_name(atSocketData->socket1.is_set() ? atSocketData->socket1.get() : atSocketData->socket0.get());
	value0 = atSocketData->value0.get();
	value1 = atSocketData->value1.get();
	blendTime = atSocketData->blendTime.get();
	blendCurve = atSocketData->blendCurve.is_set()? atSocketData->blendCurve.get() : blendCurve;
	transitionSound = atSocketData->transitionSound.get(transitionSound);
	if (atSocketData->transitionSoundAtSocket.is_set())
	{
		transitionSoundAtSocket = atSocketData->transitionSoundAtSocket.get();
	}
}

AtSocket::~AtSocket()
{
}

void AtSocket::initialise(ModuleAppearance* _owner)
{
	base::initialise(_owner);

	bone.look_up(get_owner()->get_skeleton()->get_skeleton());
	socket0.look_up(get_owner()->get_mesh());
	socket1.look_up(get_owner()->get_mesh());

	auto & varStorage = get_owner()->access_controllers_variable_storage();
	placementVar.look_up<Transform>(varStorage);
	variable.look_up<float>(varStorage);
}

void AtSocket::advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::advance_and_adjust_preliminary_pose(_context);
}

void AtSocket::calculate_final_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::calculate_final_pose(_context);

#ifdef MEASURE_PERFORMANCE_APPEARANCE_CONTROLLERS_DETAILED
	MEASURE_PERFORMANCE(atSocket_finalPose);
#endif

	auto const * mesh = get_owner()->get_mesh();

	if (!mesh ||
		(!bone.is_valid() && !placementVar.is_valid()) ||
		!socket0.is_valid() ||
		(!socket1.is_valid() && variable.is_valid()))
	{
		return;
	}

	Meshes::Pose * poseLS = _context.access_pose_LS();

	int boneIdx = bone.is_valid()? bone.get_index() : NONE;
	int boneParentIdx = boneIdx != NONE? poseLS->get_skeleton()->get_parent_of(boneIdx) : NONE;

	// get parent info
	Transform const boneParentMS = boneParentIdx != NONE? poseLS->get_bone_ms_from_ls(boneParentIdx) : Transform::identity;

	Transform currentPlacementMS = Transform::identity;

	if (bone.is_valid())
	{
		currentPlacementMS = poseLS->get_bone_ms_from_ls(boneIdx);
	}
	if (placementVar.is_valid())
	{
		currentPlacementMS = placementVar.get<Transform>();
	}

	// get both sockets
	Transform socket0MS = mesh->calculate_socket_using_ls(socket0.get_index(), poseLS);
	Transform socket1MS = socket1.is_valid()? mesh->calculate_socket_using_ls(socket1.get_index(), poseLS) : socket0MS;
	if (!socket1.is_valid())
	{
		socket1MS = socket0MS;
		socket0MS = currentPlacementMS;
	}

	// where should we be
	float shouldBeAt = variable.is_valid() ? (has_just_activated() ? variable.access<float>() : blend_to_using_speed_based_on_time(variableValue, variable.access<float>(), blendTime, _context.get_delta_time()))
										   : 1.0f;
	variableValue = shouldBeAt;
	if (value1 != value0)
	{
		shouldBeAt = (shouldBeAt - value0) / (value1 - value0);
		shouldBeAt = clamp(shouldBeAt, 0.0f, 1.0f);
	}
	else
	{
		shouldBeAt = 1.0f;
	}

	// sounds
	if (transitionSound.is_valid())
	{
		bool sameState = shouldBeAt == lastShouldBeAt;
		if (!playedTransitionSound.is_set() && !sameState)
		{
			if (auto* ms = get_owner()->get_owner()->get_sound())
			{
				playedTransitionSound = ms->play_sound(transitionSound, transitionSoundAtSocket);
			}
		}
		if (playedTransitionSound.is_set() && sameState)
		{
			playedTransitionSound->stop();
			playedTransitionSound = nullptr;
		}
	}

	lastShouldBeAt = shouldBeAt;

	// apply blend type
	shouldBeAt = BlendCurve::apply(blendCurve, shouldBeAt);
	float useActive = BlendCurve::apply(blendCurve, get_active());

	// blend sockets/placements + active
	Transform resultMS = Transform::lerp(shouldBeAt, socket0MS, socket1MS);
	resultMS = Transform::lerp(useActive, currentPlacementMS, resultMS);

	if (bone.is_valid())
	{
		// write back
		poseLS->access_bones()[boneIdx] = boneParentMS.to_local(resultMS);
	}
	if (placementVar.is_valid())
	{
		placementVar.access<Transform>() = resultMS;
	}
}

void AtSocket::get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
	OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const
{
	base::get_info_for_auto_processing_order(OUT_ dependsOnBones, OUT_ dependsOnParentBones, OUT_ usesBones, OUT_ providesBones,
		OUT_ dependsOnVariables, OUT_ providesVariables, OUT_ dependsOnControllers, OUT_ providesControllers);

	if (bone.is_valid())
	{
		dependsOnParentBones.push_back(bone.get_index());
		providesBones.push_back(bone.get_index());
	}
	if (variable.is_valid()) dependsOnVariables.push_back(variable.get_name());
	if (auto const * mesh = get_owner()->get_mesh())
	{
		if (auto const * skeleton = get_owner()->get_skeleton())
		{
			int boneIdx;
			Transform relativePlacement;
			if (socket0.is_valid() && mesh->get_socket_info(socket0.get_index(), OUT_ boneIdx, OUT_ relativePlacement))
			{
				dependsOnBones.push_back(boneIdx);
			}
			if (socket1.is_valid() && mesh->get_socket_info(socket1.get_index(), OUT_ boneIdx, OUT_ relativePlacement))
			{
				dependsOnBones.push_back(boneIdx);
			}
		}
	}
}

void AtSocket::internal_log(LogInfoContext & _log) const
{
	_log.log(TXT("bone: %S [%i]"), bone.get_name().to_char(), bone.get_index());
	_log.log(TXT("socket0: %S"), socket0.get_name().to_char());
	_log.log(TXT("socket1: %S"), socket1.get_name().to_char());
	_log.log(TXT("variable: %S"), variable.get_name().to_char());
	_log.log(TXT("variableValue: %.3f"), variableValue);
}

//

REGISTER_FOR_FAST_CAST(AtSocketData);

AppearanceControllerData* AtSocketData::create_data()
{
	return new AtSocketData();
}

void AtSocketData::register_itself()
{
	AppearanceControllers::register_appearance_controller(NAME(atSocket), create_data);
}

AtSocketData::AtSocketData()
{
}

bool AtSocketData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= bone.load_from_xml(_node, TXT("bone"));
	result &= placementVar.load_from_xml(_node, TXT("placementVarID"));
	result &= variable.load_from_xml(_node, TXT("varID"));
	result &= socket0.load_from_xml(_node, TXT("socket"));
	result &= socket0.load_from_xml(_node, TXT("socket0"));
	result &= socket1.load_from_xml(_node, TXT("socket1"));
	result &= value0.load_from_xml(_node, TXT("value0"));
	result &= value1.load_from_xml(_node, TXT("value1"));
	result &= blendTime.load_from_xml(_node, TXT("blendTime"));
	result &= blendCurve.load_from_xml(_node, TXT("blendCurve"));
	result &= transitionSound.load_from_xml(_node, TXT("transitionSound"));
	result &= transitionSoundAtSocket.load_from_xml(_node, TXT("transitionSoundAtSocket"));

	if (!bone.is_set() && ! placementVar.is_set())
	{
		error_loading_xml(_node, TXT("no bone or placementVar provided"));
		result = false;
	}
	if (!socket0.is_set())
	{
		error_loading_xml(_node, TXT("at least one socket required"));
		result = false;
	}

	return result;
}

bool AtSocketData::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

AppearanceControllerData* AtSocketData::create_copy() const
{
	AtSocketData* copy = new AtSocketData();
	*copy = *this;
	return copy;
}

AppearanceController* AtSocketData::create_controller()
{
	return new AtSocket(this);
}

void AtSocketData::reskin(Meshes::BoneRenameFunc _rename)
{
	base::reskin(_rename);
	bone.if_value_set([this, _rename](){ bone = _rename(bone.get(), NP); });
	placementVar.if_value_set([this, _rename](){ placementVar = _rename(placementVar.get(), NP); });
	variable.if_value_set([this, _rename](){ variable = _rename(variable.get(), NP); });
	socket0.if_value_set([this, _rename](){ socket0 = _rename(socket0.get(), NP); });
	socket1.if_value_set([this, _rename](){ socket1 = _rename(socket1.get(), NP); });
}

void AtSocketData::apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context)
{
	base::apply_mesh_gen_params(_context);

	bone.fill_value_with(_context);
	placementVar.fill_value_with(_context);
	variable.fill_value_with(_context);
	socket0.fill_value_with(_context);
	socket1.fill_value_with(_context);
	value0.fill_value_with(_context);
	value1.fill_value_with(_context);
	blendTime.fill_value_with(_context);
	blendCurve.fill_value_with(_context);
}


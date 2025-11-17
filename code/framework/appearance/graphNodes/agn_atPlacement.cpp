#include "agn_atPlacement.h"

#include "..\animationGraphNodes.h"
#include "..\animationSet.h"

#include "..\..\module\moduleAppearance.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace AnimationGraphNodesLib;

//

REGISTER_FOR_FAST_CAST(AtPlacement);

IAnimationGraphNode* AtPlacement::create_node()
{
	return new AtPlacement();
}

bool AtPlacement::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	XML_LOAD_NAME_ATTR(_node, bone);
	placement.load_from_xml(_node);
	placementType = AppearanceControllersLib::AtPlacementType::parse(_node->get_string_attribute_or_from_child(TXT("type")), placementType);

	return result;
}

bool AtPlacement::prepare_runtime(AnimationGraphRuntime& _runtime, AnimationGraphNodeSet& _inSet)
{
	bool result = base::prepare_runtime(_runtime, _inSet);

	ANIMATION_GRAPH_RUNTIME_VARIABLES_START_DECLARING(AtPlacement);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_DECLARE_NO_DEFAULT(Meshes::BoneID, bone);

	return result;
}

void AtPlacement::prepare_instance(AnimationGraphRuntime& _runtime) const
{
	base::prepare_instance(_runtime);

	ANIMATION_GRAPH_RUNTIME_VARIABLES_START_USING(AtPlacement);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_USE(Meshes::BoneID, bone);

	bone.invalidate();
}

void AtPlacement::on_activate(AnimationGraphRuntime& _runtime) const
{
	base::on_activate(_runtime);

	ANIMATION_GRAPH_RUNTIME_VARIABLES_START_USING(AtPlacement);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_NOT_USED(Meshes::BoneID, bone);
}

void AtPlacement::on_reactivate(AnimationGraphRuntime& _runtime) const
{
	base::on_reactivate(_runtime);

	//ANIMATION_GRAPH_RUNTIME_VARIABLES_START_USING(AtPlacement);
}

void AtPlacement::on_deactivate(AnimationGraphRuntime& _runtime) const
{
	base::on_deactivate(_runtime);

	//ANIMATION_GRAPH_RUNTIME_VARIABLES_START_USING(AtPlacement);
}

void AtPlacement::execute(AnimationGraphRuntime& _runtime, AnimationGraphExecuteContext& _context, OUT_ AnimationGraphOutput& _output) const
{
	base::execute(_runtime, _context, _output);

	ANIMATION_GRAPH_RUNTIME_VARIABLES_START_USING(AtPlacement);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_USE(Meshes::BoneID, bone);

	if (!bone.is_valid())
	{
		bone.set_name(this->bone);
		bone.look_up(_output.access_pose()->get_skeleton());
	}
	if (bone.is_valid())
	{
		if (placementType == AppearanceControllersLib::AtPlacementType::BS)
		{
			_output.access_pose()->set_bone(bone.get_index(), placement);
		}
		else if (placementType == AppearanceControllersLib::AtPlacementType::WS)
		{
			Transform placementMS = _runtime.owner->get_ms_to_ws_transform().to_local(placement);
			_output.access_pose()->set_bone_ms(bone.get_index(), placementMS);
		}
		else
		{
			an_assert(placementType == AppearanceControllersLib::AtPlacementType::MS);
			_output.access_pose()->set_bone_ms(bone.get_index(), placement);
		}
	}
}

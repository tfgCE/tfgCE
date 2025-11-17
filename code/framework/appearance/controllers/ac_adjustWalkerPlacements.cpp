#include "ac_adjustWalkerPlacements.h"

#include "..\appearanceControllerPoseContext.h"
#include "..\appearanceControllers.h"

#include "..\..\meshGen\meshGenGenerationContext.h"
#include "..\..\meshGen\meshGenParamImpl.inl"
#include "..\..\module\moduleAppearance.h"
#include "..\..\module\moduleSound.h"

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

DEFINE_STATIC_NAME(adjustWalkerPlacements);

//

REGISTER_FOR_FAST_CAST(AdjustWalkerPlacements);

AdjustWalkerPlacements::AdjustWalkerPlacements(AdjustWalkerPlacementsData const * _data)
: base(_data)
{
	adjust.make_space_for(_data->adjust.get_size());
	for_every(as, _data->adjust)
	{
		adjust.grow_size(1);
		Adjust& a = adjust.get_last();
		if (as->input.useBoneMSLocZPt.is_set())
		{
			a.input.useBoneMSLocZPt = as->input.useBoneMSLocZPt.get();
		}

		a.walkerPlacements.make_space_for(as->walkerPlacements.get_size());
		for_every(wps, as->walkerPlacements)
		{
			a.walkerPlacements.grow_size(1);
			Adjust::WalkerPlacement& wp = a.walkerPlacements.get_last();
			wp.bone = wps->bone.get();
			wp.offsetLoc = wps->offsetLoc;
			wp.rotate = wps->rotate;

			{
				bool found = false;
				for_every(awp, allWalkerPlacements)
				{
					if (wp.bone.get_name() == awp->bone.get_name())
					{
						found = true;
						break;
					}
				}
				if (!found)
				{
					WalkerPlacement nwp;
					nwp.bone = wp.bone;
					allWalkerPlacements.push_back(nwp);
				}
			}
		}

		a.provideDirs.make_space_for(as->provideDirs.get_size());
		for_every(pds, as->provideDirs)
		{
			a.provideDirs.grow_size(1);
			Adjust::ProvideDir& pd = a.provideDirs.get_last();
			pd.dirVar = pds->dirVar.get();
			pd.dir = pds->dir;
		}
	}

	offsetCentre.make_space_for(_data->offsetCentre.get_size());
	for_every(ocs, _data->offsetCentre)
	{
		offsetCentre.grow_size(1);
		OffsetCentre& oc = offsetCentre.get_last();
		if (ocs->bone.is_set())
		{
			oc.bone = ocs->bone.get();
		}
		if (ocs->weight.is_set())
		{
			oc.weight = ocs->weight.get();
		}
	}
}

AdjustWalkerPlacements::~AdjustWalkerPlacements()
{
}

void AdjustWalkerPlacements::initialise(ModuleAppearance* _owner)
{
	base::initialise(_owner);

	auto& varStorage = get_owner()->access_controllers_variable_storage();

	if (auto* skel = get_owner()->get_skeleton()->get_skeleton())
	{
		for_every(a, adjust)
		{
			a->input.useBoneMSLocZPt.look_up(skel);
			for_every(wp, a->walkerPlacements)
			{
				wp->bone.look_up(skel);
				wp->parentBone = Meshes::BoneID(skel, skel->get_parent_of(wp->bone.get_index()));
			}
			for_every(pd, a->provideDirs)
			{
				pd->dirVar.look_up<Vector3>(varStorage);
			}
		}
		for_every(oc, offsetCentre)
		{
			oc->bone.look_up(skel);
		}
		for_every(awp, allWalkerPlacements)
		{
			awp->bone.look_up(skel);
			awp->parentBone = Meshes::BoneID(skel, skel->get_parent_of(awp->bone.get_index()));
		}
	}
}

void AdjustWalkerPlacements::advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::advance_and_adjust_preliminary_pose(_context);
}

void AdjustWalkerPlacements::calculate_final_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::calculate_final_pose(_context);

#ifdef MEASURE_PERFORMANCE_APPEARANCE_CONTROLLERS_DETAILED
	MEASURE_PERFORMANCE(adjustWalkerPlacements_finalPose);
#endif

	auto const * mesh = get_owner()->get_mesh();

	if (!mesh ||
		get_active() == 0.0f)
	{
		return;
	}

	Meshes::Pose * poseLS = _context.access_pose_LS();

	struct CalculateCentre
	{
		static Vector3 calc(Array<OffsetCentre> const & offsetCentre, Meshes::Pose const * _usingPoseLS)
		{
			Vector3 weightCentre = Vector3::zero;
			float weightTotal = 0.0f;
			for_every(oc, offsetCentre)
			{
				if (oc->bone.is_valid())
				{
					weightTotal += oc->weight;
					weightCentre += _usingPoseLS->get_bone_ms_from_ls(oc->bone.get_index()).get_translation() * oc->weight;
				}
			}

			if (weightTotal != 0.0f)
			{
				return weightCentre / weightTotal;
			}
			else
			{
				return Vector3::zero;
			}
		}
	};

	Vector3 useOffsetCentre = Vector3::zero;
	{
		useOffsetCentre += CalculateCentre::calc(offsetCentre, poseLS);
	}
	if (!defaultCentreMS.is_set())
	{
		defaultCentreMS = CalculateCentre::calc(offsetCentre, poseLS->get_skeleton()->get_default_pose_ls());
	}

	{
		Vector3 oc = useOffsetCentre - defaultCentreMS.get();
		oc.z = 0.0f;
		for_every(awp, allWalkerPlacements)
		{
			if (awp->bone.is_valid())
			{
				Transform parentBoneMS= poseLS->get_bone_ms_from_ls(awp->parentBone.get_index());
				Transform boneMS = parentBoneMS.to_world(poseLS->get_bone(awp->bone.get_index()));

				boneMS.set_translation(boneMS.get_translation() + oc);

				poseLS->set_bone(awp->bone.get_index(), parentBoneMS.to_local(boneMS));
			}
		}
	}

	for_every(a, adjust)
	{
		float value = 0.0f;
		{
			if (a->input.useBoneMSLocZPt.is_valid())
			{
				Vector3 loc = poseLS->get_bone_ms_from_ls(a->input.useBoneMSLocZPt.get_index()).get_translation();
				Vector3 defloc = poseLS->get_skeleton()->get_default_pose_ls()->get_bone_ms_from_ls(a->input.useBoneMSLocZPt.get_index()).get_translation();
				if (defloc.z != 0.0f)
				{
					value += loc.z / defloc.z;
				}
			}
		}
		for_every(wp, a->walkerPlacements)
		{
			if (wp->bone.is_valid())
			{
				Transform parentBoneMS = poseLS->get_bone_ms_from_ls(wp->parentBone.get_index());
				Transform boneMS = parentBoneMS.to_world(poseLS->get_bone(wp->bone.get_index()));

				if (wp->offsetLoc.is_valid())
				{
					boneMS.set_translation(boneMS.get_translation() + wp->offsetLoc.remap(value));
				}
				if (wp->rotate.is_valid())
				{
					boneMS.set_orientation(boneMS.get_orientation().to_world(wp->rotate.remap(value).to_quat()));
				}

				poseLS->set_bone(wp->bone.get_index(), parentBoneMS.to_local(boneMS));
			}
		}
		for_every(pd, a->provideDirs)
		{
			if (pd->dirVar.is_valid())
			{
				Vector3 dir = Vector3::zero;

				if (pd->dir.is_valid())
				{
					dir += pd->dir.remap(value);
				}

				if (!dir.is_zero())
				{
					pd->dirVar.access<Vector3>() = dir.normal();
				}
			}
		}
	}
}

void AdjustWalkerPlacements::get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
	OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const
{
	base::get_info_for_auto_processing_order(OUT_ dependsOnBones, OUT_ dependsOnParentBones, OUT_ usesBones, OUT_ providesBones,
		OUT_ dependsOnVariables, OUT_ providesVariables, OUT_ dependsOnControllers, OUT_ providesControllers);

	for_every(oc, offsetCentre)
	{
		dependsOnBones.push_back(oc->bone.get_index());
	}
	for_every(a, adjust)
	{
		if (a->input.useBoneMSLocZPt.is_valid())
		{
			dependsOnBones.push_back(a->input.useBoneMSLocZPt.get_index());
		}
		for_every(pd, a->provideDirs)
		{
			providesVariables.push_back(pd->dirVar.get_name());
		}
	}
	for_every(awp, allWalkerPlacements)
	{
		providesBones.push_back(awp->bone.get_index());
	}
}

//

REGISTER_FOR_FAST_CAST(AdjustWalkerPlacementsData);

AppearanceControllerData* AdjustWalkerPlacementsData::create_data()
{
	return new AdjustWalkerPlacementsData();
}

void AdjustWalkerPlacementsData::register_itself()
{
	AppearanceControllers::register_appearance_controller(NAME(adjustWalkerPlacements), create_data);
}

AdjustWalkerPlacementsData::AdjustWalkerPlacementsData()
{
}

bool AdjustWalkerPlacementsData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	adjust.clear();
	for_every(node, _node->children_named(TXT("adjust")))
	{
		Adjust a;
		for_every(inputNode, node->children_named(TXT("input")))
		{
			a.input.useBoneMSLocZPt.load_from_xml(inputNode, TXT("useBoneMSLocZPt"));
		}
		for_every(wpNode, node->children_named(TXT("walkerPlacement")))
		{
			Adjust::WalkerPlacement wp;
			wp.bone.load_from_xml(wpNode, TXT("bone"));
			wp.offsetLoc.load_from_xml(wpNode->first_child_named(TXT("offsetLoc")));
			wp.rotate.load_from_xml(wpNode->first_child_named(TXT("rotate")));
			a.walkerPlacements.push_back(wp);
		}
		for_every(pdNode, node->children_named(TXT("provideDir")))
		{
			Adjust::ProvideDir pd;
			pd.dirVar.load_from_xml(pdNode, TXT("varID"));
			pd.dir.load_from_xml(pdNode->first_child_named(TXT("dir")));
			a.provideDirs.push_back(pd);
		}
		adjust.push_back(a);
	}

	offsetCentre.clear();
	for_every(node, _node->children_named(TXT("offsetCentre")))
	{
		OffsetCentre oc;
		oc.bone.load_from_xml(node, TXT("bone"));
		oc.weight.load_from_xml(node, TXT("weight"));
		offsetCentre.push_back(oc);
	}

	return result;
}

bool AdjustWalkerPlacementsData::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

AppearanceControllerData* AdjustWalkerPlacementsData::create_copy() const
{
	AdjustWalkerPlacementsData* copy = new AdjustWalkerPlacementsData();
	*copy = *this;
	return copy;
}

AppearanceController* AdjustWalkerPlacementsData::create_controller()
{
	return new AdjustWalkerPlacements(this);
}

void AdjustWalkerPlacementsData::reskin(Meshes::BoneRenameFunc _rename)
{
	base::reskin(_rename);
	for_every(a, adjust)
	{
		a->input.useBoneMSLocZPt.if_value_set([a, _rename]() { a->input.useBoneMSLocZPt = _rename(a->input.useBoneMSLocZPt.get(), NP); });
		for_every(wp, a->walkerPlacements)
		{
			wp->bone.if_value_set([wp, _rename]() { wp->bone = _rename(wp->bone.get(), NP); });
		}
		for_every(pd, a->provideDirs)
		{
			pd->dirVar.if_value_set([pd, _rename]() { pd->dirVar = _rename(pd->dirVar.get(), NP); });
		}
	}
	for_every(oc, offsetCentre)
	{
		oc->bone.if_value_set([oc, _rename]() { oc->bone = _rename(oc->bone.get(), NP); });
	}
}

void AdjustWalkerPlacementsData::apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context)
{
	base::apply_mesh_gen_params(_context);

	for_every(a, adjust)
	{
		a->input.useBoneMSLocZPt.fill_value_with(_context);
		for_every(wp, a->walkerPlacements)
		{
			wp->bone.fill_value_with(_context);
		}
		for_every(pd, a->provideDirs)
		{
			pd->dirVar.fill_value_with(_context);
		}
	}
	for_every(oc, offsetCentre)
	{
		oc->bone.fill_value_with(_context);
		oc->weight.fill_value_with(_context);
	}
}


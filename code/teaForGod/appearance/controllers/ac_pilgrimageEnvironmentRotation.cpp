#include "ac_pilgrimageEnvironmentRotation.h"

#include "..\..\pilgrimage\pilgrimageInstance.h"

#include "..\..\..\framework\appearance\appearanceControllerPoseContext.h"
#include "..\..\..\framework\appearance\appearanceControllers.h"
#include "..\..\..\framework\meshGen\meshGenGenerationContext.h"
#include "..\..\..\framework\meshGen\meshGenParamImpl.inl"
#include "..\..\..\framework\module\moduleAppearance.h"

#include "..\..\..\core\performance\performanceUtils.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace AppearanceControllersLib;

//

DEFINE_STATIC_NAME(PilgrimageEnvironmentRotation);

//

REGISTER_FOR_FAST_CAST(PilgrimageEnvironmentRotation);

PilgrimageEnvironmentRotation::PilgrimageEnvironmentRotation(PilgrimageEnvironmentRotationData const * _data)
: base(_data)
, pilgrimageEnvironmentRotationData(_data)
{
	asRotatorVar = pilgrimageEnvironmentRotationData->asRotatorVar.get();
	negate = pilgrimageEnvironmentRotationData->negate;
}

PilgrimageEnvironmentRotation::~PilgrimageEnvironmentRotation()
{
}

void PilgrimageEnvironmentRotation::initialise(Framework::ModuleAppearance* _owner)
{
	base::initialise(_owner);

	auto & varStorage = get_owner()->access_controllers_variable_storage();
	asRotatorVar.look_up<Rotator3>(varStorage);
}

void PilgrimageEnvironmentRotation::advance_and_adjust_preliminary_pose(REF_ Framework::AppearanceControllerPoseContext & _context)
{
	base::advance_and_adjust_preliminary_pose(_context);
}

void PilgrimageEnvironmentRotation::calculate_final_pose(REF_ Framework::AppearanceControllerPoseContext & _context)
{
	base::calculate_final_pose(_context);

#ifdef MEASURE_PERFORMANCE_APPEARANCE_CONTROLLERS_DETAILED
	MEASURE_PERFORMANCE(restPointPlacement_finalPose);
#endif

	auto const * mesh = get_owner()->get_mesh();

	if (!mesh)
	{
		return;
	}

	if (auto* pi = PilgrimageInstance::get())
	{
		float yaw = pi->get_environment_rotation_yaw();
		if (negate) yaw = -yaw;

		if (asRotatorVar.is_valid())
		{
			asRotatorVar.access<Rotator3>() = Rotator3(0.0f, yaw, 0.0f);
		}
	}
}

void PilgrimageEnvironmentRotation::get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
	OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const
{
	if (asRotatorVar.is_valid())
	{
		providesVariables.push_back(asRotatorVar.get_name());
	}
}

//

REGISTER_FOR_FAST_CAST(PilgrimageEnvironmentRotationData);

Framework::AppearanceControllerData* PilgrimageEnvironmentRotationData::create_data()
{
	return new PilgrimageEnvironmentRotationData();
}

void PilgrimageEnvironmentRotationData::register_itself()
{
	Framework::AppearanceControllers::register_appearance_controller(NAME(PilgrimageEnvironmentRotation), create_data);
}

PilgrimageEnvironmentRotationData::PilgrimageEnvironmentRotationData()
{
}

bool PilgrimageEnvironmentRotationData::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= asRotatorVar.load_from_xml(_node, TXT("asRotatorVarID"));

	negate = _node->get_bool_attribute_or_from_child_presence(TXT("negate"), negate);

	return result;
}

bool PilgrimageEnvironmentRotationData::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

Framework::AppearanceControllerData* PilgrimageEnvironmentRotationData::create_copy() const
{
	PilgrimageEnvironmentRotationData* copy = new PilgrimageEnvironmentRotationData();
	*copy = *this;
	return copy;
}

Framework::AppearanceController* PilgrimageEnvironmentRotationData::create_controller()
{
	return new PilgrimageEnvironmentRotation(this);
}

void PilgrimageEnvironmentRotationData::apply_mesh_gen_params(Framework::MeshGeneration::GenerationContext const & _context)
{
	base::apply_mesh_gen_params(_context);

	asRotatorVar.fill_value_with(_context);
}

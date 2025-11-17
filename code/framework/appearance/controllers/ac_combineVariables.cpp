#include "ac_combineVariables.h"

#include "..\appearanceControllerPoseContext.h"
#include "..\appearanceControllers.h"

#include "..\..\meshGen\meshGenGenerationContext.h"
#include "..\..\meshGen\meshGenParamImpl.inl"
#include "..\..\module\moduleAppearance.h"

#include "..\..\..\core\mesh\pose.h"

#include "..\..\..\core\performance\performanceUtils.h"

//

using namespace Framework;
using namespace AppearanceControllersLib;

//

DEFINE_STATIC_NAME(combineVariables);

//

REGISTER_FOR_FAST_CAST(CombineVariables);

CombineVariables::CombineVariables(CombineVariablesData const * _data)
: base(_data)
, combineVariablesData(_data)
{
	combine.make_space_for(combineVariablesData->combine.get_size());
	for_every(cs, combineVariablesData->combine)
	{
		Combine c;
		if (cs->aVar.is_set()) c.aVar = cs->aVar.get();
		if (cs->bVar.is_set()) c.bVar = cs->bVar.get();
		if (cs->outVar.is_set()) c.outVar = cs->outVar.get();
		c.operation = cs->operation;
		combine.push_back(c);
	}
}

CombineVariables::~CombineVariables()
{
}

void CombineVariables::initialise(ModuleAppearance* _owner)
{
	base::initialise(_owner);

	auto& storage = _owner->get_owner()->access_variables();

	for_every(c, combine)
	{
		c->aVar.look_up<bool>(storage);
		c->bVar.look_up<bool>(storage);
		c->outVar.look_up<bool>(storage);
	}
}

void CombineVariables::advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::advance_and_adjust_preliminary_pose(_context);
}

void CombineVariables::calculate_final_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::calculate_final_pose(_context);

	if (! get_active())
	{
		return;
	}

#ifdef MEASURE_PERFORMANCE_APPEARANCE_CONTROLLERS_DETAILED
	MEASURE_PERFORMANCE(combineVariables_finalPose);
#endif

	for_every(c, combine)
	{
		bool result = false;
		if (c->operation == Operation::And) result = c->aVar.get<bool>() && c->bVar.get<bool>();
		if (c->operation == Operation::Or) result = c->aVar.get<bool>() || c->bVar.get<bool>();
		if (c->operation == Operation::Not) result = ! c->aVar.get<bool>();
		c->outVar.access<bool>() = result;
	}
}

void CombineVariables::get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
	OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const
{
	base::get_info_for_auto_processing_order(OUT_ dependsOnBones, OUT_ dependsOnParentBones, OUT_ usesBones, OUT_ providesBones,
		OUT_ dependsOnVariables, OUT_ providesVariables, OUT_ dependsOnControllers, OUT_ providesControllers);

	for_every(c, combine)
	{
		if (c->aVar.get_name().is_valid())
		{
			dependsOnVariables.push_back(c->aVar.get_name());
		}
		if (c->bVar.get_name().is_valid())
		{
			dependsOnVariables.push_back(c->bVar.get_name());
		}
		if (c->outVar.get_name().is_valid())
		{
			providesVariables.push_back(c->outVar.get_name());
		}
	}
}

//

REGISTER_FOR_FAST_CAST(CombineVariablesData);

AppearanceControllerData* CombineVariablesData::create_data()
{
	return new CombineVariablesData();
}

void CombineVariablesData::register_itself()
{
	AppearanceControllers::register_appearance_controller(NAME(combineVariables), create_data);
}

CombineVariablesData::CombineVariablesData()
{
}

bool CombineVariablesData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	for_every(node, _node->all_children())
	{
		Combine c;
		bool valid = false;
		int inReq = 0;
		if (node->get_name() == TXT("and")) { c.operation = CombineVariables::Operation::And; valid = true; inReq = 2; }
		if (node->get_name() == TXT("or")) { c.operation = CombineVariables::Operation::Or; valid = true; inReq = 2; }
		if (node->get_name() == TXT("not")) { c.operation = CombineVariables::Operation::Not; valid = true; inReq = 1; }
		c.aVar.load_from_xml(node, TXT("aVarID"));
		c.bVar.load_from_xml(node, TXT("bVarID"));
		c.outVar.load_from_xml(node, TXT("outVarID"));

		if (inReq >= 1)
		{
			error_loading_xml_on_assert(c.aVar.is_set(), result, node, TXT("no aVarID provided"));
		}
		if (inReq >= 2)
		{
			error_loading_xml_on_assert(c.bVar.is_set(), result, node, TXT("no bVarID provided"));
		}
		error_loading_xml_on_assert(c.outVar.is_set(), result, node, TXT("no outVarID provided"));

		combine.push_back(c);
	}

	return result;
}

bool CombineVariablesData::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

AppearanceControllerData* CombineVariablesData::create_copy() const
{
	CombineVariablesData* copy = new CombineVariablesData();
	*copy = *this;
	return copy;
}

AppearanceController* CombineVariablesData::create_controller()
{
	return new CombineVariables(this);
}

void CombineVariablesData::apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context)
{
	base::apply_mesh_gen_params(_context);

	for_every(c, combine)
	{
		c->aVar.fill_value_with(_context);
		c->bVar.fill_value_with(_context);
		c->outVar.fill_value_with(_context);
	}
}

void CombineVariablesData::apply_transform(Matrix44 const & _transform)
{
	base::apply_transform(_transform);
}

void CombineVariablesData::reskin(Meshes::BoneRenameFunc _rename)
{
	base::reskin(_rename);

	for_every(c, combine)
	{
		c->aVar.if_value_set([&c, _rename](){ c->aVar = _rename(c->aVar.get(), NP); });
		c->bVar.if_value_set([&c, _rename](){ c->bVar = _rename(c->bVar.get(), NP); });
		c->outVar.if_value_set([&c, _rename](){ c->outVar = _rename(c->outVar.get(), NP); });
	}
}

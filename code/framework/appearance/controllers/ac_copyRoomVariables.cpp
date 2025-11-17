#include "ac_copyRoomVariables.h"

#include "..\appearanceControllerPoseContext.h"
#include "..\appearanceControllers.h"

#include "..\..\meshGen\meshGenGenerationContext.h"
#include "..\..\meshGen\meshGenParamImpl.inl"
#include "..\..\module\moduleAppearance.h"
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

DEFINE_STATIC_NAME(copyRoomVariables);

//

REGISTER_FOR_FAST_CAST(CopyRoomVariables);

CopyRoomVariables::CopyRoomVariables(CopyRoomVariablesData const * _data)
: base(_data)
, copyRoomVariablesData(_data)
{
	copy.make_space_for(copyRoomVariablesData->copy.get_size());
	for_every(cs, copyRoomVariablesData->copy)
	{
		Copy c;
		if (cs->srcVar.is_set()) c.srcVar = cs->srcVar.get();
		if (cs->destVar.is_set()) c.destVar = cs->destVar.get();
		c.varType = cs->varType;
		copy.push_back(c);
	}

	interval = _data->interval.get();
	timeToCopyLeft = rg.get(interval);
}

CopyRoomVariables::~CopyRoomVariables()
{
}

void CopyRoomVariables::initialise(ModuleAppearance* _owner)
{
	base::initialise(_owner);

	auto& ownerStorage = _owner->get_owner()->access_variables();

	for_every(c, copy)
	{
		c->destVar.look_up(ownerStorage, c->varType);
	}

	rg.set_seed(_owner);
}

void CopyRoomVariables::advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::advance_and_adjust_preliminary_pose(_context);
}

void CopyRoomVariables::calculate_final_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::calculate_final_pose(_context);

	if (! get_active())
	{
		return;
	}

	timeToCopyLeft -= _context.get_delta_time();

	if (timeToCopyLeft < 0.0f)
	{
		timeToCopyLeft = rg.get(interval);

		auto* imo = get_owner()->get_owner();
		if (auto* inRoom = imo->get_presence()->get_in_room())
		{
			for_every(c, copy)
			{
				if (auto* src = inRoom->get_variables().get_existing(c->srcVar, c->varType))
				{
					RegisteredType::copy(c->varType, c->destVar.access_raw(), src);
				}
			}
		}
	}
}

void CopyRoomVariables::get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
	OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const
{
	base::get_info_for_auto_processing_order(OUT_ dependsOnBones, OUT_ dependsOnParentBones, OUT_ usesBones, OUT_ providesBones,
		OUT_ dependsOnVariables, OUT_ providesVariables, OUT_ dependsOnControllers, OUT_ providesControllers);

	for_every(c, copy)
	{
		if (c->destVar.get_name().is_valid())
		{
			providesVariables.push_back(c->destVar.get_name());
		}
	}
}

//

REGISTER_FOR_FAST_CAST(CopyRoomVariablesData);

AppearanceControllerData* CopyRoomVariablesData::create_data()
{
	return new CopyRoomVariablesData();
}

void CopyRoomVariablesData::register_itself()
{
	AppearanceControllers::register_appearance_controller(NAME(copyRoomVariables), create_data);
}

CopyRoomVariablesData::CopyRoomVariablesData()
{
}

bool CopyRoomVariablesData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	interval.load_from_xml(_node, TXT("interval"));

	for_every(node, _node->all_children())
	{
		// type is node name or "type" attribute
		Copy c;
		c.srcVar.load_from_xml(node, TXT("srcVarID"));
		c.destVar.load_from_xml(node, TXT("destVarID"));
		String typeId;
		if (auto* a = node->get_attribute(TXT("type")))
		{
			typeId = a->get_as_string();
		}
		else
		{
			typeId = node->get_name();
		}
		c.varType = RegisteredType::get_type_id_by_name(typeId.to_char());

		copy.push_back(c);
	}

	return result;
}

bool CopyRoomVariablesData::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

AppearanceControllerData* CopyRoomVariablesData::create_copy() const
{
	CopyRoomVariablesData* copy = new CopyRoomVariablesData();
	*copy = *this;
	return copy;
}

AppearanceController* CopyRoomVariablesData::create_controller()
{
	return new CopyRoomVariables(this);
}

void CopyRoomVariablesData::apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context)
{
	base::apply_mesh_gen_params(_context);

	interval.fill_value_with(_context);
	for_every(c, copy)
	{
		c->srcVar.fill_value_with(_context);
		c->destVar.fill_value_with(_context);
	}
}

void CopyRoomVariablesData::apply_transform(Matrix44 const & _transform)
{
	base::apply_transform(_transform);
}

void CopyRoomVariablesData::reskin(Meshes::BoneRenameFunc _rename)
{
	base::reskin(_rename);

	for_every(c, copy)
	{
		c->srcVar.if_value_set([&c, _rename](){ c->srcVar = _rename(c->srcVar.get(), NP); });
		c->destVar.if_value_set([&c, _rename](){ c->destVar = _rename(c->destVar.get(), NP); });
	}
}

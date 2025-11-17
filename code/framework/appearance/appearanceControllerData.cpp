#include "appearanceControllerData.h"

#include "..\..\core\io\xml.h"

#include "..\meshGen\meshGenGenerationContext.h"
#include "..\meshGen\meshGenParamImpl.inl"

//

using namespace Framework;

//

REGISTER_FOR_FAST_CAST(AppearanceControllerData);

AppearanceControllerData::AppearanceControllerData()
: processAtLevel(0)
{
}

AppearanceControllerData::~AppearanceControllerData()
{
}

bool AppearanceControllerData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;

#ifdef AN_DEVELOPMENT
	locationInfo = _node->get_name() + String::space() + _node->get_location_info();
	desc = _node->get_string_attribute_or_from_child(TXT("desc"), desc);
#endif

	if (_node->has_attribute(TXT("activeVarID")) ||
		_node->has_attribute(TXT("activeVarIDParam")))
	{
		MeshGenParam<Name> activeVar;
		activeVar.load_from_xml(_node, TXT("activeVarID"));
		activeVars.push_back(activeVar);
	}
	for_every(node, _node->children_named(TXT("active")))
	{
		MeshGenParam<Name> activeVar;
		activeVar.load_from_xml(node, TXT("varID"));
		activeVars.push_back(activeVar);
	}

	if (_node->has_attribute(TXT("blockActiveVarID")) ||
		_node->has_attribute(TXT("blockActiveVarIDParam")))
	{
		MeshGenParam<Name> blockActiveVar;
		blockActiveVar.load_from_xml(_node, TXT("blockActiveVarID"));
		blockActiveVars.push_back(blockActiveVar);
	}
	for_every(node, _node->children_named(TXT("blockActive")))
	{
		MeshGenParam<Name> blockActiveVar;
		blockActiveVar.load_from_xml(node, TXT("varID"));
		blockActiveVars.push_back(blockActiveVar);
	}

	if (_node->has_attribute(TXT("holdVarID")) ||
		_node->has_attribute(TXT("holdVarIDParam")))
	{
		MeshGenParam<Name> holdVar;
		holdVar.load_from_xml(_node, TXT("holdVarID"));
		holdVars.push_back(holdVar);
	}
	for_every(node, _node->children_named(TXT("hold")))
	{
		MeshGenParam<Name> holdVar;
		holdVar.load_from_xml(node, TXT("varID"));
		holdVars.push_back(holdVar);
	}

	processAtLevel.load_from_xml(_node, TXT("processAtLevel"));
	activeBlendCurve.load_from_xml(_node, TXT("activeBlendCurve"));
	activeInitial.load_from_xml(_node, TXT("active"));
	activeInitial.load_from_xml(_node, TXT("activeInitial"));
	activeBlendInTime.load_from_xml(_node, TXT("activeBlendTime"));
	activeBlendOutTime.load_from_xml(_node, TXT("activeBlendTime"));
	activeBlendInTime.load_from_xml(_node, TXT("activeBlendInTime"));
	activeBlendOutTime.load_from_xml(_node, TXT("activeBlendOutTime"));
	activeBlendTimeVar.load_from_xml(_node, TXT("activeBlendTimeVarID"));
	outActiveVar.load_from_xml(_node, TXT("outActiveVarID"));
	activeMask.load_from_xml(_node, TXT("activeMask"));

	return result;
}

bool AppearanceControllerData::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	return result;
}

AppearanceController* AppearanceControllerData::create_controller()
{
	todo_important(TXT("implement_ creating controller!"));
	return nullptr;
}

AppearanceControllerData* AppearanceControllerData::create_copy() const
{
	error(TXT("can't create copy of this appearance controller data!"));
	todo_important(TXT("implement_ for this type?"));
	return nullptr;
}

void AppearanceControllerData::apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context)
{
	for_every(v, activeVars)
	{
		v->fill_value_with(_context);
	}
	for_every(v, blockActiveVars)
	{
		v->fill_value_with(_context);
	}
	for_every(v, holdVars)
	{
		v->fill_value_with(_context);
	}
	processAtLevel.fill_value_with(_context);
	activeBlendCurve.fill_value_with(_context);
	activeInitial.fill_value_with(_context);
	activeBlendInTime.fill_value_with(_context);
	activeBlendOutTime.fill_value_with(_context);
	activeBlendTimeVar.fill_value_with(_context);
	outActiveVar.fill_value_with(_context);
	activeMask.fill_value_with(_context);
}

#ifdef AN_DEVELOPMENT
int AppearanceControllerData::compare_process_at_level(void const * _a, void const * _b)
{
	AppearanceControllerData const * a = *(plain_cast<AppearanceControllerData*>(_a));
	AppearanceControllerData const * b = *(plain_cast<AppearanceControllerData*>(_b));
	int aPAT = a->get_process_at_level();
	int bPAT = b->get_process_at_level();
	if (aPAT > bPAT)
	{
		return B_BEFORE_A;
	}
	if (aPAT < bPAT)
	{
		return A_BEFORE_B;
	}
	return A_AS_B;
}
#endif

#include "mc_schematic.h"

#include "..\..\game\persistence.h"

#include "..\..\..\framework\module\modules.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"

using namespace TeaForGodEmperor;
using namespace CustomModules;

//

// module params
DEFINE_STATIC_NAME(nominalPowerOutput);
DEFINE_STATIC_NAME(powerOutput);

//

REGISTER_FOR_FAST_CAST(CustomModules::Schematic);

static Framework::ModuleCustom* create_module(Framework::IModulesOwner* _owner)
{
	return new CustomModules::Schematic(_owner);
}

static Framework::ModuleData* create_module_data(::Framework::LibraryStored* _inLibraryStored)
{
	return new CustomModules::SchematicData(_inLibraryStored);
}

Framework::RegisteredModule<Framework::ModuleCustom> & CustomModules::Schematic::register_itself()
{
	return Framework::Modules::custom.register_module(String(TXT("schematic")), create_module, create_module_data);
}

CustomModules::Schematic::Schematic(Framework::IModulesOwner* _owner)
: base(_owner)
{
}

CustomModules::Schematic::~Schematic()
{
}

void CustomModules::Schematic::setup_with(Framework::ModuleData const * _moduleData)
{
	base::setup_with(_moduleData);

	auto* sData = fast_cast<CustomModules::SchematicData>(_moduleData);
	if (sData)
	{
		schematic = sData->get_schematic();
	}
}

//

REGISTER_FOR_FAST_CAST(CustomModules::SchematicData);

CustomModules::SchematicData::SchematicData(Framework::LibraryStored* _inLibraryStored)
: base(_inLibraryStored)
{
}

CustomModules::SchematicData::~SchematicData()
{
}

bool CustomModules::SchematicData::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	schematic = new TeaForGodEmperor::Schematic();

	if (schematic->load_from_xml(_node, _lc))
	{
		// ok
	}
	else
	{
		error_loading_xml(_node, TXT("couldn't load schematic"));
		result = false;
	}

	return result;
}

bool CustomModules::SchematicData::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	IF_PREPARE_LEVEL(_pfgContext, Framework::LibraryPrepareLevel::GenerateMeshes)
	{
		schematic->build_mesh();
	}

	return result;
}

void CustomModules::SchematicData::prepare_to_unload()
{
	schematic.clear();
	base::prepare_to_unload();
}

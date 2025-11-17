#include "mc_hubIcons.h"

#include "..\..\..\framework\module\modules.h"

#ifdef AN_CLANG
#include "..\..\..\framework\library\usedLibraryStored.inl"
#endif

using namespace TeaForGodEmperor;
using namespace CustomModules;

//

// params
DEFINE_STATIC_NAME(icon);

//

REGISTER_FOR_FAST_CAST(HubIcons);

static Framework::ModuleCustom* create_module(Framework::IModulesOwner* _owner)
{
	return new HubIcons(_owner);
}

Framework::ModuleData* HubIconsData::create_module_data(::Framework::LibraryStored* _inLibraryStored)
{
	return new HubIconsData(_inLibraryStored);
}

Framework::RegisteredModule<Framework::ModuleCustom> & HubIcons::register_itself()
{
	return Framework::Modules::custom.register_module(String(TXT("hubIcons")), create_module, HubIconsData::create_module_data);
}

HubIcons::HubIcons(Framework::IModulesOwner* _owner)
: base(_owner)
{
}

HubIcons::~HubIcons()
{
}

void HubIcons::setup_with(Framework::ModuleData const * _moduleData)
{
	base::setup_with(_moduleData);
	hubIconsData = fast_cast<HubIconsData>(_moduleData);
	an_assert(hubIconsData);
}

Framework::TexturePart* HubIcons::get_icon() const
{
	if (hubIconsData)
	{
		if (hubIconsData->icon.is_set())
		{
			return hubIconsData->icon.get();
		}
	}
	return nullptr;
}

//

REGISTER_FOR_FAST_CAST(HubIconsData);

HubIconsData::HubIconsData(Framework::LibraryStored* _inLibraryStored)
: base(_inLibraryStored)
{
}

HubIconsData::~HubIconsData()
{
}

bool HubIconsData::read_parameter_from(IO::XML::Attribute const * _attr, Framework::LibraryLoadingContext & _lc)
{
	if (_attr->get_name() == TXT("iconTexturePart"))
	{
		icon.set_name(_attr->get_as_string());
		return true;
	}
	else
	{
		return base::read_parameter_from(_attr, _lc);
	}
}

bool HubIconsData::prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	result &= icon.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);

	return result;
}


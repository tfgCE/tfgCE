#include "defaultPilgrimSetupPreset.h"

//

using namespace TeaForGodEmperor;

//

REGISTER_FOR_FAST_CAST(DefaultPilgrimSetupPreset);
LIBRARY_STORED_DEFINE_TYPE(DefaultPilgrimSetupPreset, defaultPilgrimSetupPreset);

DefaultPilgrimSetupPreset::DefaultPilgrimSetupPreset(Framework::Library* _library, Framework::LibraryName const& _name)
: base(_library, _name)
{
}

DefaultPilgrimSetupPreset::~DefaultPilgrimSetupPreset()
{
}

bool DefaultPilgrimSetupPreset::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	pilgrimSetupPreset.id = _node->get_string_attribute_or_from_child(TXT("id"), pilgrimSetupPreset.id);
	result &= pilgrimSetupPreset.setup.load_from_xml(_node);


	error_loading_xml_on_assert(!pilgrimSetupPreset.id.is_empty(), result, _node, TXT("\"id\" not provided"));

	return result;
}

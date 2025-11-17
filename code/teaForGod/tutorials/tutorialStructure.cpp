#include "tutorialStructure.h"

#include "..\..\framework\library\library.h"

//

using namespace TeaForGodEmperor;

//

REGISTER_FOR_FAST_CAST(TutorialStructure);
LIBRARY_STORED_DEFINE_TYPE(TutorialStructure, tutorialStructure);

TutorialStructure::TutorialStructure(Framework::Library* _library, Framework::LibraryName const& _name)
: base(_library, _name)
{
}

TutorialStructure::~TutorialStructure()
{
}

bool TutorialStructure::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= load_as_tutorial_tree_element_from_xml_child_node(_node, _lc);

	return result;
}

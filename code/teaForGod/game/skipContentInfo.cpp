#include "skipContentInfo.h"

#include "playerSetup.h"

//

using namespace TeaForGodEmperor;

//

bool SkipContentInfo::load_from_xml(IO::XML::Node const* _node)
{
	bool result = true;

	XML_LOAD_NAME_ATTR(_node, generalProgressTag);
	XML_LOAD(_node, gameSlotThreshold);
	XML_LOAD(_node, profileThreshold);

	return result;
}

//

bool SkipContentSet::load_from_xml(IO::XML::Node const* _node)
{
	bool result = true;

	for_every(node, _node->children_named(TXT("skipContent")))
	{
		SkipContentInfo sci;
		if (sci.load_from_xml(node))
		{
			skipContentInfos.push_back(sci);
		}
		else
		{
			result = false;
		}
	}

	return result;
}

void SkipContentSet::process(OUT_ Tags& _skipContentTags) const
{
	_skipContentTags.clear();
	for_every(sci, skipContentInfos)
	{
		if (PlayerPreferences::should_skip_content(*sci))
		{
			_skipContentTags.set_tag(sci->generalProgressTag);
		}
	}
}
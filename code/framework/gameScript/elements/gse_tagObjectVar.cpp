#include "gse_tagObjectVar.h"

#include "..\..\..\core\concurrency\scopedSpinLock.h"

#include "..\..\modulesOwner\modulesOwner.h"
#include "..\..\object\object.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace GameScript;
using namespace Elements;

//

DEFINE_STATIC_NAME(self);

//

bool TagObjectVar::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	objectVar = _node->get_name_attribute(TXT("objectVar"), objectVar);

	if (!objectVar.is_valid())
	{
		objectVar = NAME(self);
	}

	setTags.load_from_xml_attribute_or_child_node(_node, TXT("setTags"));
	removeTags.load_from_xml_attribute_or_child_node(_node, TXT("removeTags"));

	mayFail = _node->get_bool_attribute_or_from_child_presence(TXT("mayFail"), mayFail);

	return result;
}

ScriptExecutionResult::Type TagObjectVar::execute(ScriptExecution& _execution, ScriptExecution::Flags _flags) const
{
	if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::IModulesOwner>>(objectVar))
	{
		if (auto* imo = exPtr->get())
		{
			if (auto* o = imo->get_as_object())
			{
				o->access_tags().set_tags_from(setTags);
				o->access_tags().remove_tags(removeTags);
			}
		}
	}
	else if (!mayFail)
	{
		warn(TXT("missing object"));
	}

	return ScriptExecutionResult::Continue;
}

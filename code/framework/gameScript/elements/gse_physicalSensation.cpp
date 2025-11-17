#include "gse_physicalSensation.h"

#include "..\..\..\core\other\parserUtils.h"
#include "..\..\..\core\other\parserUtilsImpl.inl"
#include "..\..\..\core\physicalSensations\iPhysicalSensations.h"

//

using namespace Framework;
using namespace GameScript;
using namespace Elements;

//

bool Elements::PhysicalSensation::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	if (auto* a = _node->get_attribute(TXT("single")))
	{
		::PhysicalSensations::SingleSensation::Type s;
		if (ParserUtils::parse_using_to_char<::PhysicalSensations::SingleSensation>(a->get_as_string(), s))
		{
			singleSensation = s;
		}
	}
	if (auto* a = _node->get_attribute(TXT("ongoing")))
	{
		::PhysicalSensations::OngoingSensation::Type s;
		if (ParserUtils::parse_using_to_char<::PhysicalSensations::OngoingSensation>(a->get_as_string(), s))
		{
			ongoingSensation = s;
		}

		duration.load_from_xml(_node, TXT("duration"));
	}

	error_loading_xml_on_assert(singleSensation.is_set() || ongoingSensation.is_set(), result, _node, TXT("provide singleSensation or ongoingSensation"));

	return result;
}

ScriptExecutionResult::Type Elements::PhysicalSensation::execute(ScriptExecution& _execution, ScriptExecution::Flags _flags) const
{
	if (auto* ps = PhysicalSensations::IPhysicalSensations::get())
	{
		if (singleSensation.is_set())
		{
			PhysicalSensations::SingleSensation s(singleSensation.get());
			ps->start_sensation(s);
		}
		else if (ongoingSensation.is_set())
		{
			PhysicalSensations::OngoingSensation s(ongoingSensation.get(), duration);
			
			ps->start_sensation(s);
		}
	}
	return ScriptExecutionResult::Continue;
}

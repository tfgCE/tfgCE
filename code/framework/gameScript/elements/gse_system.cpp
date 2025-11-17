#include "gse_system.h"

#include "..\..\..\core\vr\iVR.h"

//

using namespace Framework;
using namespace GameScript;
using namespace Elements;

//

bool GameScript::Elements::System::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	boostPerformanceTime.load_from_xml(_node, TXT("boostPerformanceTime"));
	boostPerformanceLevel.load_from_xml(_node, TXT("boostPerformanceLevel"));

	for_every(node, _node->children_named(TXT("boostPerformance")))
	{
		boostPerformanceTime.load_from_xml(node, TXT("time"));
		boostPerformanceLevel.load_from_xml(node, TXT("level"));
	}

	error_loading_xml_on_assert(boostPerformanceTime.is_set() || !boostPerformanceLevel.is_set(), result, _node, TXT("provide time for boostPerformance"));

	return result;
}

bool GameScript::Elements::System::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

ScriptExecutionResult::Type GameScript::Elements::System::execute(ScriptExecution& _execution, ScriptExecution::Flags _flags) const
{
	if (boostPerformanceTime.is_set())
	{
		if (auto* vr = VR::IVR::get())
		{
			float level = boostPerformanceLevel.get(1.0f);
			vr->set_processing_levels(level, level, boostPerformanceTime.get());
		}
	}
	return ScriptExecutionResult::Continue;
}

#include "gse_output.h"

#include "..\gameScript.h"

#include "..\..\game\game.h"
#include "..\..\library\library.h"
#include "..\..\module\moduleAI.h"

#include "..\..\..\core\io\xml.h"
#include "..\..\ai\aiMessage.h"
#include "..\..\library\usedLibraryStored.inl"
#include "..\..\module\modulePresence.h"
#include "..\..\world\room.h"

//

using namespace Framework;
using namespace GameScript;
using namespace Elements;

//

bool Output::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	text = _node->get_text();

	var = _node->get_name_attribute(TXT("var"), var);

	return result;
}

ScriptExecutionResult::Type Output::execute(ScriptExecution& _execution, ScriptExecution::Flags _flags) const
{
#ifdef AN_ALLOW_EXTENSIVE_LOGS
	lock_output();
	output_colour(0, 1, 1, 1);
	String scriptName;
	if (auto* s = _execution.get_script())
	{
		scriptName = s->get_name().to_string().to_char();
	}
	if (! text.is_empty())
	{
		output(TXT("GAME SCRIPT [%S] %S"), scriptName.to_char(), text.to_char());
	}
	if (var.is_valid())
	{
		TypeId typeId;
		void const* rawValue;
		if (_execution.get_variables().get_existing_of_any_type_id(var, typeId, rawValue))
		{
			LogInfoContext log;
			RegisteredType::log_value(log, typeId, rawValue);
			for_every(line, log.get_lines())
			{
				output(TXT("GAME SCRIPT [%S] %S"), scriptName.to_char(), line->text.to_char());
			}
		}
	}
	output_colour();
	unlock_output();
#endif

	return ScriptExecutionResult::Continue;
}

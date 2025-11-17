#include "gameScriptElement.h"

//

using namespace Framework;
using namespace GameScript;

//

REGISTER_FOR_FAST_CAST(ScriptElement);

ScriptElement::ScriptElement()
{
}

ScriptElement::~ScriptElement()
{
}

bool ScriptElement::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	return result;
}

bool ScriptElement::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	return result;
}


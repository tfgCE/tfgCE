#include "registeredAILogic.h"

using namespace Framework;
using namespace AI;

RegisteredLogic::RegisteredLogic(Name const & _name, CREATE_AI_LOGIC_FUNCTION(_cail), CREATE_AI_LOGIC_DATA_FUNCTION(_caild))
{
	name = Name(_name);
	create_ai_logic_fn = _cail;
	create_ai_logic_data_fn = _caild;
}


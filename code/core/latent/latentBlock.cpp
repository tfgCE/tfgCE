#include "latentBlock.h"

#include "latentFrame.h"

using namespace Latent;

void Block::start(REF_ Frame & _frame)
{
	variablesCountBefore = _frame.get_stack_variables().get_variable_count();
	currentState = 0;
	hasFinished = false;
}

void Block::end(REF_ Frame & _frame)
{
	_frame.set_current_variable(min(_frame.get_current_variable(), variablesCountBefore - 1));
	_frame.access_stack_variables().remove_variables_to_keep(variablesCountBefore);
}

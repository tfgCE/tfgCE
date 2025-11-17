#include "latentScope.h"

#include "latentFrame.h"

using namespace Latent;

Scope::Scope(Frame & _frame)
: frame(_frame)
, block(frame.enter_block())
{
}

Scope::~Scope()
{
	frame.leave_block(block.has_finished());
}

Block & Scope::append_block()
{
	return frame.append_block();
}

bool Scope::is_last_block() const
{
	return frame.is_last_block();
}

void Scope::mark_has_finished()
{
	block.mark_has_finished();
}

bool Scope::has_finished() const
{
	return block.has_finished();
}

void Scope::set_current_state(int _state)
{
	block.set_state(_state);
}

int Scope::get_current_state() const
{
	return block.get_state();
}

#ifdef AN_INSPECT_LATENT
void Scope::add_info(tchar const * _info)
{
	frame.add_info(_info);
}
#endif

void Scope::end_params()
{
	StackVariables & stackVariables = frame.access_stack_variables();
	while (frame.get_current_variable() < stackVariables.get_variable_count() && stackVariables.is_param(frame.get_current_variable()))
	{
		frame.increase_current_variable();
	}
	if (frame.get_current_variable() >= stackVariables.get_variable_count())
	{
		// we're entering here first time, we should add end
		frame.access_stack_variables().make_sure_params_ended();
	}
	an_assert_log_always(stackVariables.is_end_params(frame.get_current_variable()));
}
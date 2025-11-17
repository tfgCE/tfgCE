#include "latentFrame.h"

#include "..\concurrency\scopedSpinLock.h"

#include "..\debug\logInfoContext.h"

using namespace Latent;

Frame::Frame()
: currentBlock(-1)
, isBreaking(false)
, deltaTime(0.0f)
{
}

Frame::~Frame()
{
}

void Frame::reset()
{
#ifdef AN_INSPECT_LATENT
	Concurrency::ScopedSpinLock lock(stackVariablesLock);
#endif
	for (int i = blocks.get_size() - 1; i >= 0; --i)
	{
		blocks[i]->end(*this);
		delete blocks[i];
	}
	blocks.set_size(0);
	stackVariables.remove_all_variables();
	isBreaking = false;
#ifdef AN_INSPECT_LATENT
	inspectLog.clear();
#endif
}

Frame & Frame::fresh_call()
{
#ifdef AN_INSPECT_LATENT
	info.clear();
#endif
	currentBlock = -1;
	currentVariable = -1;
	//isBreaking = false;
	deltaTime = 0.0f;
	wantsToWaitFor = 0.0f;
	callTS.reset();
	return *this;
}

Frame & Frame::break_now()
{
	isBreaking = true;
	return *this;
}

void Frame::begin_execution()
{
	currentBlock = -1;
	currentVariable = -1;
}

void Frame::end_execution()
{
	// all waiting should already end
	endWaiting = false;
}

Block & Frame::append_block()
{
	stackVariables.make_sure_params_ended();
	an_assert(currentBlock >= blocks.get_size() - 1);
	blocks.push_back(new Block());
	Block & block = *blocks[blocks.get_size() - 1];
	block.start(*this);
	stackVariables.mark_begin_params();
	return block;
}

Block & Frame::enter_block()
{
	if (currentBlock < 0)
	{
		an_assert(currentBlock == -1);
		begin_execution();
	}
	++currentBlock;
	if (currentBlock >= blocks.get_size())
	{
		blocks.push_back(new Block());
		Block & block = *blocks[blocks.get_size() - 1];
		block.start(*this);
		return block;
	}
	else
	{
		return *blocks[currentBlock];
	}
}

bool Frame::is_last_block() const
{
	return currentBlock >= blocks.get_size() - 1;
}

void Frame::leave_block(bool _hasFinished)
{
	if (_hasFinished && currentBlock >= 0)
	{
		for (int i = blocks.get_size() - 1; i >= currentBlock; --i)
		{
			blocks[i]->end(*this);
			delete blocks[i];
		}
		blocks.set_size(currentBlock);
	}
	--currentBlock;
	if (currentBlock < 0)
	{
		an_assert(currentBlock == -1);
		end_execution();
	}
}

void Frame::add_params(StackVariablesBase const * _variables)
{
	StackVariableEntry const * variables;
	int count;
	if (_variables->get_variables(variables, count))
	{
		while (count)
		{
#ifdef AN_INSPECT_LATENT
			Concurrency::ScopedSpinLock lock(stackVariablesLock);
#endif
			stackVariables.push_param(*variables);
			++variables;
			--count;
		}
	}
}

#ifdef AN_INSPECT_LATENT
void Frame::add_info(tchar const * _info)
{
	info.push_back(String(_info));
}

void Frame::remove_info_at(int _idx)
{
	info.remove_at(_idx);
}

String Frame::get_info() const
{
	String result = get_info_without_section();
	if (!sectionInfo.is_empty())
	{
		result += TXT(" [");
		result += sectionInfo;
		result += TXT("]");
	}
	return result;
}

String Frame::get_info_without_section() const
{
	String result;
	for_every(i, info)
	{
		if (!result.is_empty())
		{
			result += TXT(" -> ");
		}
		result += *i;
	}
	return result;
}

void Frame::set_section_info(tchar const * _info)
{
	sectionInfo = _info;
}

#endif

void Frame::log(LogInfoContext & _log) const
{
	_log.set_colour(Colour::cyan);
#ifdef AN_INSPECT_LATENT
	Concurrency::ScopedSpinLock lock(inspectLogLock);
	_log.log(TXT("frame : %S"), get_info_without_section().to_char());
	_log.log(TXT("section : %S"), get_section_info().to_char());
#else
	_log.log(TXT("no information available in this build"));
#endif
	_log.set_colour();
#ifdef AN_INSPECT_LATENT
	_log.set_colour(Colour::cyan);
	_log.log(TXT("variables and params"));
	_log.set_colour();
	{
		Concurrency::ScopedSpinLock lock(stackVariablesLock);
		LOG_INDENT(_log);
		StackVariableEntry const *variable;
		int variableCount;
		stackVariables.get_variables(variable, variableCount);
		for_count(int, i, variableCount)
		{
			if (variable->type != StackVariableEntry::EndParams)
			{
				String name;
				if (variable->type == StackVariableEntry::ParamNotProvided)
				{
					name = TXT("[not provided]");
				}
				else
				{
					name = String::printf(variable->type == StackVariableEntry::Param ? TXT("p->: %S") : TXT(" v=: %S"), variable->name ? variable->name : TXT("??"));
				}
				RegisteredType::log_value(_log, variable->typeId, variable->get_data(), name.to_char());
			}
			++variable;
		}
	}
	_log.set_colour(Colour::cyan);
	_log.log(TXT("log"));
	_log.set_colour();
	{
		LOG_INDENT(_log);
		for_every(line, inspectLog.get_lines())
		{
			_log.log(*line);
		}
	}
#endif
}

#include "wmp_context.h"

using namespace WheresMyPoint;

Context::Context(IOwner * _owner)
: owner(_owner)
{

}

Value const * Context::get_temp_value(Name const & _byName) const
{
	for_every_reverse(valueEntry, tempStorage)
	{
		if (valueEntry->name == _byName)
		{
			return &valueEntry->value;
		}
	}
	return nullptr;
}

void Context::set_temp_value(Name const & _byName, Value const & _value)
{
	for_every_reverse(valueEntry, tempStorage)
	{
		if (for_everys_index(valueEntry) < stackAt)
		{
			break;
		}
		if (valueEntry->name == _byName)
		{
			valueEntry->value = _value;
			return;
		}
	}

	ValueEntry newValueEntry;
	newValueEntry.name = _byName;
	newValueEntry.value = _value;
	tempStorage.push_back(newValueEntry);
}

bool Context::push_temp_stack()
{
	stacked.push_back(stackAt);
	stackAt = tempStorage.get_size();
	return true;
}

bool Context::keep_temp_before_popping(Name const & _valueName)
{
	for_every_reverse(valueEntry, tempStorage)
	{
		if (for_everys_index(valueEntry) < stackAt)
		{
			break;
		}
		if (valueEntry->name == _valueName)
		{
			ValueEntry move = *valueEntry;
			tempStorage.insert_at(stackAt, move);
			++stackAt;
			return true;
		}
	}
	error(TXT("couldn't find \"%S\" to keep before popping (stack)"), _valueName.to_char());
	return false;
}

bool Context::pop_temp_stack()
{
	if (stacked.is_empty())
	{
		error(TXT("can't pop empty stack"));
		return false;
	}
	tempStorage.set_size(stackAt);
	stackAt = stacked.get_last();
	stacked.pop_back();
	return true;
}

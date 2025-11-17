#pragma once

/**
 *	Small utility class to keep value for life time of scope
 */
template <typename Class>
struct KeepValue
{
public:
	KeepValue(Class& _variable) : variable(_variable), valueStored(_variable) {}
	~KeepValue() { variable = valueStored; }

private:
	Class& variable;
	Class valueStored;
};
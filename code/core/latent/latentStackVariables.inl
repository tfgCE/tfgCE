#ifdef AN_CLANG
#include "..\memory\objectHelpers.h"
#endif

template <typename VariablesArray>
StackVariablesTemplate<VariablesArray>::StackVariablesTemplate()
{
}

template <typename VariablesArray>
StackVariablesTemplate<VariablesArray>::~StackVariablesTemplate()
{
	remove_all_variables();
}

template <typename VariablesArray>
void StackVariablesTemplate<VariablesArray>::remove_variables_to_keep(int _howManyShouldBeLeft)
{
	_howManyShouldBeLeft = max(_howManyShouldBeLeft, 0);
	for (int i = variables.get_size() - 1; i >= _howManyShouldBeLeft; --i)
	{
		variables[i].destroy();
	}
	variables.set_size(clamp(_howManyShouldBeLeft, 0, variables.get_size()));
}

template <typename VariablesArray>
void StackVariablesTemplate<VariablesArray>::remove_all_variables()
{
	remove_variables_to_keep(0);
}

template <typename VariablesArray>
int StackVariablesTemplate<VariablesArray>::make_sure_params_began()
{
	int result = 0;
	if (variables.is_empty())
	{
		mark_begin_params();
		++result;
	}
	an_assert(variables.get_last().type == StackVariableEntry::BeginParams || variables.get_last().type == StackVariableEntry::Param || variables.get_last().type == StackVariableEntry::ParamNotProvided);
	return result;
}

template <typename VariablesArray>
int StackVariablesTemplate<VariablesArray>::make_sure_params_ended()
{
	int result = 0;
	if (! variables.is_empty() && (variables.get_last().type == StackVariableEntry::BeginParams || variables.get_last().type == StackVariableEntry::Param || variables.get_last().type == StackVariableEntry::ParamNotProvided))
	{
		mark_end_params();
		++result;
	}
	return result;
}

template <typename VariablesArray>
template <typename Class>
void StackVariablesTemplate<VariablesArray>::push_var(Class const & _var)
{
	make_sure_params_ended();
	StackDataBuffer* inData;
	int at;
	int size = sizeof(_var);
	find_place_for(size, OUT_ inData, OUT_ at);
	variables.push_back(StackVariableEntry(inData, at, size, ObjectHelpers<Class>::call_destructor_on, ObjectHelpers<Class>::call_copy_constructor_on));
	variables.get_last().type = StackVariableEntry::Var;
#ifdef AN_INSPECT_LATENT
	variables.get_last().typeId = type_id_if_registered<Class>();
#endif
	ObjectHelpers<Class>::call_copy_constructor_on(variables.get_last().get_data(), &_var);
}

template <typename VariablesArray>
template <typename Class>
void StackVariablesTemplate<VariablesArray>::push_param(Class const & _var)
{
	make_sure_params_began();
	StackDataBuffer* inData;
	int at;
	int size = sizeof(_var);
	find_place_for(size, OUT_ inData, OUT_ at);
	variables.push_back(StackVariableEntry(inData, at, size, ObjectHelpers<Class>::call_destructor_on, ObjectHelpers<Class>::call_copy_constructor_on));
	variables.get_last().type = StackVariableEntry::Param;
#ifdef AN_INSPECT_LATENT
	variables.get_last().typeId = type_id_if_registered<Class>();
#endif
	ObjectHelpers<Class>::call_copy_constructor_on(variables.get_last().get_data(), &_var);
}

template <typename VariablesArray>
void StackVariablesTemplate<VariablesArray>::push_param_optional_not_provided()
{
	make_sure_params_began();
	StackDataBuffer* inData;
	int at;
	int size = sizeof(int);
	find_place_for(size, OUT_ inData, OUT_ at);
	variables.push_back(StackVariableEntry(inData, at, size, ObjectHelpers<int>::call_destructor_on, ObjectHelpers<int>::call_copy_constructor_on));
	variables.get_last().type = StackVariableEntry::ParamNotProvided;
#ifdef AN_INSPECT_LATENT
	variables.get_last().typeId = type_id_none();
#endif
}

template <typename VariablesArray>
void StackVariablesTemplate<VariablesArray>::push_var(StackVariableEntry const & _variable)
{
	make_sure_params_ended();
	variables.push_back(StackVariableEntry(dataBuffer)); // get first dataBuffer, we will get to the last one with find_place_for
	variables.get_last().copy_from(_variable);
	an_assert(variables.get_last().type == StackVariableEntry::Var);
}

template <typename VariablesArray>
void StackVariablesTemplate<VariablesArray>::push_param(StackVariableEntry const & _variable)
{
	if (_variable.type != StackVariableEntry::Param &&
		_variable.type != StackVariableEntry::ParamNotProvided)
	{
		return;
	}
	make_sure_params_began();
	variables.push_back(StackVariableEntry(dataBuffer)); // get first dataBuffer, we will get to the last one with find_place_for
	variables.get_last().copy_from(_variable);
	an_assert(variables.get_last().type == StackVariableEntry::Param ||
		   variables.get_last().type == StackVariableEntry::ParamNotProvided ||
		   variables.get_last().type == StackVariableEntry::EndParams);
}

template <typename VariablesArray>
StackVariableEntry::Type StackVariablesTemplate<VariablesArray>::get_type(int _varIndex) const
{
	if (!variables.is_index_valid(_varIndex))
	{
		return StackVariableEntry::None;
	}
	StackVariableEntry const & variableEntry = variables[_varIndex];
	return variableEntry.type;
}

template <typename VariablesArray>
void StackVariablesTemplate<VariablesArray>::mark_begin_params()
{
	StackDataBuffer* inData;
	int at;
	int size = 4; // to have things aligned
	find_place_for(size, OUT_ inData, OUT_ at);
	variables.push_back(StackVariableEntry(inData, at, size, nullptr, nullptr));
	variables.get_last().type = StackVariableEntry::BeginParams;
}

template <typename VariablesArray>
void StackVariablesTemplate<VariablesArray>::mark_end_params()
{
	StackDataBuffer* inData;
	int at;
	int size = 4; // to have things aligned
	find_place_for(size, OUT_ inData, OUT_ at);
	variables.push_back(StackVariableEntry(inData, at, size, nullptr, nullptr));
	variables.get_last().type = StackVariableEntry::EndParams;
}

template <typename VariablesArray>
template <typename Class>
Class & StackVariablesTemplate<VariablesArray>::push_or_get_var(int _varIndex, tchar const * _name)
{
	if (! variables.is_index_valid(_varIndex))
	{
		an_assert(_varIndex > 0 && (variables[_varIndex - 1].type == StackVariableEntry::EndParams || variables[_varIndex - 1].type == StackVariableEntry::Var));
		push_var(Class());
		an_assert(variables.is_index_valid(_varIndex));
	}
	StackVariableEntry const & variableEntry = variables[_varIndex];
	an_assert(variableEntry.type == StackVariableEntry::Var, TXT("expecting var, this is a param"));
#ifdef AN_INSPECT_LATENT
	an_assert(variableEntry.typeId == type_id_if_registered<Class>(), TXT("var type mismatch? expecting \"%S\", got \"%S\""),
		RegisteredType::get_name_of(type_id_if_registered<Class>()),
		RegisteredType::get_name_of(variableEntry.typeId)
		);
	variables[_varIndex].name = _name;
#endif
#ifndef AN_CLANG // fun with clang, it's an assert, forget about it
	an_assert(variableEntry.copyConstructFunc == ObjectHelpers<Class>::call_copy_constructor_on, TXT("var type mismatch?"));
#endif
	return *(plain_cast<Class>(variableEntry.get_data()));
}

template <typename VariablesArray>
template <typename Class>
Class & StackVariablesTemplate<VariablesArray>::get_param(int _varIndex, tchar const * _name)
{
	an_assert(variables.is_index_valid(_varIndex));
	StackVariableEntry const & variableEntry = variables[_varIndex];
	an_assert(variableEntry.type == StackVariableEntry::Param, TXT("expecting param, this is a var"));
#ifdef AN_INSPECT_LATENT
	an_assert(variableEntry.typeId == type_id_if_registered<Class>(), TXT("param type mismatch? expecting \"%S\", got \"%S\""),
		RegisteredType::get_name_of(type_id_if_registered<Class>()),
		RegisteredType::get_name_of(variableEntry.typeId)
	);
	variables[_varIndex].name = _name;
#endif
#ifndef AN_CLANG // fun with clang, it's an assert, forget about it
	an_assert(variableEntry.copyConstructFunc == ObjectHelpers<Class>::call_copy_constructor_on, TXT("parameter type mismatch?"));
#endif
	return *(plain_cast<Class>(variableEntry.get_data()));
}

template <typename VariablesArray>
template <typename Class>
Class * StackVariablesTemplate<VariablesArray>::get_param_optional(int _varIndex, tchar const * _name)
{
	an_assert(variables.is_index_valid(_varIndex));
	StackVariableEntry const & variableEntry = variables[_varIndex];
	if (variableEntry.type == StackVariableEntry::ParamNotProvided)
	{
		return nullptr;
	}
	return &get_param<Class>(_varIndex, _name);
}

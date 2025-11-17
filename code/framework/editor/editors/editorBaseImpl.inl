template<typename Class>
void Framework::Editor::Base::read(SimpleVariableStorage const& _setup, OUT_ Class& _value, Name const& _name)
{
	if (auto* e = _setup.get_existing<Class>(_name))
	{
		_value = *e;
	}
}

template<typename Class>
void Framework::Editor::Base::write(SimpleVariableStorage & _setup, Class const & _value, Name const& _name)
{
	_setup.access<Class>(_name) = _value;}

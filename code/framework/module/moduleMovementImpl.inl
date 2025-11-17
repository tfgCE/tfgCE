template <typename Class>
Class const & Framework::ModuleMovement::find_prop(Name const & _prop, Name const & _gaitName) const
{
	if (ModuleMovementData::Gait const * gait = find_gait(_gaitName))
	{
		if (auto* existing = gait->params.get_existing<Class>(_prop))
		{
			return *existing;
		}
	}
	if (moduleMovementData)
	{
		if (auto* existing = moduleMovementData->baseGait.params.get_existing<Class>(_prop))
		{
			return *existing;
		}
	}
	return gibberish_value<Class>();
}


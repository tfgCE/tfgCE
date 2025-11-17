template<typename Class>
inline void Framework::MeshGenParam<Class>::fill_value_with(MeshGeneration::GenerationContext const & _context, ShouldAllowToFail _allowToFail)
{
	if (AN_CLANG_BASE valueParam.is_set())
	{
		if (Class const * cValue = _context.get_parameter<Class>(AN_CLANG_BASE valueParam.get()))
		{
			AN_CLANG_BASE value = *cValue;
			return;
		}
		if (_allowToFail == DisallowToFail)
		{
			error(TXT("could not find parameter \"%S\" of type \"%S\""), AN_CLANG_BASE valueParam.get().to_char(), RegisteredType::get_name_of(type_id<Class>()));
		}
	}
}

template<typename Class>
inline void Framework::MeshGenParam<Class>::fill_value_with(SimpleVariableStorage const & _storage, ShouldAllowToFail _allowToFail)
{
	if (AN_CLANG_BASE valueParam.is_set())
	{
		if (Class const * cValue = _storage.get_existing<Class>(AN_CLANG_BASE valueParam.get()))
		{
			AN_CLANG_BASE value = *cValue;
			return;
		}
		if (_allowToFail == DisallowToFail)
		{
			error(TXT("could not find parameter \"%S\" of type \"%S\""), AN_CLANG_BASE valueParam.get().to_char(), RegisteredType::get_name_of(type_id<Class>()));
		}
	}
}

template<typename Class>
Class const * Framework::MeshGenParam<Class>::find(SimpleVariableStorage const & _storage) const
{
	if (AN_CLANG_BASE valueParam.is_set())
	{
		if (Class const * cValue = _storage.get_existing<Class>(AN_CLANG_BASE valueParam.get()))
		{
			return cValue;
		}
	}
	if (AN_CLANG_BASE value.is_set())
	{
		return &AN_CLANG_BASE value.get();
	}
	return nullptr;
}

template<typename Class>
Class const & Framework::MeshGenParam<Class>::get(SimpleVariableStorage const & _storage) const
{
	if (Class const * found = find(_storage))
	{
		return *found;
	}
	error(TXT("it breaks now... missing value of type \"%S\""), RegisteredType::get_name_of(type_id<Class>()));
	return *(plain_cast<Class>(RegisteredType::get_initial_value(type_id<Class>())));
}

template<typename Class>
Class const * Framework::MeshGenParam<Class>::find(SimpleVariableStorage const & _storage, MeshGenParam<Class> const & _general) const
{
	if (AN_CLANG_BASE valueParam.is_set())
	{
		if (Class const * cValue = _storage.get_existing<Class>(AN_CLANG_BASE valueParam.get()))
		{
			return cValue;
		}
	}
	if (_general.valueParam.is_set())
	{
		if (Class const * cValue = _storage.get_existing<Class>(_general.valueParam.get()))
		{
			return cValue;
		}
	}
	if (AN_CLANG_BASE value.is_set())
	{
		return &AN_CLANG_BASE value.get();
	}
	if (_general.value.is_set())
	{
		return &_general.value.get();
	}
	return nullptr;
}

template<typename Class>
Class const & Framework::MeshGenParam<Class>::get(SimpleVariableStorage const & _storage, MeshGenParam<Class> const & _general) const
{
	if (Class const * found = find(_storage, _general))
	{
		return *found;
	}
	error(TXT("it breaks now... missing value of type \"%S\""), RegisteredType::get_name_of(type_id<Class>()));
	return *(plain_cast<Class>(RegisteredType::get_initial_value(type_id<Class>())));
}

template<typename Class>
Class const & Framework::MeshGenParam<Class>::get(SimpleVariableStorage const & _storage, Class const & _defaultValue, ShouldAllowToFail _shouldAllowToFail) const
{
	if (AN_CLANG_BASE valueParam.is_set())
	{
		if (Class const * cValue = _storage.get_existing<Class>(AN_CLANG_BASE valueParam.get()))
		{
			return *cValue;
		}
		if (_shouldAllowToFail == DisallowToFail)
		{
			error(TXT("could not find parameter \"%S\" of type \"%S\""), AN_CLANG_BASE valueParam.get().to_char(), RegisteredType::get_name_of(type_id<Class>()));
		}
	}
	if (AN_CLANG_BASE value.is_set())
	{
		return AN_CLANG_BASE value.get();
	}
	return _defaultValue;
}

template<typename Class>
Class Framework::MeshGenParam<Class>::get(WheresMyPoint::IOwner const* _wmpOwner, Class const& _defaultValue, ShouldAllowToFail _shouldAllowToFail) const
{
	if (AN_CLANG_BASE valueParam.is_set())
	{
		Class value;
		if (_wmpOwner->restore_value<Class>(AN_CLANG_BASE valueParam.get(), value))
		{
			return value;
		}
		if (_shouldAllowToFail == DisallowToFail)
		{
			error(TXT("could not find parameter \"%S\" of type \"%S\""), AN_CLANG_BASE valueParam.get().to_char(), RegisteredType::get_name_of(type_id<Class>()));
		}
	}
	if (AN_CLANG_BASE value.is_set())
	{
		return AN_CLANG_BASE value.get();
	}
	return _defaultValue;
}

template<typename Class>
bool Framework::MeshGenParam<Class>::process_for_wheres_my_point(WheresMyPoint::ITool const* _tool, WheresMyPoint::IOwner const* _wmpOwner, REF_ Class & _outValue, ShouldAllowToFail _shouldAllowToFail) const
{
	bool result = true;
	if (AN_CLANG_BASE valueParam.is_set())
	{
		WheresMyPoint::Value value;
		if (_wmpOwner->restore_value_for_wheres_my_point(AN_CLANG_BASE valueParam.get(), value))
		{
			if (value.get_type() == type_id<Class>())
			{
				_outValue = value.get_as<Class>();
			}
			else
			{
				error_processing_wmp_tool(_tool, TXT("restored value for \"%S\" not a %S"), AN_CLANG_BASE valueParam.get().to_char(), RegisteredType::get_name_of(type_id<Class>()));
				result = false;
			}
		}
		else
		{
			result = false;
		}
		if (! result && _shouldAllowToFail == DisallowToFail)
		{
			error_processing_wmp_tool(_tool, TXT("could not find parameter \"%S\" of type \"%S\""), AN_CLANG_BASE valueParam.get().to_char(), RegisteredType::get_name_of(type_id<Class>()));
		}
	}
	if (AN_CLANG_BASE value.is_set())
	{
		_outValue = AN_CLANG_BASE value.get();
	}
	/*
	if (_shouldAllowToFail != DisallowToFail)
	{
		// always ok
		result = true;
	}
	*/
	return result;
}

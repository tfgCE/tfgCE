
template<typename Class, typename Container>
bool TeaForGodEmperor::serialise_game_objects_container(Serialiser & _serialiser, REF_ Container & _container)
{
	bool result = true;

	int count = _container.get_size();
	result &= serialise_data(_serialiser, count);
	if (_serialiser.is_writing())
	{
		for_every_ref(obj, _container)
		{
			result &= obj->serialise_game_object_id(_serialiser);
			result &= serialise_data(_serialiser, *obj);
		}
	}
	if (_serialiser.is_reading())
	{
		for_count(int, idx, count)
		{
			Class* o = new Class();
			result &= o->serialise_game_object_id(_serialiser);
			result &= serialise_data(_serialiser, o);
			_container.push_back(Framework::GameObjectPtr<Class>(o));
		}
	}

	return result;
}

template<typename Class>
bool TeaForGodEmperor::serialise_game_objects(Serialiser & _serialiser, REF_ Array<Framework::GameObjectPtr<Class>> & _array)
{
	return serialise_game_objects_container<Class, Array<Framework::GameObjectPtr<Class>>>(_serialiser, _array);
}

template<typename Class>
bool TeaForGodEmperor::serialise_game_objects_list(Serialiser & _serialiser, REF_ List<Framework::GameObjectPtr<Class>> & _list)
{
	return serialise_game_objects_container<Class, List<Framework::GameObjectPtr<Class>>>(_serialiser, _list);
}

template<typename Class, typename Container>
bool TeaForGodEmperor::serialise_game_objects_container_with_class_name(Serialiser & _serialiser, REF_ Container & _container,
	std::function<Name(Class* _obj)> _get_class_name, std::function<Class*(Name& _className)> _create_object)
{
	bool result = true;

	int count = _container.get_size();
	result &= serialise_data(_serialiser, count);
	if (_serialiser.is_writing())
	{
		for_every_ref(obj, _container)
		{
			Name type = _get_class_name(obj);
			result &= type.is_valid();
			result &= serialise_data(_serialiser, type);
			result &= obj->serialise_game_object_id(_serialiser);
			result &= serialise_data(_serialiser, *obj);
		}
	}
	if (_serialiser.is_reading())
	{
		for_count(int, idx, count)
		{
			Name type;
			result &= serialise_data(_serialiser, type);
			if (Class* o = _create_object(type))
			{
				result &= o->serialise_game_object_id(_serialiser);
				result &= serialise_data(_serialiser, o);
				_container.push_back(Framework::GameObjectPtr<Class>(o));
			}
			else
			{
				result = false;
			}
		}
	}

	return result;
}

template<typename Class>
bool TeaForGodEmperor::serialise_game_objects_with_class_name(Serialiser & _serialiser, REF_ Array<Framework::GameObjectPtr<Class>> & _array,
	std::function<Name(Class* _obj)> _get_class_name, std::function<Class*(Name& _className)> _create_object)
{
	return serialise_game_objects_container_with_class_name<Class, Array<Framework::GameObjectPtr<Class>>>(_serialiser, _array, _get_class_name, _create_object);
}

template<typename Class>
bool TeaForGodEmperor::serialise_game_objects_list_with_class_name(Serialiser & _serialiser, REF_ List<Framework::GameObjectPtr<Class>> & _list,
	std::function<Name(Class* _obj)> _get_class_name, std::function<Class*(Name& _className)> _create_object)
{
	return serialise_game_objects_container_with_class_name<Class, List<Framework::GameObjectPtr<Class>>>(_serialiser, _list, _get_class_name, _create_object);
}

template<typename Class>
bool TeaForGodEmperor::serialise_ref_count_objects_with_class_name(Serialiser & _serialiser, REF_ Array<RefCountObjectPtr<Class>> & _array,
	std::function<Name(Class* _obj)> _get_class_name, std::function<Class*(Name& _className)> _create_object)
{
	bool result = true;

	int count = _array.get_size();
	result &= serialise_data(_serialiser, count);
	if (_serialiser.is_writing())
	{
		for_every_ref(obj, _array)
		{
			Name type = _get_class_name(obj);
			result &= type.is_valid();
			result &= serialise_data(_serialiser, type);
			result &= serialise_data(_serialiser, *obj);
		}
	}
	if (_serialiser.is_reading())
	{
		_array.set_size(count);
		for_every(obj, _array)
		{
			Name type;
			result &= serialise_data(_serialiser, type);
			if (Class* o = _create_object(type))
			{
				result &= serialise_data(_serialiser, o);
				*obj = o;
			}
			else
			{
				result = false;
			}
		}
	}

	return result;
}

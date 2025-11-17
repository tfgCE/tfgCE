template <typename Class>
SafeObjectPoint<Class>::SafeObjectPoint(SafeObject
#ifdef AN_CLANG
	<Class>
#endif
	* _safeObject)
: safeObject(_safeObject)
{
	NAME_POOLED_OBJECT_IF_NOT_NAMED(SafeObjectPoint<Class>);
}

template <typename Class>
SafeObjectPoint<Class>::~SafeObjectPoint()
{
	an_assert(safeObject == nullptr, TXT("explicitly make it unavailable!"));
}

template <typename Class>
void SafeObjectPoint<Class>::make_safe_object_unavailable()
{
	safeObject = nullptr;
}

//

template <typename Class>
SafeObject<Class>::SafeObject(Class* _availableAsObject)
{
	if (_availableAsObject)
	{
		make_safe_object_available(_availableAsObject);
	}
}

template <typename Class>
SafeObject<Class>::~SafeObject()
{
	make_safe_object_unavailable();
}

template <typename Class>
void SafeObject<Class>::make_safe_object_available(Class* _object)
{
	an_assert(!safeObjectPoint.is_set(), TXT("already made available, why do it again?"));
	safeObjectPoint = new SafeObjectPoint
#ifdef AN_CLANG
		<Class>
#endif
		(_object);
	object = _object;
}

template <typename Class>
void SafeObject<Class>::make_safe_object_unavailable()
{
	if (safeObjectPoint.is_set())
	{
		safeObjectPoint->make_safe_object_unavailable();
	}
	safeObjectPoint.clear();
	object = nullptr;
}

//

template <typename Class>
SafePtr<Class>::SafePtr()
{
}

template <typename Class>
SafePtr<Class>::SafePtr(SafePtr const & _source)
{
	if (_source.point.get() && _source.point->get())
	{
		point = _source.point;
	}
}

template <typename Class>
SafePtr<Class>::SafePtr(Class const * _object)
{
	operator=(_object);
}

template <typename Class>
SafePtr<Class>::~SafePtr()
{
}

template <typename Class>
SafePtr<Class>& SafePtr<Class>::operator=(SafePtr const& _source)
{
	if (_source.point.get() && _source.point->get())
	{
		point = _source.point;
	}
	else
	{
		point.clear();
	}
	return *this;
}

template <typename Class>
SafePtr<Class>& SafePtr<Class>::operator=(Class const * _object)
{
	if (_object)
	{
		if (_object->SafeObject<Class>::is_available_as_safe_object())
		{
			point = _object->SafeObject<Class>::safeObjectPoint;
		}
		else
		{
			point.clear();
		}
	}
	else
	{
		point.clear();
	}

	return *this;
}

template <typename Class>
GameObjectPtr<Class>::GameObjectPtr()
{}

template <typename Class>
GameObjectPtr<Class>::GameObjectPtr(GameObjectPtr const & _source)
: base(_source)
{
	if (is_pending())
	{
		add_to_pending();
	}
}

template <typename Class>
GameObjectPtr<Class>::GameObjectPtr(Class* _value)
: base(_value)
{
#ifdef AN_NAT_VIS
	natVis = _value;
#endif
	update_id();
}

template <typename Class>
GameObjectPtr<Class>::~GameObjectPtr()
{
	remove_from_pending();
}

template <typename Class>
GameObjectPtr<Class>& GameObjectPtr<Class>::operator=(GameObjectPtr const & _source)
{
	base::operator=(_source);
	if (is_pending())
	{
		add_to_pending();
	}
#ifdef AN_NAT_VIS
	natVis = (Class*)base::get();
#endif
	return *this;
}

template <typename Class>
GameObjectPtr<Class>& GameObjectPtr<Class>::operator=(Class* _value)
{
	base::operator=(_value);
	update_id();
#ifdef AN_NAT_VIS
	natVis = (Class*)base::get();
#endif
	return *this;
}

template <typename Class>
GameObjectID GameObjectPtr<Class>::get_id() const
{
	if (auto const * gameObject = (Class*)base::get())
	{
		return gameObject->get_game_object_id();
	}
	return id;
}

template <typename Class>
void GameObjectPtr<Class>::set_id(int _id)
{
	id = _id;
	update_with_id();
}

template <typename Class>
void GameObjectPtr<Class>::update_id()
{
	if (auto const * gameObject = (Class*)base::get())
	{
		id = gameObject->get_game_object_id();
		an_assert(id != NONE);
	}
	else
	{
		id = NONE;
	}
}

template <typename Class>
bool GameObjectPtr<Class>::serialise(Serialiser & _serialiser)
{
	bool result = true;

	if (_serialiser.is_writing())
	{
		update_id();
	}

	result &= serialise_data(_serialiser, id);

	if (_serialiser.is_reading())
	{
		base::operator=(nullptr);
#ifdef AN_NAT_VIS
		natVis = (Class*)base::get();
#endif
		if (is_pending())
		{
			add_to_pending();
		}
	}

	return result;
}

template <typename Class>
bool GameObjectPtr<Class>::serialise_write_for(Serialiser & _serialiser, Class const * _value)
{
	an_assert(_serialiser.is_writing());

	bool result = true;

	GameObjectID id = _value->get_game_object_id();
	result &= serialise_data(_serialiser, id);

	return result;
}

template <typename Class>
Class* GameObjectPtr<Class>::get() const
{
	if (Class * gameObject = (Class*)base::get())
	{
		return gameObject;
	}

	if (id != NONE)
	{
		an_assert(false, TXT("maybe we should never get here?"));

		return cast_to_nonconst(this)->update_with_id(); // yes... but this is because we know id but don't have game object (yet)
	}
	else
	{
		return nullptr;
	}
}

template <typename Class>
Class* GameObjectPtr<Class>::update_with_id()
{
	remove_from_pending();
	if (id != NONE)
	{
		if (auto* gameObject = GameObject<Class>::find_game_object_by_id(id))
		{
			base::operator=(gameObject);
		}
		else
		{
			error_stop(TXT("no existing game object, although game object id (%i) is provided"), id);
		}
#ifdef AN_NAT_VIS
		natVis = (Class*)base::get();
#endif
	}
	return (Class*)base::get();
}

template <typename Class>
Array<GameObjectPtr<Class>*>* GameObjectPtr<Class>::pending = nullptr;

template <typename Class>
void GameObjectPtr<Class>::add_to_pending()
{
	if (!pending) pending = new Array<GameObjectPtr<Class>*>();
	pending->push_back(this);
}

template <typename Class>
void GameObjectPtr<Class>::remove_from_pending()
{
	if (pending)
	{
		pending->remove_fast(this);
		if (pending->is_empty())
		{
			delete_and_clear(pending);
		}
	}
}

template <typename Class>
bool GameObjectPtr<Class>::resolve_pending()
{
	bool result = true;
	while (pending)
	{
		GameObjectPtr* currentPending = pending->get_last();
#ifdef AN_DEVELOPMENT
		GameObjectID id = currentPending->get_id();
#endif
		Class* obj = currentPending->update_with_id();
#ifdef AN_DEVELOPMENT
		an_assert(id == currentPending->get_id(), TXT("we changed id from %i to %i"), id, currentPending->get_id());
#endif
		if (currentPending->get_id() != NONE && !obj)
		{
			an_assert(currentPending->get_id() != NONE && !obj, TXT("we couldn't resolve id %i"), currentPending->get_id())
			result = false;
		}
#ifdef AN_DEVELOPMENT
		an_assert(! pending || currentPending != pending->get_last(), TXT("we didn't remove current pending!"));
#endif
	}
	return result;
}

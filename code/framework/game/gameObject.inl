#ifdef AN_CLANG
#include "..\..\core\serialisation\serialiser.h"
#endif

template <typename Class>
REGISTER_FOR_FAST_CAST(Framework::GameObject<Class>);

template <typename Class>
Framework::GameObject<Class>* Framework::GameObject<Class>::s_first = nullptr;

template <typename Class>
Framework::GameObject<Class>* Framework::GameObject<Class>::s_last = nullptr;

#ifdef GAME_OBJECT_USES_SPIN_LOCK
template <typename Class>
Concurrency::SpinLock Framework::GameObject<Class>::chainLock = Concurrency::SpinLock(TXT("Framework.GameObject.chainLock"));
#endif

template <typename Class>
Framework::GameObject<Class>::GameObject()
{
	gameObjectId = 0;
	insert_into_list();
}

template <typename Class>
Framework::GameObject<Class>::~GameObject()
{
	remove_from_list();
}

template <typename Class>
void Framework::GameObject<Class>::insert_into_list()
{
#ifdef GAME_OBJECT_USES_SPIN_LOCK
	Concurrency::ScopedSpinLock lock(chainLock);
#endif
	auto* go = s_first;
	while (go)
	{
		if (gameObjectId == go->get_game_object_id())
		{
			++gameObjectId;
		}
		else
		{
			prev = go->prev;
			if (go->prev)
			{
				go->prev->next = this;
			}
			next = go;
			go->prev = this;
			break;
		}
		go = go->next;
	}
	if (!prev && !next)
	{
		// couldn't add between
		if (!s_last)
		{
			// only element!
			s_first = this;
			s_last = this;
		}
		else
		{
			// add at the end
			s_last->next = this;
			prev = s_last;
			s_last = this;
		}
	}
	else
	{
		// first one or last one
		if (!prev)
		{
			s_first = this;
		}
		if (!next)
		{
			s_last = this;
		}
	}
}

template <typename Class>
void Framework::GameObject<Class>::remove_from_list()
{
#ifdef GAME_OBJECT_USES_SPIN_LOCK
	Concurrency::ScopedSpinLock lock(chainLock);
#endif
	if (next)
	{
		next->prev = prev;
	}
	else
	{
		s_last = prev;
	}
	if (prev)
	{
		prev->next = next;
	}
	else
	{
		s_first = next;
	}
}

template <typename Class>
Class* Framework::GameObject<Class>::find_by_id(GameObjectID _gameObjectId)
{
#ifdef GAME_OBJECT_USES_SPIN_LOCK
	todo_note(TXT("nice place for one-modifes, many-access"));
	Concurrency::ScopedSpinLock lock(chainLock);
#endif
	auto* go = s_first;
	while (go)
	{
		if (_gameObjectId == go->get_game_object_id())
		{
			return (Class*)go;
		}
		go = go->next;
	}
	return nullptr;
}

template <typename Class>
Framework::GameObject<Class>* Framework::GameObject<Class>::find_game_object_by_id(GameObjectID _gameObjectId)
{
#ifdef GAME_OBJECT_USES_SPIN_LOCK
	todo_note(TXT("nice place for one-modifes, many-access"));
	Concurrency::ScopedSpinLock lock(chainLock);
#endif
	GameObject* go = s_first;
	while (go)
	{
		if (_gameObjectId == go->get_game_object_id())
		{
			return go;
		}
		go = go->next;
	}
	return nullptr;
}

template <typename Class>
bool Framework::GameObject<Class>::serialise_game_object_id(Serialiser & _serialiser)
{
	bool result = true;

	// when reading, remove and insert in proper place
	if (_serialiser.is_reading())
	{
		remove_from_list();
	}
	result &= serialise_data(_serialiser, gameObjectId);
	if (_serialiser.is_reading())
	{
		GameObjectID readGameObjectId = gameObjectId;
		insert_into_list();
		if (readGameObjectId != gameObjectId)
		{
			an_assert(false, TXT("we haven't removed game objects?"));
			result = false;
		}
	}

	return result;
}

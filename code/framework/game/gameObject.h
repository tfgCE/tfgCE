#pragma once

// do not use spin lock - everything related to game objects should happen on single thread
//#define GAME_OBJECT_USES_SPIN_LOCK

#include "..\..\core\fastCast.h"
#include "..\..\core\memory\refCountObject.h"
#include "..\..\core\types\string.h"

#ifdef GAME_OBJECT_USES_SPIN_LOCK
#include "..\..\core\concurrency\spinLock.h"
#include "..\..\core\concurrency\scopedSpinLock.h"
#endif

namespace Framework
{
	typedef int GameObjectID;

	/**
	 *	GameObjects are identified by id and are held in a list.
	 *	This, combined with GameObjectPtr should make it easier to load/save etc.
	 */
	template<typename Class>
	class GameObject
	: public RefCountObject
	{
		FAST_CAST_DECLARE(GameObject<Class>);
		FAST_CAST_END();
	public:
		static Class* find_by_id(GameObjectID _gameObjectId);
		static GameObject* find_game_object_by_id(GameObjectID _gameObjectId);

		GameObject();
		virtual ~GameObject();

		GameObjectID get_game_object_id() const { return gameObjectId; }

#ifdef AN_DEVELOPMENT
		virtual String get_game_object_info() { return String::printf(TXT("(%i)"), get_ref_count()); }
		static GameObject* get_first_game_object() { return s_first; }
		GameObject* get_next_game_object() const { return next; }
#endif

	public:
		bool serialise_game_object_id(Serialiser & _serialiser);

	private:
		// global array to have all characters accessible via gameObjectId
		// ordered by gameObjectId (created this way, when loading it has to be kept like that)
		static GameObject* s_first;
		static GameObject* s_last;
#ifdef GAME_OBJECT_USES_SPIN_LOCK
		static Concurrency::SpinLock chainLock; // this is sort of nasty thing, but we need this to make sure we won't do anything in the same time - not used as it was something else doing nasty stuff
#endif
		GameObject* prev = nullptr;
		GameObject* next = nullptr;
		GameObjectID gameObjectId;

		void insert_into_list();
		void remove_from_list();
	};

};

#include "gameObject.inl"
#include "gameObjectPtr.h"

#pragma once

#include "gameObject.h"

#include "..\..\core\memory\refCountObjectPtr.h"
#include "..\..\core\globalDefinitions.h"

namespace Framework
{
	template <typename Class> class GameObject;

	/**
	 *	This class should make it easier to restore character net when loading game
	 */
	template <typename Class>
	struct GameObjectPtr
	: private RefCountObjectPtr<GameObject<Class>>
	{
		typedef RefCountObjectPtr<GameObject<Class>> base;
	public:
		GameObjectPtr();
		GameObjectPtr(GameObjectPtr const & _source);
		explicit GameObjectPtr(Class* _value);
		~GameObjectPtr();

		GameObjectPtr& operator=(GameObjectPtr const & _source);
		GameObjectPtr& operator=(Class* _value);

		static bool resolve_pending();

		inline bool operator==(GameObjectPtr const & _other) const { return base::operator==(_other); }
		inline bool operator!=(GameObjectPtr const & _other) const { return base::operator!=(_other); }
		inline bool operator==(Class const * _other) const { return get() == _other; }
		inline bool operator!=(Class const * _other) const { return get() != _other; }

		inline Class* operator * ()  const { an_assert(is_set()); return get(); }
		inline Class* operator -> () const { an_assert(is_set()); return get(); }

		inline Class* get() const;
		inline bool is_set() const { return get() != nullptr; }

		void set_id(GameObjectID _id);
		GameObjectID get_id() const;

		inline int get_ref_count() const { return base::get_ref_count(); }

	public:
		bool serialise(Serialiser & _serialiser);
		static bool serialise_write_for(Serialiser & _serialiser, Class const * _value); // just writing as it would be gameObjectPtr for a random object

	private:
		GameObjectID id = NONE; // this is used only if ref count object ptr is not set

#ifdef AN_NAT_VIS
		Class* natVis = nullptr;
#endif

		static Array<GameObjectPtr*>* pending;

		void update_id();
		Class* update_with_id();

		// this doesn't have to be exhaustive list of all pending objects, but it's better to have as many as possible to resolve them at some point
		void add_to_pending();
		void remove_from_pending();
		inline bool is_pending() const { return id != NONE && base::get() == nullptr; }
	};

	#include "gameObjectPtr.inl"
};

#pragma once

#include "libraryName.h"
#include "libraryStored.h"

#include "..\..\core\memory\refCountObjectPtr.h"

#include "..\world\worldSetting.h"
#include "..\world\worldSettingCondition.h"

namespace Framework
{
	template <typename StoredClass> class LibraryStore;
	
	/*
	 *	Helper struct to get all tagged objects.
	 *	It traverses through all stored objects and picks up only tagged ones.
	 */
	template <typename StoredClass>
	struct LibraryStoreTaggedObjects
	{
		struct Iterator
		{
		public:
			Iterator(LibraryStoreTaggedObjects const* _owner, typename List<RefCountObjectPtr<StoredClass>>::ConstIterator const & _startAt);
			Iterator(Iterator const & _other);
			
			inline Iterator & operator ++ ();
			inline Iterator & operator -- ();
			// post
			inline Iterator operator ++ (int);
			inline Iterator operator -- (int);
			// comparison
			inline bool operator == (Iterator const & _other) const { return owner == _other.owner && iterator == _other.iterator; }
			inline bool operator != (Iterator const & _other) const { return owner != _other.owner || iterator != _other.iterator; }

			inline StoredClass * operator * ()  const { an_assert((*iterator).get()); return (*iterator).get(); }
			inline StoredClass * operator -> () const { an_assert((*iterator).get()); return (*iterator).get(); }
		private:
			LibraryStoreTaggedObjects const* owner;
			typename List<RefCountObjectPtr<StoredClass>>::ConstIterator iterator;
		};
		
		LibraryStoreTaggedObjects(LibraryStore<StoredClass>* _store, TagCondition const & _tagCondition);
		LibraryStoreTaggedObjects(LibraryStore<StoredClass>* _store, WorldSettingCondition const & _settingCondition, WorldSettingConditionContext const & _settingConditionContext);

		Iterator const begin() const { return storeBegin; }
		Iterator const end() const { return storeEnd; }
		
		bool check(StoredClass const * _object) const;

	private:
		LibraryStore<StoredClass>* store;
		Iterator storeBegin;
		Iterator storeEnd;
		TagCondition const * tagCondition = nullptr;
		WorldSettingCondition const * settingCondition = nullptr;
		WorldSettingConditionContext settingConditionContext;
		
		friend struct Iterator;
	};

	#include "libraryStoreTaggedObjects.inl"
};

#pragma once

#include "libraryName.h"

// don't forget to include usedLibraryStored.inl in cpp

namespace Framework
{
	class Library;
	struct LibraryLoadingContext;
	struct LibraryPrepareContext;

	template <typename Class>
	struct UsedLibraryStored
	{
	public:
		static void change_to_pointer_array(Array<Framework::UsedLibraryStored<Class> > const& _from, Array<Class*>& _to)
		{
			for_every_ref(l, _from)
			{
				_to.push_back(l);
			}
		}

	public:
		AN_NOT_CLANG_INLINE UsedLibraryStored();
		AN_NOT_CLANG_INLINE UsedLibraryStored(UsedLibraryStored const & _source);
		AN_NOT_CLANG_INLINE explicit UsedLibraryStored(Class* _value);
		AN_NOT_CLANG_INLINE explicit UsedLibraryStored(Class const * _value);
		AN_NOT_CLANG_INLINE ~UsedLibraryStored();

		AN_NOT_CLANG_INLINE UsedLibraryStored& operator=(UsedLibraryStored const & _source);
		AN_NOT_CLANG_INLINE UsedLibraryStored& operator=(Class* _value);
		AN_NOT_CLANG_INLINE UsedLibraryStored& operator=(Class const * _value);

		AN_NOT_CLANG_INLINE void clear() { set(nullptr); }

		/**
		 *	locking mechanism prevents from any further changes - it is especially useful when we want to point always at particular object (owner in most cases)
		 *	before sending owner to delete it should be unlocked and cleared
		 */
		AN_NOT_CLANG_INLINE void lock() { an_assert(!locked); locked = true; }
		AN_NOT_CLANG_INLINE void lock(Class* value) { an_assert(!locked); set(value); locked = true; }
		AN_NOT_CLANG_INLINE void unlock_and_clear() { an_assert(locked); locked = false; set(nullptr); }

		AN_NOT_CLANG_INLINE Class* operator * ()  const { an_assert(value); return value; }
		AN_NOT_CLANG_INLINE Class* operator -> () const { an_assert(value); return value; }
		AN_NOT_CLANG_INLINE bool operator==(Class const * _object) const { return (_object && value == _object) || (! _object && ! name.is_valid()); }
		AN_NOT_CLANG_INLINE bool operator==(UsedLibraryStored const & _other) const { return (value && _other.value && value == _other.value) || ((! value || ! _other.value) && (name == _other.name)); }
		AN_NOT_CLANG_INLINE bool operator!=(UsedLibraryStored const & _other) const { return ! (operator==(_other)); }

		AN_NOT_CLANG_INLINE void set(Class* _value);
		AN_NOT_CLANG_INLINE Class* get() const { return value; }
		AN_NOT_CLANG_INLINE bool is_set() const { return value != nullptr; }

		AN_NOT_CLANG_INLINE bool is_name_valid() const { return name.is_valid(); }
		AN_NOT_CLANG_INLINE LibraryName const & get_name() const { return value ? value->get_name() : name; }
		AN_NOT_CLANG_INLINE void set_name(LibraryName const & _name) { set(nullptr); name = _name; }

		// won't use renamer, provide full names!
		bool find_if_required(Library* _library, Name const & _typeNameOverride = Name::invalid());
		bool find(Library* _library, Name const & _typeNameOverride = Name::invalid());
		bool find_may_fail(Library* _library, Name const & _typeNameOverride = Name::invalid());
		bool find_or_create(Library* _library, Name const & _typeNameOverride = Name::invalid());

		bool load_from_xml_child_node(IO::XML::Node const * _node, tchar const * _childName, tchar const * _attrName);
		bool load_from_xml_child_node(IO::XML::Node const * _node, tchar const * _childName, tchar const * _attrName, LibraryLoadingContext & _lc);
		bool load_from_xml(IO::XML::Node const* _node); // text value
		bool load_from_xml(IO::XML::Node const* _node, tchar const* _attrName);
		bool load_from_xml(IO::XML::Node const * _node, tchar const * _attrName, LibraryLoadingContext & _lc);
		bool load_from_xml(IO::XML::Attribute const * _attr, LibraryLoadingContext & _lc);
		bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc); // text value

		bool save_to_xml(IO::XML::Node * _node, tchar const * _attrName) const;

		// will use renamer of pfgContext
		bool prepare_for_game_find(Library* _library, LibraryPrepareContext& _pfgContext, int32 _atLevel, Name const & _typeNameOverride = Name::invalid());
		bool prepare_for_game_find_may_fail(Library* _library, LibraryPrepareContext& _pfgContext, int32 _atLevel, Name const & _typeNameOverride = Name::invalid());
		bool prepare_for_game_find_or_create(Library* _library, LibraryPrepareContext& _pfgContext, int32 _atLevel, Name const & _typeNameOverride = Name::invalid());

	public:
		bool serialise(Serialiser & _serialiser);

		static inline int compare(void const* _a, void const* _b)
		{
			UsedLibraryStored const& a = *plain_cast<UsedLibraryStored>(_a);
			UsedLibraryStored const& b = *plain_cast<UsedLibraryStored>(_b);
			return Framework::LibraryName::compare(&a.get_name(), &b.get_name());
		}

	private:
		LibraryName name;
		Class* value = nullptr;
		bool locked = false;
	};
};

#ifdef AN_CLANG
//#include "usedLibraryStored.inl"
#endif

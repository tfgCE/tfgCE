#pragma once

#include "libraryName.h"

namespace Framework
{
	class Library;
	class LibraryStored;
	struct LibraryLoadingContext;
	struct LibraryPrepareContext;

	struct UsedLibraryStoredAny
	{
	public:
		UsedLibraryStoredAny();
		explicit UsedLibraryStoredAny(UsedLibraryStoredAny const & _source);
		explicit UsedLibraryStoredAny(LibraryStored* _value);
		~UsedLibraryStoredAny();

		UsedLibraryStoredAny& operator=(UsedLibraryStoredAny const & _source);
		UsedLibraryStoredAny& operator=(LibraryStored* _value);
		
		bool operator==(LibraryStored const* _value) const;

		LibraryStored* operator * ()  const { an_assert(value); return value; }
		LibraryStored* operator -> () const { an_assert(value); return value; }

		void set(LibraryStored* _value);
		LibraryStored* get() const { return value; }

		Name const & get_type() const { return type; }
		LibraryName const& get_name() const;

		bool find(Library* _library);
		bool find_or_create(Library* _library);
		void set(LibraryName const & _name, Name const& _type);
		bool set_and_find(LibraryName const & _name, Name const& _type, Library* _library);
		bool load_from_xml(IO::XML::Node const * _node, tchar const * _attrType, tchar const * _attrName, LibraryLoadingContext & _lc); // if _attrType is null, it will be taken from node name
		bool load_from_xml(IO::XML::Node const * _node, tchar const * _attrType, tchar const * _attrName); // if _attrType is null, it will be taken from node name
		bool save_to_xml(IO::XML::Node * _node, tchar const * _attrType, tchar const * _attrName) const; // requires attrType!
		IO::XML::Node* save_to_xml_child_node(IO::XML::Node * _node, tchar const * _attrType, tchar const * _attrName) const; // if _attrType is null, it will be saved to node name
		bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext, bool _mayFail = false);

	private:
		Name type;
		LibraryName name;
		LibraryStored* value;

	public:
		static inline int compare(void const* _a, void const* _b)
		{
			UsedLibraryStoredAny const& a = *plain_cast<UsedLibraryStoredAny>(_a);
			UsedLibraryStoredAny const& b = *plain_cast<UsedLibraryStoredAny>(_b);
			return LibraryName::compare(&a.get_name(), &b.get_name());
		}
	};
};

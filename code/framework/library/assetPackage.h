#pragma once

#include "..\library\libraryStored.h"
#include "..\library\usedLibraryStored.h"

namespace Framework
{
	class AssetPackage
	: public LibraryStored
	{
		FAST_CAST_DECLARE(AssetPackage);
		FAST_CAST_BASE(LibraryStored);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef LibraryStored base;
	public:
		AssetPackage(Library* _library, LibraryName const& _name);
		virtual ~AssetPackage();

	public:
		/**
		 *	The temporary copy feature is done for sprites to allow modifying them more easily
		 *
		 *	First, create a temporary copy of asset package
		 *	Then create temporary copies of objects inside
		 */

		implement_ LibraryStored* create_temporary_copy() const { return create_temporary_copy_using_copy(); }

	protected:
		implement_ void copy_from_for_temporary_copy(LibraryStored const* _source);

	public:
		// temporary copies are created per unique object
		template <typename Class>
		void create_temporary_copies_of(); // the stored objects inside will get temporary copies (only objects of particular class as it uses save+load which is not implemented for everything)

		void create_temporary_copies();

	public:
		template <typename Class>
		void do_for_all_of(std::function<void(Class* _stored, Name const & _id, int _idx)> _do);

		void do_for_all(std::function<void(LibraryStored* _stored, Name const& _id, int _idx)> _do);

		// unique means _do will be called only on one instance of an asset (will skip repeated)
		template <typename Class>
		void do_for_all_unique_of(std::function<void(Class* _stored, Name const& _id, int _idx)> _do);
		
		void do_for_all_unique(std::function<void(LibraryStored* _stored, Name const& _id, int _idx)> _do);

	public:
		template <typename Class>
		Class * get() const; // no parameters will give the default asset
		
		template <typename Class>
		Class * get(Name const & _id, int _idx = 0, bool _allowDefault = false) const;
		
		template <typename Class>
		Class * get_clamp(Name const & _id, int & _idx, bool _allowDefault = false) const; // will modify _idx if required to fit into existing bits
		
		template <typename Class>
		Class * get_mod(Name const & _id, int & _idx, bool _allowDefault = false) const; // will modify _idx if required to fit into existing bits
		
		template <typename Class>
		Class * get_random(Name const & _id, Random::Generator & _rg, bool _allowDefault = false) const;

		template <typename Class>
		Class * get_random_any(Random::Generator & _rg) const; // any id, any index

	public: // LibraryStored
		override_ bool load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc);
		override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);
		override_ void prepare_to_unload();

		override_ bool save_to_xml(IO::XML::Node* _node) const; // this method will save to an existing node

		override_ Editor::Base* create_editor(Editor::Base* _current) const;

	private:
		// when loading doubled asset, it will load over the existing one
		// if we want to replace something, just use the same id
		// but if we want to get rid off of all the existing indices, we should use clearAllIndicesFirst="true"
		struct Entry
		{
			bool isDefault = false;
			Name id;
			int idx = 0; // may work as animation frames or a variant, should be continuous, if not, missing assets will be added between
			CACHED_ int count = 0; // how many of them are, prepared in prepare_for_game
			UsedLibraryStoredAny libraryStored;
		};
		Array<Entry> entries;

		bool noIndices = false;
		bool keepEntries = false;

		void create_temporary_copies_of_type(Name const& libType);
		void do_for_all_unique_of_type(Name const& libType, std::function<void(LibraryStored* _stored, Name const& _id, int _idx)> _do);
	};

};

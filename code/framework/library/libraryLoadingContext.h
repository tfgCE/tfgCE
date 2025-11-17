#pragma once

#include "..\..\core\io\utils.h"
#include "..\..\core\types\name.h"

#include <functional>

class SimpleVariableStorage;

namespace Framework
{
	class Library;
	class LibraryStored;
	struct LibraryName;

	typedef std::function< bool(LibraryStored* _object) > LibraryStoredReloadRequiredCheck;

	namespace LibraryLoadingMode
	{
		enum Type
		{
			Config,
			Settings,
			Tools,
			Tweaks,
#ifdef AN_ALLOW_BULLSHOTS
			Bullshots,
#endif
			Load,
		};
	};

	namespace LibrarySource
	{
		enum Type
		{
			Files,
			Assets, // ANDROID ONLY!

			TemporaryMemory, // for temporary objects
		};
	};

	namespace MeshGeneration
	{
		template<typename Class> struct ValueDef;
	};

	struct LibraryLoadingContext
	{
	public:
		LibraryLoadingContext(Library* _library, String const & _dir, LibrarySource::Type _librarySource, LibraryLoadingMode::Type _loadingMode)
		{
			library = _library;
			librarySource = _librarySource;
			loadingMode = _loadingMode;
			dir = IO::Utils::as_valid_directory(_dir);
		}

		void reload_only(LibraryStored* _reloadOnlyLibraryStored) { reloadOnlyLibraryStored = _reloadOnlyLibraryStored; }
		void reload_only(LibraryStoredReloadRequiredCheck _reload_required_check) { reload_required_check = _reload_required_check; }
		//
		LibraryStored* get_reload_only() const { return reloadOnlyLibraryStored; }
		bool should_be_reloaded(LibraryStored* _object) const { return reload_required_check && _object && reload_required_check(_object); }
		//
		bool is_reloading_only() const { return reloadOnlyLibraryStored != nullptr || reload_required_check != nullptr; }

		bool is_loading_for_library() const { return library != nullptr; }
		LibrarySource::Type get_library_source() const { return librarySource; }
		LibraryLoadingMode::Type get_loading_mode() const { return loadingMode; } // not relevant if not loading for library

		bool load_group_into(SimpleVariableStorage & _storage) const;
		bool load_group_into(MeshGeneration::ValueDef<LibraryName> & _libraryName) const;

		String const & get_current_file() const { return currentFileName; }
		String const & get_dir() const { return dir; }
		String get_path_in_dir(String const & _filename) const { return dir + TXT("/") + _filename; }
		Name const & get_current_group() const { return currentGroup; }
		void set_current_group(Name const & _currentGroup) { currentGroup = _currentGroup; }
		Library* get_library() const { return library; }

		void push_id(String const & _id) { idStack.push_back(_id); }
		void pop_id() { idStack.pop_back(); }
		String build_id() const;

		void push_owner(LibraryStored * _owner) { ownerStack.push_back(_owner); }
		void pop_owner() { ownerStack.pop_back(); }
		LibraryStored* get_owner() const { an_assert(!ownerStack.is_empty(), TXT("expecting owner, but not set!")); return ownerStack.get_last(); }

	private:
		Library* library; // if no library, only settings are loaded
		LibraryStored* reloadOnlyLibraryStored = nullptr;
		LibraryStoredReloadRequiredCheck reload_required_check = nullptr;
		LibrarySource::Type librarySource;
		LibraryLoadingMode::Type loadingMode;
		String currentFileName;
		String dir;
		Name currentGroup;
		Array<String> idStack; // to allow building stack for ids for localised strings etc - should allow to be unique for everything in all libraries
		Array<LibraryStored*> ownerStack;

		friend class Library;

		void set_current_file(String const & _filename = String::empty()) { currentFileName = _filename; }
	};

	struct ForLibraryLoadingContext
	{
		ForLibraryLoadingContext(LibraryLoadingContext & _context)
		: context(_context)
		{
		}

		~ForLibraryLoadingContext()
		{
			if (pushedId)
			{
				context.pop_id();
			}
			if (pushedOwner)
			{
				context.pop_owner();
			}
		}

		LibraryLoadingContext & push_owner(LibraryStored* _owner)
		{
			an_assert(!pushedOwner);
			context.push_owner(_owner);
			pushedOwner = true;
			return context;
		}

		LibraryLoadingContext & push_id(String const & _id)
		{
			an_assert(!pushedId);
			context.push_id(_id);
			pushedId = true;
			return context;
		}

		LibraryLoadingContext & push_id(tchar const * _id)
		{
			return push_id(String(_id));
		}

	private:
		LibraryLoadingContext & context;
		bool pushedId = false;
		bool pushedOwner = false;
	};

};

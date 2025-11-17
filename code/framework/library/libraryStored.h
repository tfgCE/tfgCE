#pragma once

#include <functional>

#include "..\..\core\fastCast.h"
#include "..\..\core\memory\refCountObject.h"
#include "..\..\core\other\simpleVariableStorage.h"
#include "..\..\core\tags\tag.h"
#include "..\..\core\tags\tagCondition.h"

#include "libraryLoadingContext.h"
#include "libraryPrepareContext.h"
#include "libraryName.h"

#include "usedLibraryStored.h"

#include "libraryLoadLevel.h"

#ifdef AN_DEVELOPMENT
#include "..\..\core\concurrency\scopedSpinLock.h"
#endif

#ifdef AN_DEVELOPMENT
	#define LIBRARY_STORED_WITH_LOADED_FROM_FILE
#else
	#ifdef WITH_IN_GAME_EDITOR
		#define LIBRARY_STORED_WITH_LOADED_FROM_FILE
	#endif
#endif

struct LogInfoContext;

#define LIBRARY_STORED_DECLARE_TYPE() \
	public: \
		static Name const & library_type(); \
		override_ Name const & get_library_type() const { return library_type(); }

#define LIBRARY_STORED_DEFINE_TYPE(Class, name) \
	DEFINE_STATIC_NAME_STR(libraryType##name, TXT(#name)); \
	Name const & Class::library_type() { return NAME(libraryType##name); }

namespace Framework
{
	class LibraryStored;
	class CreateFromTemplate;
	class Room;
	class SubWorld;

	class EditorWorkspace;
	namespace Editor
	{
		class Base;
	};

	/**
	 *	There are two ways to get rid of some objects
	 *		1. Create them and mark them as temporary. Remove all temporary objects with usage count 0.
	 *		2. Set library load level in library or in newly created object and unload all objects from library for given load level.
	 *
	 *	All LibraryStored may have probCoef tag to have probability of being chosen
	 */
	class LibraryStored
	: public virtual RefCountObject
	// IDebugableObject
	{
		FAST_CAST_DECLARE(LibraryStored);
		FAST_CAST_END();
	public:
		typedef LibraryStored* (*ConstructStoredClassObjectWithNameFunc)(Library * _library, LibraryName const & _name);

		static int compare_for_sort(void const* _a, void const* _b);

		LibraryStored(Library * _library, LibraryName const & _name);

		void be_temporary(bool _autoUnloadUnused = false) { temporaryObject = true; autoUnloadUnused = _autoUnloadUnused; }
		bool is_temporary() const { return temporaryObject; }

		void set_name(LibraryName const& _name) { name = _name; } // will only change name of this one, use it only if you are sure what you are doing (like saving to create a copy)
		LibraryName const& get_name() const { return name; }

		Tags const& get_tags() const { return tags; }
		Tags& access_tags() { return tags; }

		// check library's custom attributes notes
		int get_custom_attributes() const { return customAttributes; }
		void set_custom_attributes(int _customAttributes) { customAttributes = _customAttributes; }

#ifdef LIBRARY_STORED_WITH_LOADED_FROM_FILE
		String const & get_loaded_from_file() const { return loadedFromFile; }
#endif

#ifdef AN_DEVELOPMENT
		virtual void ready_for_reload();
#endif
		bool load_from_xml_for_store(IO::XML::Node const* _node, LibraryLoadingContext& _lc); // this will check if should be kept for loading-on-demand
		virtual bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
		virtual void unload_outside_queue(tchar const* _reason); // if we have some other means to load on demand (like partially, heavy resourcee, override this)
		virtual void load_outside_queue(tchar const* _reason); // if we have some other means to load on demand (like partially, heavy resourcee, override this)
		static bool load_later_on_demand_loading(IO::XML::Node const* _node, OUT_ Optional<int>& _loadLaterPrio, OUT_ bool& _loadOnDemand, OUT_ bool& _allowUnloading);
		virtual bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);
		virtual bool create_from_template(Library* _library, CreateFromTemplate const & _createFromTemplate) const;
		virtual void prepare_to_unload() {}

		static bool save_to_file(IO::Interface* _file, Array<LibraryStored const*> const& _libraryStored);
		bool save_to_file(IO::Interface * _file) const; // this method will create new node and will use save_to_xml
		bool add_to_xml(IO::XML::Node* _node) const; // this method will create new node and will use save_to_xml
		virtual bool save_to_xml(IO::XML::Node* _node) const; // this method will save to an existing node
		virtual Editor::Base* create_editor(Editor::Base* _current) const;

		bool does_require_further_loading() const { return requiresLoadingOnDemand || laterLoadingPriority.is_set(); }
		bool does_allow_unloading() const { return allowUnloading; }
		void load_on_demand_if_required(); // make sure that for unloadable this is called only the amount it requires
		void unload_for_load_on_demand(); // only if allowUnloading

		void mark_prepared_for_game() { preparedForGame = true; }
		bool is_prepared_for_game() { return preparedForGame; }

		Library * get_library() const { an_assert(library); return library; }
		void set_library(Library * _library) { library = _library; }
		SimpleVariableStorage const & get_custom_parameters() const { return customParameters; }
		SimpleVariableStorage & access_custom_parameters() { return customParameters; }

		void set_library_load_level(LibraryLoadLevel::Type _libraryLoadLevel) { libraryLoadLevel = _libraryLoadLevel; }
		LibraryLoadLevel::Type get_library_load_level() const { return libraryLoadLevel; }

		virtual void debug_draw() const;
		virtual bool editor_render() const; // rendered for the editor, may use debug renderer as well, returns true if did something
		virtual bool preview_render() const; // rendered by preview game to main buffer, return false if not rendered anything
		virtual void log(LogInfoContext & _context) const;

		virtual Name const & get_library_type() const { an_assert(false, TXT("get_library_type not implemented, check LIBRARY_STORED_DECLARE_TYPE")); return Name::invalid(); }

		static void output_all();
		static bool does_any_exist();

		template <typename Class>
		static float get_prob_coef_from_tags(Class const * _class) { if (!_class) return 0.0f; an_assert(fast_cast<LibraryStored>(_class)); return _class->get_tags().get_tag(nameProbCoef, 1.0f); }
		
		float get_prob_coef_from_tags() const { return get_tags().get_tag(nameProbCoef, 1.0f); }

		static bool has_something_for_later_loading(OPTIONAL_ OUT_ bool * _onlyOnDemand = nullptr); // returns true if something to load (later-loading or on demand)
		static bool load_one_of_later_loading(); // returns true if loaded (which may mean that there is still something to load)

	public:
		virtual LibraryStored* create_temporary_copy() const { return create_temporary_copy_using_save_load(); } // or create_temporary_copy_using_copy if copy_from_for_temporary_copy fully implemented

	protected:
		LibraryStored* create_temporary_copy_using_save_load() const;
		LibraryStored* create_temporary_copy_using_copy() const;
		virtual void copy_from_for_temporary_copy(LibraryStored const* _source); // copies as much as it can

	public:
		virtual bool setup_custom_preview_room(SubWorld* _testSubWorld, REF_ Room*& _testRoomForCustomPreview, Random::Generator const& _rg);

	public: // usage can be manipulated on const objects too - it isn't strictly modification of those objects, right?
		void add_usage() const;
		void release_usage() const;
		int get_usage_count() const { return usageCount.get(); }

	public: // should be used only while loading!
		void set_requires_later_loading(Optional<int> const& _priority);
		void set_requires_loading_on_demand(bool _loadOnDemand, bool _allowUnloading);
		void setup_load_outside_queue_context(IO::XML::Node const* _node, LibraryLoadingContext& _lc);

	private:
		void do_auto_unload() const;

	protected:
		virtual ~LibraryStored();

		virtual bool fill_on_create_from_template(LibraryStored & _newObject, CreateFromTemplate const & _createFromTemplate) const;

		bool load_basic_info_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc);

#ifdef WITH_IN_GAME_EDITOR
	public:
		void mark_requires_saving(bool _requiresSaving = true) { requiresSaving = _requiresSaving; }
		bool does_require_saving() const { return requiresSaving; }
	private:
		bool requiresSaving = false;
#endif

	private:
		const static Name nameProbCoef;

		// both methods mean that we can load at any time. load later just indicates higher priority on loading post main load, load on demand may be disabled for auto loading (now it just loads post "later load")

		static Concurrency::SpinLock s_loadLaterLock;
		static List<LibraryStored*> s_loadLater;
		Optional<int> laterLoadingPriority;

		static Concurrency::SpinLock s_loadOnDemandLock;
		static List<LibraryStored*> s_loadOnDemand;
		static Concurrency::SpinLock s_loadOnDemandUnloadableLock;
		static List<LibraryStored*> s_loadOnDemandUnloadable;
		bool requiresLoadingOnDemand = false;
		bool allowUnloading = false; // if set to true, requiresLoadingOnDemand means if it requires loading or is already loaded
		Concurrency::Counter unloadableUsageCount;
		RefCountObjectPtr<IO::XML::Node> loadOutsideQueueFromNode;
		LibraryLoadingContext* loadOutsideQueueContext = nullptr;

		mutable Concurrency::Counter usageCount;
		Library* library;
		LibraryLoadLevel::Type libraryLoadLevel;
		LibraryName name;
		bool basicInfoLoaded = false;
		bool temporaryObject = false; // temporary objects might be unloaded and removed from library if not used
		bool autoUnloadUnused = false; // auto unloads object when released usage
#ifdef LIBRARY_STORED_WITH_LOADED_FROM_FILE
		String loadedFromFile;
#endif
		Tags tags;
		SimpleVariableStorage customParameters; // any kind of parameters
		int customAttributes = 0; // check library's custom attributes notes
		bool preparedForGame = false;
#ifdef AN_DEVELOPMENT
		static LibraryStored* s_global;
		LibraryStored* prevGlobal = nullptr;
		LibraryStored* nextGlobal = nullptr;
#endif

		struct CopyCustomParameters
		{
			Name fromType;
			LibraryName from;
			bool overrideExisting = false;
		};
		Array<CopyCustomParameters> copyCustomParameters; // please do not chain them together, sources should be only sources

	public:
		static inline int compare(void const* _a, void const* _b)
		{
			LibraryStored const* a = plain_cast<LibraryStored>(_a);
			LibraryStored const* b = plain_cast<LibraryStored>(_b);
			return LibraryName::compare(&a->get_name(), &b->get_name());
		}
		static inline int compare_ptr(void const* _a, void const* _b)
		{
			LibraryStored const* a = *plain_cast<LibraryStored*>(_a);
			LibraryStored const* b = *plain_cast<LibraryStored*>(_b);
			return LibraryName::compare(&a->get_name(), &b->get_name());
		}
	};

	typedef std::function< void(LibraryStored * _libraryStored) > DoForLibraryStored;
	typedef std::function< bool(Name const & _typeName) > DoForLibraryType;
};

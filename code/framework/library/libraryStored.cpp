#include "libraryStored.h"

#include "library.h"

#include "..\debug\previewGame.h"
#include "..\game\game.h"

#include "..\..\core\other\parserUtils.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#define INVESTIGATE_ALLOW_UNLOADING

//

using namespace Framework;

//

REGISTER_FOR_FAST_CAST(LibraryStored);

#ifdef AN_DEVELOPMENT
LibraryStored* LibraryStored::s_global = nullptr;
#endif

Name const LibraryStored::nameProbCoef(TXT("probCoef")); // NAME(probCoef);
List<LibraryStored*> LibraryStored::s_loadLater;
Concurrency::SpinLock LibraryStored::s_loadLaterLock(TXT("LibraryStored::s_loadLaterLock"));
List<LibraryStored*> LibraryStored::s_loadOnDemand;
Concurrency::SpinLock LibraryStored::s_loadOnDemandLock(TXT("LibraryStored::s_loadOnDemandLock"));
List<LibraryStored*> LibraryStored::s_loadOnDemandUnloadable;
Concurrency::SpinLock LibraryStored::s_loadOnDemandUnloadableLock(TXT("LibraryStored::s_loadOnDemandUnloadableLock"));

int LibraryStored::compare_for_sort(void const* _a, void const* _b)
{
	LibraryStored const* a = *(plain_cast<LibraryStored*>(_a));
	LibraryStored const* b = *(plain_cast<LibraryStored*>(_b));

	return LibraryName::compare(&a->get_name(), &b->get_name());
}

LibraryStored::LibraryStored(Library * _library, LibraryName const & _name)
: library(_library)
, libraryLoadLevel(_library? _library->get_current_library_load_level() : LibraryLoadLevel::WithoutLibrary)
, name(_name)
, preparedForGame(false)
{
#ifdef AN_DEVELOPMENT
	prevGlobal = nullptr;
	nextGlobal = s_global;
	if (nextGlobal)
	{
		nextGlobal->prevGlobal = this;
	}
	s_global = this;
#endif
}

LibraryStored::~LibraryStored()
{
	set_requires_later_loading(NP);
	set_requires_loading_on_demand(false, false);

	delete_and_clear(loadOutsideQueueContext);
#ifdef AN_DEVELOPMENT
	if (prevGlobal)
	{
		prevGlobal->nextGlobal = nextGlobal;
	}
	else
	{
		s_global = nextGlobal;
	}
	if (nextGlobal)
	{
		nextGlobal->prevGlobal = prevGlobal;
	}
#endif
}

void LibraryStored::set_requires_later_loading(Optional<int> const& _priority)
{
	if (_priority.is_set() == laterLoadingPriority.is_set() &&
		_priority.get(NONE) == laterLoadingPriority.get(NONE))
	{
		return;
	}

	if (!_priority.is_set() ||
		_priority.get(NONE) != laterLoadingPriority.get(NONE))
	{
		Concurrency::ScopedSpinLock lock(s_loadLaterLock);
		s_loadLater.remove(this);
		laterLoadingPriority.clear();
	}

	if (_priority.is_set())
	{
		laterLoadingPriority = _priority;

		Concurrency::ScopedSpinLock lock(s_loadLaterLock);
		bool added = false;
		int prio = _priority.get();
		for_every_ptr(l, s_loadLater)
		{
			if (l->laterLoadingPriority.get(prio) <= prio)
			{
				s_loadLater.insert(for_everys_iterator(l), this);
				added = true;
				break;
			}
		}
		if (!added)
		{
			s_loadLater.push_back(this);
		}
	}
}

void LibraryStored::set_requires_loading_on_demand(bool _loadOnDemand, bool _allowUnloading)
{
	if (_loadOnDemand == requiresLoadingOnDemand)
	{
		return;
	}

	if (!_loadOnDemand || _allowUnloading)
	{
		if (requiresLoadingOnDemand)
		{
			Concurrency::ScopedSpinLock lock(s_loadOnDemandLock);
			s_loadOnDemand.remove(this);
		}
	}
	if (!_allowUnloading)
	{
		if (allowUnloading)
		{
			Concurrency::ScopedSpinLock lock(s_loadOnDemandUnloadableLock);
			s_loadOnDemandUnloadable.remove(this);
		}
	}
	if (_allowUnloading)
	{
		if (! allowUnloading)
		{
			Concurrency::ScopedSpinLock lock(s_loadOnDemandUnloadableLock);
			s_loadOnDemandUnloadable.push_back(this);
		}
	}
	else if (_loadOnDemand && ! _allowUnloading)
	{
		if (!requiresLoadingOnDemand)
		{
			Concurrency::ScopedSpinLock lock(s_loadOnDemandLock);
			s_loadOnDemand.push_back(this);
		}
	}

	requiresLoadingOnDemand = _loadOnDemand;
	allowUnloading = _allowUnloading;
}

void LibraryStored::do_auto_unload() const
{
	if (library)
	{
		library->unload_and_remove(cast_to_nonconst(this));
	}
}

bool LibraryStored::has_something_for_later_loading(OPTIONAL_ OUT_ bool* _onlyOnDemand)
{
	bool loadLater = !s_loadLater.is_empty();
	bool loadOnDemand = !s_loadOnDemand.is_empty();
	assign_optional_out_param(_onlyOnDemand, !loadLater && loadOnDemand);
	return loadLater || loadOnDemand;
}

bool LibraryStored::load_one_of_later_loading()
{
	if (! has_something_for_later_loading())
	{
		return false;
	}

	ASSERT_ASYNC_OR_LOADING_OR_PREVIEW;

	RefCountObjectPtr<LibraryStored> toLoad;
	tchar const* reason = TXT("unknown");
	
	if (!toLoad.get())
	{
		{
			Concurrency::ScopedSpinLock lock(s_loadLaterLock, true);
			if (!s_loadLater.is_empty())
			{
				toLoad = s_loadLater.get_first();
				s_loadLater.pop_front();
			}
		}
		reason = TXT("on later-loading");
		if (auto* tl = toLoad.get())
		{
			tl->laterLoadingPriority.clear(); // as is already removed from the queue
		}
	}

	if (!toLoad.get())
	{
		{
			Concurrency::ScopedSpinLock lock(s_loadOnDemandLock, true);
			if (!s_loadOnDemand.is_empty())
			{
				toLoad = s_loadOnDemand.get_first();
				s_loadOnDemand.pop_front();
			}
		}
		reason = TXT("on pre-demand");
		if (auto* tl = toLoad.get())
		{
			tl->requiresLoadingOnDemand = false; // as is already removed from the queue
		}
	}

	if (auto* tl = toLoad.get())
	{
		tl->load_outside_queue(reason);
	}

	return true;
}


void LibraryStored::load_on_demand_if_required()
{
	if (allowUnloading)
	{
#ifdef INVESTIGATE_ALLOW_UNLOADING
		output(TXT("[LibraryStored; allow unloading] load on demand [%i] \"%S\""), unloadableUsageCount.get(), get_name().to_string().to_char());
#endif
		unloadableUsageCount.increase();
	}

	if (does_require_further_loading())
	{
		ASSERT_ASYNC_OR_LOADING_OR_PREVIEW;

		load_outside_queue(TXT("on demand"));
	}
}

void LibraryStored::unload_for_load_on_demand()
{
	if (allowUnloading)
	{
		ASSERT_ASYNC_OR_LOADING_OR_PREVIEW;

#ifdef INVESTIGATE_ALLOW_UNLOADING
		output(TXT("[LibraryStored; allow unloading] unload on demand [%i] \"%S\""), unloadableUsageCount.get(), get_name().to_string().to_char());
#endif
		if (unloadableUsageCount.decrease() == 0)
		{
#ifdef INVESTIGATE_ALLOW_UNLOADING
			output(TXT("[LibraryStored; allow unloading] unload now \"%S\""), get_name().to_string().to_char());
#endif
			unload_outside_queue(TXT("on demand"));
		}
	}
}

void LibraryStored::load_outside_queue(tchar const * _reason)
{
	set_requires_later_loading(NP);
	set_requires_loading_on_demand(false, allowUnloading); // keep unloadable

	if (loadOutsideQueueContext && loadOutsideQueueFromNode.get())
	{
		an_assert(loadOutsideQueueContext);
		Library* library = loadOutsideQueueContext->get_library();

		if (library->get_state() != LibraryState::Idle)
		{
			error_dev_investigate(TXT("library not idle (%i), consider \"%S\" not for \"%S\"! will break now most likely"), library->get_state(), get_name().to_string().to_char(), _reason);
			AN_BREAK;
		}

		auto prevState = library->set_state(LibraryState::LoadingOutsideQueue);

		// load
#ifdef OUTPUT_LOAD_AND_PREPARE_TIMES
		float loadedTime = 0.0f;
		float preparedTime = 0.0f;
#endif
		{
#ifdef OUTPUT_LOAD_AND_PREPARE_TIMES
			::System::TimeStamp loadTimeStamp;
#endif
			bool result = load_from_xml(loadOutsideQueueFromNode.get(), *loadOutsideQueueContext);
			if (!result)
			{
				error_dev_investigate(TXT("could not load \"%S\" %S"), get_name().to_string().to_char(), _reason);
			}
			if (!allowUnloading)
			{
				loadOutsideQueueFromNode.clear();
				delete_and_clear(loadOutsideQueueContext);
			}
#ifdef OUTPUT_LOAD_AND_PREPARE_TIMES
			loadedTime = loadTimeStamp.get_time_since();
#endif
		}

		// prepare
		{
#ifdef OUTPUT_LOAD_AND_PREPARE_TIMES
			::System::TimeStamp prepareTimeStamp;
#endif
			bool result = library->prepare_for_game_during_async(this);
			if (!result)
			{
				error_dev_investigate(TXT("could not prepare \"%S\" loaded %S"), get_name().to_string().to_char(), _reason);
			}
#ifdef OUTPUT_LOAD_AND_PREPARE_TIMES
			preparedTime = prepareTimeStamp.get_time_since();
#endif
		}

		library->set_state(prevState);

#ifdef OUTPUT_LOAD_AND_PREPARE_TIMES
		output(TXT("load %S \"%S\" in %.3fs (%.3fs + %.3fs)"), _reason, get_name().to_string().to_char(), loadedTime + preparedTime, loadedTime, preparedTime);
#endif
	}
}

void LibraryStored::unload_outside_queue(tchar const * _reason)
{
	an_assert(allowUnloading);
	set_requires_later_loading(NP);
	set_requires_loading_on_demand(true, true); // put it back to queue

	// check we can load it back
	an_assert(loadOutsideQueueContext);
	an_assert(loadOutsideQueueFromNode.get());
}

bool LibraryStored::load_basic_info_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	if (_lc.get_reload_only())
	{
		preparedForGame = false;
	}
#ifdef LIBRARY_STORED_WITH_LOADED_FROM_FILE
	if (auto* di = _node->get_document_info())
	{
		loadedFromFile = di->get_loaded_from_file();
	}
#endif
	if (!_node)
	{
		// TODO Utils.Log.error("invalid xml node when loading library stored data");
		return false;
	}

	if (basicInfoLoaded)
	{
		return true;
	}

	bool result = name.load_from_xml(_node, String(TXT("name")), _lc);
	result &= tags.load_from_xml(_node, TXT("tags"));

	basicInfoLoaded = true;

	return result;
}

bool LibraryStored::load_from_xml_for_store(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = load_basic_info_from_xml(_node, _lc);

	Optional<int> loadLaterPrio;
	bool readLoadOnDemand;
	bool readAllowUnloading;
	result &= LibraryStored::load_later_on_demand_loading(_node, OUT_ loadLaterPrio, OUT_ readLoadOnDemand, readAllowUnloading);

	if (loadLaterPrio.is_set())
	{
		set_requires_later_loading(loadLaterPrio);
		set_requires_loading_on_demand(false, false);
	}
	else if (readLoadOnDemand)
	{
		set_requires_later_loading(NP);
		set_requires_loading_on_demand(true, readAllowUnloading);
	}

	if (laterLoadingPriority.is_set() || requiresLoadingOnDemand)
	{
		setup_load_outside_queue_context(_node, _lc);
	}
	else
	{
		result &= load_from_xml(_node, _lc);
	}

	return result;
}

bool LibraryStored::load_later_on_demand_loading(IO::XML::Node const* _node, OUT_ Optional<int> & _loadLaterPrio, OUT_ bool & _loadOnDemand, OUT_ bool& _allowUnloading)
{
	bool result = true;
	if (auto* ch = _node->first_child_named(TXT("loadLater")))
	{
		_loadLaterPrio = ch->get_int_attribute(TXT("priority"), 0);
	}
	if (auto* attr = _node->get_attribute(TXT("loadLaterPriority")))
	{
		_loadLaterPrio = attr->get_as_int();
	}

	_loadOnDemand = _node->get_bool_attribute_or_from_child_presence(TXT("loadOnDemand"));
	_allowUnloading = _node->get_bool_attribute_or_from_child_presence(TXT("allowUnloading"));

	if (_allowUnloading && !_loadOnDemand)
	{
		warn_loading_xml(_node, TXT("if allowUnloading requested, loadOnDemand should be requested as well"));
		_loadOnDemand = true;
	}

	error_loading_xml_on_assert(!_loadLaterPrio.is_set() || !_loadOnDemand, result, _node, TXT("can't have both \"loadLater\" and \"loadOnDemand\" set"));

	if (!Library::does_allow_loading_on_demand())
	{
		_loadLaterPrio.clear();
		_loadOnDemand = false;
		_allowUnloading = false;
	}

	return result;
}

void LibraryStored::setup_load_outside_queue_context(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	delete_and_clear(loadOutsideQueueContext);
	loadOutsideQueueContext = new LibraryLoadingContext(_lc.get_library(), _lc.get_dir(), _lc.get_library_source(), _lc.get_loading_mode());
	*loadOutsideQueueContext = _lc;
	loadOutsideQueueFromNode = _node;
}

#ifdef AN_DEVELOPMENT
void LibraryStored::ready_for_reload()
{
	customParameters.clear();
	copyCustomParameters.clear();
}
#endif

bool LibraryStored::save_to_file(IO::Interface* _file, Array<LibraryStored const*> const& _libraryStored)
{
	bool result = true;

	IO::XML::Document doc;
	if (auto* n = doc.get_root()->add_node(TXT("library")))
	{
		if (!_libraryStored.is_empty())
		{
			n->set_attribute(TXT("group"), _libraryStored.get_first()->get_name().get_group().to_char());
		}
		for_every_ptr(ls, _libraryStored)
		{
			result &= ls->add_to_xml(n);
		}

		result &= doc.save_xml(_file, NP, true); // we want comments!
	}
	else
	{
		error(TXT("could not save to file"));
		result = false;
	}

	return result;
}

bool LibraryStored::save_to_file(IO::Interface* _file) const
{
	Array<LibraryStored const*> justOne;
	justOne.push_back(this);
	return save_to_file(_file, justOne);
}

bool LibraryStored::add_to_xml(IO::XML::Node* _node) const
{
	bool result = true;

	if (auto* n = _node->add_node(get_library_type().to_char()))
	{
		result &= save_to_xml(n);
	}
	else
	{
		error(TXT("could not save to xml"));
		result = false;
	}

	return result;
}

Editor::Base* LibraryStored::create_editor(Editor::Base* _current) const
{
	todo_important(TXT("not editable, implement"));
	return nullptr;
}

bool LibraryStored::save_to_xml(IO::XML::Node* _node) const
{
	bool result = true;

	_node->set_attribute(TXT("name"), name.to_string());
	if (!tags.is_empty())
	{
		_node->set_attribute(TXT("tags"), tags.to_string_all(true));
	}
	if (customAttributes != 0)
	{
		_node->set_attribute(TXT("customAttributes"), Library::custom_attributes_to_string(customAttributes));
	}
	if (!customParameters.is_empty())
	{
		customParameters.save_to_xml_child_node(_node, TXT("customParameters"));
	}

	return result;
}

bool LibraryStored::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = load_basic_info_from_xml(_node, _lc);
	if (auto* attr = _node->get_attribute(TXT("customAttributes")))
	{
		customAttributes = Library::parse_custom_attributes(attr->get_as_string());
	}
	result &= customParameters.load_from_xml_child_node(_node, TXT("customParameters"));
	result &= customParameters.load_from_xml_child_node(_node, TXT("parameters")); // sort of a fallback (also LibraryBasedParameters use it)
	_lc.load_group_into(customParameters);
	for_every(node, _node->children_named(TXT("copyCustomParameters")))
	{
		CopyCustomParameters ccp;
		ccp.fromType = node->get_name_attribute(TXT("fromType"));
		ccp.from.load_from_xml(node, TXT("from"), _lc);
		ccp.overrideExisting = node->get_bool_attribute_or_from_child_presence(TXT("overrideExisting"), ccp.overrideExisting);
		if (ccp.fromType.is_valid() && ccp.from.is_valid())
		{
			copyCustomParameters.push_back(ccp);
		}
		else
		{
			error_loading_xml(node, TXT("no valid \"fromType\" or \"from\""));
			result = false;
		}
	}

	preparedForGame = false;
#ifdef AN_CHECK_MEMORY_LEAKS
#ifdef AN_DEBUG
#ifdef AN_LOG_LOADING_AND_PREPARING
	output(TXT("loading %S..."), name.to_string().to_char());
#endif
#endif
#endif
	return result;
}
		
bool LibraryStored::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;
	IF_PREPARE_LEVEL(_pfgContext, LibraryPrepareLevel::CreateFromTemplates)
	{
		for_every(ccp, copyCustomParameters)
		{
			if (auto* store = _library->get_store(ccp->fromType))
			{
				if (auto* ls = store->find_stored(ccp->from))
				{
					if (ccp->overrideExisting)
					{
						access_custom_parameters().set_from(ls->get_custom_parameters());
					}
					else
					{
						access_custom_parameters().set_missing_from(ls->get_custom_parameters());
					}
				}
				else
				{
#ifdef AN_DEVELOPMENT_OR_PROFILER
					if (!Framework::Library::may_ignore_missing_references())
#endif
					{
						error(TXT("couldn't find stored \"%S\" for \"%S\""), ccp->from.to_string().to_char(), get_name().to_string().to_char());
						result = false;
					}
				}
			}
			else
			{
#ifdef AN_DEVELOPMENT_OR_PROFILER
				if (!Framework::Library::may_ignore_missing_references())
#endif
				{
					error(TXT("couldn't find store \"%S\" for \"%S\""), ccp->fromType.to_char(), get_name().to_string().to_char());
					result = false;
				}
			}
		}
	}
#ifdef AN_CHECK_MEMORY_LEAKS
#ifdef AN_DEBUG
#ifdef AN_LOG_LOADING_AND_PREPARING
	//output(TXT("preparing [%04i] %S..."), _pfgContext.get_current_level(), name.to_string().to_char());
#endif
#endif
#endif
	return result;
}

bool LibraryStored::create_from_template(Library* _library, CreateFromTemplate const & _createFromTemplate) const
{
	todo_important(TXT("implement_ create from template for \"%S\". it is not available right now"), get_name().to_string().to_char());
	return false;
}

void LibraryStored::debug_draw() const
{
	warn(TXT("create method for this class and implement_ it!"));
}

bool LibraryStored::editor_render() const
{
	return false;
}

bool LibraryStored::preview_render() const
{
	return false;
}

void LibraryStored::log(LogInfoContext & _context) const
{
	_context.log(TXT("name: %S"), name.to_string().to_char());
	_context.log(TXT("type: %S"), get_library_type().to_char());
	_context.log(TXT("::"));
}

bool LibraryStored::does_any_exist()
{
#ifdef AN_DEVELOPMENT
	return s_global != nullptr;
#else
	return false;
#endif
}

void LibraryStored::output_all()
{
#ifdef AN_DEVELOPMENT
	if (LibraryStored* obj = s_global)
	{
		output(TXT("list of all library stored objects"));
		int idx = 0;
		while (obj)
		{
			output(TXT(" %3i. (%S) %S (%i : %i)"), idx, obj->get_library_type().to_char(), obj->get_name().to_string().to_char(), obj->get_usage_count(), obj->get_ref_count());
			obj = obj->nextGlobal;
			++idx;
		}
	}
#endif
}

bool LibraryStored::fill_on_create_from_template(LibraryStored & _newObject, CreateFromTemplate const & _createFromTemplate) const
{
	bool result = true;

	_newObject.libraryLoadLevel = libraryLoadLevel;
	_newObject.tags = tags;
	return result;
}

void LibraryStored::add_usage() const
{
	if (get_name().to_string() == TXT("airships:room for 05"))
	{
		//AN_BREAK;
	}
	usageCount.increase();
	add_ref();
}

void LibraryStored::release_usage() const
{
	if (get_name().to_string() == TXT("airships:room for 05"))
	{
		//AN_BREAK;
	}
	an_assert(usageCount.get() > 0, TXT("can't release (%S) %S"), get_library_type().to_char(), get_name().to_string().to_char());
	usageCount.decrease();
	if (autoUnloadUnused && usageCount.get() == 0)
	{
		do_auto_unload();
	}
	release_ref();
}

bool LibraryStored::setup_custom_preview_room(SubWorld* _testSubWorld, REF_ Room*& _testRoomForCustomPreview, Random::Generator const& _rg)
{
	if (_testRoomForCustomPreview)
	{
		if (auto* g = Game::get())
		{
			g->add_sync_world_job(TXT("destroy test room for custom preview"),
				[_testRoomForCustomPreview]()
				{
					_testRoomForCustomPreview->mark_to_be_deleted();
					delete _testRoomForCustomPreview;
				});
		}
		else
		{
			delete _testRoomForCustomPreview;
		}		
		_testRoomForCustomPreview = nullptr;
	}
	return false;
}

LibraryStored* LibraryStored::create_temporary_copy_using_save_load() const
{
	if (auto* s = get_library()->get_store(get_library_type()))
	{
		auto* o = s->create_temporary_object();
		o->be_temporary();

		IO::XML::Document doc;
		auto* node = doc.get_root()->add_node(TXT("object"));

		save_to_xml(node);

		//

		LibraryLoadingContext lc = LibraryLoadingContext(get_library(), String(TXT("memory::")), LibrarySource::TemporaryMemory, LibraryLoadingMode::Load);

		ForLibraryLoadingContext flc(lc);
		flc.push_id(name.to_string());
		flc.push_owner(o);
		bool result = true;

		result &= o->load_from_xml(node, lc);
		an_assert(result);

		result &= get_library()->prepare_for_game(o);
		an_assert(result);

		o->set_name(LibraryName(get_name().to_string() + TXT(" [copy]")));

		return o;
	}
	else
	{
		return nullptr;
	}
}

LibraryStored* LibraryStored::create_temporary_copy_using_copy() const
{
	if (auto* s = get_library()->get_store(get_library_type()))
	{
		auto* o = s->create_temporary_object();
		o->be_temporary();

		o->set_name(LibraryName(get_name().to_string() + TXT(" [copy]")));

		o->copy_from_for_temporary_copy(this);

		return o;
	}
	else
	{
		return nullptr;
	}
}

void LibraryStored::copy_from_for_temporary_copy(LibraryStored const* _source)
{
	access_tags().set_tags_from(_source->get_tags());
	access_custom_parameters().set_from(_source->get_custom_parameters());
}

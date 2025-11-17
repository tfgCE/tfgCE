#include "name.h"

#include "..\globalDefinitions.h"
#include "..\memory\memory.h"
#include "..\debug\detectMemoryLeaks.h"
#include "..\performance\performanceUtils.h"
#include "..\containers\array.h"
#include "..\types\string.h"
#include "..\utils.h"
#include "..\serialisation\serialiser.h"
#include "..\containers\arrayStack.h"
#include "..\system\timeStamp.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

tchar const * NameStorage::s_none = TXT("none");

#ifdef AN_DEVELOPMENT_OR_PROFILER
bool NameStorage::shouldLogCreatingNames = false;

bool NameStorage::should_log_creating_names()
{
	return shouldLogCreatingNames;
}

void NameStorage::set_should_log_creating_names(bool _should)
{
	shouldLogCreatingNames = _should;
}
#endif

int NameStorage::create_at_no_lock(tchar const* _name, int _lastValidIndex, int _lastICase)
{
	an_assert(NameStorage::access_storage_lock().is_acquired_write_on_this_thread());
#ifdef BUILD_NON_RELEASE
	if (tstrlen(_name) > MAX_NAME_LENGTH)
	{
		error_dev_investigate(TXT("name too long for name storage, make it shorter : \"%S\" (%i > %i)"), _name, tstrlen(_name), MAX_NAME_LENGTH);
	}
#endif
	an_assert(tstrlen(_name) <= MAX_NAME_LENGTH, TXT("name \"%S\" exceeded max length of %i"), _name, MAX_NAME_LENGTH);
	MEASURE_PERFORMANCE_STR_COLOURED(TXT("NameStorage::create_at_no_lock"), Colour::black);
	MEASURE_PERFORMANCE_STR_COLOURED(_name, Colour::white);
	auto& wholeNameStorage = NameStorage::access_storage();
	if (wholeNameStorage.get_reserved_size() < MAX_NAME_COUNT)
	{
		wholeNameStorage.make_space_for(MAX_NAME_COUNT);
	}
	int newIdx = wholeNameStorage.get_size();
#ifdef AN_ASSERT
	static bool failedAssertOnce = false;
	if (!failedAssertOnce &&
		wholeNameStorage.get_space_left() < 100)
	{
		failedAssertOnce = true;
		an_assert(false, TXT("nearing the end of whole name storage"));
	}
#endif
	wholeNameStorage.push_back(NameStorage(_name));
	an_assert(_lastValidIndex < newIdx);
	if (wholeNameStorage.is_index_valid(_lastValidIndex))
	{
		if (_lastICase < 0) wholeNameStorage[_lastValidIndex].icaseNegIdx = newIdx;
		if (_lastICase > 0) wholeNameStorage[_lastValidIndex].icasePosIdx = newIdx;
	}
	assert_slow(is_storage_valid_no_lock());
#ifdef AN_DEVELOPMENT_OR_PROFILER
	// commented out as currently this may lead to deadlocks (other thread trying to print out a name, hangs in to_string
	/*
	if (should_log_creating_names())
	{
		output(TXT("[name][add] \"%S\""), _name.to_char());
	}
	*/
#endif
	return newIdx;
}

//

NameStorage::Keeper NameStorage::keeper;
WholeNameStorage* NameStorage::storage = nullptr;

NameStorage::Keeper::Keeper()
{

}

NameStorage::Keeper::~Keeper()
{
	clear();
}

void NameStorage::Keeper::clear()
{
	NameStorage::access_storage().clear();
}

//

Name Name::c_invalid;

void Name::initialise_static()
{
	NameStorage::storage = &NameStorage::access_storage();
	Name::c_invalid = Name(); // it should be empty, invalid name with index == NONE
}

void Name::close_static()
{
	NameStorage::keeper.clear();
}

bool Name::serialise(Serialiser & _serialiser)
{
	if (_serialiser.is_using_name_library())
	{
		return _serialiser.serialise_name_using_name_library(*this);
	}
	else
	{
		if (_serialiser.is_writing())
		{
			String asString(to_char());
			return asString.serialise(_serialiser);
		}
		else
		{
			an_assert(_serialiser.is_reading());
			String asString;
			if (asString.serialise(_serialiser))
			{
				operator =(Name(asString));
				return true;
			}
			return false;
		}
		todo_important(TXT("what should I do?"));
		return false;
	}
}

bool Name::save_to_xml(IO::XML::Node* _node) const
{
	if (is_valid())
	{
		_node->add_text(to_char());
	}
	return true;
}

#ifdef BUILD_NON_RELEASE
void Name::dev_auto_check()
{
	Concurrency::ScopedMRSWLockWrite lock(NameStorage::access_storage_lock());

	auto& wholeNameStorage = NameStorage::access_storage();

	for_every(n, wholeNameStorage)
	{
		Name checkCreate(n->name);
		an_assert(for_everys_index(n) == checkCreate.index);
	}
}
#endif

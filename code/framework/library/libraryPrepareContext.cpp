#include "libraryPrepareContext.h"

#ifdef AN_DEVELOPMENT
#include "..\debug\previewGame.h"
#endif

#include "..\..\core\concurrency\scopedSpinLock.h"
#include "..\..\core\concurrency\threadManager.h"

using namespace Framework;

LibraryPrepareContext::LibraryPrepareContext()
: currentLevel(0)
, requestedLevel(0)
, failed(false)
{
}

void LibraryPrepareContext::request_level(int32 _level)
{
	bool allow = false;

	{
		Concurrency::ScopedSpinLock scopedLock(levelLock);

		if (_level > currentLevel)
		{
			allow = true;
			requestedLevel = requestedLevel < 0 ? _level : min(requestedLevel, _level);
		}
	}

	if (allow)
	{
		Concurrency::ScopedSpinLock scopedLock(currentObjectRequiresMoreWorkLock);

		an_assert(Concurrency::ThreadManager::get());
		int threadId = Concurrency::ThreadManager::get_current_thread_id();
		while (currentObjectRequiresMoreWork.get_size() <= threadId)
		{
			currentObjectRequiresMoreWork.push_back(false);
		}
		currentObjectRequiresMoreWork[threadId] = true;
	}
}

void LibraryPrepareContext::ready_for_next_object()
{
	Concurrency::ScopedSpinLock scopedLock(currentObjectRequiresMoreWorkLock);

	an_assert(Concurrency::ThreadManager::get());
	int threadId = Concurrency::ThreadManager::get_current_thread_id();
	while (currentObjectRequiresMoreWork.get_size() <= threadId)
	{
		currentObjectRequiresMoreWork.push_back(false);
	}
	currentObjectRequiresMoreWork[threadId] = false;
}

bool LibraryPrepareContext::does_current_object_require_more_work() const
{
	Concurrency::ScopedSpinLock scopedLock(currentObjectRequiresMoreWorkLock);

	an_assert(Concurrency::ThreadManager::get());
	int threadId = Concurrency::ThreadManager::get_current_thread_id();
	if (currentObjectRequiresMoreWork.is_index_valid(threadId))
	{
		return currentObjectRequiresMoreWork[threadId];
	}
	else
	{
		return false;
	}
}

bool LibraryPrepareContext::start_next_requested_level()
{
	if (requestedLevel < 0)
	{
		return false;
	}
	else
	{
		currentLevel = requestedLevel;
		requestedLevel = NONE;
		return true;
	}
}

#ifdef AN_DEVELOPMENT
bool LibraryPrepareContext::should_allow_failing() const
{
	return Game::get_as<PreviewGame>() != nullptr;
}
#endif

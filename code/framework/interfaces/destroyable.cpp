#include "destroyable.h"

#include "..\..\core\debug\debug.h"
#include "..\..\core\types\names.h"

#include "..\module\modulePresence.h"

#include "..\game\game.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#ifdef AN_DEVELOPMENT
//#define OUTPUT_DESTRUCTION
#endif

//

using namespace Framework;

REGISTER_FOR_FAST_CAST(IDestroyable);

IDestroyable::IDestroyable()
: destroyed(false)
, destructionBlockGuard(new DestructionBlockGuard())
, nextDestructionPendingObject(nullptr)
, prevDestructionPendingObject(nullptr)
{
}

IDestroyable::~IDestroyable()
{
	// remove from destruction pending objects chain
	if (nextDestructionPendingObject)
	{
		nextDestructionPendingObject->prevDestructionPendingObject = prevDestructionPendingObject;
	}
	if (prevDestructionPendingObject)
	{
		prevDestructionPendingObject->nextDestructionPendingObject = nextDestructionPendingObject;
	}
	else
	{
		an_assert(Game::get());
		Game* const game = Game::get();
#ifdef WITH_GAME_DESTUTION_PENDING_OBJECT_LOCK
		Concurrency::ScopedSpinLock lock(game->destructionPendingObjectLock, true);
#endif
		if (game->destructionPendingObject == this)
		{
			game->destructionPendingObject = nextDestructionPendingObject;
		}
	}
}

void IDestroyable::destruct_and_delete()
{
	if (!can_destruct())
	{
		warn(TXT("IDestroyable should not be destroyed at this time!"));
	}
	remove_destroyable_from_world();
	delete this;
}

void IDestroyable::destroy(bool _removeFromWorldImmediately)
{
#ifdef DEBUG_WORLD_CREATION_AND_DESTRUCTION
	output(TXT("IDestroyable::destroy o%p"), this);
#endif
#ifdef OUTPUT_DESTRUCTION
	output(TXT("  destroy?"));
#endif
	Concurrency::ScopedSpinLock lock(destroyLock);

	if (destroyed)
	{
#ifdef OUTPUT_DESTRUCTION
		output(TXT("  already destroyed"));
#endif
		// already destroyed
		return;
	}

	// mark as destroyed
	destroyed = true;
	
	on_destroy(_removeFromWorldImmediately);

	if (_removeFromWorldImmediately)
	{
#ifdef OUTPUT_DESTRUCTION
		output(TXT("  remove from world"));
#endif
		remove_destroyable_from_world();
	}

	// add to destruction pending objects chain
	an_assert(Game::get());
	{
		Game* const game = Game::get();
#ifdef WITH_GAME_DESTUTION_PENDING_OBJECT_LOCK
		Concurrency::ScopedSpinLock lock(game->destructionPendingObjectLock, true);
#endif
		an_assert(nextDestructionPendingObject == nullptr && prevDestructionPendingObject == nullptr);
		nextDestructionPendingObject = game->destructionPendingObject;
		if (game->destructionPendingObject)
		{
			game->destructionPendingObject->prevDestructionPendingObject = this;
		}
		game->destructionPendingObject = this;
	}
}

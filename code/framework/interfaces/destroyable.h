#pragma once

#include "..\modulesOwner\modulesOwner.h"

namespace Framework
{
	class Game;
	struct BlockDestruction;

	/**
	 */
	interface_class IDestroyable
	{
		FAST_CAST_DECLARE(IDestroyable);
		FAST_CAST_END();
	public:
		IDestroyable();
		virtual ~IDestroyable();

		virtual void destruct_and_delete();

		void block_destruction() { destructionBlockGuard->block(); }
		void allow_destruction() { destructionBlockGuard->allow(); }
		bool can_destruct() const { return !destructionBlockGuard->can_destruct(); }

		bool is_destroyed() const { return destroyed; }
		void destroy(bool _removeFromWorldImmediately = true); // will actually mark them destruction pending and will remove from wherever they are (although they will be still part of subworld/world)

		IDestroyable* get_next_destuction_pending_object() const { return nextDestructionPendingObject; }

	protected:
		virtual void on_destroy(bool _removeFromWorldImmediately) {}
		virtual void remove_destroyable_from_world() {}

	private:
		// this is a struct to always allow BlockDestruction, even if object has been destroyed
		// world destruction is done as an async job so it should not cross with any other async job
		// destruction while game is running will be blocked though
		struct DestructionBlockGuard
		: public RefCountObject
		{
		public:
			void block() { destructionBlocked.increase(); }
			void allow() { destructionBlocked.decrease(); }

			bool can_destruct() const { return destructionBlocked.get(); }

		private:
			Concurrency::Counter destructionBlocked;
		};

		Concurrency::SpinLock destroyLock = Concurrency::SpinLock(TXT("Framework.IDestroyable.destroyLock"));
		bool destroyed;
		RefCountObjectPtr<DestructionBlockGuard> destructionBlockGuard;

		// so we have short list of destroyed objects pending removal
		IDestroyable* nextDestructionPendingObject;
		IDestroyable* prevDestructionPendingObject;

		friend class Game;
		friend struct BlockDestruction;
	};

	struct BlockDestruction
	{
		RefCountObjectPtr<IDestroyable::DestructionBlockGuard> destructionBlockGuard;
		BlockDestruction(IDestroyable* _destroyable)
		{
			destructionBlockGuard = _destroyable->destructionBlockGuard;
			if (destructionBlockGuard.get())
			{
				destructionBlockGuard->block();
			}
		}
		BlockDestruction(BlockDestruction const & _bd)
		{
			destructionBlockGuard = _bd.destructionBlockGuard;
			if (destructionBlockGuard.get())
			{
				destructionBlockGuard->block();
			}
		}
		BlockDestruction & operator = (BlockDestruction const & _bd)
		{
			if (destructionBlockGuard.get())
			{
				destructionBlockGuard->allow();
			}
			destructionBlockGuard = _bd.destructionBlockGuard;
			if (destructionBlockGuard.get())
			{
				destructionBlockGuard->block();
			}
			return *this;
		}
		~BlockDestruction()
		{
			if (destructionBlockGuard.get())
			{
				destructionBlockGuard->allow();
			}
		}
	};
};

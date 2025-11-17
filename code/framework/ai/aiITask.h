#pragma once

#include "..\..\core\globalDefinitions.h"
#include "..\..\core\fastCast.h"
#include "..\..\core\concurrency\counter.h"
#include "..\..\core\containers\array.h"
#include "..\..\core\types\optional.h"

namespace Framework
{
	namespace AI
	{
		interface_class ITaskOwner;
		struct MindInstanceContext;
		struct TaskHandle;

		interface_class ITask
		{
			FAST_CAST_DECLARE(ITask);
			FAST_CAST_END();
		public:
			ITask(); // owner should have lifetime longer than task, unless it is a handle
			virtual ~ITask();

			void get_new_id();

			void release_task();

			int get_id() const { return id; }
			Array<ITaskOwner*> const & get_owners() const { return owners; }

			Optional<float> const& get_rare_advance_wait() const { return rareAdvanceWait; }
			void clear_rare_advance_wait() { rareAdvanceWait.clear(); }

		public:
			virtual bool advance(float _deltaTime) { rareAdvanceWait.clear(); return true; } // return true if finished
			virtual void on_break() {} // when stopped before finish

		protected:
			virtual void release_task_internal() { delete this; }

		protected: friend struct TaskHandle;
			void add_owner(ITaskOwner* _owner) { owners.push_back_unique(_owner); }
			void remove_owner(ITaskOwner* _owner) { owners.remove(_owner); }

		protected:
			Optional<float> rareAdvanceWait; // set during advance/by advance method if wants to wait

		private:
			static Concurrency::Counter s_id;
			int id;
			Array<ITaskOwner*> owners;
		};
	};
};

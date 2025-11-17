template <typename ElementClass>
struct BatchedJobs
{
private:
	void set_job_func(::Concurrency::ImmediateJobFunc _job_func) { job_func = _job_func; }
	void add(ElementClass* _element)
	{
		an_assert_log_always(elements.has_place_left(), TXT("batched jobs has no place left"));
		elements.push_back(_element);
	}
	bool is_full(int _max) { return elements.get_size() >= _max || ! elements.has_place_left(); }

	static void perform(::Concurrency::JobExecutionContext const * _executionContext, void** _data, int _count)
	{
		FOR_EVERY_IMMEDIATE_JOB_DATA(BatchedJobs, bj)
		{
			MEASURE_PERFORMANCE_COLOURED(batchedJob, Colour::blue.with_alpha(0.2f));
			PERFORMANCE_GUARD_LIMIT_2(0.010f * (float)bj->elements.get_size(), TXT("[immediate job]"), JobSystem::get_performance_measure_info());
			PERFORMANCE_LIMIT_GUARD_START();
			bj->job_func(_executionContext, bj->elements.get_data(), bj->elements.get_size());
			PERFORMANCE_LIMIT_GUARD_STOP(0.001f * (float)bj->elements.get_size(), TXT("[immediate job] %S"), JobSystem::get_performance_measure_info());
			bj->release();
		}
	}

	static void manage_released()
	{
		MEASURE_PERFORMANCE_COLOURED(batchedJob_manageReleased, (Colour::blue * 0.5f).with_alpha(0.2f));
		if (releasedFree.is_empty())
		{
			int threadCount = max(Concurrency::ThreadSystemUtils::get_number_of_cores(), Concurrency::ThreadManager::get_thread_count());
			while (releasedFree.get_size() < threadCount && releasedFree.has_place_left())
			{
				releasedFree.push_back(nullptr);
			}
		}
		move_released_to_free();
	}

	static void move_released_to_free()
	{
		for_every_ptr(rfQueue, releasedFree)
		{
			auto* rf = rfQueue;
			while (rf)
			{
				auto* f = rf;
				rf = f->next;
				f->next = firstFree;
				firstFree = f;
			}
			releasedFree[for_everys_index(rfQueue)] = nullptr;
		}
	}

	// this is simplified version of soft pooled object - we do not have any locks when getting new batched job as we're doing that from one thread
	// when we release, we have lock to sort them properly
	// we also have a keeper to get rid of batched jobs created during run time
	static BatchedJobs* get_one()
	{
		if (!firstFree)
		{
			if (count == 0)
			{
				atexit(&destroy_at_exit);
			}
			++count;
			firstFree = new BatchedJobs();
			SET_EXTRA_DEBUG_INFO(firstFree->elements, TXT("JobSystem::BatchedJobs.elements"));
#ifdef AN_DEVELOPMENT
			firstFree->nextAll = all;
			all = firstFree;
#endif
		}
		BatchedJobs* one = firstFree;
#ifdef AN_DEVELOPMENT
		an_assert_immediate(one->next != one);
#endif
		firstFree = one->next;
#ifdef AN_DEVELOPMENT
		an_assert_immediate(!one->inUse);
		one->inUse = true;
#endif
		one->job_func = nullptr;
		one->next = nullptr; // remove from the list
		one->elements.clear();
		return one;
	}

	static int count;
	static BatchedJobs* firstFree;
	static ArrayStatic<BatchedJobs<ElementClass>*, THREAD_LIMIT> releasedFree;
#ifdef AN_DEVELOPMENT
	static BatchedJobs* all;
	static Concurrency::SpinLock debugReleaseLock;
#endif
#ifdef AN_DEVELOPMENT
	bool inUse = false;
#endif
	BatchedJobs* next = nullptr;
#ifdef AN_DEVELOPMENT
	BatchedJobs* nextAll = nullptr;
#endif
	ArrayStatic<void*, MAX_IMMEDIATE_JOB_BATCH_SIZE> elements;
	::Concurrency::ImmediateJobFunc job_func;

	void release()
	{
		//MEASURE_PERFORMANCE_COLOURED(batchedJob_release, (Colour::blue * 0.5f).with_alpha(0.2f));
#ifdef AN_DEVELOPMENT_SUPER_SLOW
#ifdef AN_CONCURRENCY_STATS
		debugReleaseLock.do_not_report_stats();
#endif
		Concurrency::ScopedSpinLock lock(debugReleaseLock);

		// check if already added
		{
			MEASURE_PERFORMANCE_COLOURED(batchedJob_release_checkIfAdded, (Colour::blue * 0.5f).with_alpha(0.2f));
			BatchedJobs* check = firstFree;
			while (check)
			{
				an_assert_immediate(check != this);
				check = check->next;
			}
			for_every_ptr(rf, releasedFree)
			{
				BatchedJobs* check = rf;
				while (check)
				{
					an_assert_immediate(check != this);
					check = check->next;
				}
			}
		}
#endif
#ifdef AN_DEVELOPMENT
		an_assert_immediate(inUse);
#endif
		an_assert_immediate(!next);
#ifdef AN_DEVELOPMENT
		inUse = false;
#endif
		int const currentThreadId = Concurrency::ThreadManager::get_current_thread_id();
		an_assert_log_always(currentThreadId < releasedFree.get_size(), TXT("not enough place for threads"));

		auto* & firstFreeInReleasedFree = releasedFree[currentThreadId];
		an_assert_immediate(firstFreeInReleasedFree != this);
		next = firstFreeInReleasedFree;
		firstFreeInReleasedFree = this;
		an_assert_immediate(next != this);
#ifdef AN_DEVELOPMENT_SUPER_SLOW
		{
			MEASURE_PERFORMANCE_COLOURED(batchedJob_release_checkForLoops, (Colour::blue * 0.5f).with_alpha(0.2f));
			// check for loops
			for_every_ptr(rf, releasedFree)
			{
				BatchedJobs* check = rf;
				while (check)
				{
					BatchedJobs* go = check->next;
					while (go)
					{
						an_assert_immediate(go != check);
						go = go->next;
					}
					check = check->next;
				}
			}
		}
#endif
	}

	static void destroy_at_exit()
	{
		move_released_to_free();

		while (firstFree)
		{
			BatchedJobs* go = firstFree->next;
#ifdef AN_DEVELOPMENT
			an_assert_immediate(!firstFree->inUse);
#endif
			delete firstFree;
			firstFree = go;
		}
	}

	friend class JobSystem;
};

#ifdef AN_DEVELOPMENT
template <typename ElementClass>
BatchedJobs<ElementClass>* BatchedJobs<ElementClass>::all = nullptr;
#endif

template <typename ElementClass>
BatchedJobs<ElementClass>* BatchedJobs<ElementClass>::firstFree = nullptr;

template <typename ElementClass>
ArrayStatic<BatchedJobs<ElementClass>*, THREAD_LIMIT> BatchedJobs<ElementClass>::releasedFree;

template <typename ElementClass>
int BatchedJobs<ElementClass>::count = 0;

#ifdef AN_DEVELOPMENT
template <typename ElementClass>
Concurrency::SpinLock BatchedJobs<ElementClass>::debugReleaseLock = Concurrency::SpinLock(TXT("Framework.BatchedJobs.debugReleaseLock"));
#endif

template <typename ElementClass, typename Container>
void JobSystem::do_immediate_jobs(Name const & _listName, Concurrency::ImmediateJobFunc _jobFunc, Container & _container, std::function<bool(ElementClass const *)> _check, int _batchLimit)
{
	Concurrency::ImmediateJobList* immediateJobList = get_immediate_list(_listName);

	// collect immediate jobs
	{
		MEASURE_PERFORMANCE_COLOURED(collectImmediateJobs, Colour::black.with_alpha(0.2f));
		an_assert(immediateJobList);

		// check first as we may divide batches differently
		// ARRAY_STACK does not work with Clang, this is exactly the same thing as in macro
		int validJobsSize = _container.get_size();
		allocate_stack_var(ElementClass*, validJobsStack, validJobsSize);
		ArrayStack<ElementClass*> validJobs((ElementClass**)validJobsStack, validJobsSize);
		SET_EXTRA_DEBUG_INFO(validJobs, TXT("validJobs"));

		{
			scoped_call_stack_info(TXT("(job system) collect valid jobs"));
			for_every_ptr(contained, _container)
			{
				ElementClass* element = up_cast<ElementClass>(contained);
				if (!_check || _check(element))
				{
					validJobs.push_back(element);
				}
			}
		}

		if (_batchLimit == NONE)
		{
			an_assert(immediateJobList->get_executors_count());
			_batchLimit = clamp(validJobs.get_size() / immediateJobList->get_executors_count(), 1, MAX_IMMEDIATE_JOB_BATCH_SIZE);
		}
#ifdef AN_DEVELOPMENT_OR_PROFILER
		if (!useBatches)
		{
			_batchLimit = 1;
		}
#endif

		scoped_call_stack_info(TXT("(job system) ready jobs"));

		immediateJobList->ready_for_job_addition();
		if (_batchLimit <= 1)
		{
			//MEASURE_PERFORMANCE_COLOURED(collectImmediateJobs_nonBatched_add, Colour::white.with_alpha(0.2f));
			for_every_ptr(element, validJobs)
			{
				immediateJobList->add_job(_jobFunc, element);
			}
		}
		else
		{
			//MEASURE_PERFORMANCE_COLOURED(collectImmediateJobs_batched_add, Colour::white.with_alpha(0.2f));
#ifdef AN_DEVELOPMENT
			// ARRAY_STACK does not work with Clang, this is exactly the same thing as in macro
			int addedBatchedJobsSize = _container.get_size();
			allocate_stack_var(BatchedJobs<ElementClass>*, addedBatchedJobsStack, addedBatchedJobsSize);
			ArrayStack<BatchedJobs<ElementClass>*> addedBatchedJobs((BatchedJobs<ElementClass>**)addedBatchedJobsStack, addedBatchedJobsSize); SET_EXTRA_DEBUG_INFO(addedBatchedJobs, TXT("JobSystem::do_immediate_jobs.addedBatchedJobs"));
#ifndef AN_CLANG
			auto & all = BatchedJobs<ElementClass>::all;
#endif
#endif
			BatchedJobs<ElementClass>* batchedJobs = nullptr;
			BatchedJobs<ElementClass>::manage_released();
#ifdef MIX_IN_BATCHES
			int const batchLimit = _batchLimit;
			// mix as most likely similar objects are grouped as they were created in batches
			for (int startPoint = 0; startPoint < batchLimit && startPoint < validJobs.get_size(); ++startPoint)
			{
				ElementClass** pElement = &validJobs[startPoint];
				for (int idx = startPoint; idx < validJobs.get_size(); idx += batchLimit, pElement += batchLimit)
				{
					ElementClass* element = *pElement;
#else
			{
				for_every_ptr(element, validJobs)
				{
#endif
					if (batchedJobs && batchedJobs->is_full(_batchLimit))
					{
						immediateJobList->add_job(BatchedJobs<ElementClass>::perform, batchedJobs);
#ifdef AN_DEVELOPMENT
						an_assert(!addedBatchedJobs.does_contain(batchedJobs));
						addedBatchedJobs.push_back(batchedJobs);
#endif
						batchedJobs = nullptr;
					}
					if (!batchedJobs)
					{
						batchedJobs = BatchedJobs<ElementClass>::get_one();
						batchedJobs->set_job_func(_jobFunc);
					}
					batchedJobs->add(element);
				}
			}
			if (batchedJobs)
			{
				immediateJobList->add_job(BatchedJobs<ElementClass>::perform, batchedJobs);
#ifdef AN_DEVELOPMENT
				an_assert(!addedBatchedJobs.does_contain(batchedJobs));
				addedBatchedJobs.push_back(batchedJobs);
#endif
				batchedJobs = nullptr;
			}
#ifdef AN_DEVELOPMENT
			// make sure we don't double them
			for_every_ptr(bj, addedBatchedJobs)
			{
#ifdef AN_DEVELOPMENT
				an_assert(bj->inUse);
#endif

				for_every_ptr(obj, addedBatchedJobs)
				{
					if (for_everys_index(bj) != for_everys_index(obj))
					{
						an_assert(bj != obj);
					}
				}
			}
#endif
		}
		immediateJobList->ready_for_job_execution();
	}

	// execute immediate jobs
#ifdef JOB_SYSTEM_USES_SYNCHRONISATION_LOUNGE
	synchronisationLounge.wake_up_all();
#endif
	{
		scoped_call_stack_info(TXT("(job system) execute jobs"));

#ifdef AN_JOB_SYSTEM_CHECK_FOR_LOCKS
		auto* jobSystemThread = get_current_job_system_thread();
#endif
		MEASURE_PERFORMANCE_COLOURED(executeImmediateJobs, Colour::blue.with_alpha(0.2f));
		while (JobSystemExecutor::execute_immediate_job(immediateJobList, &access_advance_context()) || !immediateJobList->has_finished())
		{
#ifdef AN_JOB_SYSTEM_CHECK_FOR_LOCKS
			if (jobSystemThread)
			{
				jobSystemThread->aliveAndKickingTS.reset();
			}
			else
			{
				// executing on main
				aliveAndKickingTS.reset();
			}
#endif
		}
	}

	immediateJobList->end_job_execution();

	Concurrency::memory_fence();
}

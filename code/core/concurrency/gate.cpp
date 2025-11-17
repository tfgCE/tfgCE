#include "gate.h"

#include "..\memory\memory.h"
#include "..\debug\debug.h"

//

using namespace Concurrency;

//

#ifdef AN_WINDOWS

#include <Windows.h>

struct Concurrency::GateImpl
{
private:
	HANDLE semaphore;

public:
	GateImpl()
	{
		semaphore = CreateSemaphore(nullptr,
			0,  // initial count
			1,  // maximum count (will we use more than 1?
			nullptr);
	}

	~GateImpl()
	{
		CloseHandle(semaphore);
	}

	void allow_one()
	{
		ReleaseSemaphore(semaphore, 1, nullptr);
	}

	void wait()
	{
		DWORD result = WaitForSingleObject(semaphore, 1000);
		if (result == WAIT_OBJECT_0)
		{
			return;
		}
		else
		{
#ifdef AN_DEVELOPMENT
			error_dev_ignore(TXT("hung on semaphore?"));
#else
			error(TXT("hung on semaphore?"));
#endif
			WaitForSingleObject(semaphore, INFINITE);
		}
	}

	bool wait_for(float _time)
	{
		DWORD result = WaitForSingleObject(semaphore, (int)(_time * 1000.0f));
		if (result == WAIT_OBJECT_0)
		{
			return true;
		}
		else
		{
			return false;
		}
	}
};
#else
#include <semaphore.h>

#ifdef AN_LINUX_OR_ANDROID
struct Concurrency::GateImpl
{
private:
	sem_t semaphore;

public:
	GateImpl()
	{
		sem_init(&semaphore, 0, 0);
	}

	~GateImpl()
	{
		sem_destroy(&semaphore);
	}

	void allow_one()
	{
		sem_post(&semaphore);
	}

	void wait()
	{
		sem_wait(&semaphore);
	}

	bool wait_for(float _time)
	{
		timespec ts;
		int t_ms = (int)(_time * 1000.0f);
		int ms = t_ms % 1000;
		int s_ms = (t_ms - ms) / 1000;
		ts.tv_nsec = ms * 1000000;
		ts.tv_sec = s_ms;

		int result = sem_timedwait(&semaphore, &ts);

		return result != -1; // -1 means error or breaking
	}
};
#else
#error TODO implement gate
#endif
#endif

Gate::Gate()
{
#ifdef AN_CHECK_MEMORY_LEAKS
	supress_memory_leak_detection();
#endif
	impl = new GateImpl();
#ifdef AN_CHECK_MEMORY_LEAKS
	resume_memory_leak_detection();
#endif
}

Gate::~Gate()
{
#ifdef AN_CHECK_MEMORY_LEAKS
	supress_memory_leak_detection();
#endif
	delete impl;
#ifdef AN_CHECK_MEMORY_LEAKS
	resume_memory_leak_detection();
#endif
}

void Gate::allow_one()
{
	impl->allow_one();
}

void Gate::wait()
{
	impl->wait();
}

bool Gate::wait_for(float _time)
{
	return impl->wait_for(_time);
}
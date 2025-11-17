#pragma once

class RegisteredPool
{
public:
	RegisteredPool();
	virtual ~RegisteredPool();

	static void prune_all();

	// to be implemented in pools
	virtual void prune() = 0;

protected:
	virtual void destroy() = 0;

private:
	static RegisteredPool* s_registeredPools;
	RegisteredPool* nextPool;

	static void destroy_at_exit();
};

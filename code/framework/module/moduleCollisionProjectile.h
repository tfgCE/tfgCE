#pragma once

#include "moduleCollision.h"

namespace Framework
{
	class ModuleCollisionProjectile
	: public ModuleCollision
	{
		FAST_CAST_DECLARE(ModuleCollisionProjectile);
		FAST_CAST_BASE(ModuleCollision);
		FAST_CAST_END();

		typedef ModuleCollision base;
	public:
		static RegisteredModule<ModuleCollision> & register_itself();

		ModuleCollisionProjectile(IModulesOwner* _owner);
		virtual ~ModuleCollisionProjectile();

	public: // ModuleCollision
		override_ void update_gradient_and_detection();

	};

};


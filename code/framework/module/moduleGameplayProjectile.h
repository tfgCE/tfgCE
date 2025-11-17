#pragma once

#include "moduleGameplay.h"

namespace Framework
{
	struct CollisionInfo;

	class ModuleGameplayProjectile
	: public ModuleGameplay
	{
		FAST_CAST_DECLARE(ModuleGameplayProjectile);
		FAST_CAST_BASE(ModuleGameplay);
		FAST_CAST_END();

		typedef ModuleGameplay base;
	public:
		static RegisteredModule<ModuleGameplay> & register_itself();

		ModuleGameplayProjectile(IModulesOwner* _owner);
		virtual ~ModuleGameplayProjectile();

	public: // ModuleGameplay
		override_ void advance_post_move(float _deltaTime);

	public: // Module
		override_ void reset();
		override_ void setup_with(ModuleData const * _moduleData);

	protected:
		virtual bool process_hit(CollisionInfo const & _ci); // returns true if should continue processing

	protected:
		float timeToLive = 30.0f; // should take no more than 30 seconds
		float scaleBlendTime = 0.4f;
		float pulseScale = 0.0f;
		float pulseScalePeriod = 0.5f;

		// run
		float startingScale = 1.0f;
		float scale = 1.0f;
		float scaleTarget = 1.0f;
		float actualScale = 1.0f;
		float pulseScaleAtPt = 0.0f;
	};

};


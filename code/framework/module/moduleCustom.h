#pragma once

#include "module.h"
#include "..\jobSystem\jobSystemsImmediateJobPerformer.h"

namespace Framework
{
	/**
	 *	Extra functionality or just data containers
	 */
	class ModuleCustom
	: public Module
	{
		FAST_CAST_DECLARE(ModuleCustom);
		FAST_CAST_BASE(Module);
		FAST_CAST_END();
	public:
		static RegisteredModule<ModuleCustom> & register_itself();

		ModuleCustom(IModulesOwner* _owner);

	public:
		virtual void on_placed_in_world() {} // to allow to register, as we maybe did not register our functions when we were not placed in world, called ONLY when was not placed in room before

	public: // advancements
		// advance - begin
		static void all_customs__advance_post(IMMEDIATE_JOB_PARAMS);
		// advance - end

		static void all_customs__update_rare_advance(IModulesOwner const* _modulesOwner, float _deltaTime);
		static bool all_customs__does_require(Concurrency::ImmediateJobFunc _jobFunc, IModulesOwner const* _modulesOwner);

	protected:
		virtual void advance_post(float _deltaTime) {}

	protected: // to know whether we need to advance or no
		void mark_requires(Concurrency::ImmediateJobFunc _jobFunc, int _flags = 0xffffffff);
		void mark_no_longer_requires(Concurrency::ImmediateJobFunc _jobFunc, int _flags = 0xffffffff);

	public: // Module
		override_ bool does_require(Concurrency::ImmediateJobFunc _jobFunc) const;

	protected:
		ModuleRareAdvance raAdvancePost;

		void rarer_post_advance_if_not_visible();

	private:
		Concurrency::SpinLock requiresLock = Concurrency::SpinLock(TXT("Framework.ModuleCustom.requiresLock"));
		struct Requires
		{
			Concurrency::ImmediateJobFunc func;
			int flags = 0;
		};
		Array<Requires> requires;
	};
};


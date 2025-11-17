#pragma once

#include "module.h"
#include "..\jobSystem\jobSystemsImmediateJobPerformer.h"

namespace Framework
{
	/**
	 *	Base for actual gameplay related modules - this one is not registred and as empty module should not be created.
	 */
	class ModuleGameplay
	: public Module
	{
		FAST_CAST_DECLARE(ModuleGameplay);
		FAST_CAST_BASE(Module);
		FAST_CAST_END();

		typedef Module base;
	public:
		static RegisteredModule<ModuleGameplay> & register_itself();

		ModuleGameplay(IModulesOwner* _owner);

		static bool does_use__advance__post_logic();
		static void use__advance__post_logic(bool _use);

	public:
		// for some objects we may want to always update in specific order, for most we don't care
		// if a user holds an item, it might be advised (or even required!) to keep the update order
		// if we update this way, we go through the whole hierarchy
		void set_update_attached(bool _udpateAttached) { updateAttached = _udpateAttached; }
		void set_update_with_attached_to(bool _udpateWithAttachedTo) { updateWithAttachedTo = _udpateWithAttachedTo; }

		bool should_update_attached() const { return updateAttached; }
		bool should_update_with_attached_to() const { return updateWithAttachedTo; }

	public:
		// advance - begin
		static void advance__post_logic(IMMEDIATE_JOB_PARAMS);
		static void advance__post_move(IMMEDIATE_JOB_PARAMS);
		// advance - end

	public: // if particular modules have to be forced to exist
		virtual bool does_require_temporary_objects() const { return false; }

	protected:
		bool updateAttached = false; // should be updating attached objects
		bool updateWithAttachedTo = false; // should be updated when attached to

		virtual void advance_post_logic(float _deltaTime) {}
		virtual void advance_post_move(float _deltaTime) {}
	};
};


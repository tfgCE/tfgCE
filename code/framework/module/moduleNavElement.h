#pragma once

#include "module.h"

namespace Framework
{
	namespace Nav
	{
		class Mesh;
	};

	/**
	 *	Base for actual gameplay related modules - this one is not registred and as empty module should not be created.
	 */
	class ModuleNavElement
	: public Module
	{
		FAST_CAST_DECLARE(ModuleNavElement);
		FAST_CAST_BASE(Module);
		FAST_CAST_END();
	public:
		static RegisteredModule<ModuleNavElement> & register_itself();

		ModuleNavElement(IModulesOwner* _owner);

	public:
		virtual void add_to(Nav::Mesh* _navMesh);
		virtual bool is_moveable() const { return false; }
	};
};


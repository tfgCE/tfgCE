#pragma once

#include "moduleNavElement.h"

#include "..\nav\navNode.h"

namespace Framework
{
	class ModuleNavElementNode
	: public ModuleNavElement
	{
		FAST_CAST_DECLARE(ModuleNavElementNode);
		FAST_CAST_BASE(ModuleNavElement);
		FAST_CAST_END();

		typedef ModuleNavElement base;
	public:
		static RegisteredModule<ModuleNavElement> & register_itself();

		ModuleNavElementNode(IModulesOwner* _owner);

	public:
		implement_ void add_to(Nav::Mesh* _navMesh);

	private:
		Nav::NodePtr node;
	};
};


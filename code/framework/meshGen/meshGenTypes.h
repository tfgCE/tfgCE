#pragma once

#include <functional>

namespace Framework
{
	class MeshGenerator;

	namespace MeshGeneration
	{
		struct GenerationContext;

		/**
		 *	This function's purpose is to provide mesh generator basing on include-stack.
		 *	Include-stack is an array of names. It might be used to have code-based solution for including particular mesh generators. For data oriented approach, ti is better to use "mesh gen selectors"
		 */
		typedef std::function<MeshGenerator * (GenerationContext const& _context)> IncludeStackProcessorFunc;

	};
};

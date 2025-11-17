#pragma once

#include "..\..\core\memory\refCountObject.h"

#include "gameScriptExecution.h"

namespace IO
{
	namespace XML
	{
		class Node;
	};
};

namespace Framework
{
	class Library;
	struct LibraryLoadingContext;
	struct LibraryPrepareContext;

	namespace GameScript
	{
		class ScriptCondition
		: public RefCountObject
		{
		public:
			ScriptCondition();
			virtual ~ScriptCondition();

			virtual bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
			virtual bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);

			virtual bool check(ScriptExecution const& _execution) const = 0;
		};
	};
};

#pragma once

#include "..\..\core\fastCast.h"
#include "..\..\core\memory\refCountObject.h"
#include "..\..\core\types\colour.h"

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
		class ScriptElement
		: public RefCountObject
		{
			FAST_CAST_DECLARE(ScriptElement);
			FAST_CAST_END();
		public:
			ScriptElement();
			virtual ~ScriptElement();

			virtual bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
			virtual bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);

			virtual ScriptExecutionResult::Type execute(ScriptExecution & _execution, ScriptExecution::Flags _flags) const = 0;
			virtual void interrupted(ScriptExecution& _execution) const {} // on trap/end

			virtual tchar const* get_debug_info() const = 0;
			virtual Optional<Colour> get_colour() const { return NP; }
			virtual String get_extra_debug_info() const { return String::empty(); }

			virtual void load_on_demand_if_required() {}
		};
	};
};

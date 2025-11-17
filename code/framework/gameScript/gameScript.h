#pragma once

#include "gameScriptElement.h"

#include "..\library\libraryStored.h"

namespace Framework
{
	namespace GameScript
	{
		class ScriptExecution;

		class Script
		: public Framework::LibraryStored
		{
			FAST_CAST_DECLARE(Script);
			FAST_CAST_BASE(LibraryStored);
			FAST_CAST_END();
			LIBRARY_STORED_DECLARE_TYPE();

		typedef LibraryStored base;
		public:
			Script(Framework::Library* _library, Framework::LibraryName const& _name);
			virtual ~Script();

			bool is_empty() const { return elements.is_empty(); }

			Array<RefCountObjectPtr<ScriptElement>> const& get_elements() const { return elements; }

			int get_execution_trap(ScriptExecution& _execution, Name const& _trapName, OUT_ OPTIONAL_ bool * _sub = nullptr) const;

			void log(LogInfoContext& _log) const;

		public: // LibraryStored
			override_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
			override_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);
			override_ void prepare_to_unload();

		public:
			void load_elements_on_demand_if_required(); // go through elements and load anything that is required, done just once (!) to not fall into a loop

		private:
			Array<RefCountObjectPtr<ScriptElement>> elements;
			bool elementsLoadedOnDemand = false;

			struct ExecutionTrap
			{
				Name trapName;
				int at;
				bool sub;

				ExecutionTrap(Name const& _trapName = Name::invalid(), int _at = NONE, bool _sub = false) : trapName(_trapName), at(_at), sub(_sub) {}
			};
			Array<ExecutionTrap> executionTraps;
		};
	};
};

#pragma once

#include "..\..\..\framework\gameScript\gameScriptElement.h"

#include "..\..\game\energy.h"

#include "..\..\..\core\types\hand.h"
#include "..\..\..\framework\library\usedLibraryStored.h"

namespace Framework
{
	class ItemType;
};

namespace TeaForGodEmperor
{
	namespace GameScript
	{
		namespace Elements
		{
			class CreateMainEquipment
			: public Framework::GameScript::ScriptElement
			{
				typedef Framework::GameScript::ScriptElement base;
			public: // ScriptElement
				implement_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
				implement_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);
				implement_ Framework::GameScript::ScriptExecutionResult::Type execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const;
				implement_ tchar const* get_debug_info() const { return TXT("create main equipment"); }

			private:
				Hand::Type hand = Hand::MAX;
				Framework::UsedLibraryStored<Framework::ItemType> itemType;
				Energy energy = Energy(400);
			};
		};
	};
};

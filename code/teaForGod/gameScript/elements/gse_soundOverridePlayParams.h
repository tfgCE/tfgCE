#pragma once

#include "..\..\..\framework\gameScript\gameScriptElement.h"

#include "..\..\..\framework\library\usedLibraryStored.h"
#include "..\..\..\framework\sound\soundPlayParams.h"
#include "..\..\..\framework\sound\soundSample.h"

namespace Framework
{
	interface_class IModulesOwner;
};

namespace TeaForGodEmperor
{
	namespace GameScript
	{
		namespace Elements
		{
			class SoundOverridePlayParams
			: public Framework::GameScript::ScriptElement
			{
				typedef Framework::GameScript::ScriptElement base;
			public: // ScriptElement
				implement_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
				implement_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);
				implement_ Framework::GameScript::ScriptExecutionResult::Type execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const;
				implement_ tchar const* get_debug_info() const { return TXT("sound override play params"); }

			private:
				Name objectVar;

				bool clearAll = false;
				bool clear = false;
				bool allAttached = false;
				Name sound;
				Framework::UsedLibraryStored<Framework::Sample> sample;

				Framework::SoundPlayParams playParams;

				void apply_to(Framework::IModulesOwner* _imo) const;
			};
		};
	};
};

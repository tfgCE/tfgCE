#pragma once

#include "..\..\..\framework\gameScript\elements\gse_door.h"

namespace TeaForGodEmperor
{
	namespace GameScript
	{
		namespace Elements
		{
			class Door
			: public Framework::GameScript::Elements::Door
			{
				typedef Framework::GameScript::Elements::Door base;
			public: // ScriptElement
				implement_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
				implement_ Framework::GameScript::ScriptExecutionResult::Type execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const;
				implement_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);

			protected:
				// via game director
				Optional<float> openInTime;
				Optional<int> extendOpenInTimeByPeriodNum;
				Optional<float> closeInTime; // lock
				Optional<bool> overrideLock; // lock

				// params for openInTime
				Optional<float> openInTimePeriod;
				Framework::UsedLibraryStored<Framework::Sample> openingSound;
				Framework::UsedLibraryStored<Framework::Sample> openSound;

				// params for closeInTime
				Framework::UsedLibraryStored<Framework::Sample> closedSound;

				// extend
				Optional<float> maxTimeToExtend;
			};
		};
	};
};

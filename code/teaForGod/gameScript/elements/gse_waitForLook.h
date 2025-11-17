#pragma once

#include "..\..\..\framework\gameScript\gameScriptElement.h"
#include "..\..\..\core\math\math.h"
#include "..\..\..\core\types\optional.h"

namespace TeaForGodEmperor
{
	namespace GameScript
	{
		namespace Elements
		{
			class WaitForLook
			: public Framework::GameScript::ScriptElement
			{
				typedef Framework::GameScript::ScriptElement base;
			public:
				WaitForLook() {}

			public: // ScriptElement
				implement_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
				implement_ Framework::GameScript::ScriptExecutionResult::Type execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const;
				implement_ void interrupted(Framework::GameScript::ScriptExecution& _execution) const; // on either goto/trap
				implement_ tchar const* get_debug_info() const { return TXT("wait for look"); }

			private:
				enum Target
				{
					None,
					Hands,
					Pockets,
					EXMTrays,
					FADs,
					MainEquipment,
					Palace,
					ObjectFromVar,
					HubScreen,
					POI
				};
				Target target = Target::None;

				bool endless = false;

				Optional<float> timeLimit;
				Name goToLabelOnTimeLimit;

				Optional<float> time;
				Optional<bool> prompt;

				Optional<float> distance;

				Optional<Name> atPOI; // in room
				Optional<Name> atClosestPOI; // in room

				Optional<Name> atVar;
				Optional<Name> socket;

				Optional<float> radius;
				Optional<float> angle;
				Optional<Vector3> offsetWS;

				Optional<Name> atHubScreen;

				Optional<bool> justClear; // just clear and proceed
			};
		};
	};
};

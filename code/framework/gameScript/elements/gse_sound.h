#pragma once

#include "..\gameScriptElement.h"

#include "..\..\library\usedLibraryStored.h"
#include "..\..\sound\soundPlayParams.h"
#include "..\..\sound\soundSample.h"

namespace Framework
{
	namespace GameScript
	{
		namespace Elements
		{
			class Sound
			: public ScriptElement
			{
				typedef ScriptElement base;
			public: // ScriptElement
				implement_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
				implement_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);
				implement_ ScriptExecutionResult::Type execute(ScriptExecution& _execution, ScriptExecution::Flags _flags) const;
				implement_ tchar const* get_debug_info() const { return TXT("sound"); }

				implement_ void load_on_demand_if_required();

			protected:
				Name objectVar;
				Name sound;
				Name stop;
				Optional<float> fadeOutOnStop;

				struct MutlipleSamples
				{
					Array<Framework::UsedLibraryStored<Framework::Sample>> samples;

					bool load_from_xml(IO::XML::Node const* _node, tchar const * _what, Framework::LibraryLoadingContext& _lc);
					bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);
				};
				MutlipleSamples stopSample; // all
				MutlipleSamples sample; // all or choose one
				bool playAllSamples = false;

				Framework::SoundPlayParams playParams;
				Optional<Name> poi; // room poi!
				Optional<Name> socket;
				Optional<Name> socketVar;
				Transform relativePlacement = Transform::identity;

				bool playDetached = false;
			};
		};
	};
};

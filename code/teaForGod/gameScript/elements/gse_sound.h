#pragma once

#include "..\..\..\framework\gameScript\elements\gse_sound.h"

namespace TeaForGodEmperor
{
	namespace GameScript
	{
		namespace Elements
		{
			class Sound
			: public Framework::GameScript::Elements::Sound
			{
				typedef Framework::GameScript::Elements::Sound base;
			public: // ScriptElement
				override_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
				override_ Framework::GameScript::ScriptExecutionResult::Type execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const;

			private:
				Optional<float> dullness;
				Optional<float> dullnessBlendTime;

				bool extraPlayback = false;
				Name extraPlaybackId;
				Optional<float> extraPlaybackStartAt;

				Name stopExtraPlaybackId;
			};
		};
	};
};

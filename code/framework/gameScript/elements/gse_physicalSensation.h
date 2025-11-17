#pragma once

#include "..\gameScriptElement.h"
#include "..\..\..\core\physicalSensations\iPhysicalSensations.h"

namespace Framework
{
	namespace Story
	{
		class Scene;
	};

	namespace GameScript
	{
		namespace Elements
		{
			class PhysicalSensation
			: public ScriptElement
			{
				typedef ScriptElement base;
			public: // ScriptElement
				implement_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
				implement_ ScriptExecutionResult::Type execute(ScriptExecution& _execution, ScriptExecution::Flags _flags) const;
				implement_ tchar const* get_debug_info() const { return TXT("physical sensations"); }

			private:
				Optional<PhysicalSensations::SingleSensation::Type> singleSensation;
				Optional<PhysicalSensations::OngoingSensation::Type> ongoingSensation;

				Optional<float> duration;
			};
		};
	};
};

#pragma once

#include "..\..\..\framework\gameScript\gameScriptCondition.h"

#include "..\..\..\core\math\math.h"

namespace TeaForGodEmperor
{
	namespace GameScript
	{
		namespace Conditions
		{
			class CompareCellCoord
			: public Framework::GameScript::ScriptCondition
			{
				typedef Framework::GameScript::ScriptCondition base;
			public: // ScriptCondition
				implement_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);

				implement_ bool check(Framework::GameScript::ScriptExecution const& _execution) const;

			private:
				Name varA;
				Name varB;

				VectorInt2 aOffset = VectorInt2::zero;
				VectorInt2 bOffset = VectorInt2::zero;

				enum CompareFunc
				{
					Equal,
					AYgreaterBY,
					AYlessBY,
				};
				CompareFunc compareFunc = CompareFunc::Equal;
			};
		}
	}
}
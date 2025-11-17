#pragma once

#include "..\..\..\framework\gameScript\gameScriptElement.h"
#include "..\..\..\core\math\math.h"
#include "..\..\..\core\tags\tagCondition.h"

namespace TeaForGodEmperor
{
	namespace GameScript
	{
		namespace Elements
		{
			class ForceCamera
			: public Framework::GameScript::ScriptElement
			{
				typedef Framework::GameScript::ScriptElement base;
			public: // ScriptElement
				implement_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
				implement_ Framework::GameScript::ScriptExecutionResult::Type execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const;
				implement_ tchar const* get_debug_info() const { return TXT("force camera"); }

			private:
				Optional<Transform> placement;
				
				TagCondition inRoom;
				Name inRoomVar;

				bool drop = false;

				float blendTime = 0.0f;
				float time = 0.0f;

				Name atObjectVar;
				Optional<Transform> atRelPlacement;

				Name attachToObjectVar;

				Optional<float> fov;
				Name blend;
				Name rotationBlend;
				Optional<BezierCurve<BezierCurveTBasedPoint<float>>> blendCurve;
				Optional<BezierCurve<BezierCurveTBasedPoint<Vector3>>> locationCurve;
				Optional<BezierCurve<BezierCurveTBasedPoint<Rotator3>>> orientationCurve;

				Name offsetPlacementRelativelyToParam; // in room
			};
		};
	};
};

#pragma once

#include "..\..\..\framework\gameScript\gameScriptCondition.h"

#include "..\..\..\core\tags\tagCondition.h"

namespace TeaForGodEmperor
{
	namespace GameScript
	{
		namespace Conditions
		{
			class Pilgrim
			: public Framework::GameScript::ScriptCondition
			{
				typedef Framework::GameScript::ScriptCondition base;
			public: // ScriptCondition
				implement_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);

				implement_ bool check(Framework::GameScript::ScriptExecution const& _execution) const;

			private:
				Optional<Name> holding;

				Optional<Name> isLookingAtObjectVar;
				Optional<float> lookingAtAngle;
				Optional<float> lookingAtRadius;
				Optional<bool> useVRSpaceBasedDoorsCheck; // if true will read position using vr anchors (vr space) to check locations of both objects and will check if the line moved through doors ends where it should
														  // note that this may not work if moving platforms are involved

				bool relativeToEnvironment = false;

				Optional<float> looksUpAngle;
				
				Optional<float> looksYawAt; // where
				Optional<float> looksYawMaxOff; // max off dir
				
				Optional<float> looksPitchAt; // where
				Optional<float> looksPitchMaxOff; // max off dir
				
				Optional<bool> inNavConnectorJunctionRoom; // has at least three doors that are nav connectors

				Optional<VectorInt2> atCell;
				Optional<RangeInt> atCellX;
				Optional<RangeInt> atCellY;
				
				Name inRoomVar;
				TagCondition inRoomTagged;
			};
		}
	}
}
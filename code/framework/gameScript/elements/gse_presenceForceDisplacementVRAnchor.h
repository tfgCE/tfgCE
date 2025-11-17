#pragma once

#include "..\gameScriptElement.h"
#include "..\..\library\usedLibraryStored.h"

namespace Framework
{
	namespace GameScript
	{
		namespace Elements
		{
			class PresenceForceDisplacementVRAnchor
			: public ScriptElement
			{
				typedef ScriptElement base;
			public: // ScriptElement
				implement_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
				implement_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);
				implement_ ScriptExecutionResult::Type execute(ScriptExecution& _execution, ScriptExecution::Flags _flags) const;
				implement_ tchar const* get_debug_info() const { return TXT("presence force displacement vr anchor"); }

			private:
				Name objectVar;

				Optional<float> forceLocX; // world
				Optional<float> forceLocY; // world
				Optional<float> forceLocZ; // world
				Optional<Vector3> forceForwardDir; // world
				Optional<Vector3> forceUpDir; // world

				bool continueUsingObjectVariableForLinearVelocity = false; // means that we continue using this variable that could be used by something else
				Name useObjectVariableForLinearVelocity; // will get the variable from the object and will store it there, it might be much more reliable than using

				struct TimeBasedDisplacement
				{
					bool defined = false;
					Optional<Vector3> startingVelocityXY; // world
					Optional<float> startingVelocityZ; // world
					Optional<Vector3> velocity; // world
					Optional<Name> velocityFromObjectVar; // world
					Optional<Vector3> maintainHorizontalVelocity; // world
					Optional<Name> pullHorizontallyTowardsPOI; // world
					Optional<float> pullHorizontallySpeed;
					Optional<float> pullHorizontallyBlendTime;
					Optional<float> pullHorizontallyCoef;
					Optional<float> linearAcceleration;
					Optional<Rotator3> rotate;
					Optional<float> orientationAcceleration;
					bool ownOrientationAcceleration = false;
					bool ownOrientationAccelerationContinue = false;
				} timeBasedDisplacement;

				struct FollowPath
				{
					struct FoundPOI
					{
						Transform placement;
						float relativePlacement = 0;

						inline static int compare(void const* _a, void const* _b)
						{
							FoundPOI const* a = plain_cast<FoundPOI>(_a);
							FoundPOI const* b = plain_cast<FoundPOI>(_b);
							if (a->relativePlacement < b->relativePlacement) return A_BEFORE_B;
							if (a->relativePlacement > b->relativePlacement) return B_BEFORE_A;
							return A_AS_B;
						}
					};

					struct LocationDescription
					{
						bool defined = false;
						Optional<float> poiPlacementRelativeDistance;
						Optional<Vector3> poiPlacementLocationLS;
						Optional<Vector3> addWS;

						bool load_from_xml(IO::XML::Node const* _node);
						void update_relative_distance_range(REF_ Range& _relativeDistance) const;

						Vector3 process(FoundPOI const* _pois, int _poiCount) const;
					};
					bool defined = false;
					Name poi;
					struct Location
					{
						Optional<float> speed;
						Optional<float> acceleration;
						Optional<float> accelerationAlong;
						LocationDescription towards;
					} location;
					struct Orientation
					{
						Optional<Rotator3> speed;
						Optional<Rotator3> speedCoef;
						Optional<Rotator3> finalDiff;
						LocationDescription relativeTo;
						LocationDescription upTowards;
						LocationDescription forwardTowards;
					} orientation;
				} followPath;

				struct EndCondition
				{
					bool leavesRoom = false;
					Optional<float> afterTime;
					Optional<float> atZBelow;
				} endCondition;
			};
		};
	};
};

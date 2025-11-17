#pragma once

#include "..\aiManagerData.h"

#include "..\..\..\core\tags\tagCondition.h"
#include "..\..\..\core\types\dirFourClockwise.h"

#include "..\..\..\framework\library\libraryName.h"
#include "..\..\..\framework\meshGen\meshGenParam.h"

namespace Framework
{
	class ObjectType;
	class Region;
	class Room;
};

namespace TeaForGodEmperor
{
	namespace AI
	{
		namespace Logics
		{
			namespace Managers
			{
				class BackgroundMover;
			};
		};

		namespace ManagerDatasLib
		{
			class BackgroundMover
			: public ManagerData
			{
				FAST_CAST_DECLARE(BackgroundMover);
				FAST_CAST_BASE(ManagerData);
				FAST_CAST_END();

				typedef ManagerData base;
			public:
				static void register_itself();

			public: // ManagerData
				override_ bool load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc);
				override_ bool prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext);

				override_ void log(LogInfoContext& _log) const;

			protected:
				TagCondition useObjectsTagged; // only objects tagged accordingly will be used

				Transform restPlacement = Transform(Vector3(0.0f, 0.0f, -100.0f), Quat::identity);

				struct Move
				{
					Framework::MeshGenParam<float> speed;
					Framework::MeshGenParam<float> curveRadius; // if set, will work a bit differently (will store placement of each object, move it and offset by curve)
					Optional<float> acceleration;
					Optional<float> deceleration;
					Vector3 dir = Vector3::zero; // where we're moving - background moves in opposite direction
					bool autoStart = false;
					TagCondition backgroundMoverBasePresenceOwnersTagged; // to mark particular objects that we could be standing on them as they are moving (physical sensations etc)
					Optional<float> setRoomVelocityCoef;
				} move;

				struct Doors
				{
					Optional<DirFourClockwise::Type> alongOpenWorldDir; // based on tags on doors (!)
					bool moveDoors = false;
					int order = -1; // order they're sorted (if no alongOpenWorldDir)
					bool noStartingDoor = false;
					Name anchorPOI;
					TagCondition moveWithBackgroundTagged;
					Array<TagCondition> orderByTagged;

					struct PlaceDoorAtPOI
					{
						TagCondition tagged;
						Name atPOI;
					};
					Array<PlaceDoorAtPOI> placeDoorsAtPOI;
				} doors;

				struct ChainElement
				: public RefCountObject
				{
					enum Type
					{
						ChainRoot, // main element

						AtZero,
						AtAppearDistance, // in front
						AtDisappearDistance, // in back
						MoveBy,
						AddAndMove,
						AddAndStay,

						SetAppearDistance, 

						Repeat,
						RepeatTillEnd,

						EndPlacement, // this is where end should be placed

						End, // just stay there

						Idle, // don't do anything, just stay there, allow to move etc

						FIRST_WITH_CHILDREN = Repeat,
						LAST_WITH_CHILDREN = RepeatTillEnd,
					} type = AtZero;
					Optional<int> amount; // for repeat
					bool force = false; // will force to add, ignoring atDistance v appearDistance
					bool endPlacementInCentre = false;
					float value = 0.0f;
					Range valueR = Range::zero;
					struct TransformOffset
					{
						Range x = Range::zero;
						Range y = Range::zero;
						Range z = Range::zero;
						Range pitch = Range::zero;
						Range yaw = Range::zero;
						Range roll = Range::zero;
					} transformOffset;
					TagCondition tagged;
					Array<RefCountObjectPtr<ChainElement>> elements;

					bool load_from_xml(IO::XML::Node const* _node, OUT_ bool & _loadedEndPlacement);
				};

				struct Chain
				{
					float appearDistance = 10.0f;
					float disappearDistance = -10.0f;
					Name anchorPOI;
					RefCountObjectPtr<ChainElement> root;
					bool containsEndPlacement = false;
				} chain;

				struct Sound
				{
					Name play;
					Optional<Name> onSocket;
					TagCondition onTagged;
					Name setSpeedVar;
				};
				Array<Sound> sounds;

				struct SetDistanceCovered
				{
					Name roomVar;
					float mul = 1.0f;
				} setDistanceCovered;

				friend class Logics::Managers::BackgroundMover;
			};
		};
	};
};

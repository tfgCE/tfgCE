#pragma once

#include "aiLogic_npcBase.h"

#include "components\aiLogicComponent_upgradeCanister.h"

#include "..\..\..\framework\ai\aiLogicData.h"
#include "..\..\..\framework\meshGen\meshGenParam.h"
#include "..\..\..\framework\sound\soundSource.h"
#include "..\..\..\framework\text\localisedString.h"

namespace Framework
{
	class Display;
	interface_class IModulesOwner;
};

namespace TeaForGodEmperor
{
	class ModuleBox;

	namespace AI
	{
		namespace Logics
		{
			class H_CraftData;

			struct H_CraftMovementParams
			{
				float yawSpeed = 50.0f;
				float yawAcceleration = 25.0f;

				float linSpeed = 5.0f;
				float linAcceleration = 0.5f;

				float velocityYawToRoll = 0.2f;
				float yAccelToPitch = -2.0f;
				float xAccelToRoll = 2.5f;

				bool load_from_xml(IO::XML::Node const* _node);
			};

			struct H_CraftSetup
			{
				float inTransitEarlyStop = 5.0f;
				Range inTransitOffsetPeriod = Range(0.5f, 2.0f);
				Rotator3 inTransitOffset = Rotator3::zero;
				float inTransitMaxPitch = 5.0f;
				float inTransitApplyPitch = 0.0f;

				float changeToManeuverDistance = 10.0f;
				float changeToTransitDistance = 10.0f;

				H_CraftMovementParams maneuver;
				H_CraftMovementParams transit;

				Name engineSound;
				Name flyingSound;

				bool load_from_xml(IO::XML::Node const* _node);
			};

			class H_Craft
			: public ::Framework::AI::LogicWithLatentTask
			, public ::Framework::IPresenceObserver
			{
				FAST_CAST_DECLARE(H_Craft);
				FAST_CAST_BASE(::Framework::AI::LogicWithLatentTask);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicWithLatentTask base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new H_Craft(_mind, _logicData); }

			public:
				H_Craft(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~H_Craft();

				Transform get_placement() const;
				Vector3 const& get_velocity_linear() const { return velocityLinear; }
				Rotator3 const& get_velocity_rotation() const { return velocityRotation; }

			public: // AILogic
				override_ void advance(float _deltaTime);

			public: // Framework::IPresenceObserver
				implement_ void on_presence_changed_room(Framework::ModulePresence* _presence, Framework::Room* _intoRoom, Transform const& _intoRoomTransform, Framework::DoorInRoom* _exitThrough, Framework::DoorInRoom* _enterThrough, Framework::DoorInRoomArrayPtr const* _throughDoors);
				implement_ void on_presence_destroy(Framework::ModulePresence* _presence);

			protected:
				H_CraftData const * h_craftData = nullptr;

				Random::Generator rg;

				// setup
				H_CraftSetup setup;

				bool craftOff = false;

				// operating modes:
				//	[m] aneuver
				//	[t] ransit
				bool inTransit = false;

				Rotator3 inTransitOffset = Rotator3::zero;
				float inTransitOffsetTimeLeft = 0.0f;

				// runtime

				Framework::ModulePresence* observingPresence = nullptr;

				bool dying = false;
				float dyingTime = 0.0f;
				float dyingTimeLeft = 0.0f;
				float dyingFullAffectVelocityLinearTime = 1.0f;
				float dyingFullAffectVelocityRotationTime = 1.0f;
				float dyingAcceleration = 0.0f;
				Rotator3 dyingRotation = Rotator3::zero;

				float enginePower = 0.0f;
				SimpleVariableInfo outputEnginePowerVar;
				SimpleVariableInfo outputSpeedVar;

				Optional<Vector3> location;
				Optional<Rotator3> rotation;

				Rotator3 rotationOffsetFromLinear = Rotator3::zero;
				Rotator3 rotationOffsetFromRotation = Rotator3::zero;

				Vector3 velocityLinear = Vector3::zero;
				Rotator3 velocityRotation = Rotator3::zero;

				bool beSilent = false;
				
				bool restartFlyingEngineSounds = false;

				bool instantFacing = false;
				Optional<float> instantManeuverSpeed;
				Optional<float> instantTransitSpeed;
				
				bool instantFollowing = false;
				
				bool devTeleport = false;
				bool devTeleportEverywhere = false;

				RefCountObjectPtr<Framework::SoundSource> engineSound;
				RefCountObjectPtr<Framework::SoundSource> flyingSound;

				// request

				bool blockExtraRotation = false;
				float extraRotationBlocked = 0.0f;

				bool noLocationGoForward = false; // if no match location or offset or any other thing, just go forward if this is set
				Optional<float> matchYaw;
				Optional<float> matchPitch; // used only with go forward
				Optional<Vector3> matchLocation;
				Optional<float> forcedYawSpeed; // rotate it this speed
				Optional<float> yawSpeed; // override
				Optional<float> yawAcceleration; // override
				Optional<float> pitchAcceleration; // override
				SafePtr<Framework::IModulesOwner> matchToIMO;
				Optional<float> limitFollowRelVelocity; // if we move relatively, limit velocity to be max this
				Optional<Vector3> followRelVelocity; // if set, will move along the velocity at a constant speed
				Optional<Vector3> followOffset;
				Optional<Vector3> followOffsetAdditional;
				Optional<Range3> followOffsetAdditionalRange;
				Optional<Range> followOffsetAdditionalInterval;
				float followOffsetAdditionalIntervalWhole = 0.0f;
				float followOffsetAdditionalIntervalLeft = 0.0f;
				SafePtr<Framework::IModulesOwner> followIMO;
				Optional<bool> followIMOInTransit;
				Optional<Name> followPOIs;
				Optional<float> followPOIsRelativeDistance;
				bool followIMOIgnoreForward = false; // will ignore forward of follow placement, always assuming it is direction from us to target

				struct FollowPOIPath
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

					static Vector3 process(FoundPOI const* _pois, int _poiCount, float _relativeDistance);
				};

				bool ignoreYawMatchForTrigger = false;

				Name matchSocket;
				Name matchToIMOSocket;
				Optional<Vector3> targetSocketOffset;
				Optional<float> targetSocketOffsetYaw;

				Optional<float> requestedManeuverSpeed; // exact
				Optional<float> requestedManeuverSpeedPt;
				Optional<float> requestedManeuverAccelPt;
				Optional<float> requestedTransitSpeed; // exact
				Optional<float> requestedTransitSpeedPt;
				Optional<float> requestedTransitAccelPt;
				bool transitThrough = false;
				bool maneuverThrough = false;

				Name triggerGameScriptTrap;
				Name triggerOwnGameScriptTrap;
				Optional<float> triggerBelowDistance;

				static LATENT_FUNCTION(execute_logic);

				void reset_requests(bool _keepFollowOffsetAdditional = false);

				SimpleVariableInfo displayMessageVar;
				SimpleVariableInfo displayActiveVar;

				bool displayActive = false;
				int displayMessageVarValue = NONE;
				bool redrawNow = false;
				float updateDisplayTime = 0.0f;

				bool useCanister = true;
				UpgradeCanister canister;

				::Framework::Display* display = nullptr;

				float dSpeed = 0.0f;
				float dHeading = 0.0f;
				float dAltitude = 0.0f;
				float dBattery = 47.0f;
				float dPower = 0.0f;
				float dId = 0.0f;

				void observe_presence(Framework::ModulePresence* _presence);
				void stop_observing_presence();
			};

			class H_CraftData
			: public ::Framework::AI::LogicData
			{
				FAST_CAST_DECLARE(H_CraftData);
				FAST_CAST_BASE(::Framework::AI::LogicData);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicData base;
			public:
				static ::Framework::AI::LogicData* create_ai_logic_data() { return new H_CraftData(); }

			public: // LogicData
				override_ bool load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc);
				override_ bool prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext);

			protected:

				Name outputEnginePowerVar;
				Name outputSpeedVar;
				float minEnginePower = 0.0f;
				float useRotationForEnginePower = 0.0f;
				float useLinearAccelerationForEnginePower = 0.3f;
				float useRotationAccelerationForEnginePower = 0.0f;
				float enginePowerBlendTime = 0.3f;

				// setup
				H_CraftSetup setup;

				// overrides for sounds
				Framework::MeshGenParam<Name> engineSound;
				Framework::MeshGenParam<Name> flyingSound;

				Name displayMessageVar;
				Name displayActiveVar;
				Framework::LocalisedString layout;
				RangeInt2 messagesAt = RangeInt2::empty;
				struct Message
				{
					int varValue = NONE;
					Framework::LocalisedString message;
				};
				Array<Message> messages;

				friend class H_Craft;
			};

		};
	};
};
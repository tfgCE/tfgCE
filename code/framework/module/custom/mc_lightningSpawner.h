#pragma once

#include "..\moduleCustom.h"
#include "..\..\presence\relativeToPresencePlacement.h"
#include "..\..\sound\soundSource.h"

#include "..\..\..\core\functionParamsStruct.h"
#include "..\..\..\core\vr\iVR.h"
#include "..\..\..\core\types\hand.h"

namespace Framework
{
	namespace CustomModules
	{
		class LightningSpawnerData;

		namespace Lightning
		{
			enum Type
			{
				SocketArc, // arc between two sockets
				Strike,	// a simple strike - just straight lightning in specified direction
				Presence, // presence sphere
			};

			inline Type parse(String const & _value)
			{
				if (_value == TXT("socket arc")) return SocketArc;
				if (_value == TXT("strike")) return Strike;
				if (_value == TXT("presence")) return Presence;
				error(TXT("lightning type \'%S\" not recognised"), _value.to_char());
				return Strike;
			}

		};

		struct LightningData
		{
			Name id;
			int count = 1;
			Lightning::Type type = Lightning::Strike;
			ArrayStatic<Name, 8> temporaryObjects; // from TemporaryObjectModule
			ArrayStatic<Name, 8> sounds;
			ArrayStatic<Name, 8> lightSources;

			Name arcVarID;
			Name lengthVarID;
			Name radiusVarID;

			// life time params
			bool defaultOn = false; // if set, will automatically start spawning lightnings
			bool autoStop = true; // stop/kill: when spawning, when stopping
			bool startImmediately = false; // start immediately means there is no delay when triggered to start
			Range interval = Range::empty; // start-to-start, if not specified, is singular, required to be issued directly
			Range timeToLive = Range::empty; // if not specified, will depend on timeToLive of a temporary object
			Range timeToLiveMiss = Range::empty; // when not hit
			bool allowThroughDoors = true; // if disallowed, won't end up in another room, will be discarded
			bool allowMiss = true; // if disallowed, won't spawn temporary objects

			// placement and physical properties
			// strike / socket arc
			Range length = Range(1.0f);
			Range arc = Range(0.0f);
			Range arcSpeed = Range(0.0f);
			Collision::Flags hitFlags = Collision::Flags::none(); // if not none, will try to hit something
			Range dirAngle = Range(0.0f); // if dir is non zero, may use angle to strike in specific cone
			bool continuousPlacementUpdate = true; // for hits!
			float impulseOnHit = 0.0f; // if hitting something with a lightning strike should add impulse to movement
			bool impulesOnHitObjectsOnly = false; // if set, impules is applied only if hit actual objects
			float impulseNominalDistance = 0.0f; // full at this distance or beyond
			bool useTarget = false;
			float useTargetRadius = 0.0f;
			Range extraHitPenetration = Range::empty; // when hit something
			float extraHitRadius = 0.0f;

			// presence
			Range radiusRelativeToPresence = Range(1.0f);

			VR::Pulse vrPulse;

			// general
			bool exclusiveSockets = false; // no socket will be used in two lightnings
			bool exclusiveSocketPairs = false; // no socket pair will be used twice (they may share sockets though)
			bool crawlSocketPairs = false; // will do one pair after another
			struct AtSocket
			{
				Name socket;
				Optional<Axis::Type> upAxis;
				float upAxisDir = 1.0f;
				Optional<Hand::Type> vrPulseHand; // has to be set to play
			};
			Array<AtSocket> atSocket;
			struct SocketPair
			{
				Name a;
				Name b;
				Name altB; // altB is chosen if b is not available, this way it is possible to either have left/right sockets or daisy-chain them on one side (if there's no other side)
			};
			Array<SocketPair> socketPairs;
			Transform placement = Transform::identity;

			LightningData()
			{
				SET_EXTRA_DEBUG_INFO(temporaryObjects, TXT("LightningData.temporaryObjects"));
				SET_EXTRA_DEBUG_INFO(sounds, TXT("LightningData.sounds"));
				SET_EXTRA_DEBUG_INFO(lightSources, TXT("LightningData.lightSources"));
			}

			bool load_from_xml(IO::XML::Node const * _node);
		};

		class LightningSpawner
		: public ModuleCustom
		{
			FAST_CAST_DECLARE(LightningSpawner);
			FAST_CAST_BASE(ModuleCustom);
			FAST_CAST_END();

			typedef Framework::ModuleCustom base;
		public:
			static Framework::RegisteredModule<Framework::ModuleCustom> & register_itself();

			LightningSpawner(Framework::IModulesOwner* _owner);
			virtual ~LightningSpawner();

			struct LightningParams
			{
				ADD_FUNCTION_PARAM(LightningParams, int, count, with_count);
				ADD_FUNCTION_PARAM(LightningParams, Transform, startPlacementOS, with_start_placement_os); // to force placement if we don't really care about the socket or don't know it or for any other reason
				ADD_FUNCTION_PARAM(LightningParams, Transform, startPlacementWS, with_start_placement_ws); // to force placement if we don't really care about the socket or don't know it or for any other reason
				ADD_FUNCTION_PARAM_PLAIN_INITIAL_DEF(LightningParams, bool, isForced, forced, false, true); // to use forced target (won't do any hit checks), has to be used with target or length
				ADD_FUNCTION_PARAM(LightningParams, float, length, with_length);
				
				RelativeToPresencePlacement target;
				bool isDirectTarget = false;
				
				LightningParams& with_target(RelativeToPresencePlacement const& _target, bool _isDirectTarget) { target = _target; isDirectTarget = _isDirectTarget; return *this; }

				LightningParams(): isDirectTarget(false) {}
			};
			// methods are async, they will just issue whatever has to be done and it will be done in advance_post
			void start(Name const & _id, LightningParams const & _params = LightningParams()); // stops all existing
			void add(Name const & _id, LightningParams const& _params = LightningParams()); // adds one more
			void stop(Name const & _id);
			void single(Name const & _id, LightningParams const& _params = LightningParams());

		public:	// Module
			override_ void reset();
			override_ void setup_with(::Framework::ModuleData const * _moduleData);

			override_ void initialise();
			override_ void activate();

			override_ void on_owner_destroy();

		protected: // ModuleCustom
			override_ void advance_post(float _deltaTime);

			override_ void on_placed_in_world();

		protected:
			LightningSpawnerData const * lightningSpawnerData = nullptr;

			Random::Generator rg;

			struct Lightning
			{
				bool introduced = false;
				Name id;
				bool single = false; // if not single, continuous, will use time to spawn etc
				bool spawnPending = false;
				bool shouldStop = false; // will stop and remove
				bool shouldKill = false; // kill to
				Optional<float> timeToLive;
				LightningData const * data = nullptr;
				::SafePtr<Framework::IModulesOwner> to;
				RefCountObjectPtr<SoundSource> sound;
				float arc = 0.0f;
				float arcSpeed = 0.0f;
				int crawlSocketPairIdx = NONE;
				int socketAIdx = NONE;
				int socketBIdx = NONE;
				Optional<Axis::Type> socketAUpAxis;
				float socketAUpAxisDir = 1.0f;
				Optional<Hand::Type> vrPulseHand;
				float timeToSpawn = 0.0f;
				RelativeToPresencePlacement hitPlacement;
				float extraHitPenetration = 0.0f;

				// used when spawning, cleared afterwards
				RelativeToPresencePlacement target;
				bool isDirectTarget = false; // if not direct placement, it will use AI to place properly
				bool isForced = false;
				Optional<float> forcedLength;
				Optional<Transform> startPlacementOS;
				Optional<Transform> startPlacementWS;
			};
			Array<Lightning> lightnings;
			Array<Name> activeLightSources;
			bool inactive = true;
			struct Requests
			{
				Concurrency::SpinLock lock = Concurrency::SpinLock(TXT("Framework.CustomModules.LightningSpawner.requests.lock"));
				struct Request
				{
					enum Op
					{
						Add,
						Stop,
						Single
					};
					Op op;
					Name id;
					LightningParams params;
					Request() {}
					Request(Op _op, Name const& _id, LightningParams const & _params = LightningParams()) : op(_op), id(_id), params(_params) {}
				};
				Array<Request> requests;
			} requests;
			Array<int> socketsInUse;
			bool collectSocketsInUse = false; // if any data has exclusivity set
			struct ExSocketPair
			{
				int a, b;
				ExSocketPair() {}
				ExSocketPair(int _a, int _b) : a(_a), b(_b) {}
				bool operator==(ExSocketPair const & _other) const { return (a == _other.a && b == _other.b) || (a == _other.b && b == _other.a); }
			};
			Array<ExSocketPair> socketPairsInUse;
			bool collectSocketPairsInUse = false; // if any data has exclusivity set

			bool defaultsStarted = false;

			void clean_up_lightning(Lightning & l);
			void introduce_lightning(Lightning & l);

			void calculate_socket_arc_placement(Lightning const & l, OUT_ float & length, OUT_ Transform & placement) const;
			void calculate_socket_arc_relative_placement(Lightning const & l, OUT_ float & length, OUT_ Transform & relativePlacement) const;

			void start_defaults();

			void internal_add(Name const& _id, LightningParams const & _params); // adds one more
			void internal_stop(Name const& _id);
			void internal_single(Name const& _id, LightningParams const& _params);

		};

		class LightningSpawnerData
		: public ModuleData
		{
			FAST_CAST_DECLARE(LightningSpawnerData);
			FAST_CAST_BASE(ModuleData);
			FAST_CAST_END();
			typedef ModuleData base;
		public:
			LightningSpawnerData(LibraryStored* _inLibraryStored);

		public: // ModuleData
			override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
			override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

		protected: // ModuleData
			override_ bool read_parameter_from(IO::XML::Attribute const * _attr, LibraryLoadingContext & _lc);
			override_ bool read_parameter_from(IO::XML::Node const * _node, LibraryLoadingContext & _lc);

		protected:	friend class LightningSpawner;
			Array<LightningData> lightnings;
		};
	};
};

DECLARE_REGISTERED_TYPE(Framework::CustomModules::LightningSpawner*);
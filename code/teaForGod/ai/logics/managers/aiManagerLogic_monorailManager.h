#pragma once

#include "..\..\..\..\core\tags\tag.h"
#include "..\..\..\..\core\wheresMyPoint\wmpParam.h"

#include "..\..\..\..\framework\ai\aiLogicData.h"
#include "..\..\..\..\framework\ai\aiLogicWithLatentTask.h"
#include "..\..\..\..\framework\game\delayedObjectCreation.h"
#include "..\..\..\..\framework\library\usedLibraryStored.h"
#include "..\..\..\..\framework\sound\soundSource.h"
#include "..\..\..\..\framework\world\worldSettingCondition.h"

#ifdef AN_ALLOW_EXTENSIVE_LOGS
//#define LOG_MONORAIL_MANAGER
#endif

namespace Framework
{
	class ObjectType;
	class Region;
	struct PointOfInterestInstance;
};

namespace TeaForGodEmperor
{
	namespace AI
	{
		namespace Logics
		{
			namespace Managers
			{
				class MonorailManagerData;

				/**
				 */
				class MonorailManager
				: public ::Framework::AI::LogicWithLatentTask
				{
					FAST_CAST_DECLARE(MonorailManager);
					FAST_CAST_BASE(::Framework::AI::LogicWithLatentTask);
					FAST_CAST_END();

					typedef ::Framework::AI::LogicWithLatentTask base;
				public:
					static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new MonorailManager(_mind, _logicData); }

				public:
					MonorailManager(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
					virtual ~MonorailManager();

					Framework::Room* get_for_room() const { return inRoom.get(); }

				public: // Logic
					override_ void advance(float _deltaTime);
					override_ void ready_to_activate();

				private:
					static LATENT_FUNCTION(execute_logic);

				private:
					MonorailManagerData const * monorailManagerData = nullptr;

					Random::Generator random;
					bool started = false;

					SafePtr<Framework::Room> inRoom;

					Array<RefCountObjectPtr<Framework::DelayedObjectCreation>> cartDOCs;

					Framework::SoundSourcePtr sound;
					bool soundAlreadyAtCentre = false;
					float soundAt = -1.0f;

					struct Cart
					{
						SafePtr<Framework::IModulesOwner> cart;
						float length;
						Vector3 connectorForwardOS = Vector3::zero;
						Vector3 connectorBackOS = Vector3::zero;
						Optional<int> id; // id on the track (the one that is currently passing end point is always zero, none if outside track
					};

					Array<Cart> carts;
					bool spawningCarts = false;
					float totalCartLength = 0.0f;

					struct CartType
					{
						Array< Framework::LibraryStored*> types;
						float probCoef = 1.0f;
					};
					Array<CartType> cartTypes;

#ifdef LOG_MONORAIL_MANAGER
					float reportManagerTime = 0.0f;
#endif

					struct Track
					{
						Segment trackWS; // start to end
						Vector3 startWS;
						Vector3 endWS;
						Vector3 safeWS;
						float length = 0.0f;
					};
					Track track;

					Transform prepare_placement(Cart const& _for, Vector3 const& _forwardConnectorShouldBeAtWS) const;

					void find_track();
					void find_cart_types();

					Framework::DelayedObjectCreation* create_cart_doc(Framework::IModulesOwner* imo);
					void handle_cart_doc(Framework::DelayedObjectCreation* doc);
				};

				class MonorailManagerData
				: public ::Framework::AI::LogicData
				{
					FAST_CAST_DECLARE(MonorailManagerData);
					FAST_CAST_BASE(::Framework::AI::LogicData);
					FAST_CAST_END();

					typedef ::Framework::AI::LogicData base;
				public:
					static ::Framework::AI::LogicData* create_ai_logic_data() { return new MonorailManagerData(); }

					MonorailManagerData();
					virtual ~MonorailManagerData();

				public: // LogicData
					virtual bool load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc);
					virtual bool prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext);

				private:
					bool allowSpawnImmediatelyWhenRoomVisible = false;
					float swapEndsChance = 0.0f;

					float extraAmountPt = 0.4f; // how much extra carts above track length do we need

					Framework::UsedLibraryStored<Framework::Sample> sound;

					struct Carts
					{
						struct Type
						{
							Framework::WorldSettingCondition cartType;
							float probCoef = 1.0f;
						};
						Array<Type> types;
						Name connectorForwardSocket;
						Name connectorBackSocket;
					};
					Carts carts;

					struct Track
					{
						Name startPOI;
						Name endPOI;
						float speed = 1.0f;
						Name safePOI; // where new carts are placed
					};
					Track track; // one for time being

					friend class MonorailManager;
				};

				//

			};
		};
	};
};
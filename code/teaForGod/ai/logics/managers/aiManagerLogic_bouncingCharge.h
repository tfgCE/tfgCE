#pragma once

#include "..\..\..\..\framework\ai\aiLogicData.h"
#include "..\..\..\..\framework\ai\aiLogicWithLatentTask.h"
#include "..\..\..\..\framework\game\delayedObjectCreation.h"
#include "..\..\..\..\framework\object\actorType.h"
#include "..\..\..\..\framework\library\usedLibraryStored.h"

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
				class BouncingChargeManagerData;

				/**
				 */
				class BouncingChargeManager
				: public ::Framework::AI::LogicWithLatentTask
				{
					FAST_CAST_DECLARE(BouncingChargeManager);
					FAST_CAST_BASE(::Framework::AI::LogicWithLatentTask);
					FAST_CAST_END();

					typedef ::Framework::AI::LogicWithLatentTask base;
				public:
					static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new BouncingChargeManager(_mind, _logicData); }

				public:
					BouncingChargeManager(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
					virtual ~BouncingChargeManager();

					Framework::Room* get_for_room() const { return inRoom.get(); }

				public: // Logic
					override_ void advance(float _deltaTime);

					override_ void debug_draw(Framework::Room* _room) const;

				private:
					static LATENT_FUNCTION(execute_logic);

				private:
					BouncingChargeManagerData const * bouncingChargeManagerData = nullptr;

					bool okToAdvance = false;

					Random::Generator random;

					SafePtr<Framework::Room> inRoom;
					SafePtr<Framework::IModulesOwner> charge;
					SafePtr<Framework::PointOfInterestInstance> poiA;
					SafePtr<Framework::PointOfInterestInstance> poiB;

					float lastSafeT = 0.0f; // A
					float timeAtStop = 0.0f;
					bool isMoving = false;

					RefCountObjectPtr<Framework::DelayedObjectCreation> chargeDOC;
				};

				class BouncingChargeManagerData
				: public ::Framework::AI::LogicData
				{
					FAST_CAST_DECLARE(BouncingChargeManagerData);
					FAST_CAST_BASE(::Framework::AI::LogicData);
					FAST_CAST_END();

					typedef ::Framework::AI::LogicData base;
				public:
					static ::Framework::AI::LogicData* create_ai_logic_data() { return new BouncingChargeManagerData(); }

					BouncingChargeManagerData();
					virtual ~BouncingChargeManagerData();

				public: // LogicData
					virtual bool load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc);
					virtual bool prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext);

				private:
					Framework::UsedLibraryStored<Framework::ActorType> chargeActorType;
					Name poiA;
					Name poiB;
					float timeAtStop = 2.0f;
					float timeRunMin = 0.5f;
					float velocityBase = 2.0f;
					float velocityMax = 2.0f;

					friend class BouncingChargeManager;
				};

				//

			};
		};
	};
};
#pragma once

#include "..\..\..\game\energy.h"

#include "..\..\..\..\framework\ai\aiLogicData.h"
#include "..\..\..\..\framework\ai\aiLogicWithLatentTask.h"
#include "..\..\..\..\framework\game\delayedObjectCreation.h"
#include "..\..\..\..\framework\object\sceneryType.h"
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
				class DischargeZoneManagerData;

				/**
				 */
				class DischargeZoneManager
				: public ::Framework::AI::LogicWithLatentTask
				{
					FAST_CAST_DECLARE(DischargeZoneManager);
					FAST_CAST_BASE(::Framework::AI::LogicWithLatentTask);
					FAST_CAST_END();

					typedef ::Framework::AI::LogicWithLatentTask base;
				public:
					static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new DischargeZoneManager(_mind, _logicData); }

				public:
					DischargeZoneManager(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
					virtual ~DischargeZoneManager();

					Framework::Room* get_for_room() const { return inRoom.get(); }

				public: // Logic
					override_ void advance(float _deltaTime);

					override_ void debug_draw(Framework::Room* _room) const;

				private:
					static LATENT_FUNCTION(execute_logic);

				private:
					DischargeZoneManagerData const * dischargeZoneManagerData = nullptr;

					bool okToAdvance = false;

					Random::Generator random;

					SafePtr<Framework::Room> inRoom;
					SafePtr<Framework::IModulesOwner> dischargeZoneDischarger;
					Array<Transform> poiPlacements;

					bool isIdling = true;
					float idleIntervalTimeLeft = 0.0f;
					float toDischargeTimeLeft = 0.0f;
					float preparationTimeLeft = 0.0f;
					int dischargesLeft = 0;

					RefCountObjectPtr<Framework::DelayedObjectCreation> dzdDOC;
				};

				class DischargeZoneManagerData
				: public ::Framework::AI::LogicData
				{
					FAST_CAST_DECLARE(DischargeZoneManagerData);
					FAST_CAST_BASE(::Framework::AI::LogicData);
					FAST_CAST_END();

					typedef ::Framework::AI::LogicData base;
				public:
					static ::Framework::AI::LogicData* create_ai_logic_data() { return new DischargeZoneManagerData(); }

					DischargeZoneManagerData();
					virtual ~DischargeZoneManagerData();

				public: // LogicData
					virtual bool load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc);
					virtual bool prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext);

				private:
					Framework::UsedLibraryStored<Framework::SceneryType> dischargeSceneryType;
					Name poiName;
					Range idleInterval = Range(0.0f, 1.0f);
					float idleDischargeChance = 0.0f;

					Range dischargeInterval = Range(1.0f, 10.0f);
					Name preparationSoundID;
					float preparationTime = 3.0f;
					Energy multipleDischargeDamage = Energy(3);
					Energy idleDischargeDamage = Energy(10);
						
					RangeInt dischargesAmount = RangeInt(10);
					int dischargesBatch = 5;

					friend class DischargeZoneManager;
				};

				//

			};
		};
	};
};
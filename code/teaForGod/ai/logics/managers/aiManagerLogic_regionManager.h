#pragma once

#include "aiManagerLogic.h"

#include "..\..\..\game\energy.h"
#include "..\..\..\..\framework\world\worldAddress.h"

namespace Framework
{
	class Region;
};

namespace TeaForGodEmperor
{
	namespace AI
	{
		namespace Logics
		{
			namespace Managers
			{
				class RegionManager;
				class RegionManagerData;

				/**
				 *	Provides a variety of functionalities and features that span over a region
				 *	
				 *		1.	spawn control
				 *			contains pools that tell how many spawn value (SV) is available
				 *			each npc has a spawn value associated (if none, 1 is assumed)
				 *			to simplify things, if any amount is available, enemy can be spawned, available amount may end up being negative then
				 *	
				 */
				class RegionManager
				: public AIManager
				, public SafeObject<RegionManager>
				{
					FAST_CAST_DECLARE(RegionManager);
					FAST_CAST_BASE(AIManager);
					FAST_CAST_END();

					typedef AIManager base;
				public:
					static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new RegionManager(_mind, _logicData); }

				public:
					RegionManager(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
					virtual ~RegionManager();

					Framework::Region* get_for_region() const { return inRegion.get(); }

					static RegionManager* find_for(Framework::Room* r);
					static RegionManager* find_for(Framework::Region* r);

					static Energy get_value_for(Framework::IModulesOwner* imo);

				public:
					int get_altered_concurrent_limit(Name const& _pool, int _concurrentLimit) const;
					float get_concurrent_limit_coef(Name const& _pool) const;
					bool is_pool_active(Name const& _pool) const;
					bool is_pool_hostile(Name const& _pool) const;
					Energy get_available_for_pool(Name const& _pool) const; // only if active
					Energy get_pool_value(Name const& _pool) const; // even if not active
					Energy get_lost_pool_value(Name const& _pool) const;
					void consume_for_pool(Name const& _pool, Energy const& _combatValue);
					void restore_for_pool(Name const& _pool, Energy const& _combatValue); // give back/add
					void alter_limit_for_pool(Name const& _pool, int& _limit) const; // used only by dynamic

				public: // Logic
					override_ void advance(float _deltaTime);

					override_ void log_logic_info(LogInfoContext& _log) const;

				private:
					static LATENT_FUNCTION(execute_logic);

				private:
					RegionManagerData const * regionManagerData = nullptr;

					Random::Generator random;

					::SafePtr<Framework::Region> inRegion;

					bool started = false;

				private:
					bool hostileSpawnsBlockedByGameDirector = false;

					// pools (values are kept as they are, the values outside are affected by spawnCountModifier)
					struct Pool
					{
						Name name;
						bool active = true;
						bool hostile = false;
						bool infinite = false;
						Optional<float> concurrentLimitCoef;
						Energy available;
						Energy lost; // how many/much has already been lost
						Energy limit;
						Energy limitFull;
						Energy lowerLimitWhenDeactivated;
						Energy regenPerMinute;
						Energy regenPerMinuteInactive;
						float regenMissingBit;
						Energy stopAt;
						Energy minToRestart; // if limit gets below minToRestart, we won't be able to start another wave
						bool noRestart = false;
					};
					mutable Concurrency::MRSWLock poolLock;
					Array<Pool> pools;

				private:
					void init();
				};

				class RegionManagerData
				: public ::Framework::AI::LogicData
				{
					FAST_CAST_DECLARE(RegionManagerData);
					FAST_CAST_BASE(::Framework::AI::LogicData);
					FAST_CAST_END();

					typedef ::Framework::AI::LogicData base;
				public:
					static ::Framework::AI::LogicData* create_ai_logic_data() { return new RegionManagerData(); }

					RegionManagerData();
					virtual ~RegionManagerData();

				public: // LogicData
					virtual bool load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc);
					virtual bool prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext);

				private:
					struct Pool
					{
						Name name;
						bool hostile = false;
						bool infinite = false;
						Optional<float> concurrentLimitCoef;
						EnergyRange amount = EnergyRange(Energy(20.0f));
						Optional<Energy> lowestAmountForCoefs; // when we apply coefs, we can't get lower than this
						EnergyRange lowerAmountWhenDeactivated = EnergyRange(Energy(0.5f)); // zero - never lower, infinite; lowers amount when gets deactivated by this value
						EnergyRange regenPerMinute = EnergyRange(Energy(2.0f));
						EnergyRange regenPerMinuteInactive = EnergyRange(Energy(2.0f));
						EnergyRange stopAt = EnergyRange(Energy(0.0f));
						EnergyRange minToRestart = EnergyRange(Energy(5.0f));
						bool noRestart = false;
					};
					Array<Pool> pools;

					friend class RegionManager;
				};
			};
		};
	};
};
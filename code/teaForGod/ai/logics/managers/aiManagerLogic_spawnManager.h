#pragma once

#include "aiManagerLogic.h"

#include "..\..\..\game\energy.h"

#include "..\..\..\..\core\tags\tag.h"
#include "..\..\..\..\core\tags\tagCondition.h"
#include "..\..\..\..\core\wheresMyPoint\wmpParam.h"

#include "..\..\..\..\framework\library\usedLibraryStored.h"
#include "..\..\..\..\framework\world\worldAddress.h"

#define SPAWN_MANAGER__USE_ARRAYS_INSTEAD_OF_STACK_ARRAYS

#ifdef AN_DEVELOPMENT_OR_PROFILER
#define AN_SHOW_SPAWN_MANGER_DETAILS_INFO
#endif

namespace Framework
{
	class DoorInRoom;
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
				/**
				 *	There are two ways of using SpawnManager (can be mixed, then they work separately, only taking blocking spawn points into consideration
				 *		WAVE
				 *			each wave has elements of a different priority. only if the highest priority is done, following may go
				 *		DYNAMIC
				 *			constantly spawns and removes objects
				 *			if a dynamic spawn with a higher priority can't be done/spawned, it may allow others to spawn (this is the default behaviour)
				 */
				class RegionManager;
				class SpawnManager;
				class SpawnManagerData;

				struct SpawnChoosePOI;
				struct SpawnPlacementCriteria;
				struct SpawnRegionPoolConditions;

				struct SpawnManagerWaveElement;
				struct SpawnManagerWaveElementInstance;
				struct SpawnManagerWave;
				struct SpawnManagerWaveInstance;
				//
				struct DynamicSpawnInfo;
				struct DynamicSpawnInfoInstance;

				typedef RefCountObjectPtr<SpawnManagerWaveElement> SpawnManagerWaveElementPtr;
				typedef RefCountObjectPtr<SpawnManagerWave> SpawnManagerWavePtr;
				//
				typedef RefCountObjectPtr<DynamicSpawnInfo> DynamicSpawnInfoPtr;

				struct GlobalSpawnManagerInfo
				{
				public:
					static void initialise_static();
					static void close_static();

					static GlobalSpawnManagerInfo* get() { return s_globalSpawnManagerInfo; }

					static void reset();
					static int get(Name const& _id);
					static void increase(Name const& _id);
					static void decrease(Name const& _id);

				private:
					static GlobalSpawnManagerInfo* s_globalSpawnManagerInfo;
					struct Entry
					{
						Name id; // globalConcurrentCountId
						Concurrency::Counter count;
					};
					Concurrency::SpinLock entriesLock; // only if adding a new entry
					ArrayStatic<Entry, 16> entries;
					
					GlobalSpawnManagerInfo();
					~GlobalSpawnManagerInfo();
				};

				struct SpawnManagerPOIDefinition
				{
					Name name;
					RefCountObjectPtr<TagCondition> tagged;
					RefCountObjectPtr<TagCondition> inRoomTagged; // for POI
					int beyondDoorDepth = 1; // only this one (this works only for corridors)
					bool doorsAppearLockedIfNotOpen = false; // this is useful for far away spawn managers as it indicates "this is not the way, just a spawn room"
					RefCountObjectPtr<TagCondition> beyondDoorTagged; // works only for spawn manager in room, this will make check rooms behind doors (in this room) for POIs
					RefCountObjectPtr<TagCondition> beyondDoorInRoomTagged; // works only for spawn manager in room, this will make check rooms behind doors (in this room) for POIs

					SpawnManagerPOIDefinition();
					bool load_from_xml(IO::XML::Node const * _node);
				};

				namespace SpawnUtils
				{
					Framework::ObjectType* get_replacement_easier(Framework::ObjectType* _for);
				};

				/** object to spawn and where to spawn */
				struct SpawnManagerSpawnInfo
				{
					SafePtr<Framework::PointOfInterestInstance> atPOI;
					int poiIdx = NONE;

					bool hostile = false;

					RefCountObjectPtr<SpawnManagerWaveInstance> waveInstance;
					RefCountObjectPtr<SpawnManagerWaveElementInstance> waveElementInstance;

					RefCountObjectPtr<DynamicSpawnInfoInstance> dynamicInstance;

					Framework::ObjectType* get_type_to_spawn(SpawnManager* _spawnManager, OUT_ int& _count, OUT_ Vector3& _spawnOffset) const; // to spawn
					Framework::ObjectType* get_travel_with_type(SpawnManager* _spawnManager) const;
					void add_extra_being_spawned() const;
					void restore_to_spawn() const;
					void on_spawned(SpawnManager * _spawnManager, Framework::IModulesOwner* _spawned) const;

					void setup_variables(SimpleVariableStorage& _variables) const;
				};

				//

				struct SpawnChoosePOI
				{
					Optional<float> separationDistance; // the higher, the better
					float match = 0.0f;
					int idx = NONE;
					Framework::PointOfInterestInstance* poi = nullptr;
					Vector3 location;
					SpawnChoosePOI() {}
					SpawnChoosePOI(float _match, int _idx, Framework::PointOfInterestInstance* _poi, Vector3 const& _location) : match(_match), idx(_idx), poi(_poi), location(_location) {}

					static int compare_separtion_distance(void const* _a, void const* _b)
					{
						SpawnChoosePOI const* a = plain_cast<SpawnChoosePOI>(_a);
						SpawnChoosePOI const* b = plain_cast<SpawnChoosePOI>(_b);
						if (a->separationDistance.get(-1.0f) > b->separationDistance.get(-1.0f))
						{
							return A_BEFORE_B;
						}
						if (a->separationDistance.get(-1.0f) < b->separationDistance.get(-1.0f))
						{
							return B_BEFORE_A;
						}
						return A_AS_B;
					}

					static int compare(void const* _a, void const* _b)
					{
						SpawnChoosePOI const* a = plain_cast<SpawnChoosePOI>(_a);
						SpawnChoosePOI const* b = plain_cast<SpawnChoosePOI>(_b);
						if (a->match > b->match)
						{
							return A_BEFORE_B;
						}
						if (a->match < b->match)
						{
							return B_BEFORE_A;
						}
						return A_AS_B;
					}
				};

				struct SpawnPlacementCriteria
				{
					Optional<Range> atYaw; // if player is in the same room, choose by yaw (world space) this is an absolute criteria - if failed, won't be processed
					Optional<Range> atDistance; // distance from a player (any, most likely the closest (when it comes to room distance) - if failed, won't be processed
					Optional<Range> preferAtDistance; // distance from a player (any, most likely the closest (when it comes to room distance) - prefer in that distance
					Optional<Range> atDoorDistance; // distance from the closest door - if failed, won't be processed
					Optional<float> distanceCoef; // the higher, the more important it is to being closer
					Optional<float> addRandom = 0.1f; // just a bit of fluctuation
					Tags separationTags; // for blocked spawn points they will have access to separationTags making it possible to avoid getting too close to already used spawn point
					TagCondition separateFromTags; // use with above
					Optional<float> minSeparationDistance; // cannot be less than this
					Optional<float> keepSeparatedDistance; // how much of separation should be used, if 1.0 - all are ok, if 0.0 only last/furthest one (distance)
					Optional<float> keepSeparatedAmount; // how much of separation should be used, if 1.0 - all are ok, if 0.0 only last/furthest one (number of pois)
					int keepSeparatedMinCount = 1; // how many do we want to keep when using keepSeparation. if 1, there will be at least 1 used
					Optional<Range> minDistanceToAlive; // if set, this distance has to be maintained from other living characters
					Optional<Range> minTileDistanceToAlive; // similar to above, but relative to tile size
					Optional<Range> maxPlayAreaDistanceToAlive; // limits both above, but relative to play area size
					TagCondition minDistanceActorTags; // will check distance only against tagged (if not set, will distance against all!
					bool ignoreVisitedByPlayer = false; // if set, will ignore whether the player visited the room recently or not
					bool load_from_xml(IO::XML::Node const* _node, ::Framework::LibraryLoadingContext& _lc);
				};

				struct SpawnRegionPoolConditionsInstance
				{
					Optional<Energy> lostPoolRequired; // how much do we need to trigger/activate

					void init(SpawnManager* _spawnManager, SpawnRegionPoolConditions const& _with);
				};

				struct SpawnRegionPoolConditions
				{
					Name useRegionPool; // use pool from region manager, if in use, spawn batch size is always 1! see notes for region manager
					Optional<Name> lostRegionPoolRequired; // if not set, same pool is used
					Optional<EnergyRange> lostPoolRequired; // how much do we need to trigger/activate

					bool load_from_xml(IO::XML::Node const* _node, ::Framework::LibraryLoadingContext& _lc);

					bool check(SpawnManager* _spawnManager, SpawnRegionPoolConditionsInstance const & _instance) const;

					int get_altered_limit(SpawnManager* _spawnManager, int _limit) const; // this one is used only by dynamic

					bool is_hostile(SpawnManager* _spawnManager) const;
				};

				struct Spawned
				{
					Spawned(Framework::IModulesOwner* imo, SpawnManagerWaveElement* _byWaveElement);

					SafePtr<Framework::IModulesOwner> spawned;
					Framework::WorldAddress spawnWorldAddress;
					int spawnPoiIdx = NONE; // used only if not none
					Name poolName;
					Energy poolValue = Energy::one();
					bool killed = false;
					bool unableToAttack = false;
#ifdef AN_DEVELOPMENT_OR_PROFILER
					String name;
#endif
					RefCountObjectPtr<SpawnManagerWaveElement> byWaveElement;

					bool update_unable_to_attack(); // returns true if changed and is now unable to attack
					void on_killed(SpawnManager* _spawnManager); // consume for pool, etc
				};

				struct OnSpawnHunches
				{
					Optional<float> investigateChance;
					Optional<bool> forceInvestigate; // ignore allowing if in region etc.
					Optional<float> wanderChance;

					void override_with(OnSpawnHunches const& _hunches)
					{
						if (_hunches.investigateChance.is_set()) investigateChance = _hunches.investigateChance;
						if (_hunches.forceInvestigate.is_set()) forceInvestigate = _hunches.forceInvestigate;
						if (_hunches.wanderChance.is_set()) wanderChance = _hunches.wanderChance;
					}

					bool load_from_xml(IO::XML::Node const* _node, ::Framework::LibraryLoadingContext& _lc);
				};

				//

				/**
				 */
				class SpawnManager
				: public AIManager
				{
					FAST_CAST_DECLARE(SpawnManager);
					FAST_CAST_BASE(AIManager);
					FAST_CAST_END();

					typedef AIManager base;
				public:
					static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new SpawnManager(_mind, _logicData); }

				public:
					SpawnManager(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
					virtual ~SpawnManager();

					Framework::Room* get_for_room() const { return inRoom.get(); }
					Framework::Region* get_for_region() const { return inRegion.get(); }

					Random::Generator& access_random() { return random; }

					void hand_over_dynamic_spawns_to_game_director();

					bool has_dynamic_spawns() const { return !dynamicSpawns.is_empty(); }

				public: // Logic
					override_ void advance(float _deltaTime);

					override_ void log_logic_info(LogInfoContext& _log) const;

					void spawn_infos_async(Array<SpawnManagerSpawnInfo> const& _infos, Random::Generator rg);

				private:
					static LATENT_FUNCTION(execute_logic);

					SpawnManagerWaveInstance* start_wave(Name const & _wave);
					void start_all_dynamic();

					void update_waves(float _deltaTime);
					void update_dynamic(float _deltaTime);

					void update_rare_advance(float _deltaTime);

				private:
					SpawnManagerData const * spawnManagerData = nullptr;

					SafePtr<RegionManager> regionManager;

					Random::Generator random;

					SafePtr<Framework::Room> inRoom;
					SafePtr<Framework::Region> inRegion;
					Optional<VectorInt2> inOpenWorldCell;

					bool started = false; // advance, only if set
					float noWavesFor = 0.0f; // if exceeds some value, ceases to exist

					bool wasDisabledByGameDirector = false; // game director requested safe space 
					bool disabledByGameDirector = false; // game director requested safe space 

#ifdef AN_USE_AI_LOG
					bool shouldAlreadyCeaseToExist = false;
#endif

					struct SpawnPoint
					{
						SafePtr<Framework::PointOfInterestInstance> poi;
						SafePtr<Framework::DoorInRoom> throughDoor; // as poi might be behind the door
						RefCountObjectPtr<SpawnManagerWaveElement> blockedByWaveElement; // when blocked
						RefCountObjectPtr<DynamicSpawnInfo> blockedByDynamicSpawn; // when blocked
						float blockTime = 0.0f;
						bool inaccessible = false;
						bool beyondDoor = false;

						bool operator==(SpawnPoint const& _other) const { return poi == _other.poi; }

						bool is_ok_for(Array<SpawnManagerPOIDefinition> const & _pois) const;
					};
					Array<SpawnPoint> spawnPointsAvailable;
					Array<SpawnPoint> spawnPointsBlocked;

					// wave based
					Array<RefCountObjectPtr<SpawnManagerWaveInstance>> currentWaves;

					// dynamic spawn based
					Array<RefCountObjectPtr<DynamicSpawnInfoInstance>> dynamicSpawns; // sorted by priority

					void add_available_spawn_points(SpawnManagerPOIDefinition const & _poi);
					void update_spawn_points(float _deltaTime);
					Framework::PointOfInterestInstance* acquire_available_spawn_point(int _idx, SpawnManagerWaveElementInstance const & _elementInstance);
					Framework::PointOfInterestInstance* acquire_available_spawn_point(int _idx, DynamicSpawnInfoInstance const& _dynamicSpawnInstance);

#ifdef SPAWN_MANAGER__USE_ARRAYS_INSTEAD_OF_STACK_ARRAYS
					void gather_POIs(Array<SpawnChoosePOI>& choosePOIs, Array<SpawnManagerPOIDefinition> const* _usePOIs, Random::Generator& rg, SpawnPlacementCriteria const& _placementCriteria, Optional<RangeInt> const& _distanceFromRecentlySeenByPlayerRoom, Optional<bool> const & _allowInaccessible);
#else
					void gather_POIs(ArrayStack<SpawnChoosePOI> & choosePOIs, Array<SpawnManagerPOIDefinition> const * _usePOIs, Random::Generator & rg, SpawnPlacementCriteria const & _placementCriteria, Optional<RangeInt> const & _distanceFromRecentlySeenByPlayerRoom, Optional<bool> const& _allowInaccessible);
#endif

					void update_region_manager();

				private:
					// reserved pools are used only by normal waves and only if spawn manager allows
					struct ReservedPool
					{
						Name pool;
						Energy reserved = Energy::zero();
					};
					Array<ReservedPool> reservedPools;
					Energy get_reserved_pools(Name const& _pool) const;
					void reserve_pool(Name const& _pool, Energy const & _value);
					void free_pool(Name const& _pool, Energy const & _value);
					void recalculate_reserved_pools();

#ifdef AN_SHOW_SPAWN_MANGER_DETAILS_INFO
					List<tchar const *> detailsInfo;
#endif

				private:
					friend struct Spawned;
					friend struct SpawnManagerWave;
					friend struct SpawnManagerWaveInstance;
					friend struct SpawnManagerWaveElement;
					friend struct SpawnManagerWaveElementInstance;
					friend struct SpawnRegionPoolConditions;
					friend struct DynamicSpawnInfo;
					friend struct DynamicSpawnInfoInstance;
				};

				class SpawnManagerData
				: public ::Framework::AI::LogicData
				{
					FAST_CAST_DECLARE(SpawnManagerData);
					FAST_CAST_BASE(::Framework::AI::LogicData);
					FAST_CAST_END();

					typedef ::Framework::AI::LogicData base;
				public:
					static ::Framework::AI::LogicData* create_ai_logic_data() { return new SpawnManagerData(); }

					SpawnManagerData();
					virtual ~SpawnManagerData();

					float get_spawn_point_block_time() const { return spawnPointBlockTime; }

				public: // LogicData
					virtual bool load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc);
					virtual bool prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext);

				private:
					bool waitUntilGenerated = false; // will wait with starting until region/room is totally generated
					bool spawnIfInRoomSeenByPlayer = false; // will only spawn if player sees the room (useful for spawn rooms)
					bool spawnIfPlayerInSameRoom = false; // will only spawn if player is in the same room (useful for spawn rooms)
					Optional<bool> allowInaccessible; // sometimes we're sure we will be using inaccessible right away, skip random guesses

					Array<SpawnManagerPOIDefinition> spawnPOIs; // all (!) possible
					float spawnPointBlockTime = 5.0f;
					Tags tagSpawnedActor;
					bool ignoreGameDirector = false;
					Optional<bool> hostile;

					OnSpawnHunches onSpawnHunches;

					bool reservePools = false;

					bool noLoot = false;

					// wave based
					bool chooseOneStart = false;
					Array<Name> startWithWaves;
					Array<SpawnManagerWavePtr> waves;

					// dynamic spawner
					Array<DynamicSpawnInfoPtr> dynamicSpawns;

					friend class SpawnManager;
					friend struct SpawnManagerWaveElement;
					friend struct SpawnManagerWaveElementInstance;
					friend struct SpawnManagerWaveInstance;
					friend struct DynamicSpawnInfoInstance;
				};

				//

				struct SpawnManagerWaveElementInstance
				: public RefCountObject
				{
					RefCountObjectPtr<SpawnManagerWaveElement> element;
					RefCountObjectPtr<SpawnManagerWaveInstance> wave;
					SpawnManager* manager = nullptr;
					Array<SpawnManagerPOIDefinition> const * usePOIs = nullptr;

					SpawnRegionPoolConditionsInstance regionPoolConditionsInstance;
					float spawnPointBlockTime;
					int countLeft = 0;
					Array<SafePtr<Framework::IModulesOwner>> spawned; // to have per element, IMOs alone are okay

					void clean_up();

					void update_spawned(); // remove unavailable

					void on_spawned(SpawnManager* _spawnManager, Framework::IModulesOwner* _imo, Framework::WorldAddress const& _spawnWorldAddress, int _spawnPOIIdx);

					Framework::ObjectType* get_type_to_spawn(SpawnManager* _spawnManager, OUT_ int& _count, OUT_ Vector3& _spawnOffset) const;
					Framework::ObjectType* get_travel_with_type(SpawnManager* _spawnManager) const;
				};
				
				struct SpawnManagerWaveElement
				: public RefCountObject
				{
					WheresMyPoint::Param<int> addCount;
					float chance = 1.0f;
					Optional<bool> hostile;
					SpawnRegionPoolConditions regionPoolConditions;
					RangeInt count = RangeInt::zero; // will go down for instance (for instance it is left to spawn)
					Range countPct = Range::empty;
					float priority = 0.0f;
					bool mayBeSkipped = false;
					bool allowPlacingIfSeen = false;
					bool storeExistenceInPilgrimage = false; // if killed, will be stored in region as killed, so won't respawn if we revisit
					Optional<float> spawnPointBlockTime;
					Array<SpawnManagerPOIDefinition> spawnPOIs; // if any, will override_ level higher

					OnSpawnHunches onSpawnHunches;

					Random::Number<int> spawnBatch; // this is to spawn multiple objects in one spot
					Optional<Vector3> spawnBatchOffset; // by default we assume small ones are spawned in batches and their spawn on top of each other

					Name spawnSet;
					Array<Framework::UsedLibraryStored<Framework::ObjectType>> types;
					Framework::UsedLibraryStored<Framework::ObjectType> travelWithType;

					SimpleVariableStorage parameters;

					SpawnPlacementCriteria placementCriteria;

					bool load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc);
					bool prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext);

					SpawnManagerWaveElementInstance* instantiate(SpawnManager* _spawnManager, SpawnManagerWaveInstance* _wave) const; // ranges to single numbers, variants to single instances
				};

				//

				struct SpawnManagerWaveInstance
				: public RefCountObject
				{
					RefCountObjectPtr<SpawnManagerWave> wave;

					Array<Spawned> spawned; // to have per wave

					Array<RefCountObjectPtr<SpawnManagerWaveElementInstance>> elements;

					int spawnedNowCount = 0; // max current count to know how many to left
					int maxTotalCount = 0; // taken from elements when starting

					Optional<int> maxCountAtTheMoment;
					Optional<float> spawnBelowCountPt;

					Optional<float> endAtCountPt;
					Optional<int> endAtCount;
					Optional<int> endAtUsefulCount;

					bool safeSpaceWait = false;

					enum State
					{
						NotStarted,
						Active,
						Ended
					};

					State state = NotStarted;
					bool spawnRequested = false;
					bool spawning = false;
					int toSpawnLeft = 0;
					float toSpawnWait = 0.0f;
					bool ceaseRequested = false;
					bool endRequested = false;
					bool endRequestedWithoutNext = false; // something else wants us to end

					float timeLeft = 0.0f;

					bool does_need_to_wait() const;

					bool hasAnythingToSpawn = true;

					void clean_up();

					bool update(SpawnManager* _spawnManager, float _deltaTime); // returns false if should be removed
					void update_spawned(SpawnManager* _spawnManager); // remove unavailable

					void add_one_extra_being_spawned(SpawnManagerSpawnInfo const * _si);
					void restore_one_to_spawn(SpawnManagerSpawnInfo const * _si);
					Spawned& add(SpawnManager* _spawnManager, Framework::IModulesOwner* _imo, Framework::WorldAddress const& _spawnWorldAddress, int _spawnPOIIdx, SpawnManagerWaveElement* _byWaveElement);
					Spawned& add(SpawnManager* _spawnManager, Spawned const& _s);

#ifdef AN_SHOW_SPAWN_MANGER_DETAILS_INFO
					List<tchar const*> detailsInfo;
#endif

					void log_logic_info(LogInfoContext& _log) const;
				};

				struct SpawnManagerWave
				: public RefCountObject
				{
					struct WaveWithChance
					{
						Name name;
						float probCoef = 1.0f;

						WaveWithChance() {}
						explicit WaveWithChance(Name const & _name, float _probCoef = 1.0f) : name(_name), probCoef(_probCoef) {}
					};

					Name name;
					Name sameAs; // will get everything from other wave (during prepare) except for name, sameAs and next
					bool rareAdvance = false; // advance rarely in this wave
					bool chooseOneNext = false; // if false, all next will be triggered
					Array<Name> startWithMe; // start others with me
					Array<WaveWithChance> next; // when this wave is finished (first one, or the only one will get all spawned)
					Array<Name> endWithMe; // ends other wave if this has ended - useful for bosses to end minions (boss should be separate wave and minions should be waves triggered by boss)
					Array<Name> nextOnCease; // next when objects got ceased, not destroyed
					Optional<int> distanceToCease; // distance to rendered recently cease - works only if placed in a room
					Range spawnInterval = Range(0.5f, 6.0f); // both if failed and succeed

					OnSpawnHunches onSpawnHunches;

					// wave info - copy for each wave if "same as"

					Array<RefCountObjectPtr<SpawnManagerWaveElement>> elements;

					Optional<float> spawnPointBlockTime;
					Array<SpawnManagerPOIDefinition> spawnPOIs; // if any, will override_ level higher

					Optional<RangeInt> maxCountAtTheMoment; // enemies can be limited to exist onl
					Optional<Range> spawnBelowCountPt; // below this value will start to spawn further up to limits or if no more left

					// ending conditions
					Optional<Range> endAtCountPt; // goes at or below percentage (can't be 1.0 = 100%)
					Optional<RangeInt> endAtCount; // if reaches this count
					Optional<RangeInt> endAtUsefulCount; // if reaches this count
					bool safeSpaceWait = false; // will wait if game director wants safe space
					bool endWhenSpawned = false; // when spawned everything, jump to the next wave (remember that if there's no wave, we're no longer tracking!
					bool endIfPlayerIsIn = false; // if player is inside region/room
					bool endIfPlayerIsInCell = false; // if player is in the same cell
					Optional<int> endIfDistanceToSeenLowerThan; // player has to seen
					Optional<int> endIfDistanceToSeenAtLeast; // player has to seen
					TagCondition endIfStoryFacts;
					TagCondition skipIfStoryFacts;
					Tags setStoryFacts;

					bool keepWaveAfterEnded = false; // if this is true, wave will be kept even if it has ended, if false, it will move all spawned to the next wave, this way it is possible to work if proper number of enemies

					bool does_need_to_wait() const { return endIfPlayerIsIn || endIfPlayerIsInCell || endIfDistanceToSeenLowerThan.is_set() || endIfDistanceToSeenAtLeast.is_set() || keepWaveAfterEnded || elements.is_empty() || ! endIfStoryFacts.is_empty() /* this might be a wave that waits for some conditions */; }

					bool infiniteCount = false; // if infinite, counts won't go down, can be still time based, counts will be used as probability coef to choose enemies to spawn
					Optional<Range> timeBased; // may still end if counts get to 0

					bool load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc);
					bool prepare_for_game(Array<RefCountObjectPtr<SpawnManagerWave>> const & _waves, ::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext);

					SpawnManagerWaveInstance* instantiate(SpawnManager* _spawnManager) const; // ranges to single numbers, variants to single instances
				};

				//

				struct DynamicSpawnInfoInstance
				: public RefCountObject
				{
					DynamicSpawnInfoPtr dynamicSpawnInfo;
					
					bool active = true;
					bool hasAnythingToSpawn = true;

					int distanceToSpawnOffset = 0; // read from room/region variables

					float timeToActivate = 0.0f;

					int left = NONE; // if no limit, not used, if used, means how many are left to spawn (this could be already triggered)
					int limit = NONE;
					int concurrentLimit = NONE;
					int concurrentLimitMax = NONE;
					Name globalConcurrentCountId;
					int beingSpawned = 0;
					int toSpawn = 0;
					bool noPlaceToSpawn = false;
					float toSpawnWait = 0.0f;
					SpawnRegionPoolConditionsInstance regionPoolConditionsInstance;

					Array<SpawnManagerPOIDefinition> const* usePOIs = nullptr;
					float spawnPointBlockTime = 5.0f;

					Array<Spawned> spawned;

					void clean_up();

					void init(SpawnManager* _spawnManager);

					void update(SpawnManager* _spawnManager, float _deltaTime);
					bool spawn(SpawnManager* _spawnManager); // will be called only if one to spawn has highest priority (returns true if queued something to spawn)

					void update_spawned(SpawnManager* _spawnManager);

					void add_one_extra_being_spawned(SpawnManagerSpawnInfo const* _si);
					void restore_one_to_spawn(SpawnManagerSpawnInfo const* _si);
					void on_spawned(SpawnManager* _spawnManager, Framework::IModulesOwner* _imo, Framework::WorldAddress const& _spawnWorldAddress, int _spawnPOIIdx);

					Framework::ObjectType* get_type_to_spawn(SpawnManager* _spawnManager, OUT_ int& _count, OUT_ Vector3& _spawnOffset) const;
					Framework::ObjectType* get_travel_with_type(SpawnManager* _spawnManager) const;

					int sortRandomness = 0;
					static void sort_by_priority(Array<RefCountObjectPtr<DynamicSpawnInfoInstance>>& _array);
					static int compare_ref_priority(void const* _a, void const* _b);
				};
				
				struct DynamicSpawnInfo
				: public RefCountObject
				{
					Name name;
					Optional<bool> hostile;
					float priority = 0.0f;
					bool allowOthersToSpawnIfCantSpawn = true; // by default we allow others with a lower priority to spawn if we can't

					Name spawnSet;
					Array<Framework::UsedLibraryStored<Framework::ObjectType>> types;
					Framework::UsedLibraryStored<Framework::ObjectType> travelWithType;

					Optional<float> spawnPointBlockTime;
					Array<SpawnManagerPOIDefinition> spawnPOIs; // if any, will override_ level higher
					SimpleVariableStorage parameters;

					OnSpawnHunches onSpawnHunches;

					SpawnPlacementCriteria placementCriteria;

					Random::Number<int> spawnBatch; // this is to spawn multiple objects in one spot
					Optional<Vector3> spawnBatchOffset; // by default we assume small ones are spawned in batches and their spawn on top of each other

					int differentSpawnBatchSize = 1; // this means that multiple objects (using different POIs) may be spawn in a batch

					int distanceToSpawn = 2;
					int distanceToCease = 6;

					SpawnRegionPoolConditions regionPoolConditions;
					RangeInt limit = RangeInt::empty; // how many in total could be spawned
					RangeInt concurrentLimit = RangeInt::empty; // how many 
					Name globalConcurrentCountId;

					Range spawnInterval = Range(0.5f, 2.0f); // both if failed and succeed
					Range spawnIntervalOnDeath = Range(0.0f, 0.0f); // if any got killed/disappeared

					Range activateAfter = Range(0.0f, 0.0f); // when started

					// ? priorities when choosing
					// ? prob coef

					bool load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc);
					bool prepare_for_game(Array<RefCountObjectPtr<DynamicSpawnInfo>> const & _dynamicSpawns, ::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext);

					DynamicSpawnInfoInstance* instantiate(SpawnManager* _spawnManager) const; // ranges to single numbers, variants to single instances
				};

			};
		};
	};
};
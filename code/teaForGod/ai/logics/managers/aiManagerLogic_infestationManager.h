#pragma once

#include "..\..\..\..\framework\ai\aiLogicData.h"
#include "..\..\..\..\framework\ai\aiLogicWithLatentTask.h"
#include "..\..\..\..\framework\appearance\socketID.h"
#include "..\..\..\..\framework\library\usedLibraryStored.h"
#include "..\..\..\..\framework\presence\relativeToPresencePlacement.h"

#include "..\..\..\..\core\collision\collisionFlags.h"
#include "..\..\..\..\core\tags\tagCondition.h"

namespace Framework
{
	class TexturePart;
	class ObjectType;
	class Region;
	class SceneryType;
	interface_class IModulesOwner;
	struct DelayedObjectCreation;
	namespace AI
	{
		class Mind;
	};
};

namespace TeaForGodEmperor
{
	namespace AI
	{
		namespace Logics
		{
			namespace Managers
			{
				class InfestationManagerData;

				/**
				 */
				class InfestationManager
				: public ::Framework::AI::LogicWithLatentTask
				{
					FAST_CAST_DECLARE(InfestationManager);
					FAST_CAST_BASE(::Framework::AI::LogicWithLatentTask);
					FAST_CAST_END();

					typedef ::Framework::AI::LogicWithLatentTask base;
				public:
					static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new InfestationManager(_mind, _logicData); }

				public:
					InfestationManager(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
					virtual ~InfestationManager();

				public: // Logic
					implement_ void advance(float _deltaTime);

					implement_ void learn_from(SimpleVariableStorage & _parameters);

				private:
					static LATENT_FUNCTION(execute_logic);
				
				private:
					InfestationManagerData const * infestationManagerData = nullptr;

					bool depleted = false;

					Random::Generator rg;

					Array<SafePtr<Framework::IModulesOwner>> aiManagers;
					Array<RefCountObjectPtr<Framework::DelayedObjectCreation>> aiManagerDOCs;

					int infestationBlobsCount = 0;
					Array<SafePtr<Framework::IModulesOwner>> infestationBlobs;
					Array<RefCountObjectPtr<Framework::DelayedObjectCreation>> infestationBlobDOCs;
					SafePtr<Framework::IModulesOwner> guideTowardInfestationBlob;

					Framework::Region* mainRegion = nullptr;

					SimpleVariableInfo blobsHealthVar;

					bool handle_blobs(); // returns true if at least one blob is alive (or all not yet created)

					void queue_spawn_ai_manager(int _idx);
					void handle_ai_managers(bool _shouldExist);
				};

				class InfestationManagerData
				: public ::Framework::AI::LogicData
				{
					FAST_CAST_DECLARE(InfestationManagerData);
					FAST_CAST_BASE(::Framework::AI::LogicData);
					FAST_CAST_END();

					typedef ::Framework::AI::LogicData base;
				public:
					static ::Framework::AI::LogicData* create_ai_logic_data() { return new InfestationManagerData(); }

				public: // LogicData
					override_ bool load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc);
					override_ bool prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext);

				private:
					Array<Framework::UsedLibraryStored<Framework::AI::Mind>> aiManagerMinds;
					
					Array<Framework::UsedLibraryStored<Framework::SceneryType>> infestationBlobs;
					RangeInt infestationBlobsCount = RangeInt(3, 4);
					Name infestationBlobSpawnPointPOI;
					TagCondition infestationBlobSpawnPointTagged;
					Collision::Flags infestationBlobPlantCollisionFlags = Collision::Flags::none(); // where should it attach

					friend class InfestationManager;
				};
			};
		};
	};
};
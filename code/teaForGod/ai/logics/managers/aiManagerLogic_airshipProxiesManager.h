#pragma once

#include "..\..\..\..\core\tags\tag.h"
#include "..\..\..\..\core\wheresMyPoint\wmpParam.h"

#include "..\..\..\..\framework\ai\aiLogicData.h"
#include "..\..\..\..\framework\ai\aiLogicWithLatentTask.h"
#include "..\..\..\..\framework\game\delayedObjectCreation.h"
#include "..\..\..\..\framework\library\usedLibraryStored.h"
#include "..\..\..\..\framework\world\worldSettingCondition.h"

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
				class AirshipProxiesManagerData;

				/**
				 */
				class AirshipProxiesManager
				: public ::Framework::AI::LogicWithLatentTask
				{
					FAST_CAST_DECLARE(AirshipProxiesManager);
					FAST_CAST_BASE(::Framework::AI::LogicWithLatentTask);
					FAST_CAST_END();

					typedef ::Framework::AI::LogicWithLatentTask base;
				public:
					static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new AirshipProxiesManager(_mind, _logicData); }

				public:
					AirshipProxiesManager(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
					virtual ~AirshipProxiesManager();

					Framework::Room* get_for_room() const { return inRoom.get(); }

				public: // Logic
					override_ void advance(float _deltaTime);

				private:
					static LATENT_FUNCTION(execute_logic);

				private:
					AirshipProxiesManagerData const* airshipProxiesManagerData = nullptr;
					SafePtr<Framework::Room> inRoom;
					VectorInt2 mapCell;

					Array<RefCountObjectPtr<Framework::DelayedObjectCreation>> airshipDOCs;

					Framework::ObjectType const * cachedProxyType = nullptr;
					Framework::ObjectType const * cachedProxyTypeFor = nullptr;

					struct Airship
					{
						SafePtr<Framework::IModulesOwner> airship;
					};

					Array<Airship> airships;
					RefCountObjectPtr<Framework::DelayedObjectCreation> spawningAirshipDOC;
				};

				class AirshipProxiesManagerData
				: public ::Framework::AI::LogicData
				{
					FAST_CAST_DECLARE(AirshipProxiesManagerData);
					FAST_CAST_BASE(::Framework::AI::LogicData);
					FAST_CAST_END();

					typedef ::Framework::AI::LogicData base;
				public:
					static ::Framework::AI::LogicData* create_ai_logic_data() { return new AirshipProxiesManagerData(); }

					AirshipProxiesManagerData();
					virtual ~AirshipProxiesManagerData();

				public: // LogicData
					virtual bool load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc);
					virtual bool prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext);

				private:
					TagCondition roomTagged;

					friend class AirshipProxiesManager;
				};


				//

			};
		};
	};
};
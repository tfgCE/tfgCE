#pragma once

#include "..\..\teaEnums.h"

#include "..\..\game\energy.h"

#include "..\..\..\framework\ai\aiLogicData.h"
#include "..\..\..\framework\ai\aiLogicWithLatentTask.h"
#include "..\..\..\framework\game\delayedObjectCreation.h"
#include "..\..\..\framework\library\usedLibraryStored.h"

//

namespace Framework
{
	class ItemType;
};

namespace TeaForGodEmperor
{
	class ModuleBox;

	namespace AI
	{
		namespace Logics
		{
			class EnergyQuantumsOnPathData;
			/**
			 */
			class EnergyQuantumsOnPath
			: public ::Framework::AI::LogicWithLatentTask
			{
				FAST_CAST_DECLARE(EnergyQuantumsOnPath);
				FAST_CAST_BASE(::Framework::AI::LogicWithLatentTask);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicWithLatentTask base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new EnergyQuantumsOnPath(_mind, _logicData); }

			public:
				EnergyQuantumsOnPath(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~EnergyQuantumsOnPath();

			public: // AILogic
				override_ void learn_from(SimpleVariableStorage& _parameters);

			private:
				static LATENT_FUNCTION(execute_logic);

			private:
				EnergyQuantumsOnPathData const* eqopData;

				Array<Transform> placements;

				struct MoveObject
				{
					SafePtr<Framework::IModulesOwner> imo;
					int atIdx = 0;
				};
				Array<MoveObject> moveObjects;

				RefCountObjectPtr<Framework::DelayedObjectCreation> spawningDOC;

				bool active = false;
				bool disappear = false;
				float timeLeftToSpawn = 0.0f;
			};

			class EnergyQuantumsOnPathData
			: public ::Framework::AI::LogicData
			{
				FAST_CAST_DECLARE(EnergyQuantumsOnPathData);
				FAST_CAST_BASE(::Framework::AI::LogicData);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicData base;
			public:
				static ::Framework::AI::LogicData* create_ai_logic_data() { return new EnergyQuantumsOnPathData(); }

			public: // LogicData
				virtual bool load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc);
				virtual bool prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext);

			private:
				bool autoStart = true;

				Name alongPOIPrefix;
				Range interval = Range(5.0f);

				float speed = 3.0f;

				Optional<float> timeLimit;

				struct Entry
				{
					Framework::UsedLibraryStored<Framework::ItemType> itemType;
					Optional<Energy> energy;
					EnergyType::Type energyType = EnergyType::Ammo;
					float probCoef = 1.0f;
					Entry();
					~Entry();
				};
				Array<Entry> entries;

				friend class EnergyQuantumsOnPath;
			};
		};
	};
};
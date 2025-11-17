#pragma once

#include "..\..\teaEnums.h"
#include "..\..\game\energy.h"

#include "..\..\..\framework\ai\aiLogicData.h"
#include "..\..\..\framework\ai\aiLogicWithLatentTask.h"
#include "..\..\..\framework\appearance\socketID.h"
#include "..\..\..\framework\game\delayedObjectCreation.h"
#include "..\..\..\framework\library\usedLibraryStored.h"

namespace Framework
{
	class ItemType;
};

namespace TeaForGodEmperor
{
	namespace AI
	{
		namespace Logics
		{
			class EnergyCoilBrokenData;

			/**
			 */
			class EnergyCoilBroken
			: public ::Framework::AI::LogicWithLatentTask
			{
				FAST_CAST_DECLARE(EnergyCoilBroken);
				FAST_CAST_BASE(::Framework::AI::LogicWithLatentTask);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicWithLatentTask base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new EnergyCoilBroken(_mind, _logicData); }

			public:
				EnergyCoilBroken(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~EnergyCoilBroken();

			public: // Logic
				implement_ void advance(float _deltaTime);

				implement_ void learn_from(SimpleVariableStorage & _parameters);

			private:
				static LATENT_FUNCTION(execute_logic);
				
			private:
				Energy energy = Energy::zero();

				Name useStoryFact; // if story fact is set, will determine basing on story fact if should spawn quantum or not (and will set story fact if quantum has been taken)

				EnergyType::Type energyType = EnergyType::None;
				::Framework::SocketID baySocket;

				Framework::UsedLibraryStored<Framework::ItemType> energyQuantumType;

				bool energyItemExists = false; // this is what we think is happening, if it disappears, we set values properly
				SafePtr<Framework::IModulesOwner> energyItem;
				RefCountObjectPtr<Framework::DelayedObjectCreation> energyItemDOC;
			};
		};
	};
};
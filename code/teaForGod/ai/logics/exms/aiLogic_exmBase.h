#pragma once

#include "..\..\..\teaEnums.h"
#include "..\..\..\game\energy.h"

#include "..\..\..\..\core\types\hand.h"
#include "..\..\..\..\framework\ai\aiLogic.h"

namespace TeaForGodEmperor
{
	class ModuleEXM;
	class ModulePilgrim;

	namespace AI
	{
		namespace Logics
		{
			class EXMBase
			: public ::Framework::AI::Logic
			{
				FAST_CAST_DECLARE(EXMBase);
				FAST_CAST_BASE(::Framework::AI::Logic);
				FAST_CAST_END();

				typedef ::Framework::AI::Logic base;
			public:
				EXMBase(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~EXMBase();

			public: // Logic
				implement_ void learn_from(SimpleVariableStorage& _parameters);
				implement_ void advance(float _deltaTime);

			protected:
				Hand::Type hand = Hand::MAX;
				SafePtr<Framework::IModulesOwner> owner;
				ModuleEXM* exmModule = nullptr;
				SafePtr<Framework::IModulesOwner> pilgrimOwner;
				ModulePilgrim* pilgrimModule = nullptr;
			};

		};
	};
};

#pragma once

#include "..\..\..\game\energy.h"
#include "..\..\..\..\core\other\simpleVariableStorage.h"
#include "..\..\..\..\framework\library\usedLibraryStored.h"

namespace Framework
{
	class ItemType;
	interface_class IModulesOwner;
};

namespace TeaForGodEmperor
{
	namespace AI
	{
		namespace Logics
		{
			class NPCBase;

			struct MovementGaitTimeBased
			{
				struct GaitSetup
				{
					Name gait;
					Range timeInGait = Range(1.0f);
					float probCoef = 1.0f;

					GaitSetup(Name const& _gait, Range const& _timeInGait, Optional<float> const& _probCoef = NP)
					: gait(_gait), timeInGait(_timeInGait), probCoef(_probCoef.get(1.0f)) {}
				};

				MovementGaitTimeBased();
				~MovementGaitTimeBased();

				void set_owner_logic(NPCBase* _npcBase);

				void advance(float _deltaTime);

				void set_active(bool _active);

				void add_gait(GaitSetup const& _gs);
				void add_gait(Name const& _gait, Range const& _timeInGait, Optional<float> const& _probCoef = NP);

			private:
				NPCBase* owner = nullptr;

				Random::Generator rg;

				bool active = false;

				float timeLeft = 0.0f;
				Optional<Name> selectedGait;
				Optional<Name> setGait;

				Array<GaitSetup> gaits;
				RUNTIME_ Array<GaitSetup> availableGaits;
			};

		};
	};
};
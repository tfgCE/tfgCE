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
			struct SpawnableShield
			{
				SpawnableShield();
				~SpawnableShield();

				void set_owner(Framework::IModulesOwner* _owner) { owner = _owner; }
				void learn_from(SimpleVariableStorage& _parameters);
				
				void set_shield_requested(bool _requested) { shieldRequested = _requested; }
				void advance(float _deltaTime);

				void create_shield();
				void recall_shield(bool _removeFromWorldImmediately);

			private:
				Framework::IModulesOwner* owner = nullptr; // it always should exist within modules owner

				Framework::UsedLibraryStored<Framework::ItemType> spawnShield;
				Name spawnShieldAtSocket;

				bool shieldRequested = false;

				SimpleVariableInfo spawnedShieldVar;

				SafePtr<Framework::IModulesOwner> spawnedShield;
				bool spawnShieldIssued = false;

				Energy shieldMaxHealth = Energy(10.0f);
				Energy shieldHealth = Energy(10.0f); // where shield health is stored
				Energy shieldRebuildSpeed = Energy(0.5f); // can be setup with time value as well
				float shieldRebuildTimeBit = 0.0f;
				bool shieldDestroyed = false; // if destroyed, health has to be fully rebuild;

			};

		};
	};
};
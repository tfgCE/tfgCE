#pragma once

#include "..\..\..\utils\interactiveSwitchHandler.h"

#include "..\..\..\..\core\types\hand.h"

#include "..\..\..\..\framework\library\usedLibraryStored.h"

namespace Framework
{
	interface_class IModulesOwner;
};

namespace TeaForGodEmperor
{
	class EXMType;
	class ExtraUpgradeOption;
	class LineModel;

	namespace AI
	{
		namespace Logics
		{
			class UpgradeMachineData;

			struct UpgradeCanister
			{
				struct Content
				{
					Framework::UsedLibraryStored<EXMType> exm;
					Framework::UsedLibraryStored<ExtraUpgradeOption> extra;
				} content;

				Optional<Transform> showInfoAt;
				Name overlayInfoId;

				enum Emissive
				{
					EmissiveOff,
					EmissiveReady,
					EmissiveHeld,
					EmissiveUsed,
				};

				UpgradeCanister();
				~UpgradeCanister();

				Framework::IModulesOwner* get_held_by_owner() const { return isHeldBy.get(); }
				bool was_held() const { return wasHeld; }
				
				bool has_canister_imo() const { return ! canisterHandler.get_switches().is_empty(); }

				bool has_content() const;
				Framework::IModulesOwner* get_held_imo() const;

				void initialise(Framework::IModulesOwner* _owner, Name const& _interactiveDeviceIdVarInOwner);
				void advance(float _deltaTime);

				void set_active(bool _active);

				void drop_showing_reward();

				bool update_emissive(Emissive _emissive, bool _forceUpdate = false); // returns true if changed
				bool force_emissive(Optional<Emissive> const & _emissive = NP); // returns true if changed

				bool should_give_content(Framework::IModulesOwner* _onlyTo = nullptr) const;

				bool give_content(Framework::IModulesOwner* _onlyTo = nullptr, bool _upgradeMachineOrigin = false, bool _forceGiven = false); // returns true if given

			private:
				bool active = false;

				Colour hitIndicatorColour = Colour::white;
				Colour hitIndicatorHealthColour = Colour::white;
				Colour hitIndicatorAmmoColour = Colour::white;

				InteractiveSwitchHandler canisterHandler;
				Emissive emissive = EmissiveOff;
				Optional<Emissive> forcedEmissive;				

				float canisterAt = 0.0f;
				float prevCanisterAt = 0.0f;

				Name infoShown = Name::invalid();
				bool isHeld = false;
				bool wasHeld = false;
				Optional<Hand::Type> isHeldByHand;
				SafePtr<::Framework::IModulesOwner> isHeldBy;

				Optional<float> dropShowEXMsRewardIn;
				::SafePtr<Framework::IModulesOwner> rewardGivenTo;
			};

		};
	};
};
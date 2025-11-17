#pragma once

#include "..\..\game\weaponPart.h"
#include "..\..\game\weaponSetup.h"

#include "..\..\modules\gameplay\equipment\me_gun.h"

#include "..\..\utils\interactiveDialHandler.h"
#include "..\..\utils\interactiveSwitchHandler.h"

#include "..\..\..\framework\ai\aiLogicData.h"
#include "..\..\..\framework\ai\aiLogicWithLatentTask.h"

#include "..\..\..\core\tags\tagCondition.h"

namespace Framework
{
	class Display;
	class DisplayText;
	class TexturePart;
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
			class WeaponMixerData;

			/**
			 */
			class WeaponMixer
			: public ::Framework::AI::LogicWithLatentTask
			{
				FAST_CAST_DECLARE(WeaponMixer);
				FAST_CAST_BASE(::Framework::AI::LogicWithLatentTask);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicWithLatentTask base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new WeaponMixer(_mind, _logicData); }

			public:
				WeaponMixer(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~WeaponMixer();

			public: // Logic
				implement_ void advance(float _deltaTime);

				implement_ void learn_from(SimpleVariableStorage & _parameters);

			private:
				static LATENT_FUNCTION(execute_logic);
				
			private:
				WeaponMixerData const * weaponMixerData = nullptr;
			
				bool markedAsKnownForOpenWorldDirection = false;

				struct ShowSetupWeaponPart
				{
					RefCountObjectPtr<WeaponPart> weaponPart;
					WeaponPartType::Type weaponPartType;
					WeaponPartAddress address;
					Optional<Range2> renderAt;
					Optional<float> angle;

					inline static int compare(void const* _a, void const* _b)
					{
						ShowSetupWeaponPart const & a = *plain_cast<ShowSetupWeaponPart>(_a);
						ShowSetupWeaponPart const & b = *plain_cast<ShowSetupWeaponPart>(_b);

						return WeaponPartAddress::compare(&a.address, &b.address);
					}
				};

				struct Container
				{
					enum State
					{
						ContainerOff,
						ContainerStart,
						ContainerShowEmpty,
						ContainerShowContent,
						ContainerShutDown,
					};

					int containerIdx = NONE;
					WeaponMixer* weaponMixer = nullptr;

					SafePtr<::Framework::IModulesOwner> containerIMO;

					::Framework::Display* display = nullptr;
					bool displaySetup = false;

					WeaponSetup weaponPartsForSetup;
					Concurrency::SpinLock weaponPartsLock = Concurrency::SpinLock(TXT("TeaForGodEmperor.AI.Logics.WeaponMixer.Container.weaponPartsLock"));
					ArrayStatic<ShowSetupWeaponPart, 32> weaponParts;
					int selectedWeaponPartIdx = 0;
					int gunStatsForWeaponPartIdx = NONE;
					Array<Optional<int>> selectedWeaponPartIdxPerWeaponPartType; // this is to allow gentle revert

					WeaponSetup showWeaponSetupInfo;
					RefCountObjectPtr<WeaponPart> showWeaponPartInfo;

					Array<SafePtr<Framework::IModulesOwner>> swappingPartsTemporaryObjects;

					InteractiveDialHandler choosePartHandler;

					bool shouldBeOn = false;
					float timeToCheckOn = 0.0f;
					State state = ContainerOff;
					State drawnForState = ContainerOff;
					bool redrawNow = false;
					float timeToRedraw = 0.0f;
					int inStateDrawnFrames = 0;
					float inStateTime = 0.0f;
					
					Name activeEmissive;

					struct VariousDrawingVariables
					{
						float shutdownAtPt = 0.0f;
					} variousDrawingVariables;

					Container();

					bool does_hold_weapon() const;

					void update(float _deltaTime);

					void reset_display();
					
					void store_weapon_setup();

					~Container();
				};

				struct ExecutionData
				: public RefCountObject
				{
					bool swappingParts = false;
					bool stopSwappingParts = false;
					Array<SafePtr<Framework::IModulesOwner>> swappingPartsIn;
					float swappingPartsTime = 0.0f;
					ArrayStatic<Container, 2> containers;
					bool containersReady = false;
					bool redoGunStats = false;
					InteractiveSwitchHandler swapPartsHandler;
					SafePtr<::Framework::IModulesOwner> currentPilgrim; // for which we do stuff
					SafePtr<::Framework::IModulesOwner> holdingDialsPilgrim; // for which we do stuff
					SafePtr<::Framework::IModulesOwner> presentPilgrim; // for which we do stuff

					ModuleEquipments::GunChosenStats workingGunChosenStats;

					ExecutionData()
					{
						SET_EXTRA_DEBUG_INFO(containers, TXT("WeaponMixer::ExecutionData.containers"));
					}
				};

				RefCountObjectPtr<ExecutionData> executionData;

				friend class WeaponMixerData;
			};

			class WeaponMixerData
			: public ::Framework::AI::LogicData
			{
				FAST_CAST_DECLARE(WeaponMixerData);
				FAST_CAST_BASE(::Framework::AI::LogicData);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicData base;
			public:
				static ::Framework::AI::LogicData* create_ai_logic_data() { return new WeaponMixerData(); }

			public: // LogicData
				override_ bool load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc);
				override_ bool prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext);

			private:

				Framework::UsedLibraryStored<LineModel> emptyContainerLineModel;

				friend class WeaponMixer;
			};
		};
	};
};
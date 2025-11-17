#pragma once

#include "..\..\game\energy.h"
#include "..\..\game\gameSettingsObserver.h"

#include "..\..\..\core\other\simpleVariableStorage.h"
#include "..\..\..\core\vr\iVR.h"

#include "..\..\..\framework\ai\aiLogicData.h"
#include "..\..\..\framework\ai\aiLogicWithLatentTask.h"
#include "..\..\..\framework\ai\aiIMessageHandler.h"
#include "..\..\..\framework\game\gameInputProxy.h"
#include "..\..\..\framework\library\usedLibraryStored.h"
#include "..\..\..\framework\meshGen\meshGenParam.h"
#include "..\..\..\framework\presence\presencePath.h"
#include "..\..\..\framework\video\texturePartSequence.h"

namespace Framework
{
	class Display;
	class DisplayText;
	class Font;
	class TexturePart;
	interface_class IModulesOwner;
};

namespace TeaForGodEmperor
{
	class ModuleBox;

	namespace AI
	{
		namespace Logics
		{
			class PilgrimHandData;

			namespace PilgrimHandWord
			{
				enum Type
				{
					DontShow,
					Normal,
					Highlight,
					Blink
				};
			};

			/**
			 *	Input goes from pilgrim hand further (and as it is now):
			 *		hand_* in actual game input
			 *		is translated to Framework::GameInputProxy input but wihtout "hand_" prefix
			 */
			class PilgrimHand
			: public ::Framework::AI::LogicWithLatentTask
			, public GameSettingsObserver
			{
				FAST_CAST_DECLARE(PilgrimHand);
				FAST_CAST_BASE(::Framework::AI::LogicWithLatentTask);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicWithLatentTask base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new PilgrimHand(_mind, _logicData); }

			public:
				PilgrimHand(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~PilgrimHand();

			public: // Logic
				implement_ void advance(float _deltaTime);

				implement_ void learn_from(SimpleVariableStorage & _parameters);

			public: // GameSettingsObserver
				implement_ void on_game_settings_channged(GameSettings& _gameSettings);
			
			private:
				static LATENT_FUNCTION(execute_logic);
				static LATENT_FUNCTION(handle_pickups);
				static LATENT_FUNCTION(be_paralysed);

			private:
				void pray_kill_compute(float _timeLeft);
				void pray(PilgrimHandWord::Type _how = PilgrimHandWord::Normal);
				void kill(PilgrimHandWord::Type _how = PilgrimHandWord::Normal);
				void compute(PilgrimHandWord::Type _how = PilgrimHandWord::Normal);

			private:
				PilgrimHandData const * pilgrimHandData = nullptr;

				bool isValid = false;

				Hand::Type hand = Hand::Left;
				SafePtr<Framework::IModulesOwner> equipmentInHand;

#ifdef WITH_ENERGY_PULLING
				bool blockPulling = false;
#endif

				bool playAnim = false;
				bool playAnimDrawFirst = false;
				VectorInt2 animAt;
				int animIdx = 0;
				float animTime = 0.0f;
				Optional<float> animInterval;
				Framework::TexturePartSequence const * animSequence = nullptr;
				Optional<Colour> animColourise;

				struct UI
				{
					enum Mode
					{
						Normal,
						Equipment,
						PrayKillCompute,
					};

					bool active = false;
					Mode mode = Normal;
					Mode prevMode = Normal;

					// normal
					bool wasTutorialHighlighting = false;
					bool sectionDistActive = false;
					float sectionDistDisplayed = 0.0f;
					float totalDistDisplayed = 0.0f;
					bool healthActive = false;
					Energy healthDisplayed = Energy(0);
					bool atTopHealthDisplayed = false;
					bool regeneratingDisplayed = false;
					float healthWentDownTime = 999.0f;
					float healthBlinking = 0.0f;
					Energy healthBackupDisplayed = Energy(0);
					float healthExtraFor = 0.0f;
					bool weaponActive = false;
					Energy weaponChamberDisplayed = Energy(0);
					Energy weaponMagazineDisplayed = Energy(0);
					bool weaponMagazineChanging = false;
					Energy weaponStorageDisplayed = Energy(0);
					bool weaponStorageChanging = false;
					bool weaponMagazineInUseDisplayed = false;
					bool weaponChamberInUseDisplayed = false;
					bool weaponChamberReadyDisplayed = false;
					float weaponExtraFor = 0.0f;
					float weaponBlinking = 0.0f;
					bool otherEquipmentActive = false;
					Energy otherEquipmentPrimaryDisplayed = Energy(0);
					Energy otherEquipmentSecondaryDisplayed = Energy(0);
					float otherEquipmentPrimaryExtraFor = 0.0f;
					float otherEquipmentSecondaryExtraFor = 0.0f;
					float otherEquipmentPrimaryWentDownTime = 999.0f;
					float otherEquipmentSecondaryWentDownTime = 999.0f;

					// pray kill compute
					bool atStation = false;
					PilgrimHandWord::Type pray = PilgrimHandWord::Normal;
					PilgrimHandWord::Type kill = PilgrimHandWord::Normal;
					PilgrimHandWord::Type compute = PilgrimHandWord::Normal;
					float prayKillComputeBlockedFor = 0.0f;
					float prayKillComputeTimeLeft = 0.0f;
					float prayKillComputeBlink = 0.0f;
				} ui;

				Framework::GameInputProxy input;

				::Framework::Display* display = nullptr;
				bool inControlOfDisplay = true;
				
				void update_emissive();

				void update_interface(float _deltaTime);
				void update_interface_continuous(float _deltaTime);

				inline VectorInt2 offset_at(VectorInt2 const & _at);
			};

			struct PilgrimHandStatsData
			{
				Framework::UsedLibraryStored<Framework::Font> font;

				VectorInt2 sectionDistAt = VectorInt2(0, 0);
				int sectionDistLength = 6;
				String sectionDistFormat = String(TXT("%.0fm"));
				Optional<Colour> sectionDistColourInactive;
				Optional<Colour> sectionDistColourActive;
				VectorInt2 totalDistAt = VectorInt2(0, 8);
				int totalDistLength = 6;
				String totalDistFormat = String(TXT("%.0fm"));
				Optional<Colour> totalDistColour;

				bool load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc);
				bool prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext);
			};

			struct PilgrimHandResourceData
			{
				Framework::UsedLibraryStored<Framework::Font> font;
				Framework::UsedLibraryStored<Framework::Font> fontOther;
				Framework::UsedLibraryStored<Framework::TexturePart> indicatorTexturePart;

				VectorInt2 at = VectorInt2(0, 0);
				VectorInt2 atSecondary = VectorInt2(0, 0);
				VectorInt2 atTertiary = VectorInt2(0, 0);
				int length = 3;
				String format = String(TXT("%.0f"));
				String formatOther = String::empty();
				VectorInt2 indicatorAt = VectorInt2(0, 0);
				Optional<Colour> colour;
				Optional<Colour> notFullColour;
				Optional<Colour> extraColour;

				bool load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc);
				bool prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext);
			};

			struct PilgrimHandPrayKillComputeData
			{
				Framework::UsedLibraryStored<Framework::TexturePart> prayTexturePart;
				Framework::UsedLibraryStored<Framework::TexturePart> killTexturePart;
				Framework::UsedLibraryStored<Framework::TexturePart> computeTexturePart;

				Optional<Colour> colour;
				Optional<Colour> highlightColour;

				bool load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc);
				bool prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext);
			};
			
			class PilgrimHandData
			: public ::Framework::AI::LogicData
			{
				FAST_CAST_DECLARE(PilgrimHandData);
				FAST_CAST_BASE(::Framework::AI::LogicData);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicData base;
			public:
				static ::Framework::AI::LogicData* create_ai_logic_data() { return new PilgrimHandData(); }

			public: // LogicData
				virtual bool load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc);
				virtual bool prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext);

			private:
				Framework::GameInputProxyDefinition inputDefinition;

				PilgrimHandStatsData stats;
				PilgrimHandResourceData health;
				PilgrimHandResourceData weapon;
				PilgrimHandResourceData weaponInterim; // just for colours
				PilgrimHandResourceData otherEquipment; // based on weapon, just override_ whatever is required
				PilgrimHandPrayKillComputeData prayKillCompute;

				Array<VR::Pulse> paralysedPulses;
				Range paralysedPulseInterval = Range(0.2f, 0.5f);

				friend class PilgrimHand;
			};
		};
	};
};
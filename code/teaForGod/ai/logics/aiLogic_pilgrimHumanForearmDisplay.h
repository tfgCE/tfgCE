#pragma once

#include "..\..\game\energy.h"
#include "..\..\pilgrim\pilgrimGuidance.h"

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
			class PilgrimHumanForearmDisplayData;

			/**
			 */
			class PilgrimHumanForearmDisplay
			: public ::Framework::AI::LogicWithLatentTask
			{
				FAST_CAST_DECLARE(PilgrimHumanForearmDisplay);
				FAST_CAST_BASE(::Framework::AI::LogicWithLatentTask);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicWithLatentTask base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new PilgrimHumanForearmDisplay(_mind, _logicData); }

			public:
				PilgrimHumanForearmDisplay(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~PilgrimHumanForearmDisplay();

			public: // Logic
				implement_ void advance(float _deltaTime);

				implement_ void learn_from(SimpleVariableStorage & _parameters);

			private:
				static LATENT_FUNCTION(execute_logic);

			private:
				PilgrimHumanForearmDisplayData const * pilgrimHumanForearmDisplayData = nullptr;

				Range2 proposedElemenstBackground = Range2::empty;

				struct VisibleElement
				{
					Optional<Colour> colour;
					Optional<Range2> rect;

					VisibleElement() {}
					VisibleElement(Optional<Colour> const& _colour, Range2 const& _r) : colour(_colour), rect(_r) {}

					bool operator==(VisibleElement const& _ve) const { return colour == _ve.colour && rect == _ve.rect; }
					bool operator!=(VisibleElement const& _ve) const { return !operator==(_ve); }
				};
				Concurrency::SpinLock visibleElementsLock = Concurrency::SpinLock(TXT("TeaForGodEmperor.AI.Logics.PilgrimHumanForearmDisplay.visibleElementsLock"));
				Array<VisibleElement> visibleElements; // currently visible
				Array<VisibleElement> proposedElements; // an array we prepare, if differ, we draw them

				SafePtr<Framework::IModulesOwner> pilgrimOwner;
				
				Hand::Type hand = Hand::Left;

				float mainStoragePt = 0.0f;

				struct PrevValues
				{
					Optional<float> healthPt;
					Optional<float> healthBackupPt;
					Optional<float> mainStoragePt;
					Optional<float> magazinePt;
					Optional<float> chamberPt;
					Optional<float> itemHealthPt;
					Optional<float> itemAmmoPt;
				} prevValues;

				struct Colours
				{
					Colour health;
					Colour healthBackup;
					Colour mainStorage;
					Colour magazine;
					Colour chamber;
					Colour itemHealth;
					Colour itemAmmo;
				} colours;

				float mainActive = 1.0f;
				float healthActive = 1.0f;
				float equipmentActive = 1.0f;

				PilgrimGuidance guidance;
				bool guidanceActive = false;

				::Framework::Display* display = nullptr;

				Colour ammoColour = Colour::blue;
				Colour healthColour = Colour::red;
				Colour guidanceColour = Colour::magenta;

				Colour ammoUpColour = Colour::blue;
				Colour healthUpColour = Colour::red;

				void prepare_proposed_elements(float _deltaTime);
				void update_visible_elements();
			};

			class PilgrimHumanForearmDisplayData
			: public ::Framework::AI::LogicData
			{
				FAST_CAST_DECLARE(PilgrimHumanForearmDisplayData);
				FAST_CAST_BASE(::Framework::AI::LogicData);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicData base;
			public:
				static ::Framework::AI::LogicData* create_ai_logic_data() { return new PilgrimHumanForearmDisplayData(); }

			public: // LogicData
				virtual bool load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc);
				virtual bool prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext);

			private:
				Colour ammoColour = Colour::blue;
				Colour healthColour = Colour::red;
				Colour guidanceColour = Colour::magenta;

				Range2 screenAsRightPt = Range2(Range(0.0f, 1.0f), Range(0.0f, 1.0f)); // full, if left hand will be swapped
					
				friend class PilgrimHumanForearmDisplay;
			};
		};
	};
};
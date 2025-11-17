#pragma once

#include "..\..\game\weaponPart.h"
#include "..\..\game\weaponSetup.h"

#include "..\..\utils\interactiveButtonHandler.h"
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
			class DropCapsuleControlData;

			/**
			 */
			class DropCapsuleControl
			: public ::Framework::AI::LogicWithLatentTask
			{
				FAST_CAST_DECLARE(DropCapsuleControl);
				FAST_CAST_BASE(::Framework::AI::LogicWithLatentTask);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicWithLatentTask base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new DropCapsuleControl(_mind, _logicData); }

			public:
				DropCapsuleControl(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~DropCapsuleControl();

			public: // Logic
				implement_ void advance(float _deltaTime);

				implement_ void learn_from(SimpleVariableStorage & _parameters);

			private:
				static LATENT_FUNCTION(execute_logic);
				
			private:
				DropCapsuleControlData const * dropCapsuleControlData = nullptr;
			
				enum DropCapsuleControlState
				{
					Inactive,
					StartUp,
					Operational,
					RequiresCalibration,
					Calibrated,
					ReadyToLaunch,
					Launched, // triggered by script
					Crashed,
					Docked,
				};

				Random::Generator rg;

				::Framework::Display* display = nullptr;
				bool displaySetup = false;
				struct DisplayInfo
				{
					bool redrawNow = false;
					int inStateDrawnFrames = 0;
					float timeSinceLastDraw = 0.0f;
					float timeToRedraw = 0.0f;

					float otherBarTimeLeft = 0.0f;
					Array<float> otherBar;
					Array<float> otherBarTarget;
					float velocityBar = 0.0f;
					float accelerationBar = 0.0f;
					float proximityBar = 0.0f;
				};

				struct InteractiveControl
				{
					int interactiveDeviceId = NONE;
					int dropCapsuleControlStartUpSequenceIdx = 0;

					InteractiveButtonHandler buttonHandler;
					InteractiveDialHandler dialHandler;
					InteractiveSwitchHandler switchHandler;

					Name activeEmissive;
					Name requestedEmissive;

					bool requiresCalibration = false;
					int calibratedAt = 0;
					bool calibratedOk = false;
					bool inoperable = false;
					float crashedTimeLeft = 0.0f;

					InteractiveControl();

					~InteractiveControl();
				};

				struct ExecutionData
				: public RefCountObject
				{
					float inStateTime = 0.0f;
					Optional<int> customStateValue;
					DropCapsuleControlState currentState = DropCapsuleControlState::Inactive;
					DropCapsuleControlState requestedState = DropCapsuleControlState::Inactive;
					bool launchAllowed = false; // has to be set by script
					Optional<float> timeToHit; // has to be set by script, will auto go down when launched, if not set, we won't hit

					// setup
					ArrayStatic<InteractiveControl, 32> interactiveControls;
					InteractiveControl launchControl; // not everything is used there

					// runtime
					int dropCapsuleControlStartUpSequences = 1;
					bool calibrationPending = false;
					int calibrationPendingCount = 0;

					DisplayInfo display;

					ExecutionData()
					{
						SET_EXTRA_DEBUG_INFO(interactiveControls, TXT("DropCapsuleControl::ExecutionData.interactiveControls"));
					}
				};

				RefCountObjectPtr<ExecutionData> executionData;

				void draw_line_model(Framework::Display* _display, ::System::Video3D* _v3d, LineModel* _lm, Vector2 const& _atPt, float _relScale, bool _allowColourOverride = true);
				void draw_controls(Framework::Display* _display, ::System::Video3D* _v3d);
					
				void play_sound(Name const& _sound);
				void stop_sound(Name const& _sound);

				friend class DropCapsuleControlData;
			};

			class DropCapsuleControlData
			: public ::Framework::AI::LogicData
			{
				FAST_CAST_DECLARE(DropCapsuleControlData);
				FAST_CAST_BASE(::Framework::AI::LogicData);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicData base;
			public:
				static ::Framework::AI::LogicData* create_ai_logic_data() { return new DropCapsuleControlData(); }

			public: // LogicData
				override_ bool load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc);
				override_ bool prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext);

			private:
				Framework::UsedLibraryStored<LineModel> dropCapsuleLineModel;
				Framework::UsedLibraryStored<LineModel> obstacleLineModel;

				friend class DropCapsuleControl;
			};
		};
	};
};
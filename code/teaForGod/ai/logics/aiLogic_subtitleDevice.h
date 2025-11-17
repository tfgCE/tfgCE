#pragma once

#include "..\..\game\energy.h"
#include "..\..\utils\interactiveSwitchHandler.h"

#include "..\..\..\framework\ai\aiLogicData.h"
#include "..\..\..\framework\ai\aiLogicWithLatentTask.h"
#include "..\..\..\framework\appearance\socketID.h"
#include "..\..\..\framework\library\usedLibraryStored.h"
#include "..\..\..\framework\presence\relativeToPresencePlacement.h"

namespace Framework
{
	class Display;
	class DisplayText;
	class TexturePart;
	interface_class IModulesOwner;
};

namespace TeaForGodEmperor
{
	namespace AI
	{
		namespace Logics
		{
			class SubtitleDeviceData;

			/**
			 */
			class SubtitleDevice
			: public ::Framework::AI::LogicWithLatentTask
			{
				FAST_CAST_DECLARE(SubtitleDevice);
				FAST_CAST_BASE(::Framework::AI::LogicWithLatentTask);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicWithLatentTask base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new SubtitleDevice(_mind, _logicData); }

			public:
				SubtitleDevice(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~SubtitleDevice();

			public: // Logic
				implement_ void advance(float _deltaTime);

				implement_ void learn_from(SimpleVariableStorage & _parameters);

			private:
				static LATENT_FUNCTION(execute_logic);
				
			private:
				SubtitleDeviceData const* subtitleDeviceData = nullptr;

				::Framework::Display* display = nullptr;

				bool voiceoverActive = false;
				bool subtitlesOn = false;

				bool redrawNow = true;

				SafePtr<Framework::IModulesOwner> button;
				
				bool updateEmissiveOnce = true;

				void update_emissive(bool _subtitlesOn);

				void switch_subtitles(Optional<bool> _on = NP);
			};

			class SubtitleDeviceData
			: public ::Framework::AI::LogicData
			{
				FAST_CAST_DECLARE(SubtitleDeviceData);
				FAST_CAST_BASE(::Framework::AI::LogicData);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicData base;
			public:
				static ::Framework::AI::LogicData* create_ai_logic_data() { return new SubtitleDeviceData(); }

			public: // LogicData
				override_ bool load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc);
				override_ bool prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext);

			private:
				Colour colourOff = Colour::red;
				Colour colourOn = Colour::green;
				bool inverseOff = false;
				bool inverseOn = true;

				friend class SubtitleDevice;
			};
		};
	};
};
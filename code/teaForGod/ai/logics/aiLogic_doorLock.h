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
			class DoorLockData;

			/**
			 */
			class DoorLock
			: public ::Framework::AI::LogicWithLatentTask
			{
				FAST_CAST_DECLARE(DoorLock);
				FAST_CAST_BASE(::Framework::AI::LogicWithLatentTask);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicWithLatentTask base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new DoorLock(_mind, _logicData); }

			public:
				DoorLock(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~DoorLock();

			public: // Logic
				implement_ void advance(float _deltaTime);

				implement_ void learn_from(SimpleVariableStorage & _parameters);

			private:
				static LATENT_FUNCTION(execute_logic);
				
			private:
				DoorLockData const* doorLockData = nullptr;

				bool pilgrimIn = false;
				bool pilgrimLock = false;

				::Framework::Display* display = nullptr;

				bool redrawNow = false;
				float timeToRedraw = 0.0f;
				::System::TimeStamp drawTS;

				float timeSinceAnyChange = 0.0f;

				SafePtr<Framework::IModulesOwner> button;
				
				SafePtr<Framework::DoorInRoom> doorInRoom;
				bool doorLocked = false;
				bool doorShouldRemainLocked = false;
				float doorOpenFactor = 1.0f;
				bool doorOpen = false;

				bool updateEmissiveOnce = true;

				void update_emissive(bool _locked, bool _locking);

				void lock_door(Optional<bool> const& _lock = NP);
			};

			class DoorLockData
			: public ::Framework::AI::LogicData
			{
				FAST_CAST_DECLARE(DoorLockData);
				FAST_CAST_BASE(::Framework::AI::LogicData);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicData base;
			public:
				static ::Framework::AI::LogicData* create_ai_logic_data() { return new DoorLockData(); }

			public: // LogicData
				override_ bool load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc);
				override_ bool prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext);

			private:
				Colour buttonColourNotLocked = Colour::grey;
				Colour buttonColourLocked = Colour::red;
				
				Colour autoOpenColour = Colour::white;
				Colour autoClosedColour = Colour::blue;
				Colour lockedColour = Colour::red;

				VectorInt2 cellSize = VectorInt2(4, 4);

				friend class DoorLock;
			};
		};
	};
};
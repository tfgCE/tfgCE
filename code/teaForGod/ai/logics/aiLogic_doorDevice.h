#pragma once

#include "..\..\..\core\other\simpleVariableStorage.h"

#include "..\..\..\framework\ai\aiLogicData.h"
#include "..\..\..\framework\ai\aiLogicWithLatentTask.h"
#include "..\..\..\framework\library\usedLibraryStored.h"
#include "..\..\..\framework\text\localisedString.h"
#include "..\..\..\framework\video\texturePartSequence.h"

namespace Framework
{
	class Actor;
	class Door;
	class DoorInRoom;
	class TemporaryObjectType;
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
			class DoorDeviceData;

			namespace DoorDeviceOperation
			{
				enum Type
				{
					None,
					ReadyToOpen, // release steam
					Open, // for end door (same time)
					Close, // when player has entered
				};

				inline tchar const * to_char(Type _type)
				{
					if (_type == ReadyToOpen) return TXT("ready to open");
					if (_type == Open) return TXT("open");
					if (_type == Close) return TXT("close");
					return TXT("none");
				}
			};

			namespace DoorDeviceDoorState
			{
				enum Type
				{
					Closed,
					Open,
					Opening,
					Closing,
				};
			};

			namespace DoorDeviceTypeMode
			{
				enum Type
				{
					Unknown,
					LinearStart,
					LinearEnd
				};
			};

			/**
			 */
			class DoorDevice
			: public ::Framework::AI::LogicWithLatentTask
			{
				FAST_CAST_DECLARE(DoorDevice);
				FAST_CAST_BASE(::Framework::AI::LogicWithLatentTask);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicWithLatentTask base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new DoorDevice(_mind, _logicData); }

			public:
				DoorDevice(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~DoorDevice();

				void set_operation(DoorDeviceOperation::Type _operation);

			public: // Logic
				implement_ void advance(float _deltaTime);

				implement_ void learn_from(SimpleVariableStorage & _parameters);

			private:
				static LATENT_FUNCTION(execute_logic);
				static LATENT_FUNCTION(do_steam);
				static LATENT_FUNCTION(do_door_op);
				static LATENT_FUNCTION(choose_operation);

			private:
				DoorDeviceData const * doorDeviceData;

				bool isValid = false;

				SafePtr<Framework::Door> door;
				bool newDoor = false;
				SafePtr<Framework::DoorInRoom> insideDoor;

				// variables
				Name openVarID;
				Name actualOpenVarID;

				// steam
				::Framework::AI::LatentTaskHandle steamTask;
				Array<SafePtr<Framework::IModulesOwner>> steamObjects;
				float releaseSteamTime = 3.0f;

				// display/door management
				bool readyToOpen = false;
				bool openedOnce = false;
				DoorDeviceDoorState::Type doorState = DoorDeviceDoorState::Closed;
				float timeSinceDoorClosed = 1000.0f;

				DoorDeviceOperation::Type operation = DoorDeviceOperation::None;

				SimpleVariableInfo openVar;
				SimpleVariableInfo actualOpenVar;

#ifdef AN_DEVELOPMENT_OR_PROFILER
				bool testMode = false;
				void test_mode() { testMode = true; }
#endif
				bool is_ready_to_open_now() const { return readyToOpen; }

			};

			class DoorDeviceData
			: public ::Framework::AI::LogicData
			{
				FAST_CAST_DECLARE(DoorDeviceData);
				FAST_CAST_BASE(::Framework::AI::LogicData);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicData base;
			public:
				static ::Framework::AI::LogicData* create_ai_logic_data() { return new DoorDeviceData(); }

				DoorDeviceData();
				virtual ~DoorDeviceData();

			public: // LogicData
				virtual bool load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc);

			private:
				Name idVar;
				
				Name openVarID;
				Name actualOpenVarID;

				friend class DoorDevice;
			};
		};
	};
};
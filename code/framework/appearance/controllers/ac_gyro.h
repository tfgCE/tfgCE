#pragma once

#include "..\appearanceController.h"
#include "..\appearanceControllerData.h"
#include "..\..\module\modulePresence.h"

#include "..\..\..\core\math\math.h"
#include "..\..\..\core\mesh\boneID.h"
#include "..\..\..\core\other\simpleVariableStorage.h"

namespace Framework
{
	namespace AppearanceControllersLib
	{
		class GyroData;

		/**
		 *	keep rotation
		 */
		class Gyro
		: public AppearanceController
		, public IPresenceObserver
		{
			FAST_CAST_DECLARE(Gyro);
			FAST_CAST_BASE(AppearanceController);
			FAST_CAST_BASE(IPresenceObserver);
			FAST_CAST_END();

			typedef AppearanceController base;
		public:
			Gyro(GyroData const * _data);
			virtual ~Gyro();

		public: // AppearanceController
			override_ void initialise(ModuleAppearance* _owner);
			override_ void advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context);
			override_ void calculate_final_pose(REF_ AppearanceControllerPoseContext & _context);
			override_ void get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
				OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const;

		public: // IPresenceObserver
			implement_ void on_presence_changed_room(ModulePresence* _presence, Room* _intoRoom, Transform const& _intoRoomTransform, DoorInRoom* _exitThrough, DoorInRoom* _enterThrough, DoorInRoomArrayPtr const* _throughDoors);
			implement_ void on_presence_added_to_room(ModulePresence* _presence, Room* _room, DoorInRoom* _enteredThroughDoor) {}
			implement_ void on_presence_removed_from_room(ModulePresence* _presence, Room* _room) {}
			implement_ void on_presence_destroy(ModulePresence* _presence);

		protected:
			virtual tchar const * get_name_for_log() const { return TXT("gyro"); }

		private:
			GyroData const * gyroData;
			Meshes::BoneID bone;

			Quat currentRotation = Quat::identity;
			Rotator3 allowRotation = Rotator3(0.0f, 1.0f, 0.0f);

		private:
			ModulePresence* observingPresence = nullptr;

			void stop_observing_presence();
		};

		class GyroData
		: public AppearanceControllerData
		{
			FAST_CAST_DECLARE(GyroData);
			FAST_CAST_BASE(AppearanceControllerData);
			FAST_CAST_END();

			typedef AppearanceControllerData base;
		public:
			static void register_itself();

			GyroData();

		public: // AppearanceControllerData
			override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
			override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

			override_ void reskin(Meshes::BoneRenameFunc);
			override_ void apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context);

			override_ AppearanceControllerData* create_copy() const;
			override_ AppearanceController* create_controller();

		private:
			MeshGenParam<Name> bone;
			MeshGenParam<Rotator3> allowRotation = Rotator3(1.0f, 1.0f, 1.0f);

			static AppearanceControllerData* create_data();
				
			friend class Gyro;
		};
	};
};

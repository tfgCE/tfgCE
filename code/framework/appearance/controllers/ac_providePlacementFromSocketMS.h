#pragma once

#include "..\appearanceController.h"
#include "..\appearanceControllerData.h"
#include "..\socketID.h"

#include "..\..\..\core\mesh\boneID.h"
#include "..\..\..\core\other\simpleVariableStorage.h"

namespace Framework
{
	namespace AppearanceControllersLib
	{
		class ProvidePlacementFromSocketMSData;

		/**
		 *	
		 */
		class ProvidePlacementFromSocketMS
		: public AppearanceController
		{
			FAST_CAST_DECLARE(ProvidePlacementFromSocketMS);
			FAST_CAST_BASE(AppearanceController);
			FAST_CAST_END();

			typedef AppearanceController base;
		public:
			ProvidePlacementFromSocketMS(ProvidePlacementFromSocketMSData const * _data);
			virtual ~ProvidePlacementFromSocketMS();

		public: // AppearanceController
			override_ void initialise(ModuleAppearance* _owner);
			override_ void advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context);
			override_ void calculate_final_pose(REF_ AppearanceControllerPoseContext & _context);
			override_ void get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
				OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const;

		protected:
			virtual tchar const * get_name_for_log() const { return TXT("provide placement from socket ms"); }

		private:
			ProvidePlacementFromSocketMSData const * providePlacementFromSocketMSData;

			SimpleVariableInfo resultPlacementMSVar;

			SocketID idleSocket;
			Meshes::BoneID baseBone;

			Transform offset;

			struct State
			{
				int priority = 0;
				SimpleVariableInfo stateVar;
				SocketID socket;
				SimpleVariableInfo chooseSocketVar;

				struct ChooseSocket
				{
					SocketID socket;
					float value;
					float toNextInv;
					static int compare(void const * _a, void const * _b);
				};
				Array<ChooseSocket> chooseSockets;

				static int compare(void const * _a, void const * _b)
				{
					State const * a = plain_cast<State>(_a);
					State const * b = plain_cast<State>(_b);
					int diff = a->priority - b->priority;
					return diff > 0 ? 1 : (diff < 0 ? -1 : 0); // less important go first
				}
			};
			Array<State> states;

		};

		class ProvidePlacementFromSocketMSData
		: public AppearanceControllerData
		{
			FAST_CAST_DECLARE(ProvidePlacementFromSocketMSData);
			FAST_CAST_BASE(AppearanceControllerData);
			FAST_CAST_END();

			typedef AppearanceControllerData base;
		public:
			static void register_itself();

			ProvidePlacementFromSocketMSData();

		public: // AppearanceControllerData
			override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
			override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

			override_ void reskin(Meshes::BoneRenameFunc);
			override_ void apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context);
			override_ void apply_transform(Matrix44 const & _transform);

			override_ AppearanceControllerData* create_copy() const;
			override_ AppearanceController* create_controller();

		private:
			MeshGenParam<Name> resultPlacementMSVar;

			MeshGenParam<Name> idleSocket;
			MeshGenParam<Name> baseBone; // we will blend length relatively to this bone's placement

			MeshGenParam<Transform> offset;

			struct State
			{
				int priority = 0;
				MeshGenParam<Name> stateVar;
				MeshGenParam<Name> socket;
				MeshGenParam<Name> chooseSocketVar; // to choose between various sockets
				struct ChooseSocket
				{
					MeshGenParam<Name> socket;
					MeshGenParam<float> value;
					bool load_from_xml(IO::XML::Node const * _node);
				};
				Array<ChooseSocket> chooseSockets;
				bool load_from_xml(IO::XML::Node const * _node);
			};
			Array<State> states;

			static AppearanceControllerData* create_data();
				
			friend class ProvidePlacementFromSocketMS;
		};
	};
};

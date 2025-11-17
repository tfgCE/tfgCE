#pragma once

#include "..\appearanceController.h"
#include "..\appearanceControllerData.h"
#include "..\socketID.h"

#include "..\..\sound\soundSource.h"

#include "..\..\..\core\math\math.h"
#include "..\..\..\core\mesh\boneID.h"
#include "..\..\..\core\other\simpleVariableStorage.h"

namespace Framework
{
	namespace AppearanceControllersLib
	{
		class AtSocketData;

		/**
		 *	Lerps between two sockets
		 */
		class AtSocket
		: public AppearanceController
		{
			FAST_CAST_DECLARE(AtSocket);
			FAST_CAST_BASE(AppearanceController);
			FAST_CAST_END();

			typedef AppearanceController base;
		public:
			AtSocket(AtSocketData const * _data);
			virtual ~AtSocket();

		public: // AppearanceController
			override_ void initialise(ModuleAppearance* _owner);
			override_ void advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context);
			override_ void calculate_final_pose(REF_ AppearanceControllerPoseContext & _context);
			override_ void get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
				OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const;

		protected:
			override_ tchar const * get_name_for_log() const { return TXT("at socket"); }
			override_ void internal_log(LogInfoContext & _log) const;

		private:
			AtSocketData const * atSocketData;
			Meshes::BoneID bone;
			SimpleVariableInfo placementVar;
			SimpleVariableInfo variable; // appearance controller
			SocketID socket0;
			SocketID socket1;
			float value0 = 0.0f;
			float value1 = 1.0f;
			float blendTime = 0.0f;
			float variableValue = 0.0f;

			Name blendCurve;

			Name transitionSound;
			Optional<Name> transitionSoundAtSocket;
			SoundSourcePtr playedTransitionSound;

			float lastShouldBeAt = 0.0f;
		};

		class AtSocketData
		: public AppearanceControllerData
		{
			FAST_CAST_DECLARE(AtSocketData);
			FAST_CAST_BASE(AppearanceControllerData);
			FAST_CAST_END();

			typedef AppearanceControllerData base;
		public:
			static void register_itself();

			AtSocketData();

		public: // AppearanceControllerData
			override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
			override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

			override_ void reskin(Meshes::BoneRenameFunc);
			override_ void apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context);

			override_ AppearanceControllerData* create_copy() const;
			override_ AppearanceController* create_controller();

		private:
			// modifies bone or placementVar
			MeshGenParam<Name> bone;
			MeshGenParam<Name> placementVar;
			MeshGenParam<Name> variable; // used to blend between, if not provided 1 is assumed
			MeshGenParam<Name> socket0;
			MeshGenParam<Name> socket1; // if not provided, will use current placement
			MeshGenParam<float> value0 = 0.0f;
			MeshGenParam<float> value1 = 1.0f;
			MeshGenParam<float> blendTime = 0.0f;
			MeshGenParam<Name> blendCurve;

			MeshGenParam<Name> transitionSound;
			MeshGenParam<Name> transitionSoundAtSocket;

			static AppearanceControllerData* create_data();
				
			friend class AtSocket;
		};
	};
};

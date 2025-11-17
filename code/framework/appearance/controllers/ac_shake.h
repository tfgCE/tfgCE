#pragma once

#include "..\appearanceController.h"
#include "..\appearanceControllerData.h"

#include "..\..\..\core\math\math.h"
#include "..\..\..\core\mesh\boneID.h"
#include "..\..\..\core\other\simpleVariableStorage.h"

namespace Framework
{
	namespace AppearanceControllersLib
	{
		class ShakeData;

		/**
		 *	Lerps between two sockets
		 */
		class Shake
		: public AppearanceController
		{
			FAST_CAST_DECLARE(Shake);
			FAST_CAST_BASE(AppearanceController);
			FAST_CAST_END();

			typedef AppearanceController base;
		public:
			Shake(ShakeData const * _data);
			virtual ~Shake();

		public: // AppearanceController
			override_ void initialise(ModuleAppearance* _owner);
			override_ void advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context);
			override_ void calculate_final_pose(REF_ AppearanceControllerPoseContext & _context);
			override_ void get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
				OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const;

		protected:
			virtual tchar const * get_name_for_log() const { return TXT("shake"); }

		private:
			ShakeData const * shakeData;
			Meshes::BoneID bone;

			float shakeAt = 0.0f;
			float shakeLength = 0.5f;

			Vector3 shakeLocationBS = Vector3::zero;
			Rotator3 shakeRotationBS = Rotator3::zero;
		};

		class ShakeData
		: public AppearanceControllerData
		{
			FAST_CAST_DECLARE(ShakeData);
			FAST_CAST_BASE(AppearanceControllerData);
			FAST_CAST_END();

			typedef AppearanceControllerData base;
		public:
			static void register_itself();

			ShakeData();

		public: // AppearanceControllerData
			override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
			override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

			override_ void reskin(Meshes::BoneRenameFunc);
			override_ void apply_transform(Matrix44 const & _transform);
			override_ void apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context);

			override_ AppearanceControllerData* create_copy() const;
			override_ AppearanceController* create_controller();

		private:
			MeshGenParam<Name> bone;

			MeshGenParam<float> shakeLength = 0.5f; // how long is one shake
			MeshGenParam<Vector3> shakeLocationBS = Vector3::zero;
			MeshGenParam<Rotator3> shakeRotationBS = Rotator3::zero;

			static AppearanceControllerData* create_data();
				
			friend class Shake;
		};
	};
};

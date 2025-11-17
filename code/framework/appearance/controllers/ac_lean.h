#pragma once

#include "..\appearanceController.h"
#include "..\appearanceControllerData.h"

#include "..\..\..\core\math\math.h"
#include "..\..\..\core\mesh\boneID.h"
#include "..\..\..\core\other\simpleVariableStorage.h"
#include "..\..\..\framework\appearance\socketID.h"

namespace Framework
{
	namespace AppearanceControllersLib
	{
		class LeanData;
		class Walker;

		namespace LeanTo
		{
			enum Type
			{
				LinearVelocity,
				RotationVelocity,
				OrientationDifference
			};
		};

		/**
		 *	Leans body basing on rotation velocity
		 */
		class Lean
		: public AppearanceController
		{
			FAST_CAST_DECLARE(Lean);
			FAST_CAST_BASE(AppearanceController);
			FAST_CAST_END();

			typedef AppearanceController base;
		public:
			Lean(LeanData const * _data);
			virtual ~Lean();

		public: // AppearanceController
			override_ void initialise(ModuleAppearance* _owner);
			override_ void advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context);
			override_ void calculate_final_pose(REF_ AppearanceControllerPoseContext & _context);
			override_ void get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
				OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const;

		protected:
			virtual tchar const * get_name_for_log() const { return TXT("lean"); }

		private:
			LeanData const * leanData;
			Meshes::BoneID bone;
			Meshes::BoneID parentBone;
			Framework::SocketID pivotSocket; // point of leaning

			LeanTo::Type leanTo = LeanTo::RotationVelocity;

			float maxAngle = 20.0f;
			float speedToAngleCoef = 0.2f;
			float angleBlendTime = 0.2f;

			// runtime
			Rotator3 leanAngle = Rotator3::zero;
		};

		class LeanData
		: public AppearanceControllerData
		{
			FAST_CAST_DECLARE(LeanData);
			FAST_CAST_BASE(AppearanceControllerData);
			FAST_CAST_END();

			typedef AppearanceControllerData base;
		public:
			static void register_itself();

			LeanData();

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
			MeshGenParam<Name> pivotSocket = Name::invalid();

			LeanTo::Type leanTo = LeanTo::RotationVelocity;

			MeshGenParam<float> maxAngle = 20.0f;
			MeshGenParam<float> speedToAngleCoef = 0.2f;
			MeshGenParam<float> angleBlendTime = 0.2f;

			static AppearanceControllerData* create_data();
				
			friend class Lean;
		};
	};
};

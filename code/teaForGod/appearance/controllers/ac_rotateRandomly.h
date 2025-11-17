#pragma once

#include "..\..\..\framework\appearance\appearanceController.h"
#include "..\..\..\framework\appearance\appearanceControllerData.h"
#include "..\..\..\framework\sound\soundSource.h"

#include "..\..\..\core\math\math.h"
#include "..\..\..\core\mesh\boneID.h"
#include "..\..\..\core\other\simpleVariableStorage.h"

namespace TeaForGodEmperor
{
	namespace AppearanceControllersLib
	{
		class RotateRandomlyData;

		/**
		 *	rotates randomly around given axis
		 */
		class RotateRandomly
		: public Framework::AppearanceController
		{
			FAST_CAST_DECLARE(RotateRandomly);
			FAST_CAST_BASE(Framework::AppearanceController);
			FAST_CAST_END();

			typedef Framework::AppearanceController base;
		public:
			RotateRandomly(RotateRandomlyData const * _data);
			virtual ~RotateRandomly();

		public: // AppearanceController
			override_ void initialise(Framework::ModuleAppearance* _owner);
			override_ void advance_and_adjust_preliminary_pose(REF_ Framework::AppearanceControllerPoseContext & _context);
			override_ void calculate_final_pose(REF_ Framework::AppearanceControllerPoseContext & _context);
			override_ void get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
				OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const;

		protected:
			virtual tchar const * get_name_for_log() const { return TXT("rotate randomly (yaw)"); }

		private:
			RotateRandomlyData const * rotateRandomlyData;
			Meshes::BoneID bone;
			SimpleVariableInfo moveAtVar;
			SimpleVariableInfo stopAtVar;

			bool rotating = false;
			Framework::SoundSourcePtr soundPlayed;
			Name sound;

			int oneDir = 0;
			Range speed = Range(90.0f);
			float acceleration = 180.0f;

			float currentTurnSpeed = 0.0f;
			float currentSpeed = 0.0f;
			float currentYaw = 0.0f;

			float turnLeft = 0.0f;
			float timeToNextTurn = 1.0f;
			Range turnInterval = Range(1.0f, 5.0f);
		};

		class RotateRandomlyData
		: public Framework::AppearanceControllerData
		{
			FAST_CAST_DECLARE(RotateRandomlyData);
			FAST_CAST_BASE(Framework::AppearanceControllerData);
			FAST_CAST_END();

			typedef Framework::AppearanceControllerData base;
		public:
			static void register_itself();

			RotateRandomlyData();

		public: // AppearanceControllerData
			override_ bool load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);
			override_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);

			override_ void reskin(Meshes::BoneRenameFunc);
			override_ void apply_transform(Matrix44 const & _transform);
			override_ void apply_mesh_gen_params(Framework::MeshGeneration::GenerationContext const & _context);

			override_ Framework::AppearanceControllerData* create_copy() const;
			override_ Framework::AppearanceController* create_controller();

		private:
			Framework::MeshGenParam<Name> bone;
			
			Framework::MeshGenParam<Name> moveAtVar;
			Framework::MeshGenParam<Name> stopAtVar;

			Framework::MeshGenParam<Name> sound = Name::invalid();

			Framework::MeshGenParam<int> oneDir = 0;
			Framework::MeshGenParam<Range> speed = Range(90.0f);
			Framework::MeshGenParam<float> acceleration = 0.0f;
			Framework::MeshGenParam<Range> turnInterval = Range(1.0f, 5.0f); // if no interval, keeps on turning

			static Framework::AppearanceControllerData* create_data();
				
			friend class RotateRandomly;
		};
	};
};

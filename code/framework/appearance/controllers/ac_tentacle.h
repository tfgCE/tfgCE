#pragma once

#include "..\appearanceController.h"
#include "..\appearanceControllerData.h"

#include "..\..\meshGen\meshGenParam.h"

#include "..\..\..\core\math\math.h"
#include "..\..\..\core\mesh\boneID.h"
#include "..\..\..\core\other\simpleVariableStorage.h"

namespace Framework
{
	namespace AppearanceControllersLib
	{
		class TentacleData;

		/**
		 *	by default:
		 *		tentacle bones use Y axis to point to the next bone
		 *		target placement is different, as it is compatible with other systems, Z is along tentacle but opposite
		 */
		class Tentacle
		: public AppearanceController
		{
			FAST_CAST_DECLARE(Tentacle);
			FAST_CAST_BASE(AppearanceController);
			FAST_CAST_END();

			typedef AppearanceController base;
		public:
			Tentacle(TentacleData const * _data);
			virtual ~Tentacle();

		public: // AppearanceController
			override_ void initialise(ModuleAppearance* _owner);
			override_ void advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context);
			override_ void calculate_final_pose(REF_ AppearanceControllerPoseContext & _context);
			override_ void get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
				OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const;

		protected:
			virtual tchar const * get_name_for_log() const { return TXT("tentacle"); }

		private:
			TentacleData const * tentacleData;
			bool isValid;
			Meshes::BoneID parentBone; // of the whole tentacle
			struct BoneInfo
			{
				Meshes::BoneID bone;
				float lengthToPrev = 0.0f;
				float lengthToNext = 0.0f;
				float importanceWeight = 0.0f; // -1.0f prev, 1.0f next
				Transform placementMS;
				Vector3 velocityMS;
			};
			Array<BoneInfo> bones; // the last one is the tip
			Meshes::BoneID footBone; // might be not set

			SimpleVariableInfo targetPlacementMSVar;
			SimpleVariableInfo upDirMSVar;
			Vector3 upDirMS;

			float angleLimit;
			float alignTowardsNextSegment;

			Optional<Sphere> randomiseInSphere;
			bool relativeToStartBone = false;
			bool applyVelocity = false;
			Optional<Vector3> applyAccelerationWS;
		};

		class TentacleData
		: public AppearanceControllerData
		{
			FAST_CAST_DECLARE(TentacleData);
			FAST_CAST_BASE(AppearanceControllerData);
			FAST_CAST_END();

			typedef AppearanceControllerData base;
		public:
			static void register_itself();

			TentacleData();

		public: // AppearanceControllerData
			override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
			override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

			override_ void reskin(Meshes::BoneRenameFunc);
			override_ void apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context);
			override_ void apply_transform(Matrix44 const & _transform);

			override_ AppearanceControllerData* create_copy() const;
			override_ AppearanceController* create_controller();

		private:
			MeshGenParam<Name> startBone;
			MeshGenParam<Name> endBone;
			MeshGenParam<Name> footBone; // if set, will maintain forward dir on it
			
			MeshGenParam<Name> targetPlacementMSVar;
			
			MeshGenParam<Name> upDirMSVar;
			MeshGenParam<Vector3> upDirMS;
			
			MeshGenParam<float> angleLimit = 0.0f; // 0 as stiff as possible
			MeshGenParam<float> alignTowardsNextSegment = 1.0f;

			MeshGenParam<Sphere> randomiseInSphere;

			MeshGenParam<bool> relativeToStartBone = false;
			MeshGenParam<bool> applyVelocity = false;
			MeshGenParam<Vector3> applyAccelerationWS;

			static AppearanceControllerData* create_data();
				
			friend class Tentacle;
		};
	};
};

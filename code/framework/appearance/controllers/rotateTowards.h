#pragma once

#include "..\appearanceController.h"
#include "..\appearanceControllerData.h"

#include "..\socketID.h"

#include "..\..\meshGen\meshGenParam.h"
#include "..\..\sound\soundSource.h"

#include "..\..\..\core\math\math.h"
#include "..\..\..\core\mesh\boneID.h"
#include "..\..\..\core\other\simpleVariableStorage.h"

namespace Framework
{
	namespace AppearanceControllersLib
	{
		class RotateTowardsData;

		/**
		 *	Rotates bone towards specific point in XY plane. It turns around to face with Y axis.
		 *	If can't turn towards, it will try to align, ie. if it's closer to turn exactly away, it will try to do so.
		 */
		class RotateTowards
		: public AppearanceController
		{
			FAST_CAST_DECLARE(RotateTowards);
			FAST_CAST_BASE(AppearanceController);
			FAST_CAST_END();

			typedef AppearanceController base;
		public:
			RotateTowards(RotateTowardsData const * _data);
			virtual ~RotateTowards();

		public: // AppearanceController
			override_ void initialise(ModuleAppearance* _owner);
			override_ void advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context);
			override_ void calculate_final_pose(REF_ AppearanceControllerPoseContext & _context);
			override_ void get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
				OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const;

		protected:
			override_ tchar const * get_name_for_log() const { return TXT("rotate towards"); }
			override_ void internal_log(LogInfoContext & _log) const;

		private:
			RotateTowardsData const * rotateTowardsData;
			Meshes::BoneID bone;
			Meshes::BoneID pointBone;
			Vector3 pointBonePlaneNormalBS = Vector3::zero;
			Vector3 alignYawUsingTargetPlacementsPlaneNormalLS = Vector3::zero;
			Vector3 viewOffsetBS = Vector3::zero;

			Vector3 defaultTargetPointMS;
			
			Vector3 defaultTargetDirMS = Vector3::zero;

			SocketID targetSocket;
			Optional<Vector3> lastTargetLocationOS; // in target's space, we use it relative to the location as well

			Quat rotationSpaceBS = Quat::identity;

			Optional<Transform> lastNonHoldPoseLS;

			// in rotation space
			bool rotateYaw = true;
			bool rotatePitch = false;

			Meshes::BoneID targetBone;
			SimpleVariableInfo targetSocketVar;
			SimpleVariableInfo targetPointMSVar;
			SimpleVariableInfo targetPointMSPtrVar;
			SimpleVariableInfo targetPlacementMSVar;
			SimpleVariableInfo targetPresencePathVar;
			SimpleVariableInfo relativeToPresencePathVar;
			SimpleVariableInfo relativeToPresencePathOffsetOSPtrVar;

			SimpleVariableInfo outputRelativeTargetLocVar;

			Range range;
			bool forwardOrBackward;

			SimpleVariableInfo rotationSpeedVar;
			float rotationSpeed;

			SimpleVariableInfo outputAtTargetVar;

			Name rotationSound;
			float rotationThreshold = 0.0f;
			SoundSourcePtr playedRotationSound;

			Rotator3 lastRotationRequiredAsProvidedRS = Rotator3::zero;
			Rotator3 lastRotationRequiredRS = Rotator3::zero; // blended using active
		};

		class RotateTowardsData
		: public AppearanceControllerData
		{
			FAST_CAST_DECLARE(RotateTowardsData);
			FAST_CAST_BASE(AppearanceControllerData);
			FAST_CAST_END();

			typedef AppearanceControllerData base;
		public:
			static void register_itself();

			RotateTowardsData();

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
			MeshGenParam<Name> pointBone; // point bone tells in which direction do we point, it is useful if we don't want to use our bone's forward dir but some to point with some other bone's relative placement, for example for leg socket, point bone would be leg tip
			MeshGenParam<Vector3> pointBonePlaneNormalBS; // if target point is required to be on point bone's plane, this should be set, useful for legs using ik solvers that are restricted to bend plane
			MeshGenParam<Name> targetBone;
			MeshGenParam<Name> targetSocketVar;
			MeshGenParam<Name> targetPointMSVar; // vector3
			MeshGenParam<Name> targetPointMSPtrVar; // vector3*
			MeshGenParam<Name> targetPlacementMSVar; // placement
			MeshGenParam<Name> targetPresencePathVar; // using path and 
			MeshGenParam<Name> relativeToPresencePathVar;
			MeshGenParam<Name> relativeToPresencePathOffsetOSPtrVar; // vector3* used when relative to presence path points at target placement to offset
			MeshGenParam<Vector3> defaultTargetPointMS = Vector3::yAxis;
			MeshGenParam<Vector3> defaultTargetDirMS; // if provided, overrides where we're looking at
			MeshGenParam<Vector3> alignYawUsingTargetPlacementsPlaneNormalLS; // use plane normal of target placement to calculate yaw - we're not rotating towards point but aligning rotating to face same dir. requires target placement to be orientated properly!

			MeshGenParam<Vector3> viewOffsetBS; // to calculate rotation using this offset in bone's space

			MeshGenParam<Name> rotationSound;
			MeshGenParam<float> rotationThreshold;

			MeshGenParam<Name> outputRelativeTargetLocVar; // relative to bone, rotator3

			// in rotation space
			bool rotateYaw = true;
			bool rotatePitch = false;

			bool lookAtDoorIfRelativeToPresencePathHasNoTarget = false;

			// will be used to build rotation space
			bool pureRotationSpace = false; // with this, rotation space is used only to calculate rotations, is not used to calculate forward dir and snap to it
			MeshGenParam<Vector3> rotationAxisBS; // for yaw, main
			MeshGenParam<Vector3> forwardDirBS;

			MeshGenParam<Range> range = Range::empty;
			MeshGenParam<bool> forwardOrBackward = false; // if true, can turn using either forward or backward (away)

			MeshGenParam<Name> rotationSpeedVar;
			MeshGenParam<float> rotationSpeed = -1.0f; // if at least 0, will use speed to rotate
			
			MeshGenParam<Name> outputAtTargetVar;

			static AppearanceControllerData* create_data();
				
			friend class RotateTowards;
		};
	};
};

#pragma once

#include "..\..\..\framework\appearance\appearanceController.h"
#include "..\..\..\framework\appearance\appearanceControllerData.h"

#include "..\..\..\core\math\math.h"
#include "..\..\..\core\mesh\boneID.h"
#include "..\..\..\core\other\simpleVariableStorage.h"

namespace TeaForGodEmperor
{
	namespace AppearanceControllersLib
	{
		class A_TentacleData;

		class A_Tentacle
		: public Framework::AppearanceController
		{
			FAST_CAST_DECLARE(A_Tentacle);
			FAST_CAST_BASE(Framework::AppearanceController);
			FAST_CAST_END();

			typedef Framework::AppearanceController base;
		public:
			A_Tentacle(A_TentacleData const * _data);
			virtual ~A_Tentacle();

		public: // AppearanceController
			override_ void initialise(Framework::ModuleAppearance* _owner);
			override_ void advance_and_adjust_preliminary_pose(REF_ Framework::AppearanceControllerPoseContext & _context);
			override_ void calculate_final_pose(REF_ Framework::AppearanceControllerPoseContext & _context);
			override_ void get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
				OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const;

		protected:
			virtual tchar const * get_name_for_log() const { return TXT("a_tentacle"); }

		private:
			A_TentacleData const * a_tentacleData;

			Random::Generator rg;

			Name state;
			int stateIdx = NONE;
			Optional<float> stateTimeLeft;

			SimpleVariableInfo tentacleLength;
			SimpleVariableInfo tentacleState;
			SimpleVariableInfo tentacleStateIdx;
			SimpleVariableInfo tentacleStateLength;

			Meshes::BoneID parentBone; // of the whole tentacle
			struct BoneInfo
			{
				Meshes::BoneID bone;
				float length = 0.0f;
				float lengthToPrev = 0.0f;
				float lengthToNext = 0.0f;
				float pt = 0.0f;
				Transform defaultPlacementMS;

				Transform placementMS; // from the last frame

				struct Normal
				{
					float tight = 0.0f;
					float tightTarget = 0.0f;

					float waveOffset = 0.0f;
					float angleStickOffset = 0.0f; // if < 0 then angleStick is inverted
					struct AngleStick
					{
						float stick = 0.0f; // -1 to 1
						float power = 0.0f; // wave or angle stick
					};
					AngleStick angleStickNoPrev;
					AngleStick angleStickWithPrev;
					AngleStick angleStick;

					float angle = 0.0f;

					Transform resultPlacementLS = Transform::identity; // >>
				} normal;

				struct AngryJiggle
				{
					Transform requestedPlacementMS;
					Vector3 requestedLocationMS; // will blend to it

					Transform resultPlacementLS = Transform::identity; // >>
				} angryJiggle;

				struct Angry
				{
					float angle = 0.0f;

					Transform resultPlacementLS = Transform::identity; // >>
				} angry;
			};
			Array<BoneInfo> bones; // the last one is the tip
			
			struct Normal
			{
				float waveT = 0.0f;

				float toRelaxedBlendTime = 0.0f;
				float toTightBlendTime = 0.0f;
				float maxAngleRelaxed = 0.0f;
				float fullAngleTight = 0.0f;

				Range changeTime = Range(0.5f, 1.0f);
				Range tightenTime = Range(0.1f, 0.3f);
				
				float rollChance = 0.0f;
				BlendAdvancer<float> useRoll;

				Optional<float> angleStick;
				float timeToTight = 0.0f;
				float tightenAt = 1.0f;
				float timeToChange = 0.0f;
			} normal;

			struct AngryJiggle
			{
				Range jiggleTime = Range(0.03f, 0.2f);

				float timeToJiggle = 0.0f;
			} angryJiggle;

			struct Angry
			{
				BlendAdvancer<float> useWaveLength;
				BlendAdvancer<float> useWaveSpeed;
				BlendAdvancer<float> useAngleCoef;
				float waveChangeTimeLeft = 0.0f;

				float wavePt = 0.0f; // this is where we are, for the first bone, for following we calculate with wave length

				Range waveLength = Range(0.3f);
				Range waveSpeed = Range(0.3f);
				Range waveChangeTime = Range(0.5f, 2.0f);
				Range angleAlong = Range(0.5f, 2.0f); // the further, the larger the angel
				Range angleCoef = Range(1.0f, 1.0f); // the further, the larger the angel
				float maxAngle = 10.0f;
			} angry;

			struct Folded
			{
				// >= 1.0 - unfolded
				// <= 0.0 - folded
				float fullyFoldedPt = 1.3f;
				float fullyUnfoldedPt = 1.0f;
				float foldedAngle = 60.0f;
				float foldPTSpeed = 0.0f;
			} folded;

			struct Burst
			{
				// 0.0 - hidden
				// 1.0 - open/full
				float pt = 1.0f;
				float targetPt = 1.0f;
				float blendTime = 0.7f;
			} burst;

			BlendAdvancer<Rotator3> angleRotator;
			Rotator3 applyAngleRotator = Rotator3(1.0f, 1.0f, 0.2f);
			Range changeAngleRotatorTime = Range(0.5f, 3.0f);

			Range changePitchTime = Range(0.5f, 8.0f);
			Range pitchRange = Range(-5.0f, 5.0f);
			float changePitchTimeLeft = 0.0f;
			float currPitch = 0.0f;
			float targetPitch = 0.0f;

			Range changeMainRollTime = Range(0.5f, 8.0f);
			Range changeMainRollRange = Range(-5.0f, 5.0f);
			float changeMainRollTimeLeft = 0.0f;
			float currMainRoll = 0.0f;
			float targetMainRoll = 0.0f;

			Range changeAllTime = Range(0.5f, 1.0f);
			float angryJiggleChance = 0.2f;
			float angryJigglePower = 0.2f;
			float angryChance = 0.2f;

			float timeToChangeAll = 0.0f;
			struct AllWeights
			{
				float useNormalWithPrev = 0.0f;
				float useAngryJiggle = 0.0f;
				float useAngry = 0.0f;
			};
			AllWeights targetWeights;
			AllWeights currWeights;
		};

		class A_TentacleData
		: public Framework::AppearanceControllerData
		{
			FAST_CAST_DECLARE(A_TentacleData);
			FAST_CAST_BASE(Framework::AppearanceControllerData);
			FAST_CAST_END();

			typedef Framework::AppearanceControllerData base;
		public:
			static void register_itself();

			A_TentacleData();

		public: // AppearanceControllerData
			override_ bool load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);
			override_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);

			override_ void reskin(Meshes::BoneRenameFunc);
			override_ void apply_mesh_gen_params(Framework::MeshGeneration::GenerationContext const & _context);

			override_ Framework::AppearanceControllerData* create_copy() const;
			override_ Framework::AppearanceController* create_controller();

		private:
			Framework::MeshGenParam<Name> startBone;
			Framework::MeshGenParam<Name> endBone;

			struct Normal
			{
				Framework::MeshGenParam<float> fullAngleTight = 5.0f;
				Framework::MeshGenParam<float> maxAngleRelaxed = 3.0f;
				Framework::MeshGenParam<float> toRelaxedBlendTime = 1.0f;
				Framework::MeshGenParam<float> toTightBlendTime = 0.2f;

				Framework::MeshGenParam<float> angleStickOffsetZeroPt = 0.1f;
				Framework::MeshGenParam<float> angleStickZeroRangePt = 0.2f;
				Framework::MeshGenParam<float> waveLength = 1.5f;

				Framework::MeshGenParam<Range> changeTime = Range(0.5f, 1.0f);
				Framework::MeshGenParam<Range> tightenTime = Range(0.1f, 0.3f);
				
				Framework::MeshGenParam<float> rollChance = 0.5f;
			} normal;

			struct AngryJiggle
			{
				Framework::MeshGenParam<Range> jiggleTime = Range(0.03f, 0.3f);
			} angryJiggle;
			
			struct Angry
			{
				Framework::MeshGenParam<Range> waveLength = Range(0.3f);
				Framework::MeshGenParam<Range> waveSpeed = Range(0.3f);
				Framework::MeshGenParam<Range> waveChangeTime = Range(0.5f, 2.0f);
				Framework::MeshGenParam<Range> angleAlong = Range(0.5f, 2.0f); // the further, the larger the angel
				Framework::MeshGenParam<Range> angleCoef = Range(1.0f, 1.0f);
				Framework::MeshGenParam<float> maxAngle = 10.0f;
			} angry;
			
			Framework::MeshGenParam<Rotator3> applyAngleRotator = Rotator3(1.0f, 1.0f, 0.2f);
			Framework::MeshGenParam<Range> changeAngleRotatorTime = Range(0.5f, 3.0f);

			Framework::MeshGenParam<Range> changeAllTime = Range(0.5f, 1.0f);
			
			Framework::MeshGenParam<Range> changePitchTime = Range(0.5f, 8.0f);
			Framework::MeshGenParam<Range> pitchRange = Range(-5.0f, 5.0f);
			
			Framework::MeshGenParam<Range> changeMainRollTime = Range(0.5f, 8.0f);
			Framework::MeshGenParam<Range> changeMainRollRange = Range(-5.0f, 5.0f);

			Framework::MeshGenParam<float> angryJiggleChance = 0.2f;
			Framework::MeshGenParam<float> angryJigglePower = 0.2f;
			Framework::MeshGenParam<float> angryChance = 0.2f;

			static AppearanceControllerData* create_data();
				
			friend class A_Tentacle;
		};
	};
};

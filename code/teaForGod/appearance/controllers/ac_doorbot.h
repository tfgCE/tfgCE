#pragma once

#include "..\..\..\framework\ai\aiIMessageHandler.h"
#include "..\..\..\framework\appearance\appearanceController.h"
#include "..\..\..\framework\appearance\appearanceControllerData.h"
#include "..\..\..\framework\appearance\socketID.h"
#include "..\..\..\framework\sound\soundSource.h"

#include "..\..\..\core\math\math.h"
#include "..\..\..\core\mesh\boneID.h"

namespace TeaForGodEmperor
{
	namespace AppearanceControllersLib
	{
		class DoorbotData;

		class Doorbot
		: public Framework::AppearanceController
		, public Framework::AI::IMessageHandler
		{
			FAST_CAST_DECLARE(Doorbot);
			FAST_CAST_BASE(Framework::AppearanceController);
			FAST_CAST_END();

			typedef Framework::AppearanceController base;
		public:
			Doorbot(DoorbotData const * _data);
			virtual ~Doorbot();

		public: // AppearanceController
			override_ void initialise(Framework::ModuleAppearance* _owner);
			override_ void advance_and_adjust_preliminary_pose(REF_ Framework::AppearanceControllerPoseContext & _context);
			override_ void calculate_final_pose(REF_ Framework::AppearanceControllerPoseContext & _context);
			override_ void get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
				OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const;

		public: // IMessageHandler
			override_ void handle_message(Framework::AI::Message const& _message);

		protected:
			virtual tchar const * get_name_for_log() const { return TXT("doorbot"); }

		private:
			Meshes::BoneID headBone;
			Meshes::BoneID torsoBone;
			Meshes::BoneID leftLegBone;
			Meshes::BoneID rightLegBone;

			float headLookBlendTime = 0.1f;
			Quat headLook = Quat::identity;

			float walkCycleBlendInTime = 0.1f;
			float walkCycleBlendOutTime = 0.1f;
			float walkCycleLength = 0.5f;
			Range walkCycleLengthRange = Range::empty;
			float walkCycleLengthNominalSpeed = 0.0f;

			Framework::SocketID legPivotSocket;
			float walkCycleLegAngle = 20.0f;
			float walkCycleFootAngle = 20.0f;
			float walkCycleLegUpDist = 0.0f;

			float walkCycleJumpDist = 0.05f;

			float speakYesTime = 0.5f;
			float speakYesAngle = 20.0f;

			float speakNoTime = 0.5f;
			float speakNoAngle = 20.0f;

			float speakBlendInTime = 0.1f;
			float speakBlendOutTime = 0.5f;
			Range3 speakAngles = Range3(Range(5.0f, 10.0f), Range(-10.0f, 10.0f), Range(-5.0f, 5.0f));

			// runtime

			Random::Generator rg;

			float walkCycleActive = 0.0f;
			float walkCyclePt = 0.0f;

			float speakRequestsTime = 0;
			float speakRequestsBlock = 0.0f;

			int speakYesRequests = 0;
			float speakYesActive = 0.0f;
			float speakYesPt = 0.0f;

			int speakNoRequests = 0;
			float speakNoActive = 0.0f;
			float speakNoPt = 0.0f;

			Optional<float> speakTime;
			Quat speakAngle = Quat::identity;
			Quat speakOrientation = Quat::identity;

			struct Sound
			{
				bool playing = false;
				Framework::SoundSourcePtr sound;
			};
			Sound leftLegSound;
			Sound rightLegSound;
		};

		class DoorbotData
		: public Framework::AppearanceControllerData
		{
			FAST_CAST_DECLARE(DoorbotData);
			FAST_CAST_BASE(Framework::AppearanceControllerData);
			FAST_CAST_END();

			typedef Framework::AppearanceControllerData base;
		public:
			static void register_itself();

			DoorbotData();

		public: // AppearanceControllerData
			override_ bool load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);
			override_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);

			override_ void reskin(Meshes::BoneRenameFunc);
			override_ void apply_mesh_gen_params(Framework::MeshGeneration::GenerationContext const & _context);

			override_ Framework::AppearanceControllerData* create_copy() const;
			override_ Framework::AppearanceController* create_controller();

		private:
			Framework::MeshGenParam<Name> headBone;
			Framework::MeshGenParam<Name> torsoBone;
			Framework::MeshGenParam<Name> leftLegBone;
			Framework::MeshGenParam<Name> rightLegBone;

			Framework::MeshGenParam<float> headLookBlendTime = 0.1f;

			Framework::MeshGenParam<float> walkCycleBlendInTime = 0.1f;
			Framework::MeshGenParam<float> walkCycleBlendOutTime = 0.1f;
			Framework::MeshGenParam<float> walkCycleLength = 0.5f;
			Framework::MeshGenParam<Range> walkCycleLengthRange = Range::empty;
			Framework::MeshGenParam<float> walkCycleLengthNominalSpeed = 0.0f;

			Framework::MeshGenParam<Name> legPivotSocket;
			Framework::MeshGenParam<float> walkCycleLegAngle = 20.0f;
			Framework::MeshGenParam<float> walkCycleFootAngle = 20.0f;
			Framework::MeshGenParam<float> walkCycleLegUpDist = 0.0f;
			
			Framework::MeshGenParam<float> walkCycleJumpDist = 0.05f;

			Framework::MeshGenParam<float> speakYesTime = 0.5f;
			Framework::MeshGenParam<float> speakYesAngle = 20.0f;

			Framework::MeshGenParam<float> speakNoTime = 0.5f;
			Framework::MeshGenParam<float> speakNoAngle = 20.0f;

			Framework::MeshGenParam<float> speakBlendInTime = 0.1f;
			Framework::MeshGenParam<float> speakBlendOutTime = 0.5f;
			Framework::MeshGenParam<Range3> speakAngles = Range3(Range(5.0f, 10.0f), Range(-10.0f, 10.0f), Range(-5.0f, 5.0f));

			static AppearanceControllerData* create_data();
				
			friend class Doorbot;
		};
	};
};

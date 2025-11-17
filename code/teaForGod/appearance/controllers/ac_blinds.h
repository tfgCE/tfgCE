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
		class BlindsData;

		/**
		 *	moves blinds along rails
		 */
		class Blinds
		: public Framework::AppearanceController
		{
			FAST_CAST_DECLARE(Blinds);
			FAST_CAST_BASE(Framework::AppearanceController);
			FAST_CAST_END();

			typedef Framework::AppearanceController base;
		public:
			Blinds(BlindsData const * _data);
			virtual ~Blinds();

		public: // AppearanceController
			override_ void initialise(Framework::ModuleAppearance* _owner);
			override_ void advance_and_adjust_preliminary_pose(REF_ Framework::AppearanceControllerPoseContext & _context);
			override_ void calculate_final_pose(REF_ Framework::AppearanceControllerPoseContext & _context);
			override_ void get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
				OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const;

		protected:
			virtual tchar const * get_name_for_log() const { return TXT("blinds"); }

		private:
			BlindsData const * blindsData;
			
			struct Segment
			{
				float length;
				BezierCurve<Vector3> curve; // as curve, even if straight line
				float radius; // 0 for linear
				Vector3 centre; // if radius != 0 / non linear
			};
			FROM_DATA_ AUTO_ Array<Segment> segments;
			AUTO_ Vector3 postSegmentsTangent = Vector3::zero;

			struct BlindBone
			{
				Meshes::BoneID bone;
				Meshes::BoneID mirrored;

				float at; // udpated every frame
				float prevAt;
				float endAt; // distance on curves is >= blindLength

				AUTO_ float startAtDist = 0.0f; // where we start moving
				AUTO_ float stopAtDist = 1.0f; // where we should stop (we will rotate at stopAtDist - rotateDist)

			};
			FROM_DATA_ AUTO_ Array<BlindBone> blinds;

			Meshes::BoneID relativeToBone;
			
			SimpleVariableInfo inOpenVar;
			SimpleVariableInfo outOpenVar;
			
			Name startSound;
			Name loopSound;
			Name stopSound;
			Framework::SoundSourcePtr loopSoundPlayed;

			FROM_DATA_ float speed = 0.5f;
			float atDist = 0.0f;
			FROM_DATA_ float rotateAngle = 10.0f;

			FROM_DATA_ float blindLength = 1.0f;
			FROM_DATA_ float blindThickness = 1.0f;

			AUTO_ float totalLength = 1.0f; // total length
			AUTO_ float openDist = 1.0f; // where is open dist, where first one stops, if not provided, auto calculated
			AUTO_ float rotateDist = 1.0f; // how long (in original distance) does it take to rotate
			AUTO_ float additionalSeparationDist = 0.0f; // holes between moving ones
			AUTO_ float rotatedSeparation = 1.0f; // depends on thickness and angle - they touch

			Vector3 calculate_at(float _at) const;
		};

		class BlindsData
		: public Framework::AppearanceControllerData
		{
			FAST_CAST_DECLARE(BlindsData);
			FAST_CAST_BASE(Framework::AppearanceControllerData);
			FAST_CAST_END();

			typedef Framework::AppearanceControllerData base;
		public:
			static void register_itself();

			BlindsData();

		public: // AppearanceControllerData
			override_ bool load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);
			override_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);

			override_ void reskin(Meshes::BoneRenameFunc);
			override_ void apply_transform(Matrix44 const & _transform);
			override_ void apply_mesh_gen_params(Framework::MeshGeneration::GenerationContext const & _context);

			override_ Framework::AppearanceControllerData* create_copy() const;
			override_ Framework::AppearanceController* create_controller();

		private:
			Framework::MeshGenParam<float> speed;
			Framework::MeshGenParam<float> blindLength; // blind length
			Framework::MeshGenParam<float> blindThickness; // to calc separation dist at folded

			Framework::MeshGenParam<float> rotateAngle = 0.0f;

			Framework::MeshGenParam<Name> relativeToBone;

			Framework::MeshGenParam<Name> inOpenVar;
			Framework::MeshGenParam<Name> outOpenVar;

			Framework::MeshGenParam<Name> startSound;
			Framework::MeshGenParam<Name> loopSound;
			Framework::MeshGenParam<Name> stopSound;

			Framework::MeshGenParam<float> openDist; // if not provided, will auto calculate (read about above)
			Framework::MeshGenParam<float> rotateDist; // if not provided, will auto calculate (read about above)
			Framework::MeshGenParam<float> additionalSeparationDist; // to have holes between blinds when they move

			struct RailCorner
			{
				Framework::MeshGenParam<Vector3> location;
				Framework::MeshGenParam<float> radius = 0.0f;

				bool load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);
				void apply_mesh_gen_params(Framework::MeshGeneration::GenerationContext const & _context);
			};
			Array<RailCorner> rails;

			struct BlindBone
			{
				Framework::MeshGenParam<Name> bone;
				Framework::MeshGenParam<Name> mirrored; // will just place it as it was mirrored (just fwd dir and x)

				bool load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);
				void apply_mesh_gen_params(Framework::MeshGeneration::GenerationContext const & _context);
			};
			Array<BlindBone> blinds;

			static Framework::AppearanceControllerData* create_data();
				
			friend class Blinds;
		};
	};
};

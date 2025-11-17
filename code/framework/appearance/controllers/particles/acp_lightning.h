#pragma once

#include "..\ac_particlesBase.h"

#include "..\..\..\..\core\math\math.h"
#include "..\..\..\..\core\mesh\boneID.h"
#include "..\..\..\..\core\other\simpleVariableStorage.h"

namespace Framework
{
	namespace AppearanceControllersLib
	{
		namespace Particles
		{
			class LightningData;

			/**
			 *	Lightning always is in local space, from 0,0,0 to distance along y axis with up along z axis (to do arcs!)
			 */
			class Lightning
			: public ParticlesBase
			{
				FAST_CAST_DECLARE(Lightning);
				FAST_CAST_BASE(ParticlesBase);
				FAST_CAST_END();

				typedef ParticlesBase base;
			public:
				Lightning(LightningData const * _data);
				virtual ~Lightning();

			public: // AppearanceController
				override_ void initialise(ModuleAppearance* _owner);
				override_ void activate();
				override_ void advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context);
				override_ void calculate_final_pose(REF_ AppearanceControllerPoseContext & _context);
				override_ void get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
					OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const;

			protected:
				virtual tchar const * get_name_for_log() const { return TXT("lightning"); }

			private:
				LightningData const * lightningData;

				bool segmentsSet;

				enum Mode
				{
					Arc,
					Sphere
				};
				Mode mode = Mode::Arc;

				struct Segment
				{
					Meshes::BoneID startBone;
					Meshes::BoneID endBone;
					Meshes::BoneID startBoneParent;
					Meshes::BoneID endBoneParent;
				};
				Array<Segment> segments;

				struct Node
				{
					float t;
					Vector3 location; // local space!
					Vector3 velocity;

					Vector3 finalLoc; // location on arc
				};
				Array<Node> nodes;
				
				float timeToReset = 0.0f;

				// Arc
				float lengthFromData = 0.0f;
				float length = 0.0f; // length for which it was created
				float arc = 0.0f;
				//
				SimpleVariableInfo lengthVar;
				SimpleVariableInfo arcVar;

				// Sphere
				Range radius = Range(1.0f, 1.0f);
				float radiusGrowth = 0.0f;
				float radiusGrowthSpeed = 0.0f;
				//
				SimpleVariableInfo radiusRangeVar;
				SimpleVariableInfo radiusVar;

			};

			class LightningData
			: public ParticlesBaseData
			{
				FAST_CAST_DECLARE(LightningData);
				FAST_CAST_BASE(ParticlesBaseData);
				FAST_CAST_END();

				typedef ParticlesBaseData base;
			public:
				static void register_itself();

				LightningData();

			public: // AppearanceControllerData
				override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
				override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

				override_ void reskin(Meshes::BoneRenameFunc);
				override_ void apply_transform(Matrix44 const & _transform);
				override_ void apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context);

				override_ AppearanceControllerData* create_copy() const;
				override_ AppearanceController* create_controller();

			private:
				MeshGenParam<Name> segmentName; // name0suffix
				MeshGenParam<Name> segmentSuffixStart;
				MeshGenParam<Name> segmentSuffixEnd;

				// Arc
				MeshGenParam<Range> length;
				MeshGenParam<Name> lengthVariable;
				MeshGenParam<Name> arcVariable;

				// Sphere
				MeshGenParam<Range> radius;
				MeshGenParam<Name> radiusVariable;
				MeshGenParam<Range> radiusGrowthSpeed;

				Range resetInterval = Range::empty; // auto reset interval
				bool updateLengthAndArc = false;

				static AppearanceControllerData* create_data();
				
				friend class Lightning;
			};
		};
	};
};

#pragma once

#include "components\acc_keepChildrenWhereTheyWere.h"

#include "..\appearanceController.h"
#include "..\appearanceControllerData.h"

#include "..\..\..\core\math\math.h"
#include "..\..\..\core\mesh\boneID.h"
#include "..\..\..\core\other\simpleVariableStorage.h"

namespace Framework
{
	namespace AppearanceControllersLib
	{
		class AlignAxesData;

		/**
		 *	Using two axes of "bone" it tries to align two axes using two (one would not make sense) different bones
		 */
		class AlignAxes
		: public AppearanceController
		{
			FAST_CAST_DECLARE(AlignAxes);
			FAST_CAST_BASE(AppearanceController);
			FAST_CAST_END();

			typedef AppearanceController base;
		public:
			AlignAxes(AlignAxesData const * _data);
			virtual ~AlignAxes();

		public: // AppearanceController
			override_ void initialise(ModuleAppearance* _owner);
			override_ void advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context);
			override_ void calculate_final_pose(REF_ AppearanceControllerPoseContext & _context);
			override_ void get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
				OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const;

		protected:
			virtual tchar const * get_name_for_log() const { return TXT("align axes"); }

		private:
			AlignAxesData const * alignAxesData;
			Meshes::BoneID bone;
			Meshes::BoneID boneParent;
			float alignAmount = 1.0f;
			struct AxisInfo
			{
				Axis::Type axis;
				Meshes::BoneID bone;
				Vector3 dirMS; // as it is in model space but will be used in bone space
			};
			AxisInfo first;
			AxisInfo second;

			Components::KeepChildrenWhereTheyWere keepChildrenWhereTheyWere;
		};

		class AlignAxesData
		: public AppearanceControllerData
		{
			FAST_CAST_DECLARE(AlignAxesData);
			FAST_CAST_BASE(AppearanceControllerData);
			FAST_CAST_END();

			typedef AppearanceControllerData base;
		public:
			static void register_itself();

			AlignAxesData();

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
			MeshGenParam<float> alignAmount;

			struct AxisInfo
			{
				Axis::Type axis = Axis::X;
				MeshGenParam<Name> bone;
				MeshGenParam<Vector3> dirMS; // in model space, it is taken into current pose

				bool load_from_xml(IO::XML::Node const * _node, tchar const * _childName);
			};

			AxisInfo first;
			AxisInfo second;

#ifdef AN_DEVELOPMENT
			bool debugDraw = false;
#endif

			Components::KeepChildrenWhereTheyWereData keepChildrenWhereTheyWere;

			static AppearanceControllerData* create_data();
				
			friend class AlignAxes;
		};
	};
};

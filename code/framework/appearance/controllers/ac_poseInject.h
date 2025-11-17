#pragma once

#include "..\appearanceController.h"
#include "..\appearanceControllerData.h"

#include "..\..\..\core\mesh\boneID.h"

namespace Framework
{
	namespace AppearanceControllersLib
	{
		class PoseInjectData;

		struct PoseInjectElement
		{
			Name from;
			Name id;

			enum Inject
			{
				Unknown,
				AsBone,
				AsTransformVar,
				AsVector3Var,
				AsFloatVar,
			};
			Inject inject = Inject::Unknown;

			RUNTIME_ Meshes::BoneID parentBone;
			RUNTIME_ Meshes::BoneID asBone;
			RUNTIME_ SimpleVariableInfo asVariable;

			bool load_from_xml(IO::XML::Node const* _node);
		};

		/**
		 *	Reads stuff from pose manager and stores as a bone or a var 
		 */
		class PoseInject
		: public AppearanceController
		{
			FAST_CAST_DECLARE(PoseInject);
			FAST_CAST_BASE(AppearanceController);
			FAST_CAST_END();

			typedef AppearanceController base;
		public:
			PoseInject(PoseInjectData const * _data);
			virtual ~PoseInject();

		public: // AppearanceController
			override_ void initialise(ModuleAppearance* _owner);
			override_ void advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context);
			override_ void calculate_final_pose(REF_ AppearanceControllerPoseContext & _context);
			override_ void get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
				OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const;

		protected:
			virtual tchar const * get_name_for_log() const { return TXT("pose inject"); }

		private:
			SimpleVariableInfo poseManagerPtrVar; // will store safe ptr there

			Array<PoseInjectElement> elements;
		};

		class PoseInjectData
		: public AppearanceControllerData
		{
			FAST_CAST_DECLARE(PoseInjectData);
			FAST_CAST_BASE(AppearanceControllerData);
			FAST_CAST_END();

			typedef AppearanceControllerData base;
		public:
			static void register_itself();

			PoseInjectData();

		public: // AppearanceControllerData
			override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
			override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

			override_ void reskin(Meshes::BoneRenameFunc);
			override_ void apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context);

			override_ AppearanceControllerData* create_copy() const;
			override_ AppearanceController* create_controller();

		private:
			MeshGenParam<Name> poseManagerPtrVar;

			static AppearanceControllerData* create_data();

			Array<PoseInjectElement> elements;

			friend class PoseInject;
		};
	};
};

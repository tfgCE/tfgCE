#pragma once

#include "..\appearanceController.h"
#include "..\appearanceControllerData.h"

#include "..\..\..\core\math\math.h"

namespace Framework
{
	namespace AppearanceControllersLib
	{
		class PoseManagerData;

		struct PoseManagerPose
		{
			struct Element
			{
				Name id;
				Name fromVar;
				enum ValueType
				{
					Unknown,
					Float,
					Vector3,
					Transform,
				};
				ValueType valueType = Unknown;
				float value[8];
				Optional<bool> isDir;
				RUNTIME_ float weight = 0.0f; // for current pose it is how much effectively do we use it (controller's active included)

				::Transform& as_transform() { return *(plain_cast<::Transform>(value)); }
				::Transform const& as_transform() const { return *(plain_cast<::Transform>(value)); }
				::Vector3& as_vector3() { return *(plain_cast<::Vector3>(value)); }
				::Vector3 const& as_vector3() const { return *(plain_cast<::Vector3>(value)); }
				float& as_float() { return *(plain_cast<float>(value)); }
				float const& as_float() const { return *(plain_cast<float>(value)); }

				Element()
				{
					an_assert(sizeof(value) >= sizeof(::Transform));
					an_assert(sizeof(value) >= sizeof(::Vector3));
					an_assert(sizeof(value) >= sizeof(float));
				}
				bool load_from_xml(IO::XML::Node const* _node);
			};
			Name id;
			int priority = 0;
			float active = 0.0f; // how active it is
			SimpleVariableInfo activeVar;
			RUNTIME_ float targetActive = 0.0f; // if controlled by scripts or other means
			RUNTIME_ float targetActiveFromVar = 0.0f; // if controlled with variable

			Array<Element> elements;

			bool load_from_xml(IO::XML::Node const* _node);
		};

		/**
		 *	Blends multiple poses provided with data (or by other means) that can be activated by variables or something else
		 */
		class PoseManager
		: public AppearanceController
		, public SafeObject<PoseManager>
		{
			FAST_CAST_DECLARE(PoseManager);
			FAST_CAST_BASE(AppearanceController);
			FAST_CAST_END();

			typedef AppearanceController base;
		public:
			PoseManager(PoseManagerData const * _data);
			virtual ~PoseManager();

			PoseManagerPose::Element const * get_pose_value(Name const& _id) const;

		public: // AppearanceController
			override_ void initialise(ModuleAppearance* _owner);
			override_ void advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context);
			override_ void calculate_final_pose(REF_ AppearanceControllerPoseContext & _context);
			override_ void get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
				OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const;

		protected:
			virtual tchar const * get_name_for_log() const { return TXT("pose manager"); }

		private:
			SimpleVariableInfo poseManagerPtrVar; // will store safe ptr there
			SimpleVariableInfo activePoseIdVar;
			SimpleVariableInfo activePoseActiveVar;
			SimpleVariableInfo activePoseBlendTimeVar;

			Array<PoseManagerPose> poses;

			PoseManagerPose currentPose; // effective pose (pose's active is ignored, weights have in incorporated, controller's active weight is used)

			RUNTIME_ CACHED_ Array<int> priorities;
			struct CachedElement
			{
				float active;
				PoseManagerPose::Element element;
			};
			RUNTIME_ CACHED_ Array<CachedElement> cachedElements;

			void make_sure_current_pose_is_valid(); // has all elements
		};

		class PoseManagerData
		: public AppearanceControllerData
		{
			FAST_CAST_DECLARE(PoseManagerData);
			FAST_CAST_BASE(AppearanceControllerData);
			FAST_CAST_END();

			typedef AppearanceControllerData base;
		public:
			static void register_itself();

			PoseManagerData();

		public: // AppearanceControllerData
			override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
			override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

			override_ void reskin(Meshes::BoneRenameFunc);
			override_ void apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context);

			override_ AppearanceControllerData* create_copy() const;
			override_ AppearanceController* create_controller();

		private:
			MeshGenParam<Name> poseManagerPtrVar;
			MeshGenParam<Name> activePoseIdVar;
			MeshGenParam<Name> activePoseActiveVar;
			MeshGenParam<Name> activePoseBlendTimeVar;

			static AppearanceControllerData* create_data();

			Array<PoseManagerPose> poses;

			friend class PoseManager;
		};
	};
};

DECLARE_REGISTERED_TYPE(SafePtr<Framework::AppearanceControllersLib::PoseManager>);

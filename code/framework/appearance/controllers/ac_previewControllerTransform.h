#pragma once

#include "..\appearanceController.h"
#include "..\appearanceControllerData.h"

#include "..\..\..\core\math\math.h"
#include "..\..\..\core\mesh\boneID.h"
#include "..\..\..\core\other\simpleVariableStorage.h"

namespace Framework
{
	namespace AppearanceControllersLib
	{
		class PreviewControllerTransformData;

		struct CopyPreviewControllerTransformInstance
		{
			Name controllerTransformId;
			Name toBone;
			SimpleVariableInfo toMSVar;
		};

		struct CopyPreviewControllerTransform
		{
			MeshGenParam<Name> controllerTransformId;
			MeshGenParam<Name> toBone;
			MeshGenParam<Name> toMSVar;
		};

		/**
		 *	
		 */
		class PreviewControllerTransform
		: public AppearanceController
		{
			FAST_CAST_DECLARE(PreviewControllerTransform);
			FAST_CAST_BASE(AppearanceController);
			FAST_CAST_END();

			typedef AppearanceController base;
		public:
			PreviewControllerTransform(PreviewControllerTransformData const * _data);
			virtual ~PreviewControllerTransform();

		public: // AppearanceController
			override_ void initialise(ModuleAppearance* _owner);
			override_ void calculate_final_pose(REF_ AppearanceControllerPoseContext & _context);

		protected:
			virtual tchar const * get_name_for_log() const { return TXT("preview controller transform"); }

		private:
			//PreviewControllerTransformData const * previewControllerTransformData;

			Array<CopyPreviewControllerTransformInstance> copyControllerTransforms;
		};

		class PreviewControllerTransformData
		: public AppearanceControllerData
		{
			FAST_CAST_DECLARE(PreviewControllerTransformData);
			FAST_CAST_BASE(AppearanceControllerData);
			FAST_CAST_END();

			typedef AppearanceControllerData base;
		public:
			static void register_itself();

			PreviewControllerTransformData();

		public: // AppearanceControllerData
			override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);

			override_ void apply_mesh_gen_params(MeshGeneration::GenerationContext const& _context);

			override_ AppearanceControllerData* create_copy() const;
			override_ AppearanceController* create_controller();

		private:
			Array<CopyPreviewControllerTransform> copyControllerTransforms;

			static AppearanceControllerData* create_data();
				
			friend class PreviewControllerTransform;
		};
	};
};

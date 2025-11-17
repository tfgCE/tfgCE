#pragma once

#include "..\..\appearanceController.h"
#include "..\..\appearanceControllerData.h"

namespace Framework
{
	namespace AppearanceControllersLib
	{
		namespace ParticleModifiers
		{
			class ScaleEachData;

			/**
			 *	Scale each particle by a constant value
			 */
			class ScaleEach
			: public AppearanceController
			{
				FAST_CAST_DECLARE(ScaleEach);
				FAST_CAST_BASE(AppearanceController);
				FAST_CAST_END();

				typedef AppearanceController base;
			public:
				ScaleEach(ScaleEachData const * _data);

			public: // AppearanceController
				override_ void calculate_final_pose(REF_ AppearanceControllerPoseContext & _context);

			protected:
				virtual tchar const * get_name_for_log() const { return TXT("particle modifier / scale each"); }

			private:
				ScaleEachData const * scaleEachData;
			};

			class ScaleEachData
			: public AppearanceControllerData
			{
				FAST_CAST_DECLARE(ScaleEachData);
				FAST_CAST_BASE(AppearanceControllerData);
				FAST_CAST_END();

				typedef AppearanceControllerData base;
			public:
				static void register_itself();

				ScaleEachData();

			public: // AppearanceControllerData
				override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);

				override_ AppearanceControllerData* create_copy() const;
				override_ AppearanceController* create_controller();

			private:
				float scale = 1.0f;

				static AppearanceControllerData* create_data();
				
				friend class ScaleEach;
			};

		};
	};
};

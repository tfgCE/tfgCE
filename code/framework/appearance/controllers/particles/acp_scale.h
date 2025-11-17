#pragma once

#include "..\ac_particlesBase.h"

namespace Framework
{
	namespace AppearanceControllersLib
	{
		namespace Particles
		{
			class ScaleData;

			/**
			 *	Particles stay until told to deactivate. Should be continuous, by default is continuous.
			 */
			class Scale
			: public ParticlesBase
			{
				FAST_CAST_DECLARE(Scale);
				FAST_CAST_BASE(ParticlesBase);
				FAST_CAST_END();

				typedef ParticlesBase base;
			public:
				Scale(ScaleData const * _data);

			public: // AppearanceController
				override_ void activate();
				override_ void advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context);

			protected:
				virtual tchar const * get_name_for_log() const { return TXT("particles / scale"); }

			public:
				override_ void desire_to_deactivate();

			private:
				ScaleData const * scaleData;

				bool shouldDeactivate = false;
				float scale = 0.0f;
			};

			class ScaleData
			: public ParticlesBaseData
			{
				FAST_CAST_DECLARE(ScaleData);
				FAST_CAST_BASE(ParticlesBaseData);
				FAST_CAST_END();

				typedef ParticlesBaseData base;
			public:
				static void register_itself();

				ScaleData();

			public: // AppearanceControllerData
				override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);

				override_ AppearanceControllerData* create_copy() const;
				override_ AppearanceController* create_controller();

			private:
				float scale = 1.0f;
				float scaleUpTime = 0.5f;
				float scaleDownTime = 0.5f;

				static AppearanceControllerData* create_data();
				
				friend class Scale;
			};

		};
	};
};

#pragma once

#include "..\ac_particlesBase.h"

namespace Framework
{
	namespace AppearanceControllersLib
	{
		namespace Particles
		{
			class DissolveData;

			/**
			 *	Particles stay until told to deactivate. Should be continuous, by default is continuous.
			 */
			class Dissolve
			: public ParticlesBase
			{
				FAST_CAST_DECLARE(Dissolve);
				FAST_CAST_BASE(ParticlesBase);
				FAST_CAST_END();

				typedef ParticlesBase base;
			public:
				Dissolve(DissolveData const * _data);

			public: // AppearanceController
				override_ void activate();
				override_ void advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context);

			protected:
				virtual tchar const * get_name_for_log() const { return TXT("particles / dissolve"); }

			public:
				override_ void desire_to_deactivate();

			private:
				DissolveData const * dissolveData;

				bool shouldDeactivate = false;
				float visible = 0.0f;
			};

			class DissolveData
			: public ParticlesBaseData
			{
				FAST_CAST_DECLARE(DissolveData);
				FAST_CAST_BASE(ParticlesBaseData);
				FAST_CAST_END();

				typedef ParticlesBaseData base;
			public:
				static void register_itself();

				DissolveData();

			public: // AppearanceControllerData
				override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);

				override_ AppearanceControllerData* create_copy() const;
				override_ AppearanceController* create_controller();

			private:
				float appearTime = 0.0f;
				float dissolveTime = 0.5f;

				static AppearanceControllerData* create_data();
				
				friend class Dissolve;
			};

		};
	};
};

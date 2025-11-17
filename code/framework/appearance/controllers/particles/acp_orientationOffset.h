#pragma once

#include "..\ac_particlesBase.h"

namespace Framework
{
	namespace AppearanceControllersLib
	{
		namespace Particles
		{
			class OrientationOffsetData;

			/**
			 *	Particles stay until told to deactivate. Should be continuous, by default is continuous.
			 */
			class OrientationOffset
			: public ParticlesBase
			{
				FAST_CAST_DECLARE(OrientationOffset);
				FAST_CAST_BASE(ParticlesBase);
				FAST_CAST_END();

				typedef ParticlesBase base;
			public:
				OrientationOffset(OrientationOffsetData const * _data);

			public: // AppearanceController
				override_ void activate();
				override_ void calculate_final_pose(REF_ AppearanceControllerPoseContext& _context);

			protected:
				virtual tchar const * get_name_for_log() const { return TXT("particles / orientation offset"); }

			public:
				override_ void desire_to_deactivate();

			private:
				OrientationOffsetData const * orientationOffsetData;

				Random::Generator rg;

				bool shouldDeactivate = false;
				Array<Quat> orientationOffsets;
			};

			class OrientationOffsetData
			: public ParticlesBaseData
			{
				FAST_CAST_DECLARE(OrientationOffsetData);
				FAST_CAST_BASE(ParticlesBaseData);
				FAST_CAST_END();

				typedef ParticlesBaseData base;
			public:
				static void register_itself();

				OrientationOffsetData();

			public: // AppearanceControllerData
				override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);

				override_ AppearanceControllerData* create_copy() const;
				override_ AppearanceController* create_controller();

			private:
				Range pitch = Range::zero;
				Range yaw = Range::zero;
				Range roll = Range::zero;

				static AppearanceControllerData* create_data();
				
				friend class OrientationOffset;
			};

		};
	};
};

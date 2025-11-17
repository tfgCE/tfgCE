#pragma once

#include "..\..\appearanceController.h"
#include "..\..\appearanceControllerData.h"

namespace Framework
{
	namespace AppearanceControllersLib
	{
		namespace ParticleModifiers
		{
			class IntensityToNumberData;

			/**
			 *	Scale each particle by a constant value
			 */
			class IntensityToNumber
			: public AppearanceController
			{
				FAST_CAST_DECLARE(IntensityToNumber);
				FAST_CAST_BASE(AppearanceController);
				FAST_CAST_END();

				typedef AppearanceController base;
			public:
				IntensityToNumber(IntensityToNumberData const * _data);

			public: // AppearanceController
				override_ void initialise(ModuleAppearance* _owner);
				override_ void calculate_final_pose(REF_ AppearanceControllerPoseContext & _context);
				override_ void get_info_for_auto_processing_order(OUT_ ArrayStack<int>& dependsOnBones, OUT_ ArrayStack<int>& dependsOnParentBones, OUT_ ArrayStack<int>& usesBones, OUT_ ArrayStack<int>& providesBones,
					OUT_ ArrayStack<Name>& dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const;

			protected:
				virtual tchar const * get_name_for_log() const { return TXT("particle modifier / scale each"); }

			private:
				IntensityToNumberData const * intensityToNumberData;

				float intensity = 0.0f;

				SimpleVariableInfo intensityVar;
				float minIntensity = 0.0f;
				Optional<float> maxIntensity;
				float intensityBlendTime = 0.0f;
			};

			class IntensityToNumberData
			: public AppearanceControllerData
			{
				FAST_CAST_DECLARE(IntensityToNumberData);
				FAST_CAST_BASE(AppearanceControllerData);
				FAST_CAST_END();

				typedef AppearanceControllerData base;
			public:
				static void register_itself();

				IntensityToNumberData();

			public: // AppearanceControllerData
				override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);

				override_ void apply_mesh_gen_params(MeshGeneration::GenerationContext const& _context);

				override_ AppearanceControllerData* create_copy() const;
				override_ AppearanceController* create_controller();

			private:
				static AppearanceControllerData* create_data();
				
				MeshGenParam<Name> intensityVar;
				MeshGenParam<float> minIntensity = 0.0f;
				MeshGenParam<float> maxIntensity;
				MeshGenParam<float> intensityBlendTime = 0.5f;

				friend class IntensityToNumber;
			};

		};
	};
};

#pragma once

#include "..\appearanceController.h"
#include "..\appearanceControllerData.h"

#include "..\..\interfaces\presenceObserver.h"

#include "..\..\..\core\math\math.h"
#include "..\..\..\core\mesh\boneID.h"
#include "..\..\..\core\other\simpleVariableStorage.h"

namespace Framework
{
	namespace AppearanceControllersLib
	{
		class ParticlesBaseData;

		/**
		 *	
		 */
		class ParticlesBase
		: public AppearanceController
		{
			FAST_CAST_DECLARE(ParticlesBase);
			FAST_CAST_BASE(AppearanceController);
			FAST_CAST_END();

			typedef AppearanceController base;
		public:
			ParticlesBase(ParticlesBaseData const * _data);
			virtual ~ParticlesBase();

			virtual void desire_to_deactivate(); // by default it deactivates!

		protected:
			void particles_deactivate();

			bool are_particles_active() const { return particlesActive; }

		public: // AppearanceController
			override_ void initialise(ModuleAppearance* _owner);
			override_ void activate();

			override_ void advance_and_adjust_preliminary_pose(REF_ Framework::AppearanceControllerPoseContext & _context);

		protected:
			Random::Generator rg;

		private:
			ParticlesBaseData const * particlesBaseData;
			bool particlesActive = false;

			Optional<float> timeToLive;

			SimpleVariableInfo deactivateAtVar;
		};

		class ParticlesBaseData
		: public AppearanceControllerData
		{
			FAST_CAST_DECLARE(ParticlesBaseData);
			FAST_CAST_BASE(AppearanceControllerData);
			FAST_CAST_END();

			typedef AppearanceControllerData base;
		public:
			Optional<Range> const & get_time_to_live() const { return timeToLive; }

		public: // AppearanceControllerData
			override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
			override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

			override_ void apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context);

		protected:
			friend class ParticlesBase;

			bool deactivateIfNotAttached = false;

			Optional<Range> timeToLive;

			Framework::MeshGenParam<Name> deactivateAtVar;

			static AppearanceControllerData* create_data();
		};
	};
};

#pragma once

#include "ac_particlesMeshParticles.h"

namespace Framework
{
	namespace AppearanceControllersLib
	{
		class ParticlesContinuousData;

		/**
		*
		*/
		class ParticlesContinuous
		: public ParticlesMeshParticles
		{
			FAST_CAST_DECLARE(ParticlesContinuous);
			FAST_CAST_BASE(ParticlesMeshParticles);
			FAST_CAST_END();

			typedef ParticlesMeshParticles base;
		public:
			ParticlesContinuous(ParticlesContinuousData const * _data);

		public: // AppearanceController
			override_ void activate();
			override_ void advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context);

		public:
			override_ void desire_to_deactivate();

		protected:
			bool is_continuous_now() const { return continuousForLeft > 0.0f || continuous; }

		protected:
			ParticlesContinuousData const * particlesContinuousData;

			float continuousForLeft = 0.0f;
			bool continuous = false;
		};

		class ParticlesContinuousData
		: public ParticlesMeshParticlesData
		{
			FAST_CAST_DECLARE(ParticlesContinuousData);
			FAST_CAST_BASE(ParticlesMeshParticlesData);
			FAST_CAST_END();

			typedef ParticlesMeshParticlesData base;
		public:
			static void register_itself();

			ParticlesContinuousData();

			float calculate_dissolve(float _timeAlive, float _totalTime) const;
			float calculate_scale(float _timeAlive, float _totalTime) const;
			float calculate_full_scale(float _timeAlive, float _totalTime) const;
			float calculate_alpha(float _timeAlive, float _totalTime) const;

		public: // AppearanceControllerData
			override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);

			override_ AppearanceControllerData* create_copy() const;

		protected:
			Range continuousFor = Range(0.0f, 0.0f);
			bool continuous = false;

			struct ChangeOverTime
			{
				float delayTime = 0.0f;
				float attackTime = 0.0f;
				float attackPt = 0.0f;
				float decayTime = 0.0f;
				float decayPt = 0.0f;

				bool load_from_xml(IO::XML::Node const * _node, tchar const * _prefix);

				float calculate(float _timeAlive, float _totalTime, bool _useAttack = true, bool _useDecay = true) const;
			};
			ChangeOverTime changeScale;
			ChangeOverTime extraScale;
			float extraScaleAmount = 0.0f;
			ChangeOverTime changeAlpha;
			ChangeOverTime changeSolid; // opposite of dissolved
			bool dissolvedInsteadOfScaleDown = false;
			bool dissolvedWithScaleDown = false;
			float dissolveBase = 0.0f; // base value (when not dissolved at all)
			float dissolveExp = 1.0f; // exponent value for dissolve (1.0 - diss), the higher the value the quicker it will fade away, < 1 makes it fade slower

			static AppearanceControllerData* create_data();

			friend class ParticlesContinuous;
		};

	};
};

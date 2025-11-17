#pragma once

#include "..\moduleCustom.h"

#include "..\..\appearance\lightSource.h"

#include "..\..\meshGen\meshGenParam.h"

namespace Framework
{
	namespace CustomModules
	{
		class LightSourcesData;

		struct NamedLightSource
		{
			Name id;
			bool autoPlay = false;

			Name disallowOnVar;
			Name getColourFromVar;
			LightSource requested;
			MeshGenParam<Vector3> location;
			MeshGenParam<Vector3> velocity;
			MeshGenParam<float> scale;
			Name locationFromSocket;
			RUNTIME_ Optional<Vector3> useLocation;
			RUNTIME_ Optional<Vector3> useVelocity;
			RUNTIME_ Optional<float> useScale;
			RUNTIME_ LightSource current;
			RUNTIME_ float timeAlive = 0.0f;
			Optional<float> fadeInTime;
			Optional<float> sustainTime;
			bool explicitFadeOutRequired = false;
			bool fadeOutOnParticleDeactivate = false;
			Optional<float> fadeOutTime;
			Optional<Range> pulsePeriod; // period between pulses
			Optional<Range> pulsePower;
			Optional<Range> pulseBlendTime; // how fast do we blend
			RUNTIME_ float pulseTimeLeft; // period between pulses
			RUNTIME_ float pulseCurrentPower;
			RUNTIME_ float pulseTargetPower;
			RUNTIME_ float pulseCurrentBlendTime; // how fast do we blend
			Optional<Range> flickerPeriod;
			Optional<Range> flickerPower;
			Optional<Vector3> flickerOffset;
			RUNTIME_ float state = 0.0f;
			RUNTIME_ float flickerTimeLeft = 0.0f;
			RUNTIME_ float flickerCurrentPower = 0.0f;
			RUNTIME_ Vector3 flickerCurrentOffset = Vector3::zero;

			bool load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc);
		};

		class LightSources
		: public ModuleCustom
		{
			FAST_CAST_DECLARE(LightSources);
			FAST_CAST_BASE(ModuleCustom);
			FAST_CAST_END();

			typedef Framework::ModuleCustom base;
		public:
			static Framework::RegisteredModule<Framework::ModuleCustom> & register_itself();

			LightSources(Framework::IModulesOwner* _owner);
			virtual ~LightSources();

			void add(Name const& _id, LightSource const& _lightSource, Optional<Name> const & _getColourFromVar,
				Optional<float> const& _fadeInTime, Optional<float> const& _sustainTime, Optional<float> const& _fadeOutTime);
			void add(NamedLightSource const& _lightSource, bool _explicitFadeOutRequired = false);
			void add(Name const& _id, bool _explicitFadeOutRequired = false); // from data
			void fade_out(Name const& _id, Optional<float> const & _fadeOutTime = NP);
			void fade_out_all_on_particles_deactivation(Optional<float> const & _fadeOutTime = NP);
			void remove(Name const& _id, Optional<float> const& _fadeOutTime = NP); // will invalidate id
			bool has_active(Name const& _id) const;

			void for_every_light_source(std::function<void(LightSource const& _ls)> _do) const;

			NamedLightSource* access(Name const& _id); // note that we should not remove/add at this point!
			void mark_requires_update();

			bool has_any_light_source_active() const;

		public:	// Module
			override_ void reset();
			override_ void activate();
			override_ void setup_with(::Framework::ModuleData const * _moduleData);

			override_ void initialise();

		public: // ModuleCustom
			override_ void advance_post(float _deltaTime);

		private:
			LightSourcesData const* lightSourcesData = nullptr;

			Random::Generator rg;

			mutable Concurrency::SpinLock lightSourcesLock = Concurrency::SpinLock(TXT("Framework.CustomModules.LightSources.lightSourcesLock"));
			ArrayStatic<NamedLightSource, 8> lightSources;

			void update(float _deltaTime);

			void internal_fade_out(Name const& _id, bool _clearId, Optional<float> const& _fadeOutTime = NP);

		};

		class LightSourcesData
		: public ModuleData
		{
			FAST_CAST_DECLARE(LightSourcesData);
			FAST_CAST_BASE(ModuleData);
			FAST_CAST_END();
			typedef ModuleData base;
		public:
			LightSourcesData(LibraryStored* _inLibraryStored);

		public: // ModuleData
			override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
			override_ void prepare_to_unload();

		protected: // ModuleData
			override_ bool read_parameter_from(IO::XML::Attribute const * _attr, LibraryLoadingContext & _lc);
			override_ bool read_parameter_from(IO::XML::Node const * _node, LibraryLoadingContext & _lc);

		protected:	friend class LightSources;
			Array<NamedLightSource> lightSources;
		};
	};
};


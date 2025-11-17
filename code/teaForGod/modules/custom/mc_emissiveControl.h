#pragma once

#include "..\..\..\framework\module\moduleCustom.h"
#include "..\..\..\core\types\colour.h"

namespace TeaForGodEmperor
{
	namespace CustomModules
	{
		class EmissiveControl;
		class EmissiveControlData;

		namespace EmissiveMode
		{
			enum Type
			{
				Normal,
				Pulse,
				Square,
				Random
			};

			inline Type parse(String const & _text, Type _default = Normal)
			{
				if (_text.is_empty()) return _default;
				if (_text == TXT("pulse")) return Pulse;
				if (_text == TXT("square")) return Square;
				if (_text == TXT("normal")) return Normal;
				if (_text == TXT("random")) return Random;
				error(TXT("emissive mode \"%S\" not recognised"), _text.to_char());
				return Normal;
			}
		};

		struct EmissiveLayer
		{
		public:
			EmissiveLayer() {}
			EmissiveLayer& material_index(int _materialIdx = NONE) { materialIdx =  _materialIdx; return *this; }
			EmissiveLayer& packed_array_index(int _packedArrayIdx) { packedArrayIdx =  _packedArrayIdx; return *this; }
			EmissiveLayer& set_colour(Colour const & _colour, Optional<float> const & _colourBlendTime = NP) { colour = _colour; if (_colourBlendTime.is_set()) { colourBlendTime = _colourBlendTime.get(); } return *this; }
			EmissiveLayer& set_base_colour(Colour const & _baseColour) { baseColour = _baseColour; return *this; }
			EmissiveLayer& set_power(float const & _power, Optional<float> const & _powerBlendTime = NP) { power = _power; if (_powerBlendTime.is_set()) { powerBlendTime = _powerBlendTime.get(); } return *this; }
			EmissiveLayer& set_period(float const & _period) { period = _period; return *this; }
			EmissiveLayer& activate(Optional<float> const & _activeBlendTime = NP) { active = 1.0f; currentActivateBlendTime = _activeBlendTime.get(activateBlendTime); return *this; }
			EmissiveLayer& deactivate(Optional<float> const & _activeBlendTime = NP) { active = 0.0f; currentDeactivateBlendTime = _activeBlendTime.get(deactivateBlendTime); return *this; }
			EmissiveLayer& keep_deactivated() { removeOnDeactivated = false; return *this; }
			EmissiveLayer& remove_on_deactivated() { removeOnDeactivated = true; return *this; }

			EmissiveLayer& keep_colour() { affectColour = false; return *this; }
			EmissiveLayer& keep_power() { affectPower = false; return *this; }

			EmissiveLayer& flat() { mode = EmissiveMode::Normal; return *this; }
			EmissiveLayer& pulse(float _period, float _minLevelPct = 0.0f) { mode = EmissiveMode::Pulse;  period = _period; minActivePct = _minLevelPct; return *this; }
			EmissiveLayer& square(float _period, float _minLevelPct = 0.0f) { mode = EmissiveMode::Square;  period = _period; minActivePct = _minLevelPct; return *this; }
			EmissiveLayer& random(float _period, float _minLevelPct = 0.0f) { mode = EmissiveMode::Random;  period = _period; minActivePct = _minLevelPct; return *this; }

		private:
			Name name;
			int priority = 0;

			int materialIdx = NONE; // if none, it is for all material indices
			int packedArrayIdx = 0; // if packed array in use
			float active = 1.0f;
			Colour colour = Colour::white;
			Colour baseColour = Colour::black;
			float power = 1.0f;
			bool affectColour = true;
			bool affectPower = true;
			float period = 1.0f; // for pulse
			float minActivePct = 0.0f; // how low can pulse go
			float activateBlendTime = 0.2f;
			float currentActivateBlendTime = 0.2f;
			float deactivateBlendTime = 0.2f;
			float currentDeactivateBlendTime = 0.2f;
			float colourBlendTime = 0.2f;
			float powerBlendTime = 0.2f;
			Optional<float> autoDeactivateTime;
			bool removeOnDeactivated = false; // always keep by default
			EmissiveMode::Type mode = EmissiveMode::Normal;

			// running values
			float activeTime = 0.0f;
			bool requiresAdvancement = false;
			Colour actualColour = Colour::white;
			Colour actualBaseColour = Colour::black;
			float actualPower = 1.0f;
			float pt = 0.0f;
			float randomPt = 0.1f;
			float actualActive = 0.0f;// if active, we have to blend colour and power

			bool does_require_advancement() const { return requiresAdvancement; }
			void advance(float _deltaTime);
			void apply(REF_ Colour & _colour, REF_ Colour & _baseColour, REF_ float & _power) const;
			void be_actual();

			bool load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);

			friend class EmissiveControl;
			friend class EmissiveControlData;
		};

		class EmissiveControl
		: public Framework::ModuleCustom
		{
			FAST_CAST_DECLARE(EmissiveControl);
			FAST_CAST_BASE(Framework::ModuleCustom);
			FAST_CAST_END();

			typedef Framework::ModuleCustom base;
		public:
			static Framework::RegisteredModule<Framework::ModuleCustom> & register_itself();

			EmissiveControl(Framework::IModulesOwner* _owner);
			virtual ~EmissiveControl();

			void set_colour(Colour const & _targetColour, float _blendTime);
			void set_default_colour(float _blendTime) { set_colour(defaultColour, _blendTime); }
			void set_base_colour(Colour const & _targetBaseColour);
			void set_default_base_colour() { set_base_colour(defaultBaseColour); }
			void set_power(float _targetPower, float _blendTime);
			void set_default_power(float _blendTime) { set_power(defaultPower, _blendTime); }

			EmissiveLayer & emissive_access(Name const & _layerName, int _priority = 0);
			bool has_emissive(Name const & _layerName) const;
			void emissive_remove(Name const & _layerName);
			void emissive_activate(Name const & _layerName, Optional<float> const & _blendTime = NP);
			void emissive_deactivate(Name const & _layerName, Optional<float> const & _blendTime = NP);
			void emissive_set_active(Name const& _layerName, bool _active, Optional<float> const& _blendTime = NP) { if (_active) emissive_activate(_layerName, _blendTime); else emissive_deactivate(_layerName, _blendTime); }
			void emissive_deactivate_all(Optional<float> const & _blendTime = NP);

		public:	// Module
			override_ void reset();
			override_ void setup_with(::Framework::ModuleData const * _moduleData);

			override_ void initialise();

		protected: // ModuleCustom
			override_ void advance_post(float _deltaTime);

		protected:
			EmissiveControlData const * emissiveControlData = nullptr;
			Name colourShaderParam;
			Name baseColourShaderParam;
			Name powerShaderParam;
			Name packedArrayShaderParam; // an array of vec4, *3, +0 colour, +1 base colour +2.x power, is used with inEmissiveIndex and distributed
			Colour colour = Colour::white;
			Colour targetColour = Colour::white;
			Colour defaultColour = Colour::white;
			Colour baseColour = Colour::black;
			Colour targetBaseColour = Colour::black;
			Colour defaultBaseColour = Colour::black;
			float colourBlendTime = 0.0f;
			float power = 0.0f;
			float targetPower = 0.0f;
			float defaultPower = 0.0f;
			float powerBlendTime = 0.0f;
			bool fromLight = false;

			float lightSourcePower = 1.0f;

			Array<EmissiveLayer> layerStack; // stack works on top of actual colour, overriding it, if in use, always requires advance_post

			struct SeparateMaterialActual
			{
				bool inUse = false;
				Colour colour;
				Colour baseColour;
				float power;
			};
			Array<SeparateMaterialActual> separateMaterialActuals; // is also used for "packed array"
		};

		class EmissiveControlData
		: public Framework::ModuleData
		{
			FAST_CAST_DECLARE(EmissiveControlData);
			FAST_CAST_BASE(Framework::ModuleData);
			FAST_CAST_END();

			typedef Framework::ModuleData base;
		public:
			EmissiveControlData(Framework::LibraryStored* _inLibraryStored);
			virtual ~EmissiveControlData();

		protected: // Framework::ModuleData
			override_ bool read_parameter_from(IO::XML::Attribute const * _attr, Framework::LibraryLoadingContext & _lc);
			override_ bool read_parameter_from(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);

		private:
			Name asLightSource; // will work as light source of the same name
			int asLightSourceFromMaterialIdx = NONE;
			Array<EmissiveLayer> layers;

			friend class EmissiveControl;
		};
	};
};


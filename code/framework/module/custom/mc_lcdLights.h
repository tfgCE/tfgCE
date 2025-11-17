#pragma once

#include "..\moduleCustom.h"
#include "..\..\video\material.h"

namespace Framework
{
	namespace CustomModules
	{
		class LCDLights
		: public ModuleCustom
		{
			FAST_CAST_DECLARE(LCDLights);
			FAST_CAST_BASE(ModuleCustom);
			FAST_CAST_END();

			typedef Framework::ModuleCustom base;
		public:
			static Framework::RegisteredModule<Framework::ModuleCustom> & register_itself();

			LCDLights(Framework::IModulesOwner* _owner);
			virtual ~LCDLights();

		public:
			void set_light(int _idx, Colour const& _colour, float _power, float _blendTime = 0.0f, bool _silentUpdate = false);
			void on_update();

		public:	// Module
			override_ void reset();
			override_ void activate();
			override_ void setup_with(::Framework::ModuleData const * _moduleData);

			override_ void initialise();

		public: // ModuleCustom
			override_ void advance_post(float _deltaTime);

		private:
			Name lcdColourParamName; // lcdColours
			Name lcdPowerParamName; // lcdPower

			Concurrency::SpinLock lightsLock = Concurrency::SpinLock(TXT("Framework.CustomModules.LCDLights.lightsLock"));
			struct Light
			{
				float power = 0.0f;
				Colour colour = Colour::black;
				float targetPower = 0.0f;
				Colour targetColour = Colour::black;

				float blendTime = 0.0f;
			};
			Array<Light> lights;
			int lightCount = 0;

			Optional<int> materialIndex; // if not set, to all

			Name lightColourParamName;
			Name lightPowerParamName;
			Array<float> powerParams;
			Array<Colour> colourParams;
			Concurrency::SpinLock paramsLock = Concurrency::SpinLock(TXT("Framework.CustomModules.LCDLights.paramsLock"));

			void update_blend(float _deltaTime);
		};
	};
};


#include "mc_lcdLights.h"

#include "..\modules.h"
#include "..\moduleDataImpl.inl"
#include "..\moduleAppearanceImpl.inl"

#include "..\..\library\library.h"

//
 
#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace CustomModules;

//

// module params
DEFINE_STATIC_NAME(materialIndex);
DEFINE_STATIC_NAME(colourParamName);
DEFINE_STATIC_NAME(powerParamName);
DEFINE_STATIC_NAME(lightCount);

// material param names
DEFINE_STATIC_NAME(lcdColours);
DEFINE_STATIC_NAME(lcdPower);

//

REGISTER_FOR_FAST_CAST(CustomModules::LCDLights);

static Framework::ModuleCustom* create_module(Framework::IModulesOwner* _owner)
{
	return new CustomModules::LCDLights(_owner);
}

Framework::RegisteredModule<Framework::ModuleCustom> & CustomModules::LCDLights::register_itself()
{
	return Framework::Modules::custom.register_module(String(TXT("lcdLights")), create_module);
}

CustomModules::LCDLights::LCDLights(Framework::IModulesOwner* _owner)
: base(_owner)
, lightColourParamName(NAME(lcdColours))
, lightPowerParamName(NAME(lcdPower))
{
}

CustomModules::LCDLights::~LCDLights()
{
}

void CustomModules::LCDLights::reset()
{
	base::reset();
}

void CustomModules::LCDLights::activate()
{
	base::activate();
	
	on_update();
}

void CustomModules::LCDLights::initialise()
{
	base::initialise();
}

void CustomModules::LCDLights::setup_with(Framework::ModuleData const * _moduleData)
{
	base::setup_with(_moduleData);
	if (_moduleData)
	{
		int tempMaterialIndex = _moduleData->get_parameter<int>(this, NAME(materialIndex), NONE);
		if (tempMaterialIndex != NONE)
		{
			materialIndex = tempMaterialIndex;
		}
		lightCount = _moduleData->get_parameter<int>(this, NAME(lightCount), lightCount);

		lightColourParamName = _moduleData->get_parameter<Name>(this, NAME(colourParamName), lightColourParamName);
		lightPowerParamName = _moduleData->get_parameter<Name>(this, NAME(powerParamName), lightPowerParamName);


		lights.set_size(lightCount);
	}

	on_update();
}

void CustomModules::LCDLights::set_light(int _idx, Colour const& _colour, float _power, float _blendTime, bool _silentUpdate)
{
	{
		Concurrency::ScopedSpinLock lock(lightsLock);

		if (lights.is_index_valid(_idx))
		{
			auto& l = lights[_idx];
			l.targetPower = _power;
			l.targetColour = _colour;
			l.blendTime = _blendTime;
		}

		if (!_silentUpdate)
		{
			on_update();
		}
	}
}

void CustomModules::LCDLights::on_update()
{
	mark_requires(all_customs__advance_post);
}

void CustomModules::LCDLights::advance_post(float _deltaTime)
{
	update_blend(_deltaTime);
}

void CustomModules::LCDLights::update_blend(float _deltaTime)
{
	bool stillRequiresPost = false;
	{
		Concurrency::ScopedSpinLock lock(lightsLock);

		for_every(l, lights)
		{
			if (l->colour != l->targetColour ||
				l->power != l->targetPower)
			{
				stillRequiresPost = true;
				l->colour = blend_to_using_time(l->colour, l->targetColour, l->blendTime, _deltaTime);
				l->power = blend_to_using_time(l->power, l->targetPower, l->blendTime, _deltaTime);
			}
		}
	}

	{
		Concurrency::ScopedSpinLock lock(paramsLock);

		if (powerParams.get_size() < lights.get_size())
		{
			powerParams.make_space_for(lights.get_size());
			while (powerParams.get_size() < lights.get_size())
			{
				powerParams.push_back(0.0f);
			}
		}
		else
		{
			powerParams.set_size(lights.get_size());
		}
		if (colourParams.get_size() < lights.get_size())
		{
			colourParams.make_space_for(lights.get_size());
			while (colourParams.get_size() < lights.get_size())
			{
				colourParams.push_back(Colour::black.with_alpha(0.0f));
			}
		}
		else
		{
			colourParams.set_size(lights.get_size());
		}

		{
			auto* pp = powerParams.begin();
			auto* cp = colourParams.begin();
			for_every(l, lights)
			{
				*pp = l->power;
				*cp = l->colour;
				++pp;
				++cp;
			}
		}

		for_every_ptr(appearance, get_owner()->get_appearances())
		{
			appearance->set_shader_param(lightPowerParamName, powerParams, materialIndex.get(NONE));
			appearance->set_shader_param(lightColourParamName, colourParams, materialIndex.get(NONE));
		}
	}

	if (!stillRequiresPost)
	{
		mark_no_longer_requires(all_customs__advance_post);
	}
}

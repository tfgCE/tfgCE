#include "mc_emissiveControl.h"

#include "..\..\utils.h"

#include "..\..\game\playerSetup.h"

#include "..\..\..\framework\module\moduleDataImpl.inl"
#include "..\..\..\framework\module\modules.h"
#include "..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\framework\module\moduleAppearanceImpl.inl"
#include "..\..\..\framework\module\custom\mc_lightSources.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\framework\world\room.h"

#include "..\..\..\core\containers\arrayStack.h"

#ifdef AN_ALLOW_BULLSHOTS
#include "..\..\..\framework\game\bullshotSystem.h"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace CustomModules;

//

DEFINE_STATIC_NAME(emissiveColour);
DEFINE_STATIC_NAME(emissiveBaseColour);
DEFINE_STATIC_NAME(emissivePower);
DEFINE_STATIC_NAME(colour);
DEFINE_STATIC_NAME(colourR);
DEFINE_STATIC_NAME(colourG);
DEFINE_STATIC_NAME(colourB);
DEFINE_STATIC_NAME(colourA);
DEFINE_STATIC_NAME(baseColour);
DEFINE_STATIC_NAME(baseColourR);
DEFINE_STATIC_NAME(baseColourG);
DEFINE_STATIC_NAME(baseColourB);
DEFINE_STATIC_NAME(baseColourA);
DEFINE_STATIC_NAME(colourShaderParam);
DEFINE_STATIC_NAME(baseColourShaderParam);
DEFINE_STATIC_NAME(power);
DEFINE_STATIC_NAME(powerShaderParam);
DEFINE_STATIC_NAME(packedArrayShaderParam);
DEFINE_STATIC_NAME(fromLight);

// bullshot forced
DEFINE_STATIC_NAME(bullshotEmissiveLayer);

//

REGISTER_FOR_FAST_CAST(EmissiveControl);

static Framework::ModuleCustom* create_module(Framework::IModulesOwner* _owner)
{
	return new EmissiveControl(_owner);
}

static Framework::ModuleData* create_module_data(::Framework::LibraryStored* _inLibraryStored)
{
	return new EmissiveControlData(_inLibraryStored);
}

Framework::RegisteredModule<Framework::ModuleCustom> & EmissiveControl::register_itself()
{
	return Framework::Modules::custom.register_module(String(TXT("emissiveControl")), create_module, create_module_data);
}

EmissiveControl::EmissiveControl(Framework::IModulesOwner* _owner)
: base(_owner)
, colourShaderParam(NAME(emissiveColour))
, baseColourShaderParam(NAME(emissiveBaseColour))
, powerShaderParam(NAME(emissivePower))
{
}

EmissiveControl::~EmissiveControl()
{
}

void EmissiveControl::reset()
{
	base::reset();

	mark_requires(all_customs__advance_post);
}

void EmissiveControl::initialise()
{
	base::initialise();

	mark_requires(all_customs__advance_post);
}

void EmissiveControl::setup_with(Framework::ModuleData const * _moduleData)
{
	base::setup_with(_moduleData);
	emissiveControlData = fast_cast<EmissiveControlData>(_moduleData);
	if (_moduleData)
	{
		colourShaderParam = _moduleData->get_parameter<Name>(this, NAME(colourShaderParam), colourShaderParam);
		baseColourShaderParam = _moduleData->get_parameter<Name>(this, NAME(baseColourShaderParam), baseColourShaderParam);
		powerShaderParam = _moduleData->get_parameter<Name>(this, NAME(powerShaderParam), powerShaderParam);
		packedArrayShaderParam = _moduleData->get_parameter<Name>(this, NAME(packedArrayShaderParam), packedArrayShaderParam);
		{
			colour.r = _moduleData->get_parameter<float>(this, NAME(colourR), colour.r);
			colour.g = _moduleData->get_parameter<float>(this, NAME(colourG), colour.g);
			colour.b = _moduleData->get_parameter<float>(this, NAME(colourB), colour.b);
			colour.a = _moduleData->get_parameter<float>(this, NAME(colourA), colour.a);
			colour = _moduleData->get_parameter<Colour>(this, NAME(colour), colour);
		}
		{
			baseColour.r = _moduleData->get_parameter<float>(this, NAME(baseColourR), baseColour.r);
			baseColour.g = _moduleData->get_parameter<float>(this, NAME(baseColourG), baseColour.g);
			baseColour.b = _moduleData->get_parameter<float>(this, NAME(baseColourB), baseColour.b);
			baseColour.a = _moduleData->get_parameter<float>(this, NAME(baseColourA), baseColour.a);
			baseColour = _moduleData->get_parameter<Colour>(this, NAME(baseColour), baseColour);
		}
		power = _moduleData->get_parameter<float>(this, NAME(power), power);
		fromLight = _moduleData->get_parameter<bool>(this, NAME(fromLight), fromLight);
		targetColour = colour;
		targetBaseColour = baseColour;
		targetPower = power;
		defaultColour = colour;
		defaultBaseColour = baseColour;
		defaultPower = power;

		an_assert(emissiveControlData);
		for_every(layer, emissiveControlData->layers)
		{
			auto & l = emissive_access(layer->name, layer->priority);
			l = *layer;
			l.be_actual();
		}
	}
}

void EmissiveControl::set_colour(Colour const & _targetColour, float _blendTime)
{
	if ((_targetColour - colour).get_length() > 0.001f)
	{
		mark_requires(all_customs__advance_post);
	}
	targetColour = _targetColour;
	colourBlendTime = _blendTime;
}

void EmissiveControl::set_base_colour(Colour const & _targetBaseColour)
{
	if ((_targetBaseColour - baseColour).get_length() > 0.001f)
	{
		mark_requires(all_customs__advance_post);
	}
	targetBaseColour = _targetBaseColour;
}

void EmissiveControl::set_power(float _targetPower, float _blendTime)
{
	if (abs(_targetPower - power) > 0.001f)
	{
		mark_requires(all_customs__advance_post);
	}
	targetPower = _targetPower;
	powerBlendTime = _blendTime;
}

void EmissiveControl::advance_post(float _deltaTime)
{
#ifdef AN_ALLOW_BULLSHOTS
	if (Framework::BullshotSystem::is_active())
	{
		// force single emissive layer
		Name bullshotEmissiveLayer = get_owner()->get_variables().get_value<Name>(NAME(bullshotEmissiveLayer), Name::invalid());
		if (bullshotEmissiveLayer.is_valid())
		{
			for_every(layer, layerStack)
			{
				if (layer->name == bullshotEmissiveLayer)
				{
					layer->activate();
				}
				else
				{
					layer->deactivate();
				}
			}
		}
	}
#endif

	rarer_post_advance_if_not_visible();

	if (fromLight)
	{
		Colour colour = Utils::get_light_colour(get_owner());
		Utils::get_emissive_colour(get_owner(), REF_ colour);
		set_colour(colour, 0.1f);
		float power;
		if (Utils::get_emissive_power(get_owner(), REF_ power))
		{
			set_power(power, 0.1f);
		}
		else
		{
			set_power(colour.a * 1.25f, 0.1f);
		}
	}

	colour = blend_to_using_time(colour, targetColour, colourBlendTime, _deltaTime);
	power = blend_to_using_time(power, targetPower, powerBlendTime, _deltaTime);
	
	for_every(sma, separateMaterialActuals)
	{
		sma->inUse = false;
	}


	if (!layerStack.is_empty())
	{
		int materialCount = 0;
		// from back to top of the layer stack
		for_every_reverse(layer, layerStack)
		{
			layer->advance(_deltaTime);
			if (layer->actualActive <= 0.0f)
			{
				continue;
			}
			if (! packedArrayShaderParam.is_valid() && layer->materialIdx != NONE)
			{
				materialCount = max(materialCount, layer->materialIdx + 1);
			}
			if (packedArrayShaderParam.is_valid())
			{
				materialCount = max(materialCount, layer->packedArrayIdx + 1);
			}
		}

		separateMaterialActuals.set_size(max(separateMaterialActuals.get_size(), materialCount));
		for_every_reverse(layer, layerStack)
		{
			if (layer->actualActive <= 0.0f)
			{
				continue;
			}
			if (!packedArrayShaderParam.is_valid() && layer->materialIdx == NONE)
			{
				layer->apply(REF_ colour, REF_ baseColour, REF_ power);
				for_count(int, idx, materialCount)
				{
					auto& sma = separateMaterialActuals[idx];
					if (sma.inUse)
					{
						layer->apply(REF_ sma.colour, REF_ sma.baseColour, REF_ sma.power);
					}
				}
			}
			else
			{
				auto& sma = separateMaterialActuals[!packedArrayShaderParam.is_valid()? layer->materialIdx : layer->packedArrayIdx];
				if (!sma.inUse)
				{
					sma.colour = colour;
					sma.power = power;
				}
				sma.inUse = true;
				layer->apply(REF_ sma.colour, REF_ sma.baseColour, REF_ sma.power);
			}
		}

		for (int idx = 0; idx < layerStack.get_size(); ++idx)
		{
			auto const & layer = layerStack[idx];
			if (layer.actualActive == 0.0f && layer.removeOnDeactivated)
			{
				layerStack.remove_at(idx);
				--idx;
			}
		}
	}

	if (auto* appearance = get_owner()->get_appearance())
	{
		if (packedArrayShaderParam.is_valid())
		{
			int v4Amount = separateMaterialActuals.get_size() * 3;
			ARRAY_STACK(Vector4, packedArray, v4Amount);
			packedArray.set_size(v4Amount);
			for_every(sma, separateMaterialActuals)
			{
				int idx = for_everys_index(sma) * 3;
				if (sma->inUse)
				{
					packedArray[idx] = sma->colour.to_vector4();
					packedArray[idx + 1] = sma->baseColour.to_vector4();
					packedArray[idx + 2] = Vector4(sma->power, 0.0f, 0.0f, 0.0f);
				}
				else
				{
					packedArray[idx] = colour.to_vector4();
					packedArray[idx + 1] = baseColour.to_vector4();
					packedArray[idx + 2] = Vector4(power, 0.0f, 0.0f, 0.0f);
				}
			}
			appearance->do_for_all_appearances([this, &packedArray](Framework::ModuleAppearance* _appearance)
			{
				_appearance->set_shader_param(packedArrayShaderParam, packedArray);
			});
		}
		else
		{
			bool anySMAInUse = false;
			for_every(sma, separateMaterialActuals)
			{
				anySMAInUse |= sma->inUse;
			}
			if (!anySMAInUse)
			{
				appearance->do_for_all_appearances([this](Framework::ModuleAppearance* _appearance)
				{
					_appearance->set_shader_param(colourShaderParam, colour.to_vector4());
					_appearance->set_shader_param(baseColourShaderParam, baseColour.to_vector4());
					_appearance->set_shader_param(powerShaderParam, power);
				});
			}
			else
			{
				appearance->do_for_all_appearances([this](Framework::ModuleAppearance* _appearance)
				{
					for_count(int, i, _appearance->access_mesh_instance().get_material_instance_count())
					{
						if (separateMaterialActuals.is_index_valid(i) && separateMaterialActuals[i].inUse)
						{
							auto const & sma = separateMaterialActuals[i];
							_appearance->set_shader_param(colourShaderParam, sma.colour.to_vector4(), i);
							_appearance->set_shader_param(baseColourShaderParam, sma.baseColour.to_vector4(), i);
							_appearance->set_shader_param(powerShaderParam, sma.power, i);
						}
						else
						{
							_appearance->set_shader_param(colourShaderParam, colour.to_vector4(), i);
							_appearance->set_shader_param(baseColourShaderParam, baseColour.to_vector4(), i);
							_appearance->set_shader_param(powerShaderParam, power, i);

						}
					}
				});
			}
		}
	}

	if (emissiveControlData &&
		emissiveControlData->asLightSource.is_valid())
	{
		if (auto* ls = get_owner()->get_custom<Framework::CustomModules::LightSources>())
		{
			auto* nls = ls->access(emissiveControlData->asLightSource);
			if (!nls)
			{
				ls->add(emissiveControlData->asLightSource, true);
				nls = ls->access(emissiveControlData->asLightSource);
				lightSourcePower = nls->requested.power;
			}
			if (nls)
			{
				Colour newColour = colour;
				float newPower = power;
				if (emissiveControlData->asLightSourceFromMaterialIdx != NONE &&
					separateMaterialActuals.is_index_valid(emissiveControlData->asLightSourceFromMaterialIdx) &&
					separateMaterialActuals[emissiveControlData->asLightSourceFromMaterialIdx].inUse)
				{
					auto const& sma = separateMaterialActuals[emissiveControlData->asLightSourceFromMaterialIdx];
					newColour = sma.colour;
					newPower = sma.power;
				}
				{
					// prefer one colour component!
					float maxNC = max(newColour.r, max(newColour.g, newColour.b));
					newColour.r = max(0.0f, newColour.r + 2.0f * (newColour.r - maxNC));
					newColour.g = max(0.0f, newColour.g + 2.0f * (newColour.r - maxNC));
					newColour.b = max(0.0f, newColour.b + 2.0f * (newColour.r - maxNC));
				}
				newPower *= lightSourcePower;
				if (nls->requested.colour != newColour ||
					nls->requested.power != newPower)
				{
					nls->requested.colour = newColour;
					nls->requested.power = newPower;
					ls->mark_requires_update();
				}
			}
		}
	}

	// after everything is processed, check if we require any further advancements
	if ((targetColour - colour).get_length() < 0.001f &&
		abs(targetPower - power) < 0.001f)
	{
		bool noAdvancemeentRequired = layerStack.is_empty();
		if (!noAdvancemeentRequired)
		{
			noAdvancemeentRequired = true;
			for_every(layer, layerStack)
			{
				noAdvancemeentRequired &= !layer->does_require_advancement();
			}
		}
		if (noAdvancemeentRequired)
		{
#ifdef AN_ALLOW_BULLSHOTS
			if (! Framework::BullshotSystem::is_active())
#endif
			mark_no_longer_requires(all_customs__advance_post);
		}
	}
}

bool EmissiveControl::has_emissive(Name const& _layerName) const
{
	for_every(layer, layerStack)
	{
		if (layer->name == _layerName)
		{
			return true;
		}
	}
	return false;
}

EmissiveLayer & EmissiveControl::emissive_access(Name const & _layerName, int _priority)
{
	mark_requires(all_customs__advance_post);

	for_every(layer, layerStack)
	{
		if (layer->name == _layerName)
		{
			layer->requiresAdvancement = true;
			return *layer;
		}
	}

	EmissiveLayer layer;
	layer.name = _layerName;
	layer.priority = _priority;
	layer.requiresAdvancement = true;
	for_every(l, layerStack)
	{
		if (l->priority <= layer.priority) // so newly created will be put before existing ones
		{
			int idx = for_everys_index(l);
			layerStack.insert_at(idx, layer);
			return layerStack[idx];
		}
	}

	layerStack.push_back(layer);
	return layerStack.get_last();
}

void EmissiveControl::emissive_remove(Name const & _layerName)
{
	for_every(layer, layerStack)
	{
		if (layer->name == _layerName)
		{
			mark_requires(all_customs__advance_post);
			layerStack.remove_at(for_everys_index(layer));
			break;
		}
	}
}

void EmissiveControl::emissive_activate(Name const & _layerName, Optional<float> const & _blendTime)
{
	for_every(layer, layerStack)
	{
		if (layer->name == _layerName)
		{
			mark_requires(all_customs__advance_post);
			layer->activate(_blendTime);
			break;
		}
	}
}

void EmissiveControl::emissive_deactivate(Name const & _layerName, Optional<float> const & _blendTime)
{
	for_every(layer, layerStack)
	{
		if (layer->name == _layerName)
		{
			mark_requires(all_customs__advance_post);
			layer->deactivate(_blendTime);
			break;
		}
	}
}

void EmissiveControl::emissive_deactivate_all(Optional<float> const & _blendTime)
{
	for_every(layer, layerStack)
	{
		layer->deactivate(_blendTime);
	}
	mark_requires(all_customs__advance_post);
	targetPower = 0.0f;
	powerBlendTime = _blendTime.get(0.2f);
}

//

void EmissiveLayer::advance(float _deltaTime)
{
	bool wasActive = actualActive == 0.0f;

	if (actualActive == 0.0f)
	{
		activeTime = 0.0f;
	}
	activeTime += _deltaTime;
	if (active > 0.0f && autoDeactivateTime.is_set() && activeTime >= autoDeactivateTime.get())
	{
		deactivate();
	}

	requiresAdvancement = false;

	actualActive = blend_to_using_speed_based_on_time(actualActive, active, actualActive < active? currentActivateBlendTime : currentDeactivateBlendTime, _deltaTime);
	actualColour = blend_to_using_time(actualColour, colour, wasActive ? colourBlendTime : 0.0f, _deltaTime);
	actualBaseColour = blend_to_using_time(actualBaseColour, baseColour, wasActive ? colourBlendTime : 0.0f, _deltaTime);
	actualPower = blend_to_using_time(actualPower, power, wasActive ? powerBlendTime : 0.0f, _deltaTime);

	requiresAdvancement |= abs(actualActive - active) > 0.001f;
	requiresAdvancement |= (actualColour - colour).get_length() > 0.001f;
	requiresAdvancement |= (actualBaseColour - baseColour).get_length() > 0.001f;
	requiresAdvancement |= abs(actualPower - power) > 0.001f;

	if (mode != EmissiveMode::Normal && period > 0.0f)
	{
		pt += _deltaTime / (max(period, PlayerPreferences::are_currently_flickering_lights_allowed()? 0.0f : 0.5f));
		if (pt >= 1.0f && mode == EmissiveMode::Random)
		{
			randomPt = ::Random::get_float(0.1f, 0.9f);
		}
		pt = mod(pt, 1.0f);
		requiresAdvancement = true;
	}
}

void EmissiveLayer::apply(REF_ Colour & _colour, REF_ Colour & _baseColour, REF_ float & _power) const
{
	float useActive = actualActive;

	if (mode == EmissiveMode::Pulse)
	{
		float pulsePower = 0.5f * (1.0f + sin_deg(360.0f * pt));
		useActive *= minActivePct + (1.0f - minActivePct) * pulsePower;
	}
	else if (mode == EmissiveMode::Square)
	{
		float pulsePower = pt < 0.5f ? 1.0f : 0.0f;
		useActive *= minActivePct + (1.0f - minActivePct) * pulsePower;
	}
	else if (mode == EmissiveMode::Random)
	{
		float pulsePower = pt < randomPt ? (pt / randomPt) : ((1.0f - pt) / (1.0f - randomPt));
		pulsePower = BlendCurve::cubic(pulsePower);
		useActive *= minActivePct + (1.0f - minActivePct) * pulsePower;
	}

	if (affectColour)
	{
		_colour = Colour::lerp(useActive, _colour, actualColour);
		_baseColour = Colour::lerp(useActive, _baseColour, actualBaseColour);
	}
	if (affectPower)
	{
		_power = _power + useActive * (actualPower - _power);
	}
}

void EmissiveLayer::be_actual()
{
	actualActive = active;
	actualColour = colour;
	actualBaseColour = baseColour;
	actualPower = power;
}

bool EmissiveLayer::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	name = _node->get_name_attribute_or_from_child(TXT("name"), name);
	priority = _node->get_int_attribute_or_from_child(TXT("priority"), priority);

	materialIdx = _node->get_int_attribute_or_from_child(TXT("materialIdx"), materialIdx);
	materialIdx = _node->get_int_attribute_or_from_child(TXT("materialIndex"), materialIdx);
	packedArrayIdx = _node->get_int_attribute_or_from_child(TXT("packedArrayIdx"), packedArrayIdx);
	packedArrayIdx = _node->get_int_attribute_or_from_child(TXT("packedArrayIndex"), packedArrayIdx);

	active = _node->get_float_attribute_or_from_child(TXT("active"), active);
	power = _node->get_float_attribute_or_from_child(TXT("power"), power);
	colour.load_from_xml_child_or_attr(_node, TXT("colour"));
	baseColour.load_from_xml_child_or_attr(_node, TXT("baseColour"));
	affectColour = _node->get_bool_attribute_or_from_child_presence(TXT("affectColour"), affectColour);
	affectPower = _node->get_bool_attribute_or_from_child_presence(TXT("affectPower"), affectPower);
	affectColour = ! _node->get_bool_attribute_or_from_child_presence(TXT("keepColour"), ! affectColour);
	affectPower = ! _node->get_bool_attribute_or_from_child_presence(TXT("keepPower"), ! affectPower);

	period = _node->get_float_attribute_or_from_child(TXT("period"), period);
	minActivePct = _node->get_float_attribute_or_from_child(TXT("minActivePct"), minActivePct);
	activateBlendTime = _node->get_float_attribute_or_from_child(TXT("blendTime"), activateBlendTime);
	deactivateBlendTime = _node->get_float_attribute_or_from_child(TXT("blendTime"), deactivateBlendTime);
	colourBlendTime = _node->get_float_attribute_or_from_child(TXT("blendTime"), colourBlendTime);
	powerBlendTime = _node->get_float_attribute_or_from_child(TXT("blendTime"), powerBlendTime);
	activateBlendTime = _node->get_float_attribute_or_from_child(TXT("activateBlendTime"), activateBlendTime);
	deactivateBlendTime = _node->get_float_attribute_or_from_child(TXT("deactivateBlendTime"), deactivateBlendTime);
	colourBlendTime = _node->get_float_attribute_or_from_child(TXT("colourBlendTime"), colourBlendTime);
	powerBlendTime = _node->get_float_attribute_or_from_child(TXT("powerBlendTime"), powerBlendTime);
	autoDeactivateTime.load_from_xml(_node, TXT("autoDeactivateTime"));
	
	currentActivateBlendTime = activateBlendTime;
	currentDeactivateBlendTime = deactivateBlendTime;

	mode = EmissiveMode::parse(_node->get_string_attribute_or_from_child(TXT("mode")), mode);
	pt = _node->get_float_attribute_or_from_child(TXT("pt"), pt);

	removeOnDeactivated = false; // always keep

	return result;
}

//

REGISTER_FOR_FAST_CAST(EmissiveControlData);

EmissiveControlData::EmissiveControlData(Framework::LibraryStored* _inLibraryStored)
: base(_inLibraryStored)
{
}

EmissiveControlData::~EmissiveControlData()
{
}

bool EmissiveControlData::read_parameter_from(IO::XML::Attribute const * _attr, Framework::LibraryLoadingContext & _lc)
{
	if (_attr->get_name() == TXT("asLightSource"))
	{
		asLightSource = _attr->get_as_name();
		return true;
	}
	else if (_attr->get_name() == TXT("asLightSourceFromMaterialIdx"))
	{
		asLightSourceFromMaterialIdx = _attr->get_as_int();
		return true;
	}
	else
	{
		return base::read_parameter_from(_attr, _lc);
	}
}

bool EmissiveControlData::read_parameter_from(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	if (_node->get_name() == TXT("layer"))
	{
		EmissiveLayer layer;
		if (layer.load_from_xml(_node, _lc))
		{
			layers.push_back(layer);
			return true;
		}
		else
		{
			error_loading_xml(_node, TXT("can't load layer"));
			return false;
		}
	}
	else
	{
		return base::read_parameter_from(_node, _lc);
	}
}

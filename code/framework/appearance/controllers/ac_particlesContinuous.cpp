#include "ac_particlesContinuous.h"

#include "..\appearanceControllerPoseContext.h"

#include "..\..\module\moduleAppearance.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace AppearanceControllersLib;

//

DEFINE_STATIC_NAME(continuous);
DEFINE_STATIC_NAME(continuousFor);

//

REGISTER_FOR_FAST_CAST(ParticlesContinuous);

ParticlesContinuous::ParticlesContinuous(ParticlesContinuousData const * _data)
: base(_data)
, particlesContinuousData(_data)
{
}

void ParticlesContinuous::desire_to_deactivate()
{
	continuous = false;
	continuousForLeft = 0.0f;
}

void ParticlesContinuous::activate()
{
	base::activate();

	continuousForLeft = rg.get(particlesContinuousData->continuousFor);
	if (auto* v = get_owner()->get_owner()->get_variables().get_existing<Range>(NAME(continuousFor)))
	{
		continuousForLeft = rg.get(*v);
	}
	if (auto* v = get_owner()->get_owner()->get_variables().get_existing<float>(NAME(continuousFor)))
	{
		continuousForLeft = *v;
	}

	continuous = particlesContinuousData->continuous;
	if (auto* v = get_owner()->get_owner()->get_variables().get_existing<bool>(NAME(continuous)))
	{
		continuous = *v;
	}

	if (get_placement_separation() != 0.0f && is_continuous_now())
	{
		error(TXT("placement separation and continuous should not be used together"));
	}
}

void ParticlesContinuous::advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context)
{
	continuousForLeft = max(0.0f, continuousForLeft - _context.get_delta_time());

	base::advance_and_adjust_preliminary_pose(_context);
}

//

REGISTER_FOR_FAST_CAST(ParticlesContinuousData);

AppearanceControllerData* ParticlesContinuousData::create_data()
{
	return new ParticlesContinuousData();
}

ParticlesContinuousData::ParticlesContinuousData()
{
	initialPlacement = ParticlesPlacement::OwnerPlacement;
}

bool ParticlesContinuousData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= continuousFor.load_from_xml(_node, TXT("continuousFor"));
	continuous = _node->get_bool_attribute_or_from_child_presence(TXT("continuous"), continuous);
	if (auto * node = _node->first_child_named(TXT("continuous")))
	{
		if (auto* attr = node->get_attribute(TXT("for")))
		{
			continuous = false;
			continuousFor.load_from_string(attr->get_as_string());
		}
	}

	result &= changeScale.load_from_xml(_node, TXT("scale"));
	result &= changeAlpha.load_from_xml(_node, TXT("alpha"));
	result &= changeSolid.load_from_xml(_node, TXT("solid"));

	result &= extraScale.load_from_xml(_node, TXT("extraScale"));
	extraScaleAmount = _node->get_float_attribute(TXT("extraScaleAmount"), extraScaleAmount);
	for_every(node, _node->children_named(TXT("extraScale")))
	{
		extraScaleAmount = node->get_float_attribute(TXT("amount"), extraScaleAmount);
	}

	dissolvedInsteadOfScaleDown = _node->get_bool_attribute_or_from_child_presence(TXT("dissolveInsteadOfScaleDown"), dissolvedInsteadOfScaleDown);
	dissolvedInsteadOfScaleDown = _node->get_bool_attribute_or_from_child_presence(TXT("dissolvedInsteadOfScaleDown"), dissolvedInsteadOfScaleDown);
	dissolvedWithScaleDown = _node->get_bool_attribute_or_from_child_presence(TXT("dissolveWithScaleDown"), dissolvedWithScaleDown);
	dissolvedWithScaleDown = _node->get_bool_attribute_or_from_child_presence(TXT("dissolvedWithScaleDown"), dissolvedWithScaleDown);
	dissolveBase = _node->get_float_attribute(TXT("dissolveBase"), dissolveBase);
	dissolveExp = _node->get_float_attribute(TXT("dissolveExp"), dissolveExp);

	return result;
}

AppearanceControllerData* ParticlesContinuousData::create_copy() const
{
	ParticlesContinuousData* copy = new ParticlesContinuousData();
	*copy = *this;
	return copy;
}

float ParticlesContinuousData::calculate_dissolve(float _timeAlive, float _totalTime) const
{
	float dissolveResult = 0.0f;
	if (dissolvedInsteadOfScaleDown || dissolvedWithScaleDown)
	{
		dissolveResult = clamp(1.0f - changeScale.calculate(_timeAlive, _totalTime, false, dissolvedInsteadOfScaleDown || dissolvedWithScaleDown), 0.0f, 1.0f);
	}
	{
		float solid = changeSolid.calculate(_timeAlive, _totalTime);
		solid *= 1.0f - dissolveResult;
		dissolveResult = 1.0f - solid;
	}
	{
		float dissolve = dissolveResult;
		dissolve = clamp(1.0f - pow((1.0f - dissolve), dissolveExp), 0.0f, 1.0f);
		dissolveResult = dissolveBase + (1.0f - dissolveBase) * dissolve;
	}
	return dissolveResult;
}

float ParticlesContinuousData::calculate_scale(float _timeAlive, float _totalTime) const
{
	// changes to dissolve
	return changeScale.calculate(_timeAlive, _totalTime, true, !dissolvedInsteadOfScaleDown) * (1.0f + extraScaleAmount * BlendCurve::cubic_out(extraScale.calculate(_timeAlive, _totalTime, true, !dissolvedInsteadOfScaleDown)));
}

float ParticlesContinuousData::calculate_full_scale(float _timeAlive, float _totalTime) const
{
	return changeScale.calculate(_timeAlive, _totalTime) * (1.0f + extraScaleAmount * BlendCurve::cubic_out(extraScale.calculate(_timeAlive, _totalTime)));
}

float ParticlesContinuousData::calculate_alpha(float _timeAlive, float _totalTime) const
{
	return changeAlpha.calculate(_timeAlive, _totalTime);
}

//

bool ParticlesContinuousData::ChangeOverTime::load_from_xml(IO::XML::Node const * _node, tchar const * _prefix)
{
	bool result = true;
	for_every(node, _node->children_named(_prefix))
	{
		delayTime = _node->get_float_attribute_or_from_child(TXT("delayTime"), delayTime);
		attackTime = _node->get_float_attribute_or_from_child(TXT("attackTime"), attackTime);
		attackPt = _node->get_float_attribute_or_from_child(TXT("attackPt"), attackPt);
		decayTime = _node->get_float_attribute_or_from_child(TXT("decayTime"), decayTime);
		decayPt = _node->get_float_attribute_or_from_child(TXT("decayPt"), decayPt);
	}
	delayTime = _node->get_float_attribute_or_from_child(String::printf(TXT("%SDelayTime"), _prefix).to_char(), delayTime);
	attackTime = _node->get_float_attribute_or_from_child(String::printf(TXT("%SAttackTime"), _prefix).to_char(), attackTime);
	attackPt = _node->get_float_attribute_or_from_child(String::printf(TXT("%SAttackPt"), _prefix).to_char(), attackPt);
	decayTime = _node->get_float_attribute_or_from_child(String::printf(TXT("%SDecayTime"), _prefix).to_char(), decayTime);
	decayPt = _node->get_float_attribute_or_from_child(String::printf(TXT("%SDecayPt"), _prefix).to_char(), decayPt);
	return result;
}

float ParticlesContinuousData::ChangeOverTime::calculate(float _timeAlive, float _totalTime, bool _useAttack, bool _useDecay) const
{
	_totalTime = max(_totalTime, 0.0f);

	if (_timeAlive >= _totalTime)
	{
		return 0.0f;
	}
	else
	{
		float useAttackTime = max(attackTime, attackPt * _totalTime);
		float useDecayTime = max(decayTime, decayPt * _totalTime);

		float result = 1.0f;
		if (_useAttack && useAttackTime != 0.0f)
		{
			result *= BlendCurve::cubic_out(clamp((_timeAlive - delayTime) / useAttackTime, 0.0f, 1.0f));
		}
		if (_useDecay && useDecayTime != 0.0f)
		{
			result *= clamp((_totalTime - _timeAlive) / useDecayTime, 0.0f, 1.0f);
		}

		return result;
	}
}

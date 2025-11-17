#include "meshGenShapeUtils.h"

#include "..\meshGenCreateCollisionInfo.h"
#include "..\meshGenGenerationContext.h"
#include "..\meshGenSubStepDef.h"
#include "..\meshGenUtils.h"
#include "..\meshGenValueDefImpl.inl"

#include "..\..\appearance\meshNode.h"

#include "..\..\..\core\containers\arrayStack.h"

#include "..\..\..\core\math\math.h"

#include "..\..\..\core\mesh\builders\mesh3dBuilder_IPU.h"
#include "..\..\..\core\mesh\vertexSkinningInfo.h"

#include "..\..\..\core\other\parserUtils.h"
#include "..\..\..\core\system\core.h"
#include "..\..\..\core\tags\tagCondition.h"

#include "..\..\..\core\debug\debugRenderer.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace MeshGeneration;

//

float ShapeUtils::PointOnCurve::get_pos() const
{
#ifdef AN_ASSERT
	an_assert(posCalculated);
#endif
	return pos;
}

float ShapeUtils::PointOnCurve::get_pos_not_clamped() const
{
#ifdef AN_ASSERT
	an_assert(posCalculated);
#endif
	return posNotClamped;
}

void ShapeUtils::PointOnCurve::set_pos(float _pos)
{
	pos = _pos;
#ifdef AN_ASSERT
	posCalculated = true;
#endif
}

bool ShapeUtils::PointOnCurve::calculate_pos(Range const & _ptRange, float _curveLength) const
{
	pos = 0.0f;
	if (pt.is_set())
	{
		float usePt = _ptRange.min + (_ptRange.max - _ptRange.min) * pt.get();
		pos = _curveLength * usePt;
		if (dist.is_set())
		{
			pos += dist.get();
		}
	}
	else
	{
		if (dist.is_set())
		{
			pos = dist.get();
			if (pos < 0.0f)
			{
				pos = _curveLength + pos;
			}
		}
	}
	posNotClamped = pos;
	pos = (_ptRange * _curveLength).clamp(pos);
#ifdef AN_ASSERT
	posCalculated = true;
#endif
	return pos == posNotClamped;
}

void ShapeUtils::PointOnCurve::reverse_single(float _curveLength)
{
	if (pt.is_set())
	{
		pt = 1.0f - pt.get();
	}
	if (dist.is_set())
	{
		dist = -dist.get();
	}

	pos = _curveLength - pos;
}

bool ShapeUtils::PointOnCurve::load_single_from_xml(IO::XML::Node const * _node)
{
	bool result = true;

	pt.load_from_xml(_node, TXT("pt"));
	dist.load_from_xml(_node, TXT("dist"));

	return result;
}

//

void ShapeUtils::CrossSectionOnCurve::reverse_single(float _curveLength)
{
	PointOnCurve::reverse_single(_curveLength);
	if (scaleCurve.is_set())
	{
		scaleCurve = scaleCurve.get().reversed();
		scaleCurve.access().p0.t = 1.0f - scaleCurve.get().p0.t;
		scaleCurve.access().p1.t = 1.0f - scaleCurve.get().p1.t;
		scaleCurve.access().p2.t = 1.0f - scaleCurve.get().p2.t;
		scaleCurve.access().p3.t = 1.0f - scaleCurve.get().p3.t;
	}
	if (scaleFloatCurve.is_set())
	{
		scaleFloatCurve = scaleFloatCurve.get().reversed();
		scaleFloatCurve.access().p0.t = 1.0f - scaleFloatCurve.get().p0.t;
		scaleFloatCurve.access().p1.t = 1.0f - scaleFloatCurve.get().p1.t;
		scaleFloatCurve.access().p2.t = 1.0f - scaleFloatCurve.get().p2.t;
		scaleFloatCurve.access().p3.t = 1.0f - scaleFloatCurve.get().p3.t;
	}
	if (morphCurve.is_set())
	{
		morphCurve = morphCurve.get().reversed();
		morphCurve.access().p0.t = 1.0f - morphCurve.get().p0.t;
		morphCurve.access().p1.t = 1.0f - morphCurve.get().p1.t;
		morphCurve.access().p2.t = 1.0f - morphCurve.get().p2.t;
		morphCurve.access().p3.t = 1.0f - morphCurve.get().p3.t;
	}
}

Vector2 ShapeUtils::CrossSectionOnCurve::calculate_final_scale(GenerationContext const & _context, CrossSectionOnCurve const * _prev, float _t, bool _tIsOuter) const
{
	Vector2 result = Vector2::one;
	if (!_prev)
	{
		_t = 1.0f;
	}
	float prevScaleFloat = _prev ? _prev->scaleFloat.get_with_default(_context, 1.0f) : 1.0f;
	float currScaleFloat = scaleFloat.get_with_default(_context, 1.0f);
	Vector2 prevScale = Vector2(prevScaleFloat, prevScaleFloat);
	Vector2 currScale = Vector2(currScaleFloat, currScaleFloat);
	prevScale *= _prev ? _prev->scale.get_with_default(_context, Vector2::one) : Vector2::one;
	currScale *= scale.get_with_default(_context, Vector2::one);
	result = currScale * _t + (1.0f - _t) * prevScale;
	// prefer scale float curve as it is harder to read properly
	if (scaleFloatCurve.is_set())
	{
		float useT = _t;
		if (_tIsOuter)
		{
			useT = BezierCurveTBasedPoint<float>::find_t_at(_t, scaleFloatCurve.get());
		}
		result *= scaleFloatCurve.get().calculate_at(useT).p;
	}
	else if (scaleCurve.is_set())
	{
		float useT = _t;
		if (_tIsOuter)
		{
			useT = BezierCurveTBasedPoint<float>::find_t_at(_t, scaleFloatCurve.get());
		}
		result *= scaleCurve.get().calculate_at(useT).p;
	}
	return result;
}

float ShapeUtils::CrossSectionOnCurve::calculate_final_morph(GenerationContext const & _context, CrossSectionOnCurve const * _prev, float _t, bool _tIsOuter) const
{
	float result = _t;
	if (morphCurve.is_set())
	{
		float useT = _t;
		if (_tIsOuter)
		{
			useT = BezierCurveTBasedPoint<float>::find_t_at(_t, morphCurve.get());
		}
		result = morphCurve.get().calculate_at(useT).p;
	}
	return result;
}

float ShapeUtils::CrossSectionOnCurve::transform_t(float _t) const
{
	float result = _t;
	if (morphCurve.is_set())
	{
		result = morphCurve.get().calculate_at(_t).t;
	}
	else if (scaleFloatCurve.is_set())
	{
		result = scaleFloatCurve.get().calculate_at(_t).t;
	}
	else if (scaleCurve.is_set())
	{
		result = scaleCurve.get().calculate_at(_t).t;
	}
	return result;
}

bool ShapeUtils::CrossSectionOnCurve::load_single_from_xml(IO::XML::Node const * _node)
{
	bool result = PointOnCurve::load_single_from_xml(_node);

	id = _node->get_name_attribute(TXT("id"), id);
	uId.load_from_xml(_node, TXT("uId"));
	ParserUtils::load_multiple_from_xml_into<ArrayStatic<Name, 8>, Name>(_node, TXT("altId"), REF_ altId);

	scaleFloat.load_from_xml(_node, TXT("scale"));
	scale.load_from_xml(_node, TXT("scale"));

	if (auto const * node = _node->first_child_named(TXT("scaleCurve")))
	{
		BezierCurve<BezierCurveTBasedPoint<float>> sc;
		sc.p0.t = 0.00f;
		sc.p0.p = 1.00f;
		sc.p3.t = 1.00f;
		sc.p3.p = 1.00f;
		sc.make_roundly_separated();
		if (sc.load_from_xml(node))
		{
			scaleFloatCurve = sc;
		}
	}
	if (auto const * node = _node->first_child_named(TXT("scaleCurve")))
	{
		BezierCurve<BezierCurveTBasedPoint<Vector2>> sc;
		sc.p0.t = 0.00f;
		sc.p0.p = Vector2::one;
		sc.p3.t = 1.00f;
		sc.p3.p = Vector2::one;
		sc.make_roundly_separated();
		if (sc.load_from_xml(node))
		{
			scaleCurve = sc;
		}
	}

	if (auto const * node = _node->first_child_named(TXT("morphCurve")))
	{
		BezierCurve<BezierCurveTBasedPoint<float>> mc;
		mc.p0.t = 0.00f;
		mc.p0.p = 0.00f;
		mc.p3.t = 1.00f;
		mc.p3.p = 1.00f;
		mc.make_linear();
		mc.make_roundly_separated();
		if (mc.load_from_xml(node))
		{
			morphCurve = mc;
		}
		morphCurveStepLength.load_from_xml(node, TXT("stepLength"));
	}

	return result;
}

void ShapeUtils::CrossSectionOnCurve::insert(ShapeUtils::CrossSectionOnCurve const & _cs, Array<ShapeUtils::CrossSectionOnCurve> & _into)
{
	float pos = _cs.get_pos();
	for_every(cs, _into)
	{
		if (cs->get_pos() > pos)
		{
			int at = for_everys_index(cs);
			_into.insert_at(at, _cs);
			_into[at].pos = pos;
			return;
		}
	}
	_into.push_back(_cs);
	_into.get_last().pos = pos;
}

void ShapeUtils::CrossSectionOnCurve::reverse(Array<ShapeUtils::CrossSectionOnCurve> & _csOnCurve, float _curveLength)
{
	if (_csOnCurve.is_empty())
	{
		return;
	}
	// first, place them in order
	Array<ShapeUtils::CrossSectionOnCurve> copy = _csOnCurve;
	_csOnCurve.clear();
	for_every(cs, copy)
	{
		ShapeUtils::CrossSectionOnCurve::insert(*cs, _csOnCurve);
	}
	for_every(cs, _csOnCurve)
	{
		cs->reverse_single(_curveLength);
	}
	Optional<Name> lastUId = _csOnCurve.get_last().uId;
	for_every_reverse(cs, _csOnCurve)
	{
		swap(cs->uId, lastUId); // will carry usePrevU to next one
	}
	// carry scaleCurves
	// source						reordered					after reverse single		final
	// 0 - not used					0 - scaleCurve b 1->0		0 - scaleCurve -b 0->1		0 - not used
	// 1 - scaleCurve a 0->1	->	1 - scaleCurve a 2->1	->	1 - scaleCurve -a 1->2	->	1 - scaleCurve -b 0->1
	// 2 - scaleCurve b 1->2		2 - not set					2 - not set					2 - scaleCurve -a 1->2
	ShapeUtils::CrossSectionOnCurve* next = nullptr;
	for_every_reverse(cs, _csOnCurve)
	{
		if (next)
		{
			swap(next->scaleFloatCurve, cs->scaleFloatCurve);
			swap(next->scaleCurve, cs->scaleCurve);
			swap(next->morphCurve, cs->morphCurve);
		}
		else
		{
			cs->scaleFloatCurve.clear();
			cs->scaleCurve.clear();
			cs->morphCurve.clear();
		}
		next = cs;
	}
}

bool ShapeUtils::CrossSectionOnCurve::load_from_xml(IO::XML::Node const * _node, Array<ShapeUtils::CrossSectionOnCurve> & _csOnCurve, tchar const * const _childName)
{
	if (!_node)
	{
		return true;
	}
	bool result = true;
	for_every(node, _node->children_named(_childName))
	{
		if (node->has_attribute(TXT("systemTagRequired")))
		{
			TagCondition systemTagRequired;
			if (systemTagRequired.load_from_xml_attribute(node, TXT("systemTagRequired")))
			{
				if (!systemTagRequired.check(System::Core::get_system_tags()))
				{
					continue;
				}
			}
		}
		ShapeUtils::CrossSectionOnCurve csoc;
		result &= csoc.load_single_from_xml(node);
		_csOnCurve.push_back(csoc);
	}
	return result;
}

//

void ShapeUtils::CustomOnCurve::insert(ShapeUtils::CustomOnCurve const & _cs, Array<ShapeUtils::CustomOnCurve> & _into)
{
	float pos = _cs.get_pos();
	for_every(cs, _into)
	{
		if (cs->get_pos() > pos)
		{
			int at = for_everys_index(cs);
			_into.insert_at(at, _cs);
			_into[at].pos = pos;
			return;
		}
	}
	_into.push_back(_cs);
	_into.get_last().pos = pos;
}

void ShapeUtils::CustomOnCurve::reverse(Array<ShapeUtils::CustomOnCurve> & _csOnCurve, float _curveLength)
{
	if (_csOnCurve.is_empty())
	{
		return;
	}
	// place them in order
	Array<ShapeUtils::CustomOnCurve> copy = _csOnCurve;
	_csOnCurve.clear();
	for_every(cs, copy)
	{
		ShapeUtils::CustomOnCurve::insert(*cs, _csOnCurve);
	}
	for_every(cs, _csOnCurve)
	{
		cs->reverse_single(_curveLength);
	}
}

//

void ShapeUtils::morph_cross_section(Array<CrossSection> const & _cs, OUT_ CrossSection & _csOut, Name const & _prevId, Name const & _id, float _morph)
{
	if (_prevId == _id)
	{
		_csOut = get_cross_section(_cs, _id);
		return;
	}
	ShapeUtils::CrossSection const & cs0 = get_cross_section(_cs, _prevId);
	ShapeUtils::CrossSection const & cs1 = get_cross_section(_cs, _id);
	_csOut = cs1;
	float morph1 = clamp(_morph, 0.0f, 1.0f);
	float morph0 = 1.0f - _morph;

	auto const *point0 = cs0.points.get_data();
	for_every(point1, _csOut.points)
	{
		*point1 = *point0 * morph0 + morph1 * *point1;
		++point0;
	}

	// keep u same
}

ShapeUtils::CrossSection const & ShapeUtils::get_cross_section(Array<ShapeUtils::CrossSection> const & _cs, Name const & _id)
{
	CrossSection const * cs = nullptr;
	for_every(csi, _cs)
	{
		if (csi->id == _id ||
			(cs == nullptr && !csi->id.is_valid()))
		{
			cs = csi;
		}
	}
	an_assert(cs != nullptr);
	return *cs;
}

Name const& ShapeUtils::get_alt_cross_section_id(Array<ShapeUtils::CrossSection> const& _cs, CrossSectionOnCurve const& _csOnCurve)
{
	return get_alt_cross_section_id(_cs, _csOnCurve.id, _csOnCurve.altId);
}

Name const & ShapeUtils::get_alt_cross_section_id(Array<ShapeUtils::CrossSection> const & _cs, Name const& _id, ArrayStatic<Name, 8> const& _altIds)
{
	for_every(altId, _altIds)
	{
		for_every(csi, _cs)
		{
			if (csi->id == *altId)
			{
				return *altId;
			}
		};
	}
	return _id;
}

void ShapeUtils::setup_cs_on_curve(OUT_ Array<CrossSectionOnCurve> & csOnCurve, GenerationContext const & _context, Array<ShapeUtils::CrossSection> const& _cs,
	BezierCurve<Vector3> const & _onCurve, float _startAtT, float _endAtT, SubStepDef const & _curveSubStep, float _curveDetailsCoef, Array<CrossSectionOnCurve> const * _csOnCurve)
{
	float curveLengthFromStartToEnd = _onCurve.distance(_startAtT, _endAtT);
	float curveLength = _onCurve.length();

	int segmentsRequired = _curveSubStep.calculate_sub_step_count_for(curveLengthFromStartToEnd * (_endAtT - _startAtT), _context, _curveDetailsCoef);

	bool curveLinear = _onCurve.is_linear() && !_curveSubStep.should_sub_divide_linear(_context);
	bool wholeCurveLinear = curveLinear;
	if (wholeCurveLinear && _csOnCurve)
	{
		// insert all in right order
		for_every(csSource, *_csOnCurve)
		{
			if (csSource->scaleCurve.is_set() ||
				csSource->scaleFloatCurve.is_set() ||
				csSource->morphCurve.is_set())
			{
				// we have curves! we want sub
				wholeCurveLinear = false;
			}
		}
	}
	if (wholeCurveLinear)
	{
		segmentsRequired = 1;
	}

	an_assert(segmentsRequired > 0);

	float stepLength = curveLengthFromStartToEnd / (float)segmentsRequired;
	Range posRange(_startAtT * curveLength, _endAtT * curveLength);

	csOnCurve.clear();
	csOnCurve.make_space_for((_csOnCurve ? _csOnCurve->get_size() : 0) + segmentsRequired); // will be extended if required for any reason
	if (_csOnCurve)
	{
		// insert all in right order
		for_every(csSource, *_csOnCurve)
		{
			ShapeUtils::CrossSectionOnCurve::insert(*csSource, csOnCurve);
		}
	}

	// change altIds into ids
	for_every(cs, csOnCurve)
	{
		cs->id = ShapeUtils::get_alt_cross_section_id(_cs, *cs);
	}

	if (csOnCurve.is_empty())
	{
		// just fill with straight segments required
		for (int segment = 0; segment <= segmentsRequired; ++segment)
		{
			float t = clamp((float)segment / (float)segmentsRequired, 0.0f, 1.0f);
			t = _startAtT + (_endAtT - _startAtT) * t;
			CrossSectionOnCurve csoc;
			csoc.set_pos(curveLength * t);
			csoc.finalScale = Vector2::one;
			csoc.finalMorph = 1.0f;
			csOnCurve.push_back(csoc);
		}
	}
	else
	{
		// add first and last if needed
		if (csOnCurve.get_first().get_pos() > posRange.min)
		{
			CrossSectionOnCurve csoc;
			csoc = csOnCurve.get_first();
			csoc.set_pos(posRange.min);
			csoc.id = csOnCurve.get_first().id;
			csoc.prevId = csoc.id;
			csoc.uId = csOnCurve.get_first().uId;
			csoc.scale = csOnCurve.get_first().scale;
			csoc.scaleFloat = csOnCurve.get_first().scaleFloat;
			csoc.finalScale = csOnCurve.get_first().calculate_final_scale(_context, nullptr, 1.0f);
			csoc.finalMorph = csOnCurve.get_first().calculate_final_morph(_context, nullptr, 1.0f);
			csOnCurve.insert_at(0, csoc);
		}
		if (csOnCurve.get_last().get_pos() < posRange.max)
		{
			CrossSectionOnCurve csoc;
			csoc = csOnCurve.get_last();
			csoc.set_pos(posRange.max);
			csoc.id = csOnCurve.get_last().id;
			csoc.prevId = csoc.id;
			csoc.uId = csOnCurve.get_last().uId;
			csoc.scale = csOnCurve.get_last().scale;
			csoc.scaleFloat = csOnCurve.get_last().scaleFloat;
			csoc.finalScale = csOnCurve.get_last().calculate_final_scale(_context, nullptr, 1.0f);
			csoc.finalMorph = csOnCurve.get_last().calculate_final_morph(_context, nullptr, 1.0f);
			csOnCurve.push_back(csoc);
		}
		// calculate scale for main points, before we add betweeners
		{	// first one
			auto & currCSOnCurve = csOnCurve.get_first();
			currCSOnCurve.finalScale = currCSOnCurve.calculate_final_scale(_context, nullptr, 1.0f);
			currCSOnCurve.finalMorph = currCSOnCurve.calculate_final_morph(_context, nullptr, 1.0f);
		}
		for (int i = 1; i < csOnCurve.get_size(); ++i)
		{
			auto & prevCSOnCurve = csOnCurve[i - 1];
			auto & currCSOnCurve = csOnCurve[i];
			currCSOnCurve.prevId = prevCSOnCurve.id;
			if (!prevCSOnCurve.scaleCurve.is_set() &&
				!prevCSOnCurve.scaleFloatCurve.is_set())
			{
				// to use current's scale curve
				prevCSOnCurve.finalScale = currCSOnCurve.calculate_final_scale(_context, &prevCSOnCurve, 0.0f);
			}
			if (!prevCSOnCurve.finalMorph.is_set())
			{
				prevCSOnCurve.finalMorph = currCSOnCurve.calculate_final_morph(_context, &prevCSOnCurve, 0.0f);
			}
			currCSOnCurve.finalScale = currCSOnCurve.calculate_final_scale(_context, &prevCSOnCurve, 1.0f);
			currCSOnCurve.finalMorph = currCSOnCurve.calculate_final_morph(_context, &prevCSOnCurve, 1.0f);
		}
		// fill everything between
		for (int i = 1; i < csOnCurve.get_size(); ++i)
		{
			// do not use references, as we add stuff
			auto const prevCSOnCurve = csOnCurve[i - 1];
			auto const currCSOnCurve = csOnCurve[i];
			float prevPos = prevCSOnCurve.get_pos();
			float currPos = currCSOnCurve.get_pos();
			float diff = currPos - prevPos;
			bool linearNow = false;
			float stepLengthNow = stepLength;
			if (currCSOnCurve.morphCurve.is_set() && currCSOnCurve.morphCurveStepLength.is_set())
			{
				stepLengthNow = currCSOnCurve.morphCurveStepLength.get(_context);
			}
			if (curveLinear)
			{
				if (!currCSOnCurve.scaleCurve.is_set() &&
					!currCSOnCurve.scaleFloatCurve.is_set() &&
					!currCSOnCurve.morphCurve.is_set())
				{
					// can be linear as curve is linear and there is no scale and no morph
					linearNow = true;
				}
			}
			if (diff > stepLengthNow && ! linearNow)
			{
				Name useId = currCSOnCurve.id;
				Name usePrevId = prevCSOnCurve.id;
				Optional<Name> useUId = currCSOnCurve.uId;
				int howMany = max(1, (int)((diff / stepLengthNow) - 0.5f));
				float invHowMany = 1.0f / (float)(howMany + 1); // for 1 we need to place something at 0.5, which is one between 0 and 1
				for (int j = 0; j < howMany; ++j)
				{
					float jt = (float)(j + 1) * invHowMany;
					CrossSectionOnCurve csoc;
					csoc.id = useId;
					csoc.prevId = usePrevId;
					csoc.uId = useUId;
					csoc.set_pos(prevPos + diff * currCSOnCurve.transform_t(jt));
					csoc.finalScale = currCSOnCurve.calculate_final_scale(_context, &prevCSOnCurve, jt);
					csoc.finalMorph = currCSOnCurve.calculate_final_morph(_context, &prevCSOnCurve, jt);
					ShapeUtils::CrossSectionOnCurve::insert(csoc, csOnCurve);
					++i; // we added one, get to next one
				}
			}
		}
#ifdef AN_DEVELOPMENT
		for_every(csoc, csOnCurve)
		{
			an_assert(csoc->finalScale.is_set(), TXT("we should have set it in code above"));
			an_assert(csoc->finalMorph.is_set(), TXT("we should have set it in code above"));
		}
#endif
	}
}

void ShapeUtils::build_cross_section_mesh(::Meshes::Builders::IPU & _ipu, GenerationContext & _context, ElementInstance & _elementInstance, CreateCollisionInfo const & _createMovementCollisionBoxes, CreateCollisionInfo const & _createPreciseCollisionBoxes, bool _debugDraw, Array<CrossSection> const & _cs, Vector2 const & _scaleCS, Vector2 const & _offsetCS,
	BezierCurve<Vector3> const & _onCurve, float _startAtT, float _endAtT, bool _forward, SubStepDef const & _curveSubStep, float _curveDetailsCoef, Array<CrossSectionOnCurve> const * _csOnCurve, OUT_ Array<CrossSectionOnCurve> * _usedCSOnCurve, Array<CustomOnCurve> const* _customOnCurve,
	CalculateNormal _calculate_normal, CalculateBones _calculate_bones)
{
	an_assert(_startAtT >= 0.0f && _startAtT <= 1.0f);
	an_assert(_endAtT >= 0.0f && _endAtT <= 1.0f);

	float curveLength = _onCurve.length();

	// fill with crossSectionOnCurve definitions
	Array<CrossSectionOnCurve> localCSOnCurve;
	Array<CrossSectionOnCurve>& csOnCurve = _usedCSOnCurve ? *_usedCSOnCurve : localCSOnCurve;

	setup_cs_on_curve(OUT_ csOnCurve, _context, _cs, _onCurve, _startAtT, _endAtT, _curveSubStep, _curveDetailsCoef, _csOnCurve);

	float invCurveLength = curveLength != 0.0f? 1.0f / curveLength : 0.0f;
	int prevPointsStartAt = 0;
	bool firstOne = true;
	CrossSection csCurrent;
#ifdef AN_DEVELOPMENT
	Vector3 debugDrawPrevLoc = Vector3::zero;
#endif
	Matrix44 prevMovementCollisionTransform;
	int prevMovementCollisionPolygonsSoFar = _ipu.get_polygon_count();
	float prevMovementCollisionPos = csOnCurve.get_first().get_pos();
	float movementCollisionSubStep = _createMovementCollisionBoxes.subStep.get_with_default(_context, 0.0f);

	Matrix44 prevPreciseCollisionTransform;
	int prevPreciseCollisionPolygonsSoFar = _ipu.get_polygon_count();
	float prevPreciseCollisionPos = csOnCurve.get_first().get_pos();
	float preciseCollisionSubStep = _createPreciseCollisionBoxes.subStep.get_with_default(_context, 0.0f);

	if (_customOnCurve)
	{
		for_every(coc, *_customOnCurve)
		{
			float pos = coc->get_pos();
			float t = pos * invCurveLength;

			Vector3 location;
			Vector3 tangent;
			Vector3 normal;
			Vector3 right;

			calculate_axes_for(_onCurve, t, _forward, OUT_ location, OUT_ tangent, OUT_ normal, OUT_ right, _calculate_normal);
			Transform placement = look_matrix(location, tangent, normal).to_transform();

			if (coc->createMeshNode.is_valid())
			{
				MeshNodePtr meshNode(new MeshNode());
				meshNode->name = coc->createMeshNode;
				meshNode->placement = placement;
				_context.access_mesh_nodes().push_back(meshNode);
			}
		}
	}

	for_every(csoc, csOnCurve)
	{
		float pos = csoc->get_pos();
		float t = pos * invCurveLength;

		an_assert(csoc->finalScale.is_set(), TXT("we should have set it in code above"));
		an_assert(csoc->finalMorph.is_set(), TXT("we should have set it in code above"));

		Matrix44 transform;
		morph_cross_section(_cs, OUT_ csCurrent, csoc->prevId, csoc->id, csoc->finalMorph.get());
		int pointsStartAt = add_single_cross_section_points(_ipu, _context, _debugDraw, csCurrent, _scaleCS * csoc->finalScale.get(), _offsetCS, _onCurve, t, _forward, _calculate_normal, _calculate_bones, &transform);

#ifdef AN_DEVELOPMENT
		if (_debugDraw && _context.debugRendererFrame)
		{
			Vector3 debugDrawCurrLoc = _onCurve.calculate_at(t);
			if (for_everys_index(csoc) > 0)
			{
				_context.debugRendererFrame->add_line(true, Colour::green, debugDrawPrevLoc, debugDrawCurrLoc);
			}
			debugDrawPrevLoc = debugDrawCurrLoc;
		}
#endif

		bool createMovementCollisionNow = (pos - prevMovementCollisionPos) >= movementCollisionSubStep || for_everys_index(csoc) == csOnCurve.get_size() - 1;
		bool createPreciseCollisionNow = (pos - prevPreciseCollisionPos) >= preciseCollisionSubStep || for_everys_index(csoc) == csOnCurve.get_size() - 1;
		if (!firstOne)
		{
			int pointIdx = pointsStartAt;
			int prevSegmentPointIdx = prevPointsStartAt;
			CrossSection const & csForU = get_cross_section(_cs, csoc->uId.get(csoc->id));
			for_every(u, csForU.us)
			{
				_ipu.add_quad(u->get_u(_context), prevSegmentPointIdx, pointIdx, pointIdx + 1, prevSegmentPointIdx + 1);
				++pointIdx;
				++prevSegmentPointIdx;
			}

			if (_createMovementCollisionBoxes.create && createMovementCollisionNow &&
				prevMovementCollisionTransform.get_translation() != transform.get_translation())
			{
				Matrix44 useTransform = look_at_matrix(prevMovementCollisionTransform.get_translation(), transform.get_translation(), prevMovementCollisionTransform.get_z_axis());
				_context.store_movement_collision_part(create_collision_box_at(useTransform, _ipu, _context, _elementInstance, _createMovementCollisionBoxes, prevMovementCollisionPolygonsSoFar));
			}
			if (_createPreciseCollisionBoxes.create && createPreciseCollisionNow &&
				prevPreciseCollisionTransform.get_translation() != transform.get_translation())
			{
				Matrix44 useTransform = look_at_matrix(prevPreciseCollisionTransform.get_translation(), transform.get_translation(), prevPreciseCollisionTransform.get_z_axis());
				_context.store_precise_collision_part(create_collision_box_at(useTransform, _ipu, _context, _elementInstance, _createPreciseCollisionBoxes, prevPreciseCollisionPolygonsSoFar));
			}
		}

		prevPointsStartAt = pointsStartAt;
		if (createMovementCollisionNow || firstOne)
		{
			prevMovementCollisionTransform = transform;
			prevMovementCollisionPolygonsSoFar = _ipu.get_polygon_count();
			prevMovementCollisionPos = pos;
		}
		if (createPreciseCollisionNow || firstOne)
		{
			prevPreciseCollisionTransform = transform;
			prevPreciseCollisionPolygonsSoFar = _ipu.get_polygon_count();
			prevPreciseCollisionPos = pos;
		}
		firstOne = false;
 	}
}

void ShapeUtils::build_cross_section_mesh_using_window_corners(::Meshes::Builders::IPU & _ipu, GenerationContext & _context, ElementInstance & _elementInstance, CreateCollisionInfo const & _createMovementCollisionBoxes, CreateCollisionInfo const & _createPreciseCollisionBoxes, bool _debugDraw, Array<CrossSection> const & _cs, Vector2 const & _scaleCS, Vector2 const & _offsetCS,
	Range2 const & _curveWindow, BezierCurve<Vector3> const * _onCurveWindowCorners, 
	BezierCurve<Vector3> const & _onCurve, float _startAtT, float _endAtT, SubStepDef const & _curveSubStep, float _curveDetailsCoef, Array<CrossSectionOnCurve> const * _csOnCurve,
	CalculateBones _calculate_bones)
{
	an_assert(_startAtT >= 0.0f && _startAtT <= 1.0f);
	an_assert(_endAtT >= 0.0f && _endAtT <= 1.0f);

	float curveLength = _onCurve.length();

	// fill with crossSectionOnCurve definitions
	Array<CrossSectionOnCurve> csOnCurve;

	setup_cs_on_curve(OUT_ csOnCurve, _context, _cs, _onCurve, _startAtT, _endAtT, _curveSubStep, _curveDetailsCoef, _csOnCurve);

	float invCurveLength = 1.0f / curveLength;
	int prevPointsStartAt = 0;
	bool firstOne = true;
#ifdef AN_DEVELOPMENT
	Vector3 debugDrawPrevLoc = Vector3::zero;
#endif
	Matrix44 prevMovementCollisionTransform;
	int prevMovementCollisionPolygonsSoFar = _ipu.get_polygon_count();
	float prevMovementCollisionPos = csOnCurve.get_first().get_pos();
	float movementCollisionSubStep = _createMovementCollisionBoxes.subStep.get_with_default(_context, 0.0f);

	Matrix44 prevPreciseCollisionTransform;
	int prevPreciseCollisionPolygonsSoFar = _ipu.get_polygon_count();
	float prevPreciseCollisionPos = csOnCurve.get_first().get_pos();
	float preciseCollisionSubStep = _createPreciseCollisionBoxes.subStep.get_with_default(_context, 0.0f);

	for_every(csoc, csOnCurve)
	{
		float pos = csoc->get_pos();
		float t = pos * invCurveLength;

		CrossSection const & cs = get_cross_section(_cs, csoc->id);
		an_assert(csoc->finalScale.is_set(), TXT("we should have set it in code above"));
		int pointsStartAt;
		Matrix44 transform;
		{
			Meshes::VertexSkinningInfo skinningInfo = _calculate_bones(_onCurve, t);

			Vector3 corners[4];
			for_count(int, c, 4)
			{
				corners[c] = _onCurveWindowCorners[c].calculate_at(t);
			}

			pointsStartAt = add_single_cross_section_points_using_window_corners(_ipu, _context, _debugDraw, cs, _scaleCS * csoc->finalScale.get(), _offsetCS, _curveWindow, corners, &skinningInfo, &transform);
		}

#ifdef AN_DEVELOPMENT
		if (_debugDraw && _context.debugRendererFrame)
		{
			Vector3 debugDrawCurrLoc = _onCurve.calculate_at(t);
			if (for_everys_index(csoc) > 0)
			{
				_context.debugRendererFrame->add_line(true, Colour::yellow, debugDrawPrevLoc, debugDrawCurrLoc);
			}
			debugDrawPrevLoc = debugDrawCurrLoc;
		}
#endif

		bool createMovementCollisionNow = (pos - prevMovementCollisionPos) >= movementCollisionSubStep || for_everys_index(csoc) == csOnCurve.get_size() - 1;
		bool createPreciseCollisionNow = (pos - prevPreciseCollisionPos) >= preciseCollisionSubStep || for_everys_index(csoc) == csOnCurve.get_size() - 1;
		if (!firstOne)
		{
			int pointIdx = pointsStartAt;
			int prevSegmentPointIdx = prevPointsStartAt;
			CrossSection const & csForU = get_cross_section(_cs, csoc->uId.get(csoc->id));
			for_every(u, csForU.us)
			{
				_ipu.add_quad(u->get_u(_context), prevSegmentPointIdx, pointIdx, pointIdx + 1, prevSegmentPointIdx + 1);
				++pointIdx;
				++prevSegmentPointIdx;
			}

			if (_createMovementCollisionBoxes.create && createMovementCollisionNow &&
				prevMovementCollisionTransform.get_translation() != transform.get_translation())
			{
				Matrix44 useTransform = look_at_matrix(prevMovementCollisionTransform.get_translation(), transform.get_translation(), prevMovementCollisionTransform.get_z_axis());
				_context.store_movement_collision_part(create_collision_box_at(useTransform, _ipu, _context, _elementInstance, _createMovementCollisionBoxes, prevMovementCollisionPolygonsSoFar));
			}
			if (_createPreciseCollisionBoxes.create && createPreciseCollisionNow &&
				prevPreciseCollisionTransform.get_translation() != transform.get_translation())
			{
				Matrix44 useTransform = look_at_matrix(prevPreciseCollisionTransform.get_translation(), transform.get_translation(), prevPreciseCollisionTransform.get_z_axis());
				_context.store_precise_collision_part(create_collision_box_at(useTransform, _ipu, _context, _elementInstance, _createPreciseCollisionBoxes, prevPreciseCollisionPolygonsSoFar));
			}
		}

		prevPointsStartAt = pointsStartAt;
		if (createMovementCollisionNow || firstOne)
		{
			prevMovementCollisionTransform = transform;
			prevMovementCollisionPolygonsSoFar = _ipu.get_polygon_count();
			prevMovementCollisionPos = pos;
		}
		if (createPreciseCollisionNow || firstOne)
		{
			prevPreciseCollisionTransform = transform;
			prevPreciseCollisionPolygonsSoFar = _ipu.get_polygon_count();
			prevPreciseCollisionPos = pos;
		}
		firstOne = false;
	}
}

int ShapeUtils::add_single_cross_section_points(::Meshes::Builders::IPU & _ipu, GenerationContext const & _context, bool _debugDraw, CrossSection const & _cs, Vector2 const & _scaleCS, Vector2 const & _offsetCS,
	BezierCurve<Vector3> const & _onCurve, float _atT, bool _forward,
	CalculateNormal _calculate_normal, CalculateBones _calculate_bones,
	OPTIONAL_ Matrix44* _transform)
{
	Vector3 location;
	Vector3 tangent;
	Vector3 normal;
	Vector3 right;

	calculate_axes_for(_onCurve, _atT, _forward, OUT_ location, OUT_ tangent, OUT_ normal, OUT_ right, _calculate_normal);

	an_assert(!right.is_zero());
	an_assert(!normal.is_zero());
	an_assert(!tangent.is_zero());

	Meshes::VertexSkinningInfo skinningInfo = _calculate_bones(_onCurve, _atT);

	return add_single_cross_section_points(_ipu, _context, _debugDraw, _cs, _scaleCS, _offsetCS, location, tangent, normal, right, &skinningInfo, _transform);
}

void ShapeUtils::calculate_axes_for(BezierCurve<Vector3> const & _onCurve, float _atT, bool _forward,
	OUT_ Vector3 & _location, OUT_ Vector3 & _tangent, OUT_ Vector3 & _normal, OUT_ Vector3 & _right,
	CalculateNormal _calculate_normal)
{
	an_assert(_atT >= 0.0f && _atT <= 1.0f);

	_location = _onCurve.calculate_at(_atT);
	_tangent = _onCurve.calculate_tangent_at(_atT) * (_forward? 1.0f : -1.0f);
	_normal = _calculate_normal(_onCurve, _atT, _tangent);
	an_assert(!_normal.is_zero());
	_right = Vector3::cross(_tangent, _normal).normal(); // right should be perpendicular to both tangent and normal
	_normal = Vector3::cross(_right, _tangent).normal(); // normal we get from calculation could be not perpendicular to tangent, make it be
}

void ShapeUtils::calculate_matrix_for(BezierCurve<Vector3> const & _onCurve, float _atT, bool _forward, OUT_ Matrix44 & _matrix,
	CalculateNormal _calculate_normal)
{
	Vector3 location;
	Vector3 tangent;
	Vector3 normal;
	Vector3 right;

	calculate_axes_for(_onCurve, _atT, _forward, OUT_ location, OUT_ tangent, OUT_ normal, OUT_ right, _calculate_normal);

	_matrix = matrix_from_axes_orthonormal_check(right, tangent, normal, location);
}

void ShapeUtils::calculate_transform_for(BezierCurve<Vector3> const & _onCurve, float _atT, bool _forward, OUT_ Transform & _transform,
	CalculateNormal _calculate_normal)
{
	Vector3 location;
	Vector3 tangent;
	Vector3 normal;
	Vector3 right;

	calculate_axes_for(_onCurve, _atT, _forward, OUT_ location, OUT_ tangent, OUT_ normal, OUT_ right, _calculate_normal);

	_transform.build_from_axes(Axis::Forward, Axis::Up, tangent, Vector3::zero /* will be auto calculated */, normal, location);
}

int ShapeUtils::add_single_cross_section_points(::Meshes::Builders::IPU & _ipu, GenerationContext const & _context, bool _debugDraw, CrossSection const & _cs, Vector2 const & _scaleCS, Vector2 const & _offsetCS,
	Vector3 const & _location, Vector3 const & _tangent, Vector3 const & _normal, Vector3 const & _right,
	Meshes::VertexSkinningInfo const * _skinningInfo,
	OPTIONAL_ Matrix44* _transform)
{
	Matrix44 transform = matrix_from_axes_orthonormal_check(_right, _tangent, _normal, _location);

	assign_optional_out_param(_transform, transform);

	int pointsStartAt = _ipu.get_point_count();
	for_every(point, _cs.points)
	{
		_ipu.add_point(transform.location_to_world(Vector3(point->x * _scaleCS.x + _offsetCS.x, 0.0f, point->y * _scaleCS.y + _offsetCS.y)), NP, _skinningInfo);
	}

	return pointsStartAt;
}

int ShapeUtils::add_single_cross_section_points_using_window_corners(::Meshes::Builders::IPU & _ipu, GenerationContext const & _context, bool _debugDraw, CrossSection const & _cs, Vector2 const & _scaleCS, Vector2 const & _offsetCS,
	Range2 const & _curveWindow, Vector3 const * _corners,
	Meshes::VertexSkinningInfo const * _skinningInfo,
	OPTIONAL_ Matrix44* _transform)
{
	int pointsStartAt = _ipu.get_point_count();
	for_every(point, _cs.points)
	{
		// get actual point (scaled and offset)
		Vector2 p = *point * _scaleCS + _offsetCS;
		// where in window it is
		Vector2 pt(_curveWindow.x.get_pt_from_value(p.x), _curveWindow.y.get_pt_from_value(p.y));
		// calculate location using corners and in-window location
		Vector3 l = _corners[ShapeUtils::LB] * (1.0f - pt.y) + _corners[ShapeUtils::LT] * pt.y;
		Vector3 r = _corners[ShapeUtils::RB] * (1.0f - pt.y) + _corners[ShapeUtils::RT] * pt.y;
		Vector3 loc = l * (1.0f - pt.x) + r * pt.x;
		_ipu.add_point(loc, NP, _skinningInfo);
	}

	if (_transform)
	{
		Vector2 pt(_curveWindow.x.get_pt_from_value(0.0f), _curveWindow.y.get_pt_from_value(0.0f));
		// calculate location using corners and in-window location
		Vector3 l = _corners[ShapeUtils::LB] * (1.0f - pt.y) + _corners[ShapeUtils::LT] * pt.y;
		Vector3 r = _corners[ShapeUtils::RB] * (1.0f - pt.y) + _corners[ShapeUtils::RT] * pt.y;
		Vector3 loc = l * (1.0f - pt.x) + r * pt.x;

		// (corners: LB, RB, LT, RT)
		Vector3 up = ((_corners[ShapeUtils::LT] - _corners[ShapeUtils::LB]).normal() + (_corners[ShapeUtils::RT] - _corners[ShapeUtils::RB]).normal());
		Vector3 right = ((_corners[ShapeUtils::RB] - _corners[ShapeUtils::LB]).normal() + (_corners[ShapeUtils::RT] - _corners[ShapeUtils::LT]).normal());
		*_transform = matrix_from_up_right(loc, up, right);
	}
	return pointsStartAt;
}

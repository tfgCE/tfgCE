#include "meshGenEdge.h"

#include "meshGenSpline.h"

#include "..\meshGenElement.h"

#include "..\..\library\library.h"

#ifdef AN_CLANG
DEFINE_STATIC_NAME(zero);
#include "meshGenSplineImpl.inl"
#include "meshGenSplineRefImpl.inl"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace MeshGeneration;
using namespace CustomData;

//

EdgeNormalAlignment::Type EdgeNormalAlignment::parse(String const & _text)
{
	if (_text == TXT("align with normals") ||
		_text == TXT("normals")) return AlignWithNormals;
	if (_text == TXT("align with left normal") ||
		_text == TXT("left normal") ||
		_text == TXT("left")) return AlignWithLeftNormal;
	if (_text == TXT("align with right normal") ||
		_text == TXT("right normal") ||
		_text == TXT("right")) return AlignWithRightNormal;
	if (_text == TXT("along left surface") ||
		_text == TXT("left surface")) return AlongLeftSurface;
	if (_text == TXT("along right surface") ||
		_text == TXT("right surface")) return AlongRightSurface;
	if (_text == TXT("aling with left surface edge") ||
		_text == TXT("left surface edge")) return AlignWithLeftSurfaceEdge;
	if (_text == TXT("aling with right surface edge") ||
		_text == TXT("right surface edge")) return AlighWithRightSurfaceEdge;
	if (_text == TXT("towards ref point")) return TowardsRefPoint;
	if (_text == TXT("away from ref point")) return AwayFromRefPoint;
	if (_text == TXT("ref dir")) return RefDir;
	error(TXT("edge normal alignment \"%S\") not recognised"), _text.to_char());
	return AlignWithNormals;
}

//

EdgeCap::~EdgeCap()
{
}

bool EdgeCap::load_from_xml(IO::XML::Node const * _node, tchar const * _childName, LibraryLoadingContext & _lc)
{
	bool result = true;

	IO::XML::Node const * node = _node->first_child_named(_childName);

	if (!node)
	{
		return true;
	}

	result &= Element::load_single_element_from_xml(node, _lc, TXT("element"), element);

	result &= generationCondition.load_from_xml(node);

	shortenEdge.load_from_xml(node, TXT("shortenEdge"));

	moveByShortenEdge = node->get_bool_attribute(TXT("moveByShortenEdge"), moveByShortenEdge);

	return result;
}

bool EdgeCap::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;
	if (element.is_set())
	{
		result &= element->prepare_for_game(_library, _pfgContext);
	}
	return result;
}

#ifdef AN_DEVELOPMENT
void EdgeCap::for_included_mesh_generators(std::function<void(MeshGenerator*)> _do) const
{
	if (element.is_set())
	{
		element->for_included_mesh_generators(_do);
	}
}
#endif

//

REGISTER_FOR_FAST_CAST(Edge);

Edge::~Edge()
{
	crossSections.clear();
	autoCSOCs.clear();
}

#ifdef AN_DEVELOPMENT
void Edge::ready_for_reload()
{
	base::ready_for_reload();

	crossSections.clear();
	autoCSOCs.clear();

	cap = EdgeCap();
	capStart = EdgeCap();
	capEnd = EdgeCap();
}
#endif

bool Edge::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= edgeSubStep.load_from_xml_child_node(_node, TXT("edgeSubStep"));
	if (IO::XML::Attribute const * attr = _node->get_attribute(TXT("normalAlignment")))
	{
		normalAlignment = EdgeNormalAlignment::parse(attr->get_as_string());
	}
	cornerRadius = _node->get_float_attribute(TXT("cornerRadius"), cornerRadius);
	cutCorner = _node->get_bool_attribute(TXT("cutCorner"), cutCorner);

	{
		IO::XML::Node const * crossSectionNoIDNode = nullptr;
		for_every(crossSectionNode, _node->children_named(TXT("crossSectionSpline")))
		{
			if (!crossSectionNode->has_attribute(TXT("id")))
			{
				crossSectionNoIDNode = crossSectionNode;
				break;
			}
		}

		// load default if we have set it as attribute or we have cross section node without id provided
		if (crossSectionNoIDNode || _node->has_attribute(TXT("crossSectionSpline")))
		{
			auto& cs = access_cross_section(Name::invalid());
			result &= cs.spline.load_from_xml_with_child_provided(_node, TXT("crossSectionSpline"), crossSectionNoIDNode, _lc);
			if (auto* s = cs.spline.get())
			{
				s->load_replace_u_from_xml(_node, _lc);
			}
		}

		// load other cross sections
		for_every(crossSectionNode, _node->children_named(TXT("crossSectionSpline")))
		{
			if (IO::XML::Attribute const* attr = crossSectionNode->get_attribute(TXT("id")))
			{
				auto& cs = access_cross_section(attr->get_as_name());
				Name copyId = crossSectionNode->get_name_attribute(TXT("copyId"));
				if (copyId.is_valid())
				{
					cs.spline.be_copy_of(access_cross_section(copyId).spline);
				}
				else if (crossSectionNode->get_bool_attribute(TXT("copyMain")))
				{
					cs.spline.be_copy_of(access_cross_section(Name::invalid()).spline);
				}
				else
				{
					result &= cs.spline.load_from_xml(crossSectionNode, TXT("crossSectionSpline"), nullptr, _lc);
				}
				Range spreadX = Range::empty;
				spreadX.load_from_attribute_or_from_child(crossSectionNode, TXT("spreadX"));
				Array<Range2> keepForSpreadX;
				for_every(kn, crossSectionNode->children_named(TXT("keepForSpreadX")))
				{
					Range2 kr = Range2::empty;
					kr.load_from_xml(kn);
					if (!kr.is_empty())
					{
						keepForSpreadX.push_back(kr);
					}
				}
				if (!spreadX.is_empty())
				{
					if (auto* s = cs.spline.get())
					{
						struct Spread
						{
							Spread(Range const& _usingRange, Array<Range2> const & _keepForSpreadX)
							: usingRange(_usingRange)
							, keepForSpreadX(_keepForSpreadX)
							{
								centre = usingRange.centre();
							}
							void spread(Vector2& _p) const
							{
								bool ok = true;
								for_every(r, keepForSpreadX)
								{
									if (r->does_contain(_p))
									{
										ok = false;
										break;
									}
								}
								if (ok)
								{
									_p.x = _p.x < centre ? usingRange.min : usingRange.max;
								}
							}
						private:
							Range usingRange;
							float centre;
							Array<Range2> const & keepForSpreadX;
						} spread(spreadX, keepForSpreadX);
						s->do_for_every_segment([spread](Spline<Vector2>::Segment& segment)
							{
								spread.spread(segment.segment.p0);
								spread.spread(segment.segment.p1);
								spread.spread(segment.segment.p2);
								spread.spread(segment.segment.p3);
							});
					}
				}
				if (auto* s = cs.spline.get())
				{
					s->load_replace_u_from_xml(crossSectionNode, _lc);
				}
			}
		}
		bool thereIsDefault = false;
		for_every(crossSection, crossSections)
		{
			thereIsDefault |= !crossSection->id.is_valid();
		}
		if (!thereIsDefault)
		{
			error_loading_xml(_node, TXT("there was no default cross section spline"));
			result = false;
		}
	}

	for_every(node, _node->children_named(TXT("autoCrossSectionOnCurve")))
	{
		if (IO::XML::Attribute const * attr = node->get_attribute(TXT("id")))
		{
			access_auto_csoc(attr->get_as_name()).load_from_xml(node);
		}
	}

	result &= crossSectionSubStep.load_from_xml_child_node(_node, TXT("crossSectionSubStep"));

	result &= cap.load_from_xml(_node, TXT("cap"), _lc);
	result &= capStart.load_from_xml(_node, TXT("capStart"), _lc);
	result &= capEnd.load_from_xml(_node, TXT("capEnd"), _lc);

	return result;
}

bool Edge::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	for_every(crossSection, crossSections)
	{
		result &= crossSection->spline.prepare_for_game(_library, _pfgContext);
	}

	result &= cap.prepare_for_game(_library, _pfgContext);
	result &= capStart.prepare_for_game(_library, _pfgContext);
	result &= capEnd.prepare_for_game(_library, _pfgContext);

	return result;
}

Edge::CrossSection & Edge::access_cross_section(Name const & _id)
{
	for_every(crossSection, crossSections)
	{
		if (crossSection->id == _id)
		{
			return *crossSection;
		}
	}

	crossSections.push_back(CrossSection());
	crossSections.get_last().id = _id;
	return crossSections.get_last();
}

Edge::AutoCrossSectionOnCurve & Edge::access_auto_csoc(Name const & _id)
{
	for_every(autoCSOC, autoCSOCs)
	{
		if (autoCSOC->id == _id)
		{
			return *autoCSOC;
		}
	}

	autoCSOCs.push_back(AutoCrossSectionOnCurve());
	autoCSOCs.get_last().id = _id;
	return autoCSOCs.get_last();
}

SplineRef<Vector2> const & Edge::get_cross_section_spline(Name const & _id) const
{
	for_every(crossSection, crossSections)
	{
		if (crossSection->id == _id)
		{
			return crossSection->spline;
		}
	}
	// get default, hoping that it does exist
	for_every(crossSection, crossSections)
	{
		if (! crossSection->id.is_valid())
		{
			return crossSection->spline;
		}
	}
	an_assert(false, TXT("could not find \"%S\" cross section"), _id.to_char());
	todo_important(TXT("what now?!"));
	return crossSections.get_first().spline;
}

#ifdef AN_DEVELOPMENT
void Edge::for_included_mesh_generators(std::function<void(MeshGenerator*)> _do) const
{
	cap.for_included_mesh_generators(_do);
	capStart.for_included_mesh_generators(_do);
	capEnd.for_included_mesh_generators(_do);
}
#endif

Edge::AutoCrossSectionOnCurve const * Edge::get_auto_csoc(Name const & _id) const
{
	for_every(autoCSOC, autoCSOCs)
	{
		if (autoCSOC->id == _id)
		{
			return autoCSOC;
		}
	}
	return nullptr;
}

//

bool Edge::AutoCrossSectionOnCurve::load_from_xml(IO::XML::Node const * _node)
{
	bool result = true;

	id = _node->get_name_attribute(TXT("id"), id);

	error_loading_xml_on_assert(id.is_valid(), result, _node, TXT("id has to be set"));

	atEdgesCentreOnly = _node->get_bool_attribute_or_from_child_presence(TXT("atEdgesCentreOnly"), atEdgesCentreOnly);
	addOnePerEdge = _node->get_bool_attribute_or_from_child_presence(TXT("addOnePerEdge"), addOnePerEdge);
	notAtCorners = _node->get_bool_attribute_or_from_child_presence(TXT("notAtCorners"), notAtCorners);

	minLength.load_from_xml(_node, TXT("minLength"));
	spacing.load_from_xml(_node, TXT("spacing"));
	border.load_from_xml(_node, TXT("border"));
	cornerBorder.load_from_xml(_node, TXT("cornerBorder"));

	if (_node->first_child_named(TXT("crossSection")))
	{
		sequences.push_back();
		result &= ShapeUtils::CrossSectionOnCurve::load_from_xml(_node, sequences.get_last().csocs, TXT("crossSection"));
	}
	for_every(node, _node->children_named(TXT("sequence")))
	{
		sequences.push_back();
		sequences.get_last().probCoef = node->get_float_attribute(TXT("probCoef"), sequences.get_last().probCoef);
		sequences.get_last().createMeshNode = node->get_name_attribute(TXT("createMeshNode"), sequences.get_last().createMeshNode);
		sequences.get_last().enforceAtEnds = node->get_bool_attribute_or_from_child_presence(TXT("enforceAtEnds"), sequences.get_last().enforceAtEnds);
		result &= ShapeUtils::CrossSectionOnCurve::load_from_xml(node, sequences.get_last().csocs, TXT("crossSection"));
	}

	return result;
}



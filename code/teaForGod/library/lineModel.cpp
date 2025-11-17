#include "lineModel.h"

#include "..\library\library.h"

#include "..\..\core\debug\debugRenderer.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;

//

REGISTER_FOR_FAST_CAST(LineModel);
LIBRARY_STORED_DEFINE_TYPE(LineModel, lineModel);

LineModel::LineModel(Framework::Library * _library, Framework::LibraryName const & _name)
: base(_library, _name)
{
}

LineModel::~LineModel()
{
}

void LineModel::debug_draw() const
{
#ifdef AN_DEBUG_RENDERER
	for_every(line, lines)
	{
		Vector3 a = line->a;
		Vector3 b = line->b;
		Colour c = line->colour.get(Colour::white);
		debug_draw_line(true, c, a, b);
	}
#endif
}

#ifdef AN_DEVELOPMENT
void LineModel::ready_for_reload()
{
	base::ready_for_reload();

	lines.clear();
}
#endif

bool LineModel::load_lines_into(IO::XML::Node const* _node, Array<Line> & _lines, LineModel const * _forLineModel)
{
	if (!_node)
	{
		error_loading_xml(_node, TXT("no lines node found"));
		return false;
	}

	bool result = true;

	Vector3 lastTo = Vector3::zero;
	Optional<Colour> colour;
	for_every(node, _node->all_children())
	{
		if (node->get_name() == TXT("colour"))
		{
			Colour c = colour.get(Colour::white);
			c.load_from_xml(node);
			colour = c;
		}
		if (node->get_name() == TXT("noColour"))
		{
			colour.clear();
		}
		if (node->get_name() == TXT("line"))
		{
			Line l;
			l.a = lastTo;
			l.colour = colour;
			if (l.load_from_xml(node))
			{
				_lines.push_back(l);
				lastTo = l.b;
			}
			else
			{
				result = false;
			}
		}
		if (node->get_name() == TXT("curve"))
		{
			BezierCurve<Vector3> curve;
			curve.p0 = lastTo;
			if (curve.load_from_xml(node, false, true))
			{
				float length = curve.length();
				float step = length * 0.1f;
				step = node->get_float_attribute_or_from_child(TXT("step"), step);
				int num = max(1, TypeConversions::Normal::f_i_closest(length / step));
				lastTo = curve.p0;
				for (int i = 1; i <= num; ++i)
				{
					Line l;
					l.colour = colour;
					l.a = lastTo;
					l.b = curve.calculate_at((float)i / (float)num);
					lastTo = l.b;
					_lines.push_back(l);
				}
			}
		}
		if (node->get_name() == TXT("include"))
		{
			Framework::LibraryName ln;
			ln.load_from_xml(node, TXT("lineModel"));
			if (!ln.get_group().is_valid())
			{
				// fill missing group, most likely the same as us
				ln.set_group(_forLineModel->get_name().get_group());
			}
			if (ln.is_valid())
			{
				Transform placement = Transform::identity;
				Vector3 scaleVector = Vector3::one;
				placement.load_from_xml_child_node(node, TXT("placement"));
				scaleVector.load_from_xml_child_node(node, TXT("scaleVector"), true);
				if (auto* lineModel = Library::get_current_as<Library>()->get_line_models().find(ln))
				{
					lineModel->load_lines_on_demand(); // make sure it is loaded
					for_every(l, lineModel->get_lines())
					{
						auto lo = *l;
						if (!lo.colour.is_set())
						{
							lo.colour = colour;
						}
						lo.a = placement.location_to_world(lo.a * scaleVector);
						lo.b = placement.location_to_world(lo.b * scaleVector);
						_lines.push_back(lo);						
					}
				}
				else
				{
#ifdef AN_DEVELOPMENT_OR_PROFILER
					if (!Library::may_ignore_missing_references())
#endif
					{
						error_loading_xml(node, TXT("did not find line model \"%S\", make sure it is loaded before this one"), ln.to_string().to_char());
						result = false;
					}
				}
			}
			else
			{
#ifdef AN_DEVELOPMENT_OR_PROFILER
				if (!Library::may_ignore_missing_references())
#endif
				{
					error_loading_xml(node, TXT("no \"lineModel\" provided"));
					result = false;
				}
			}
		}
		if (node->get_name() == TXT("place"))
		{
			bool asIs = node->get_bool_attribute_or_from_child_presence(TXT("asIs"));
			Array<Line> lines;
			if (load_lines_into(node->first_child_named(TXT("lines")), lines, _forLineModel))
			{
				Range3 r = Range3::empty;
				for_every(l, lines)
				{
					r.include(l->a);
					r.include(l->b);
				}
				Vector3 fromCentre = asIs? Vector3::zero : r.centre();
				Vector3 rFromSize = r.length();
				float fromSize = asIs ? 1.0f : max(rFromSize.x, max(rFromSize.y, rFromSize.z));

				fromCentre.load_from_xml_child_node(node, TXT("fromAt"), TXT("x"), TXT("y"), TXT("z"), TXT("axis"), true);
				fromSize = node->get_float_attribute_or_from_child(TXT("fromSize"), fromSize);

				Vector3 toCentre = fromCentre;
				float toSize = fromSize;
				Rotator3 rotate = Rotator3::zero;
				toCentre.load_from_xml_child_node(node, TXT("toAt"), TXT("x"), TXT("y"), TXT("z"), TXT("axis"), true);
				toCentre.load_from_xml_child_node(node, TXT("at"), TXT("x"), TXT("y"), TXT("z"), TXT("axis"), true);
				rotate.load_from_xml_child_node(node, TXT("rotate"));
				toSize = node->get_float_attribute_or_from_child(TXT("toSize"), toSize);
				toSize = node->get_float_attribute_or_from_child(TXT("size"), toSize);

				float resize = fromSize != 0.0f ? toSize / fromSize : toSize;
				for_every(l, lines)
				{
					Line lo = *l;
					lo.a = (lo.a - fromCentre) * resize;
					lo.b = (lo.b - fromCentre) * resize;
					if (! rotate.is_zero())
					{
						if (rotate.pitch != 0.0f)
						{
							lo.a.rotate_by_pitch(rotate.pitch);
							lo.b.rotate_by_pitch(rotate.pitch);
						}
						if (rotate.yaw != 0.0f)
						{
							lo.a.rotate_by_yaw(rotate.yaw);
							lo.b.rotate_by_yaw(rotate.yaw);
						}
						if (rotate.roll != 0.0f)
						{
							lo.a.rotate_by_roll(rotate.roll);
							lo.b.rotate_by_roll(rotate.roll);
						}
					}
					lo.a += toCentre;
					lo.b += toCentre;
					if (!lo.colour.is_set())
					{
						lo.colour = colour;
					}
					_lines.push_back(lo);
				}
			}
			else
			{
				result = false;
			}
		}
	}
	
	return result;
}

bool LineModel::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	loadFromNode = _node;

	return result;
}

bool LineModel::load_lines_on_demand()
{
	bool result = true;
	Concurrency::ScopedSpinLock lock(prepareLock);

	if (auto* _node = loadFromNode.get())
	{
		Optional<float> scaleFrom;
		Optional<float> scaleTo;
		Optional<float> scaleFillTo;

		for_every(node, _node->children_named(TXT("scale")))
		{
			scaleFrom.load_from_xml(node, TXT("from"));
			scaleTo.load_from_xml(node, TXT("to"));
		}
		for_every(node, _node->children_named(TXT("scaleFill")))
		{
			scaleFillTo.load_from_xml(node, TXT("to"));
		}

		result &= load_lines_into(_node, lines, this);

		if (scaleTo.is_set())
		{
			float from = 0.0f;
			if (scaleFrom.is_set())
			{
				from = scaleFrom.get();
			}
			else
			{
				for_every(l, lines)
				{
					from = max(from, abs(l->a.x));
					from = max(from, abs(l->a.y));
					from = max(from, abs(l->a.z));
					from = max(from, abs(l->b.x));
					from = max(from, abs(l->b.y));
					from = max(from, abs(l->b.z));
				}
			}

			if (from > 0.0f)
			{
				float scale = scaleTo.get() / from;
				for_every(l, lines)
				{
					l->a *= scale;
					l->b *= scale;
				}
			}
		}

		if (scaleFillTo.is_set())
		{
			Range3 r = Range3::empty;
			for_every(l, lines)
			{
				r.include(l->a);
				r.include(l->b);
			}

			Vector3 offset = -r.centre();
			r = Range3::empty;
			for_every(l, lines)
			{
				l->a += offset;
				l->b += offset;
				r.include(l->a);
				r.include(l->b);
			}

			Range rr = r.x;
			rr.include(r.y);
			rr.include(r.z);

			for_every(l, lines)
			{
				if (rr.max > rr.min &&
					r.x.max > r.x.min)
				{
					l->b.x = scaleFillTo.get() * (-1.0f + 2.0f * (l->b.x - rr.min) / (rr.max - rr.min));
					l->a.x = scaleFillTo.get() * (-1.0f + 2.0f * (l->a.x - rr.min) / (rr.max - rr.min));
				}
				if (rr.max > rr.min &&
					r.y.max > r.y.min)
				{
					l->a.y = scaleFillTo.get() * (-1.0f + 2.0f * (l->a.y - rr.min) / (rr.max - rr.min));
					l->b.y = scaleFillTo.get() * (-1.0f + 2.0f * (l->b.y - rr.min) / (rr.max - rr.min));
				}
				if (rr.max > rr.min &&
					r.z.max > r.z.min)
				{
					l->a.z = scaleFillTo.get() * (-1.0f + 2.0f * (l->a.z - rr.min) / (rr.max - rr.min));
					l->b.z = scaleFillTo.get() * (-1.0f + 2.0f * (l->b.z - rr.min) / (rr.max - rr.min));
				}
			}
		}
	}

	loadFromNode.clear();

	return result;
}

bool LineModel::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	IF_PREPARE_LEVEL(_pfgContext, Framework::LibraryPrepareLevel::CreateFromTemplates)
	{
		result &= load_lines_on_demand();
	}

	return result;
}

void LineModel::clear()
{
	lines.clear();
}

void LineModel::add_line(Line const& _l)
{
	lines.push_back(_l);
}

void LineModel::mirror(Plane const& _p)
{
	for (int i = 0; i < lines.get_size(); ++i)
	{
		auto& l = lines[i];
		float af = _p.get_in_front(l.a);
		float bf = _p.get_in_front(l.b);
		if (af <= 0.0f && bf <= 0.0f && (af + bf) < 0.0f) // can be on the mirror's surface but can't be both
		{
			lines.remove_at(i);
			--i;
		}
		else
		{
			if (af != bf && af * bf < 0.0f)
			{
				float t = clamp((0.0f - af) / (bf - af), 0.0f, 1.0f);
				if (af < 0.0f)
				{
					l.a = l.a + (l.b - l.a) * t;
				}
				else if (bf < 0.0f)
				{
					l.b = l.a + (l.b - l.a) * t;
				}
			}
		}
	}

	int notMirroredCount = lines.get_size();
	lines.make_space_for(lines.get_size() * 2);
	for (int i = 0; i < notMirroredCount; ++i)
	{
		auto& l = lines[i];
		Line nl = l;
		float af = _p.get_in_front(l.a);
		float bf = _p.get_in_front(l.b);
		nl.a -= _p.get_normal() * af * 2.0f;
		nl.b -= _p.get_normal() * bf * 2.0f;
		lines.push_back(nl);
	}
}

void LineModel::fit_xy(Range2 const & _r)
{
	Range2 r = Range2::empty;
	for_every(l, lines)
	{
		r.include(l->a.to_vector2());
		r.include(l->b.to_vector2());
	}

	Vector3 centre = r.centre().to_vector3();
	Vector3 newCentre = _r.centre().to_vector3();
	float scale = 10000.0f;
	if (r.length().x > 0.0f)
	{
		scale = min(scale, _r.length().x / r.length().x);
	}
	if (r.length().y > 0.0f)
	{
		scale = min(scale, _r.length().y / r.length().y);
	}
	for_every(l, lines)
	{
		l->a = (l->a - centre) * scale + newCentre;
		l->b = (l->b - centre) * scale + newCentre;
	}
}

//

bool LineModel::Line::load_from_xml(IO::XML::Node const* _node)
{
	bool result = true;

	a.load_from_xml_child_node(_node, TXT("a"), TXT("x"), TXT("y"), TXT("z"), TXT("axis"), true);
	a.load_from_xml_child_node(_node, TXT("from"), TXT("x"), TXT("y"), TXT("z"), TXT("axis"), true);

	b.load_from_xml_child_node(_node, TXT("b"), TXT("x"), TXT("y"), TXT("z"), TXT("axis"), true);
	b.load_from_xml_child_node(_node, TXT("to"), TXT("x"), TXT("y"), TXT("z"), TXT("axis"), true);

	if (auto* n = _node->first_child_named(TXT("toRadial")))
	{
		float length = n->get_float_attribute_or_from_child(TXT("length"), 0.0f);

		b = a + a.normal() * length;
	}

	colour.load_from_xml(_node, TXT("colour"));

	return result;
}
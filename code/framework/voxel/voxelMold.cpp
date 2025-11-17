#include "voxelMold.h"

#include "..\..\core\debug\debugRenderer.h"
#include "..\..\core\other\parserUtils.h"

#include "..\editor\editors\editorVoxelMold.h"
#include "..\library\library.h"
#include "..\library\usedLibraryStored.inl"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#define RENDER_PASS_NORMAL			0
#define RENDER_PASS_EDITED_LAYER	1
#define RENDER_PASS_COUNT			2

#define CONTENT_ACCESS VoxelMoldContentAccess

//

using namespace Framework;

// layer types
DEFINE_STATIC_NAME_STR(layerType_transform, TXT("transform"));
DEFINE_STATIC_NAME_STR(layerType_fullTransform, TXT("fullTransform"));
DEFINE_STATIC_NAME_STR(layerType_include, TXT("include"));

//

//--

int voxel_index(VectorInt3 const& _coord, RangeInt3 const& _range)
{
	if (!_range.does_contain(_coord))
	{
		return NONE;
	}
	int const rangex = _range.x.length();
	int const rangey = _range.y.length();
	int const index
		/*
		=	(_coord.z - _range.z.min) * rangex * rangey
		+	(_coord.y - _range.y.min) * rangex
		+	(_coord.x - _range.x.min)
		*/
		= ((_coord.z - _range.z.min) * rangey
			+ (_coord.y - _range.y.min)) * rangex
		+ (_coord.x - _range.x.min)
		;
	return index;
}


//--

REGISTER_FOR_FAST_CAST(VoxelMold);
LIBRARY_STORED_DEFINE_TYPE(VoxelMold, voxelMold);

VoxelMold::VoxelMold(Library* _library, LibraryName const& _name)
: base(_library, _name)
{
	resultLayer = new VoxelMoldLayer();
}

VoxelMold::~VoxelMold()
{

}

#ifdef AN_DEVELOPMENT
void VoxelMold::ready_for_reload()
{
	base::ready_for_reload();

	if (auto* vl = fast_cast<VoxelMoldLayer>(resultLayer.get()))
	{
		vl->reset();
	}
}
#endif

void VoxelMold::prepare_to_unload()
{
	base::prepare_to_unload();

	if (auto* vl = fast_cast<VoxelMoldLayer>(resultLayer.get()))
	{
		vl->reset();
	}
}

bool VoxelMold::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	// clear/reset
	//
	colourPalette.clear();
	voxelSize = 0.01f;
	zeroOffset = Vector3::zero;
	layers.clear();
	resultLayer->clear();
	editedLayerIndex = NONE;

	// load
	//
	result &= colourPalette.load_from_xml(_node, TXT("colourPalette"), _lc);
	for_every(node, _node->children_named(TXT("voxel")))
	{
		voxelSize = node->get_float_attribute(TXT("size"), voxelSize);
	}
	zeroOffset.load_from_xml_child_node(_node, TXT("zeroOffset"));

	for_every(node, _node->children_named(TXT("layer")))
	{
		RefCountObjectPtr<IVoxelMoldLayer> layer;
		layer = IVoxelMoldLayer::create_layer_for(node);
		if (layer.get())
		{
			if (layer->load_from_xml(node, _lc))
			{
				layers.push_back(layer);
			}
			else
			{
				error_loading_xml(node, TXT("unable to load voxel mold layer"));
				result = false;
			}
		}
		else
		{
			error_loading_xml(node, TXT("unable to create a voxel mold layer when loading"));
			result = false;
		}
	}

	{
		for_every(node, _node->children_named(TXT("editedLayer")))
		{
			editedLayerIndex = node->get_int_attribute(TXT("index"), editedLayerIndex);
		}
	}

	return result;
}

bool VoxelMold::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	result &= colourPalette.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);

	for_every_ref(layer, layers)
	{
		result &= layer->prepare_for_game(_library, _pfgContext);
	}

	IF_PREPARE_LEVEL(_pfgContext, LibraryPrepareLevel::CombineVoxelMoldLayers)
	{
		CONTENT_ACCESS::combine_voxels(this);
	}

	return result;
}

bool VoxelMold::save_to_xml(IO::XML::Node* _node) const
{
	bool result = base::save_to_xml(_node);

	CONTENT_ACCESS::prune(cast_to_nonconst(this));

	colourPalette.save_to_xml(_node, TXT("colourPalette"));

	if (auto* node = _node->add_node(TXT("voxel")))
	{
		node->set_float_attribute(TXT("size"), voxelSize);
	}
	zeroOffset.save_to_xml_child_node(_node, TXT("zeroOffset"));

	{
		if (editedLayerIndex != NONE)
		{
			auto* n = _node->add_node(TXT("editedLayer"));
			n->set_int_attribute(TXT("index"), editedLayerIndex);
		}
	}

	for_every_ref(layer, layers)
	{
		if (auto* n = IVoxelMoldLayer::create_node_for(_node, layer))
		{
			result &= layer->save_to_xml(n);
		}
	}

	return result;
}

void VoxelMold::render(RenderContext const & _context) const
{
#ifdef AN_DEBUG_RENDERER
	struct Light
	{
		Vector3 lightDir = Vector3(-0.2f, -0.7f, 2.0f).normal();
		float useLight = 0.2f;
		Colour apply(Colour const& _colour, Vector3 const& _normal)
		{
			float lightDot = Vector3::dot(_normal, lightDir);
			float useLightNow = 1.0f - useLight * (1.0f - lightDot);
			return _colour.mul_rgb(useLightNow);
		}
	} light;
	light.lightDir = _context.lightDir.normal();
	light.useLight = _context.useLight;
	for_count(int, renderPass, RENDER_PASS_COUNT)
	{
		IVoxelMoldLayer* layer = nullptr;
		VoxelMoldLayer* voxelLayer = nullptr; // they might be different (we have IncludeVoxelMoldLayer's result layer)
		Optional<Colour> voxelColour;
		if (renderPass == RENDER_PASS_NORMAL)
		{
			layer = fast_cast<VoxelMoldLayer>(resultLayer.get());
		}
		else if (renderPass == RENDER_PASS_EDITED_LAYER && _context.editedLayerColour.is_set())
		{
			if (layers.is_index_valid(editedLayerIndex))
			{
				auto* l = layers[editedLayerIndex].get();
				if (auto* vml = fast_cast<VoxelMoldLayer>(l))
				{
					layer = vml;
					voxelLayer = vml;
				}
				if (auto* ivml = fast_cast<IncludeVoxelMoldLayer>(l))
				{
					layer = ivml;
					voxelLayer = ivml->get_included_layer();
				}
				if (layer)
				{
					voxelColour = _context.editedLayerColour;
				}
			}
		}
		auto* rl = fast_cast<VoxelMoldLayer>(resultLayer.get());
		if (layer && rl)
		{
			RangeInt3 range = rl->range;
			int rangex = range.x.length();
			int rangey = range.y.length();
			int zOffset = rangex * rangey;
			int yOffset = rangex;
			auto* cp = colourPalette.get();
			for_range(int, z, range.z.min, range.z.max)
			{
				for_range(int, y, range.y.min, range.y.max)
				{
					VectorInt3 startAt(range.x.min, y, z);
					Voxel const * v = &rl->voxels[voxel_index(startAt, range)];
					Voxel const * vxp = v - 1;
					Voxel const * vxn = v + 1;
					Voxel const * vyp = v - yOffset;
					Voxel const * vyn = v + yOffset;
					Voxel const * vzp = v - zOffset;
					Voxel const * vzn = v + zOffset;
					for_range(int, x, range.x.min, range.x.max)
					{
						bool drawVoxel = Voxel::is_drawable(v->voxel);
						if (resultLayer.get() != layer && layer && voxelLayer)
						{
							drawVoxel = false;
							VectorInt3 at(x, y, z);
							VectorInt3 atLocal = layer->to_local(cast_to_nonconst(this), at);
							if (auto* v = voxelLayer->get_at(atLocal))
							{
								drawVoxel = Voxel::is_drawable(v->voxel);
							}
						}
						if (drawVoxel)
						{
							Colour colour = Colour::greyLight;
							if (cp && cp->get_colours().is_index_valid(v->voxel))
							{
								colour = cp->get_colours()[v->voxel];
							}
							Vector3 cmin = VectorInt3(x, y, z).to_vector3();
							cmin -= zeroOffset;
							Vector3 cmax = cmin + Vector3::one;
							cmin *= voxelSize;
							cmax *= voxelSize;
							Optional<bool> highlighted;
							float alpha = 1.0f;
							if (_context.highlightPlane.is_set())
							{
								struct AboveBelow
								{
									Plane plane;
									int above = 0;
									int below = 0;
									AboveBelow(Plane const& _plane) : plane(_plane) {}
									void add(Vector3 const& _point)
									{
										float inFront = plane.get_in_front(_point);
										if (inFront <= 0.0f) ++below;
										if (inFront >= 0.0f) ++above;
									}
									bool is_on_both_sides() const { return above > 0 && below > 0; }
								} aboveBelow(_context.highlightPlane.get());
								aboveBelow.add(Vector3(cmin.x, cmin.y, cmin.z));
								aboveBelow.add(Vector3(cmax.x, cmin.y, cmin.z));
								aboveBelow.add(Vector3(cmin.x, cmax.y, cmin.z));
								aboveBelow.add(Vector3(cmax.x, cmax.y, cmin.z));
								aboveBelow.add(Vector3(cmin.x, cmin.y, cmax.z));
								aboveBelow.add(Vector3(cmax.x, cmin.y, cmax.z));
								aboveBelow.add(Vector3(cmin.x, cmax.y, cmax.z));
								aboveBelow.add(Vector3(cmax.x, cmax.y, cmax.z));
								highlighted = aboveBelow.is_on_both_sides();
								if (!highlighted.get())
								{
									Vector3 c = (cmin + cmax) * 0.5f;
									float f = abs(aboveBelow.plane.get_in_front(c));
									float af = min(1.0f, 1.0f / (0.5f + f / voxelSize));
									af = lerp(0.5f, af, sqr(af));
									alpha = 0.7f * af;
								}
							}
							if (voxelColour.is_set())
							{
								colour = voxelColour.get();
								alpha = lerp(0.5f, alpha, 1.0f);
							}
							if (Voxel::is_solid(v->voxel))
							{
								RangeInt3 sidesRendered = RangeInt3::zero;
								if ((x > range.x.min && !Voxel::makes_side(vxp->voxel)) || x == range.x.min || highlighted.get(false))
								{
									Colour c = light.apply(colour, -Vector3::xAxis).with_alpha(alpha);
									debug_draw_triangle(false, c, Vector3(cmin.x, cmin.y, cmin.z), Vector3(cmin.x, cmax.y, cmin.z), Vector3(cmin.x, cmin.y, cmax.z));
									debug_draw_triangle(false, c, Vector3(cmin.x, cmax.y, cmin.z), Vector3(cmin.x, cmax.y, cmax.z), Vector3(cmin.x, cmin.y, cmax.z));
									sidesRendered.x.min = -1;
								}
								if ((x < range.x.max && !Voxel::makes_side(vxn->voxel)) || x == range.x.max || highlighted.get(false))
								{
									Colour c = light.apply(colour, Vector3::xAxis).with_alpha(alpha);
									debug_draw_triangle(false, c, Vector3(cmax.x, cmin.y, cmin.z), Vector3(cmax.x, cmin.y, cmax.z), Vector3(cmax.x, cmax.y, cmax.z));
									debug_draw_triangle(false, c, Vector3(cmax.x, cmin.y, cmin.z), Vector3(cmax.x, cmax.y, cmax.z), Vector3(cmax.x, cmax.y, cmin.z));
									sidesRendered.x.max = 1;
								}
								if ((y > range.y.min && !Voxel::makes_side(vyp->voxel)) || y == range.y.min || highlighted.get(false))
								{
									Colour c = light.apply(colour, -Vector3::yAxis).with_alpha(alpha);
									debug_draw_triangle(false, c, Vector3(cmin.x, cmin.y, cmin.z), Vector3(cmax.x, cmin.y, cmax.z), Vector3(cmax.x, cmin.y, cmin.z));
									debug_draw_triangle(false, c, Vector3(cmin.x, cmin.y, cmin.z), Vector3(cmin.x, cmin.y, cmax.z), Vector3(cmax.x, cmin.y, cmax.z));
									sidesRendered.y.min = -1;
								}
								if ((y < range.y.max && !Voxel::makes_side(vyn->voxel)) || y == range.y.max || highlighted.get(false))
								{
									Colour c = light.apply(colour, Vector3::yAxis).with_alpha(alpha);
									debug_draw_triangle(false, c, Vector3(cmax.x, cmax.y, cmin.z), Vector3(cmin.x, cmax.y, cmin.z), Vector3(cmin.x, cmax.y, cmax.z));
									debug_draw_triangle(false, c, Vector3(cmax.x, cmax.y, cmin.z), Vector3(cmax.x, cmax.y, cmax.z), Vector3(cmin.x, cmax.y, cmax.z));
									sidesRendered.y.max = 1;
								}
								if ((z > range.z.min && !Voxel::makes_side(vzp->voxel)) || z == range.z.min || highlighted.get(false))
								{
									Colour c = light.apply(colour, -Vector3::zAxis).with_alpha(alpha);
									debug_draw_triangle(false, c, Vector3(cmin.x, cmin.y, cmin.z), Vector3(cmax.x, cmin.y, cmin.z), Vector3(cmax.x, cmax.y, cmin.z));
									debug_draw_triangle(false, c, Vector3(cmin.x, cmin.y, cmin.z), Vector3(cmax.x, cmax.y, cmin.z), Vector3(cmin.x, cmax.y, cmin.z));
									sidesRendered.z.min = -1;
								}
								if ((z < range.z.max && !Voxel::makes_side(vzn->voxel)) || z == range.z.max || highlighted.get(false))
								{
									Colour c = light.apply(colour, Vector3::zAxis).with_alpha(alpha);
									debug_draw_triangle(false, c, Vector3(cmin.x, cmin.y, cmax.z), Vector3(cmin.x, cmax.y, cmax.z), Vector3(cmax.x, cmax.y, cmax.z));
									debug_draw_triangle(false, c, Vector3(cmin.x, cmin.y, cmax.z), Vector3(cmax.x, cmax.y, cmax.z), Vector3(cmax.x, cmin.y, cmax.z));
									sidesRendered.z.max = 1;
								}
								if (renderPass == RENDER_PASS_NORMAL && _context.voxelEdgeColour.is_set())
								{
									if (sidesRendered.x.min || sidesRendered.y.min)
									{
										debug_draw_line(false, _context.voxelEdgeColour.get(), Vector3(cmin.x, cmin.y, cmin.z), Vector3(cmin.x, cmin.y, cmax.z));
									}
									if (sidesRendered.x.max || sidesRendered.y.min)
									{
										debug_draw_line(false, _context.voxelEdgeColour.get(), Vector3(cmax.x, cmin.y, cmin.z), Vector3(cmax.x, cmin.y, cmax.z));
									}
									if (sidesRendered.x.min || sidesRendered.y.max)
									{
										debug_draw_line(false, _context.voxelEdgeColour.get(), Vector3(cmin.x, cmax.y, cmin.z), Vector3(cmin.x, cmax.y, cmax.z));
									}
									if (sidesRendered.x.max || sidesRendered.y.max)
									{
										debug_draw_line(false, _context.voxelEdgeColour.get(), Vector3(cmax.x, cmax.y, cmin.z), Vector3(cmax.x, cmax.y, cmax.z));
									}
									if (sidesRendered.x.min || sidesRendered.z.min)
									{
										debug_draw_line(false, _context.voxelEdgeColour.get(), Vector3(cmin.x, cmin.y, cmin.z), Vector3(cmin.x, cmax.y, cmin.z));
									}
									if (sidesRendered.x.max || sidesRendered.z.min)
									{
										debug_draw_line(false, _context.voxelEdgeColour.get(), Vector3(cmax.x, cmin.y, cmin.z), Vector3(cmax.x, cmax.y, cmin.z));
									}
									if (sidesRendered.y.min || sidesRendered.z.min)
									{
										debug_draw_line(false, _context.voxelEdgeColour.get(), Vector3(cmin.x, cmin.y, cmin.z), Vector3(cmax.x, cmin.y, cmin.z));
									}
									if (sidesRendered.y.max || sidesRendered.z.min)
									{
										debug_draw_line(false, _context.voxelEdgeColour.get(), Vector3(cmin.x, cmax.y, cmin.z), Vector3(cmax.x, cmax.y, cmin.z));
									}
									if (sidesRendered.x.min || sidesRendered.z.max)
									{
										debug_draw_line(false, _context.voxelEdgeColour.get(), Vector3(cmin.x, cmin.y, cmax.z), Vector3(cmin.x, cmax.y, cmax.z));
									}
									if (sidesRendered.x.max || sidesRendered.z.max)
									{
										debug_draw_line(false, _context.voxelEdgeColour.get(), Vector3(cmax.x, cmin.y, cmax.z), Vector3(cmax.x, cmax.y, cmax.z));
									}
									if (sidesRendered.y.min || sidesRendered.z.max)
									{
										debug_draw_line(false, _context.voxelEdgeColour.get(), Vector3(cmin.x, cmin.y, cmax.z), Vector3(cmax.x, cmin.y, cmax.z));
									}
									if (sidesRendered.y.max || sidesRendered.z.max)
									{
										debug_draw_line(false, _context.voxelEdgeColour.get(), Vector3(cmin.x, cmax.y, cmax.z), Vector3(cmax.x, cmax.y, cmax.z));
									}
								}
							}
							else if (! _context.onlySolidVoxels)
							{
								if (v->voxel == Voxel::Invisible)
								{
									Vector3 ccen = (cmin + cmax) * 0.5f;
									colour = Colour::blue;
									debug_draw_line(false, colour, Vector3(cmin.x, ccen.y, ccen.z), Vector3(cmax.x, ccen.y, ccen.z));
									debug_draw_line(false, colour, Vector3(ccen.x, cmin.y, ccen.z), Vector3(ccen.x, cmax.y, ccen.z));
									debug_draw_line(false, colour, Vector3(ccen.x, ccen.y, cmin.z), Vector3(ccen.x, ccen.y, cmax.z));
								}
								else if (v->voxel == Voxel::ForceEmpty)
								{
									colour = Colour::red;
									debug_draw_line(false, colour, Vector3(cmin.x, cmin.y, cmin.z), Vector3(cmax.x, cmax.y, cmax.z));
									debug_draw_line(false, colour, Vector3(cmin.x, cmax.y, cmin.z), Vector3(cmax.x, cmin.y, cmax.z));
									debug_draw_line(false, colour, Vector3(cmin.x, cmin.y, cmax.z), Vector3(cmax.x, cmax.y, cmin.z));
									debug_draw_line(false, colour, Vector3(cmin.x, cmax.y, cmax.z), Vector3(cmax.x, cmin.y, cmin.z));
								}
							}
						}

						++v;
						++vxp;
						++vxn;
						++vyp;
						++vyn;
						++vzp;
						++vzn;
					}
				}
			}
		}
	}
#endif
}

void VoxelMold::debug_draw() const
{
	RenderContext context;
	render(context);
}

bool VoxelMold::editor_render() const
{
	debug_draw();
	return true;
}

void VoxelMold::debug_draw_voxel(VectorInt3 const& _at, Colour const& _colour, bool _front)
{
	Vector3 at = _at.to_vector3();
	at -= zeroOffset;
	Vector3 op = at + Vector3::one;
	at *= voxelSize;
	op *= voxelSize;

	debug_draw_box_region(at, op, _colour, _front);
}

void VoxelMold::debug_draw_box_region(Vector3 const& _a, Vector3 const& _b, Colour const& _colour, bool _front, float _fillAlpha)
{
	Range3 r = Range3::empty;
	r.include(_a);
	r.include(_b);
	if (r.x.length() == 0.0f)
	{
		r.x.max += 0.001f;
	}
	if (r.y.length() == 0.0f)
	{
		r.y.max += 0.001f;
	}
	if (r.z.length() == 0.0f)
	{
		r.z.max += 0.001f;
	}
	debug_draw_box(_front, false, _colour, _fillAlpha, Box(r));
	/*
	}
	else
	{
		// bottom
		debug_draw_line(_front, _colour, Vector3(_a.x, _a.y, _a.z), Vector3(_b.x, _a.y, _a.z));
		debug_draw_line(_front, _colour, Vector3(_b.x, _a.y, _a.z), Vector3(_b.x, _b.y, _a.z));
		debug_draw_line(_front, _colour, Vector3(_a.x, _a.y, _a.z), Vector3(_a.x, _b.y, _a.z));
		debug_draw_line(_front, _colour, Vector3(_a.x, _b.y, _a.z), Vector3(_b.x, _b.y, _a.z));

		// t_b
		debug_draw_line(_front, _colour, Vector3(_a.x, _a.y, _b.z), Vector3(_b.x, _a.y, _b.z));
		debug_draw_line(_front, _colour, Vector3(_b.x, _a.y, _b.z), Vector3(_b.x, _b.y, _b.z));
		debug_draw_line(_front, _colour, Vector3(_a.x, _a.y, _b.z), Vector3(_a.x, _b.y, _b.z));
		debug_draw_line(_front, _colour, Vector3(_a.x, _b.y, _b.z), Vector3(_b.x, _b.y, _b.z));

		// sides
		debug_draw_line(_front, _colour, Vector3(_a.x, _a.y, _a.z), Vector3(_a.x, _a.y, _b.z));
		debug_draw_line(_front, _colour, Vector3(_b.x, _a.y, _a.z), Vector3(_b.x, _a.y, _b.z));
		debug_draw_line(_front, _colour, Vector3(_a.x, _b.y, _a.z), Vector3(_a.x, _b.y, _b.z));
		debug_draw_line(_front, _colour, Vector3(_b.x, _b.y, _a.z), Vector3(_b.x, _b.y, _b.z));
	}
	*/
}

Editor::Base* VoxelMold::create_editor(Editor::Base* _current) const
{
	if (fast_cast<Editor::VoxelMold>(_current))
	{
		return _current;
	}
	else
	{
		return new Editor::VoxelMold();
	}
}

VectorInt3 VoxelMold::to_voxel_coord(Vector3 _coord) const
{
	_coord /= voxelSize;
	_coord += zeroOffset;
	VectorInt3 c = TypeConversions::Normal::f_i_cells(_coord);
	return c;
}

Vector3 VoxelMold::to_full_coord(VectorInt3 _coord, Optional<Vector3> const& _insideVoxel) const
{
	Vector3 c = _coord.to_vector3();
	c += _insideVoxel.get(Vector3::half);
	c -= zeroOffset;
	c *= voxelSize;
	return c;
}

RangeInt3 VoxelMold::to_voxel_range(Range3 const& _range, bool _fullyInlcuded) const
{
	Vector3 rmin = _range.near_bottom_left();
	Vector3 rmax = _range.far_top_right();
	rmin /= voxelSize;
	rmax /= voxelSize;
	rmin += zeroOffset;
	rmax += zeroOffset;
	RangeInt3 result = RangeInt3::empty;
	float const extra = 0.001f;
	if (_fullyInlcuded)
	{
		result.x.min = TypeConversions::Normal::f_i_cells(rmin.x - extra) + 1;
		result.y.min = TypeConversions::Normal::f_i_cells(rmin.y - extra) + 1;
		result.z.min = TypeConversions::Normal::f_i_cells(rmin.z - extra) + 1;
		result.x.max = TypeConversions::Normal::f_i_cells(rmax.x + extra) - 1;
		result.y.max = TypeConversions::Normal::f_i_cells(rmax.y + extra) - 1;
		result.z.max = TypeConversions::Normal::f_i_cells(rmax.z + extra) - 1;
	}
	else
	{
		result.x.min = TypeConversions::Normal::f_i_cells(rmin.x + extra);
		result.y.min = TypeConversions::Normal::f_i_cells(rmin.y + extra);
		result.z.min = TypeConversions::Normal::f_i_cells(rmin.z + extra);
		result.x.max = TypeConversions::Normal::f_i_cells(rmax.x - extra);
		result.y.max = TypeConversions::Normal::f_i_cells(rmax.y - extra);
		result.z.max = TypeConversions::Normal::f_i_cells(rmax.z - extra);
	}

	return result;
}

Range3 VoxelMold::to_full_range(RangeInt3 const& _range) const
{
	if (_range.is_empty())
	{
		return Range3::empty;
	}
	Vector3 rmin = _range.near_bottom_left().to_vector3();
	Vector3 rmax = (_range.far_top_right() + VectorInt3::one).to_vector3();
	rmin -= zeroOffset;
	rmax -= zeroOffset;
	rmin *= voxelSize;
	rmax *= voxelSize;
	Range3 result = Range3::empty;
	result.include(rmin);
	result.include(rmax);
	return result;
}

//--

REGISTER_FOR_FAST_CAST(IVoxelMoldLayer);

Optional<int> IVoxelMoldLayer::get_child_depth(IVoxelMoldLayer const* _for, Array<RefCountObjectPtr<IVoxelMoldLayer>> const& _layers)
{
	auto iL = _layers.begin();
	int iLIdx = 0;
	if (_for)
	{
		for (; iLIdx < _layers.get_size(); ++iLIdx, ++iL)
		{
			if (iL->get() == _for)
			{
				++iLIdx;
				++iL;
				break;
			}
		}
	}

	Optional<int> forDepth;
	if (_for)
	{
		forDepth = _for->depth;
	}

	Optional<int> childDepth;
	for (; iLIdx < _layers.get_size(); ++iLIdx, ++iL)
	{
		if (forDepth.is_set() && iL->get()->depth <= forDepth.get())
		{
			break;
		}
		int lDepth = iL->get()->depth;
		childDepth = min(lDepth, childDepth.get(lDepth));
	}

	return childDepth;
}

bool IVoxelMoldLayer::for_every_child_combining(IVoxelMoldLayer const* _of, Array<RefCountObjectPtr<IVoxelMoldLayer>> const& _layers, std::function<bool(IVoxelMoldLayer* layer)> _do)
{
	bool result = true;

	Optional<int> childDepth = get_child_depth(_of, _layers);

	if (childDepth.is_set())
	{
		auto iL = _layers.begin();
		int iLIdx = 0;
		if (_of)
		{
			for (; iLIdx < _layers.get_size(); ++iLIdx, ++iL)
			{
				if (iL->get() == _of)
				{
					++iLIdx;
					++iL;
					break;
				}
			}
		}

		RangeInt childrenRange = RangeInt::empty;
		for (; iLIdx < _layers.get_size(); ++iLIdx, ++iL)
		{
			if (iL->get()->depth < childDepth.get())
			{
				break;
			}
			if (iL->get()->depth == childDepth.get())
			{
				childrenRange.include(iLIdx);
			}
		}

		if (!childrenRange.is_empty())
		{
			for_range_reverse(int, iLIdx, childrenRange.max, childrenRange.min)
			{
				if (auto* l = _layers[iLIdx].get())
				{
					if (l->depth == childDepth.get())
					{
						result &= _do(l);
					}
				}
			}
		}
	}

	return result;
}

IVoxelMoldLayer::IVoxelMoldLayer()
{
}

IVoxelMoldLayer::~IVoxelMoldLayer()
{
}

IVoxelMoldLayer* IVoxelMoldLayer::create_layer_for(IO::XML::Node const* _node)
{
	Name type = _node->get_name_attribute(TXT("type"));
	if (type == NAME(layerType_transform))
	{
		return new VoxelMoldTransformLayer();
	}
	if (type == NAME(layerType_fullTransform))
	{
		return new VoxelMoldFullTransformLayer();
	}
	if (type == NAME(layerType_include))
	{
		return new IncludeVoxelMoldLayer();
	}
	return new VoxelMoldLayer();
}

IO::XML::Node* IVoxelMoldLayer::create_node_for(IO::XML::Node* _parentNode, IVoxelMoldLayer const* _layer)
{
	auto* n = _parentNode->add_node(TXT("layer"));

	if (fast_cast<VoxelMoldTransformLayer>(_layer))
	{
		n->set_attribute(TXT("type"), NAME(layerType_transform).to_char());
	}
	if (fast_cast<VoxelMoldFullTransformLayer>(_layer))
	{
		n->set_attribute(TXT("type"), NAME(layerType_fullTransform).to_char());
	}
	if (fast_cast<IncludeVoxelMoldLayer>(_layer))
	{
		n->set_attribute(TXT("type"), NAME(layerType_include).to_char());
	}

	return n;
}

bool IVoxelMoldLayer::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = true;

	// clear/reset
	//
	id = Name::invalid();
	depth = 0;
	visible = true;
	visibilityOnly = false;

	// load
	//
	id = _node->get_name_attribute(TXT("id"), id);
	depth = _node->get_int_attribute(TXT("depth"), depth);
	visible = _node->get_bool_attribute(TXT("visible"), visible);
	visibilityOnly = _node->get_bool_attribute(TXT("visibilityOnly"), visibilityOnly);

	return result;
}

bool IVoxelMoldLayer::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	return result;
}

bool IVoxelMoldLayer::save_to_xml(IO::XML::Node* _node) const
{
	bool result = true;

	if (id.is_valid())
	{
		_node->set_attribute(TXT("id"), id.to_char());
	}
	_node->set_int_attribute(TXT("depth"), depth);
	_node->set_bool_attribute(TXT("visible"), visible);
	_node->set_bool_attribute(TXT("visibilityOnly"), visibilityOnly);

	return result;
}

bool IVoxelMoldLayer::combine(VoxelMold* _for, VoxelMoldLayer* _resultLayer, Array<RefCountObjectPtr<IVoxelMoldLayer>> const& _allLayers, CombineVoxelsContext const& _context) const
{
	bool result = true;
	
	bool isVisibleOrVisibilityOnly = is_visible(_for);
	if (!isVisibleOrVisibilityOnly)
	{
		for_every_ref(l, _allLayers)
		{
			if (l->get_visibility_only())
			{
				isVisibleOrVisibilityOnly = true;
				break;
			}
		}
	}
	if (isVisibleOrVisibilityOnly)
	{
		result &= IVoxelMoldLayer::for_every_child_combining(this, _allLayers, [_for, _resultLayer, &_allLayers, &_context](IVoxelMoldLayer* _layer)
			{
				return _layer->combine(_for, _resultLayer, _allLayers, _context);
			});
	}

	return true;
}

void IVoxelMoldLayer::rescale(Optional<int> const& _moreVoxels, Optional<int> const& _lessVoxels)
{
	an_assert(_moreVoxels.is_set() ^ _lessVoxels.is_set(), TXT("one has to be set and only one"));
}

bool IVoxelMoldLayer::is_visible(VoxelMold* _partOf) const
{
	if (visibilityOnly)
	{
		return true;
	}
	else if (fast_cast<VoxelMoldTransformLayer>(this) == nullptr)
	{
		for_every_ref(l, CONTENT_ACCESS::get_layers(_partOf))
		{
			if (l->visibilityOnly)
			{
				return false;
			}
		}
	}
	else if (fast_cast<VoxelMoldFullTransformLayer>(this) == nullptr)
	{
		for_every_ref(l, CONTENT_ACCESS::get_layers(_partOf))
		{
			if (l->visibilityOnly)
			{
				return false;
			}
		}
	}
	if (visible)
	{
		if (auto* parent = get_parent(_partOf))
		{
			return parent->is_visible(_partOf);
		}
	}
	return visible;
}

void IVoxelMoldLayer::set_visibility_only(bool _only, VoxelMold* _partOf)
{
	if (_only)
	{
		an_assert(_partOf, TXT("requires _partOf parameter to disable in other layers"));
		for_every_ref(l, CONTENT_ACCESS::get_layers(_partOf))
		{
			l->visibilityOnly = false;
		}
	}
	visibilityOnly = _only;
}

bool IVoxelMoldLayer::is_parent_of(VoxelMold* _partOf, IVoxelMoldLayer const* _of) const
{
	while (_of)
	{
		if (_of == this)
		{
			return true;
		}
		_of = _of->get_parent(_partOf);
	}
	return false;
}

IVoxelMoldLayer* IVoxelMoldLayer::get_parent(VoxelMold* _partOf) const
{
	int ourDepth = get_depth();
	IVoxelMoldLayer* lastLessShallow = nullptr;
	for_every_ref(l, CONTENT_ACCESS::get_layers(_partOf))
	{
		if (l == this)
		{
			return lastLessShallow;
		}
		if (l->get_depth() < ourDepth)
		{
			lastLessShallow = l;
		}
	}
	return nullptr;
}

IVoxelMoldLayer* IVoxelMoldLayer::get_top_parent(VoxelMold* _partOf) const
{
	IVoxelMoldLayer* parent = cast_to_nonconst(this);
	IVoxelMoldLayer* goUp = parent;
	while (goUp)
	{
		parent = goUp;
		goUp = goUp->get_parent(_partOf);
	}
	return parent;
}

VectorInt3 IVoxelMoldLayer::to_local(VoxelMold* _partOf, VectorInt3 const& _world, bool _ignoreMirrors) const
{
	if (auto* parent = get_parent(_partOf))
	{
		return parent->to_local(_partOf, _world, _ignoreMirrors);
	}
	return _world;
}

VectorInt3 IVoxelMoldLayer::to_world(VoxelMold* _partOf, VectorInt3 const& _local, bool _ignoreMirrors) const
{
	if (auto* parent = get_parent(_partOf))
	{
		return parent->to_world(_partOf, _local, _ignoreMirrors);
	}
	return _local;
}

IVoxelMoldLayer* IVoxelMoldLayer::create_copy() const
{
	IVoxelMoldLayer* copy = create_for_copy();
	an_assert(copy);
	fill_copy(copy);
	return copy;
}

void IVoxelMoldLayer::fill_copy_basics(IVoxelMoldLayer* _copy) const
{
	_copy->id = id;
	_copy->depth = depth;
	_copy->visible = visible;
}

void IVoxelMoldLayer::fill_copy(IVoxelMoldLayer* _copy) const
{
	fill_copy_basics(_copy);
}

//--

REGISTER_FOR_FAST_CAST(VoxelMoldLayer);

VoxelMoldLayer::VoxelMoldLayer()
{
}

VoxelMoldLayer::~VoxelMoldLayer()
{
}

bool VoxelMoldLayer::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	// clear/reset
	//
	range = RangeInt3::empty;
	voxels.clear();

	// load
	//
	range.load_from_xml_child_node(_node, TXT("range"));

	voxels.make_space_for(range.x.length() * range.y.length() * range.z.length());

	for_every(node, _node->children_named(TXT("voxelsRaw")))
	{
		int chPerVoxel = node->get_int_attribute(TXT("chPerVoxel"), 1);
		int voxelOff = node->get_int_attribute(TXT("voxelOff"), 1);
		String content = node->get_text();
		String voxelsContent;
		int readCh = 0;
		for_every(ch, content.get_data())
		{
			if (*ch == 0 || readCh >= chPerVoxel)
			{
				voxels.push_back();
				auto& v = voxels.get_last();
				v.voxel = ParserUtils::parse_hex(voxelsContent.to_char()) - voxelOff;
				voxelsContent = String::empty();
				readCh = 0;
				if (*ch == 0)
				{
					break;
				}
			}
			{
				voxelsContent += *ch;
				++readCh;
			}
		}
		an_assert(voxelsContent.is_empty());
	}


	return result;
}

bool VoxelMoldLayer::save_to_xml(IO::XML::Node* _node) const
{
	bool result = base::save_to_xml(_node);

	range.save_to_xml_child_node(_node, TXT("range"));

	{
		int storedInNode = 0;
		int storedInNodeLimit = 256;
		int lastVoxelIdx = voxels.get_size() - 1;
		struct VoxelsToSave
		{
			Array<Voxel> voxels;
			String voxelsContent;
			String voxelFormat;
			bool save_to_xml(IO::XML::Node* _node)
			{
				if (voxels.is_empty())
				{
					return true;
				}
				int voxelOff = 1;
				for_every(v, voxels)
				{
					voxelOff = max(voxelOff, -v->voxel);
				}
				int maxValue = 0;
				for_every(v, voxels)
				{
					maxValue = max(maxValue, v->voxel + voxelOff);
				}
				String maxValueAsHex = String::printf(TXT("%x"), maxValue);
				int chPerVoxel = maxValueAsHex.get_length();

				auto* voxelsRawNode = _node->add_node(TXT("voxelsRaw"));
				voxelsRawNode->set_int_attribute(TXT("chPerVoxel"), chPerVoxel);
				voxelsRawNode->set_int_attribute(TXT("voxelOff"), voxelOff);

				voxelsContent = String::empty();
				voxelFormat = String::printf(TXT("%%0%ix"), chPerVoxel);
				for_every(v, voxels)
				{
					voxelsContent += String::printf(voxelFormat.to_char(), v->voxel + voxelOff); // offset, see above
				}
				voxelsRawNode->add_text(voxelsContent);

				clear();

				return true;
			}
			void clear()
			{
				voxels.clear();
				voxelsContent = String::empty();
			}
		} voxelsToSave;
		for_every(v, voxels)
		{
			voxelsToSave.voxels.push_back(*v);
			++storedInNode;
			if (storedInNode >= storedInNodeLimit ||
				for_everys_index(v) == lastVoxelIdx)
			{
				result &= voxelsToSave.save_to_xml(_node);
				storedInNode = 0;
			}
		}
		result &= voxelsToSave.save_to_xml(_node);

		an_assert(storedInNode == 0);
	}

	return result;
}

void VoxelMoldLayer::reset()
{
	depth = 0;
	clear();
}

void VoxelMoldLayer::clear()
{
	range = RangeInt3::empty;
	voxels.clear();
}

void VoxelMoldLayer::prune()
{
	RangeInt3 occupiedRange = RangeInt3::empty;

	for_range(int, z, range.z.min, range.z.max)
	{
		for_range(int, y, range.y.min, range.y.max)
		{
			VectorInt3 startAt(range.x.min, y, z);
			Voxel const* v = &voxels[voxel_index(startAt, range)];
			for_range(int, x, range.x.min, range.x.max)
			{
				if (Voxel::is_anything(v->voxel))
				{
					occupiedRange.include(VectorInt3(x, y, z));
				}
				++v;
			}
		}
	}

	if (occupiedRange != range)
	{
		Array<Voxel> oldVoxels = voxels;
		RangeInt3 oldRange = range;

		range = occupiedRange;

		voxels.set_size(range.x.length()* range.y.length()* range.z.length());
		for_every(v, voxels)
		{
			// clear
			*v = Voxel();
		}

		if (!oldVoxels.is_empty() && !oldRange.is_empty())
		{
			copy_data_from(oldRange, oldVoxels);
		}
	}
}

Voxel& VoxelMoldLayer::access_at(VectorInt3 const& _at)
{
	if (!range.does_contain(_at))
	{
		Array<Voxel> oldVoxels = voxels;
		RangeInt3 oldRange = range;

		range.include(_at);

		voxels.set_size(range.x.length() * range.y.length() * range.z.length());
		for_every(v, voxels)
		{
			// clear
			*v = Voxel();
		}

		if (!oldVoxels.is_empty() && !oldRange.is_empty())
		{
			copy_data_from(oldRange, oldVoxels);
		}
	}

	return voxels[voxel_index(_at, range)];
}

Voxel const * VoxelMoldLayer::get_at(VectorInt3 const& _at) const
{
	if (!range.does_contain(_at))
	{
		return nullptr;
	}

	return &voxels[voxel_index(_at, range)];
}

void VoxelMoldLayer::fill_at(VoxelMold* _for, VoxelMoldLayer const* _combinedResultLayer, VectorInt3 const& _worldAt, std::function<bool(Voxel& _voxel)> _doForVoxel)
{
	RangeInt3 r = CONTENT_ACCESS::get_combined_range(_for);

	struct FilledVoxel
	{
		VectorInt3 worldAt;
		bool processed = false;
		Optional<Voxel> voxel;
		FilledVoxel() {}
		explicit FilledVoxel(VectorInt3 const& _worldAt) : worldAt(_worldAt) {}
		static bool add(REF_ Array<FilledVoxel>& _filledVoxels, VectorInt3 const& _worldAt)
		{
			for_every(fv, _filledVoxels)
			{
				if (fv->worldAt == _worldAt)
				{
					return false;
				}
			}
			_filledVoxels.push_back(FilledVoxel(_worldAt));
			return true;
		}
	};
	Array<FilledVoxel> filledVoxels;
	FilledVoxel::add(REF_ filledVoxels, _worldAt);
	int i = 0;
	bool validFill = true;
	while (filledVoxels.is_empty() || i < filledVoxels.get_size())
	{
		auto& fv = filledVoxels[i];
		if (! fv.processed)
		{
			VectorInt3 fillAt = fv.worldAt;
			bool filled = false;
			Voxel v;
			if (auto* cv = _combinedResultLayer->get_at(fillAt))
			{
				v = *cv;
			}
			if (_doForVoxel(v))
			{
				filled = true;
				fv.voxel = v;
			}
			fv.processed = true;
			if (filled)
			{
				if (!r.does_contain(fillAt))
				{
					validFill = false;
					break;
				}
				FilledVoxel::add(REF_ filledVoxels, fillAt + VectorInt3( 1,  0,  0));
				FilledVoxel::add(REF_ filledVoxels, fillAt + VectorInt3(-1,  0,  0));
				FilledVoxel::add(REF_ filledVoxels, fillAt + VectorInt3( 0,  1,  0));
				FilledVoxel::add(REF_ filledVoxels, fillAt + VectorInt3( 0, -1,  0));
				FilledVoxel::add(REF_ filledVoxels, fillAt + VectorInt3( 0,  0,  1));
				FilledVoxel::add(REF_ filledVoxels, fillAt + VectorInt3( 0,  0, -1));
			}
		}
		++i;
	}
	if (validFill)
	{
		for_every(fv, filledVoxels)
		{
			if (fv->processed && fv->voxel.is_set())
			{
				VectorInt3 local = to_local(_for, fv->worldAt);
				access_at(local) = fv->voxel.get();
			}
		}
	}
}

void VoxelMoldLayer::copy_data_from(RangeInt3 const& _otherRange, Array<Voxel> const& _otherVoxels)
{
	// copy old voxels
	RangeInt3 commonRange = range;
	commonRange.intersect_with(_otherRange);
	for_range(int, z, commonRange.z.min, commonRange.z.max)
	{
		for_range(int, y, commonRange.y.min, commonRange.y.max)
		{
			VectorInt3 startAt(commonRange.x.min, y, z);
			Voxel const* src = &_otherVoxels[voxel_index(startAt, _otherRange)];
			Voxel* dst = &voxels[voxel_index(startAt, range)];
			for_range(int, x, commonRange.x.min, commonRange.x.max)
			{
				*dst = *src;
				++dst;
				++src;
			}
		}
	}
}

void VoxelMoldLayer::combine_data_from(RangeInt3 const& _otherRange, Array<Voxel> const& _otherVoxels)
{
	// copy old voxels
	RangeInt3 commonRange = range;
	commonRange.intersect_with(_otherRange);
	for_range(int, z, commonRange.z.min, commonRange.z.max)
	{
		for_range(int, y, commonRange.y.min, commonRange.y.max)
		{
			VectorInt3 startAt(commonRange.x.min, y, z);
			Voxel const* src = &_otherVoxels[voxel_index(startAt, _otherRange)];
			Voxel* dst = &voxels[voxel_index(startAt, range)];
			for_range(int, x, commonRange.x.min, commonRange.x.max)
			{
				if (Voxel::is_anything(src->voxel) &&
					(! Voxel::is_anything(dst->voxel) || src->voxel != Voxel::Invisible))
				{
					*dst = *src;
				}
				++dst;
				++src;
			}
		}
	}
}

void VoxelMoldLayer::merge_data_from(RangeInt3 const& _otherRange, Array<Voxel> const& _otherVoxels)
{
	// copy old voxels
	RangeInt3 commonRange = range;
	commonRange.intersect_with(_otherRange);
	for_range(int, z, commonRange.z.min, commonRange.z.max)
	{
		for_range(int, y, commonRange.y.min, commonRange.y.max)
		{
			VectorInt3 startAt(commonRange.x.min, y, z);
			Voxel const* src = &_otherVoxels[voxel_index(startAt, _otherRange)];
			Voxel* dst = &voxels[voxel_index(startAt, range)];
			for_range(int, x, commonRange.x.min, commonRange.x.max)
			{
				if (Voxel::is_anything(src->voxel) &&
					Voxel::can_be_replaced_in_merge(dst->voxel))
				{
					*dst = *src;
				}
				++dst;
				++src;
			}
		}
	}
}

bool VoxelMoldLayer::combine(VoxelMold* _for, VoxelMoldLayer* _resultLayer, Array<RefCountObjectPtr<IVoxelMoldLayer>> const& _allLayers, CombineVoxelsContext const& _context) const
{
	bool result = base::combine(_for, _resultLayer, _allLayers, _context);

	if (is_visible(_for) &&
		!range.is_empty())
	{
		// make space
		_resultLayer->access_at(range.near_bottom_left());
		_resultLayer->access_at(range.far_top_right());

		_resultLayer->combine_data_from(range, voxels);
	}

	return result;
}

void VoxelMoldLayer::rescale(Optional<int> const& _moreVoxels, Optional<int> const& _lessVoxels)
{
	base::rescale(_moreVoxels, _lessVoxels);

	if (_moreVoxels.is_set())
	{
		int scale = _moreVoxels.get();
		auto oldRange = range;
		auto oldVoxels = voxels;

		clear();

		for_range(int, y, oldRange.y.min, oldRange.y.max)
		{
			for_range(int, z, oldRange.z.min, oldRange.z.max)
			{
				for_range(int, x, oldRange.x.min, oldRange.x.max)
				{
					VectorInt3 srcXYZ(x, y, z);
					VectorInt3 dstXYZ = srcXYZ * scale;

					Voxel srcV = oldVoxels[voxel_index(srcXYZ, oldRange)];
					VectorInt3 xyz = dstXYZ;
					for_range(int, sy, 0, scale - 1)
					{
						xyz.y = dstXYZ.y + sy;
						for_range(int, sz, 0, scale - 1)
						{
							xyz.z = dstXYZ.z + sz;
							for_range(int, sx, 0, scale - 1)
							{
								xyz.x = dstXYZ.x + sx;
								access_at(xyz) = srcV;
							}
						}
					}
				}
			}
		}
	}

	if (_lessVoxels.is_set())
	{
		int scale = _lessVoxels.get();
		auto oldRange = range;
		auto oldVoxels = voxels;

		RangeInt3 newRange;
		newRange.x.min = floor_to(range.x.min, scale) / scale;
		newRange.y.min = floor_to(range.y.min, scale) / scale;
		newRange.z.min = floor_to(range.z.min, scale) / scale;
		newRange.x.max = ceil_to(range.x.max, scale) / scale;
		newRange.y.max = ceil_to(range.y.max, scale) / scale;
		newRange.z.max = ceil_to(range.z.max, scale) / scale;
		
		clear();

		struct MostCommon
		{
			struct CountedVoxel
			{
				Voxel voxel;
				int count = 0;
			};
			Array<CountedVoxel> countedVoxels;

			void reset()
			{
				countedVoxels.clear();
			}

			void add(Voxel const& v)
			{
				for_every(c, countedVoxels)
				{
					if (c->voxel == v)
					{
						++c->count;
						return;
					}
				}
				countedVoxels.push_back();
				auto& c = countedVoxels.get_last();
				c.voxel = v;
				c.count = 1;
			}

			Voxel get_most_common()
			{
				Voxel result;
				int best = NONE;
				for_every(c, countedVoxels)
				{
					if (c->count > best)
					{
						best = c->count;
						result = c->voxel;
					}
				}
				return result;
			}
		} mostCommon;
		for_range(int, y, newRange.y.min, newRange.y.max)
		{
			for_range(int, z, newRange.z.min, newRange.z.max)
			{
				for_range(int, x, newRange.x.min, newRange.x.max)
				{
					VectorInt3 dstXYZ(x, y, z);
					VectorInt3 srcXYZ = dstXYZ * scale;

					mostCommon.reset();
					VectorInt3 xyz = srcXYZ;
					for_range(int, sy, 0, scale - 1)
					{
						xyz.y = srcXYZ.y + sy;
						for_range(int, sz, 0, scale - 1)
						{
							xyz.z = srcXYZ.z + sz;
							for_range(int, sx, 0, scale - 1)
							{
								xyz.x = srcXYZ.x + sx;
								if (oldRange.does_contain(xyz))
								{
									Voxel srcV = oldVoxels[voxel_index(xyz, oldRange)];
									if (Voxel::is_anything(srcV.voxel))
									{
										mostCommon.add(srcV);
									}
								}
							}
						}
					}

					access_at(dstXYZ) = mostCommon.get_most_common();
				}
			}
		}
	}
}

void VoxelMoldLayer::merge(VoxelMoldLayer const* _layer)
{
	if (!_layer->range.is_empty())
	{
		access_at(_layer->range.near_bottom_left());
		access_at(_layer->range.far_top_right());

		merge_data_from(_layer->range, _layer->voxels);
	}
}

void VoxelMoldLayer::offset_placement(VectorInt3 const& _by)
{
	if (!range.is_empty())
	{
		range.offset(_by);
	}
}

IVoxelMoldLayer* VoxelMoldLayer::create_for_copy() const
{
	return new VoxelMoldLayer();
}

void VoxelMoldLayer::fill_copy(IVoxelMoldLayer* _copy) const
{
	base::fill_copy(_copy);
	auto* copy = fast_cast<VoxelMoldLayer>(_copy);
	an_assert(copy);
	copy->range = range;
	copy->voxels = voxels;
}

//--

REGISTER_FOR_FAST_CAST(VoxelMoldTransformLayer);

VoxelMoldTransformLayer::VoxelMoldTransformLayer()
{
	tempALayer = new VoxelMoldLayer();
	tempBLayer = new VoxelMoldLayer();
}

VoxelMoldTransformLayer::~VoxelMoldTransformLayer()
{
}

bool VoxelMoldTransformLayer::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	// clear
	//
	rotate = Rotate();
	mirror = Mirror();
	offset = VectorInt3::zero;

	// load
	//
	result &= rotate.load_from_xml_child_node(_node, TXT("rotate"));
	result &= mirror.load_from_xml_child_node(_node, TXT("mirror"));
	result &= offset.load_from_xml_child_node(_node, TXT("offset"));

	return result;
}

bool VoxelMoldTransformLayer::save_to_xml(IO::XML::Node* _node) const
{
	bool result = base::save_to_xml(_node);

	result &= rotate.save_to_xml_child_node(_node, TXT("rotate"));
	result &= mirror.save_to_xml_child_node(_node, TXT("mirror"));
	result &= offset.save_to_xml_child_node(_node, TXT("offset"));

	return result;
}

bool VoxelMoldTransformLayer::combine(VoxelMold* _for, VoxelMoldLayer* _resultLayer, Array<RefCountObjectPtr<IVoxelMoldLayer>> const& _allLayers, CombineVoxelsContext const& _context) const
{
	if (!is_visible(_for))
	{
		return base::combine(_for, _resultLayer, _allLayers, _context);
	}

	tempALayer->clear();
	tempBLayer->clear();

	auto* dst = tempALayer.get();
	auto* src = tempBLayer.get();
	bool result = true;

	// read children result
	{
		result &= base::combine(_for, dst, _allLayers, _context);
		// swap and prune
		swap(src, dst);	dst->clear();
		src->prune();
	}

	// actual transforms
	{
		// rotate
		{
			// build axes from rotation
			VectorInt3 xAxis, yAxis, zAxis;
			rotate.prepare_axes(OUT_ xAxis, OUT_ yAxis, OUT_ zAxis);
			// read all voxels and put them into dst
			{
				RangeInt3 srcRange = src->get_range();
				dst->clear();
				if (!srcRange.is_empty())
				{
					VectorInt3 nbl = srcRange.near_bottom_left();
					VectorInt3 ftr = srcRange.far_top_right();
					dst->access_at(xAxis * nbl.x + yAxis * nbl.y + zAxis * nbl.z);
					dst->access_at(xAxis * ftr.x + yAxis * ftr.y + zAxis * ftr.z);
					for_range(int, z, srcRange.z.min, srcRange.z.max)
					{
						for_range(int, y, srcRange.y.min, srcRange.y.max)
						{
							Voxel const * srcV = &src->voxels[voxel_index(VectorInt3(srcRange.x.min, y, z), srcRange)];
							VectorInt3 dstYZ = yAxis * y + zAxis * z;
							for_range(int, x, srcRange.x.min, srcRange.x.max)
							{
								VectorInt3 dstXYZ = dstYZ + xAxis * x;
								dst->access_at(dstXYZ) = *srcV;
								++srcV;
							}
						}
					}
				}
			}
			// swap and prune
			swap(src, dst);	dst->clear();
			src->prune();
		}

		// mirror
		{
			// for each active mirror have axes (mirror axis + two other)...
			// ...read all required voxels, flip them as required and put them into dst
			if (mirror.mirrorAxis != NONE)
			{
				RangeInt3 srcRange = src->get_range();
				if (!srcRange.is_empty())
				{
					struct MirrorWorker
					{
						int at;
						bool axis = false;
						int dir = 0;

						// A,B = other two axes
						uint elementMirror;
						uint elementA;
						uint elementB;

						VectorInt3 axisMirror;
						VectorInt3 axisA;
						VectorInt3 axisB;

						RangeInt rangeMirror; // result (!)
						RangeInt rangeA;
						RangeInt rangeB;
					}
					mirrorWorker;
					mirrorWorker.at = mirror.at;
					mirrorWorker.axis = mirror.axis;
					mirrorWorker.dir = mirror.dir;

					if (mirror.mirrorAxis == 0)
					{
						mirrorWorker.elementMirror = 0;
						mirrorWorker.elementA = 1;
						mirrorWorker.elementB = 2;
					}
					else if (mirror.mirrorAxis == 1)
					{
						mirrorWorker.elementMirror = 1;
						mirrorWorker.elementA = 0;
						mirrorWorker.elementB = 2;
					}
					else 
					{
						an_assert(mirror.mirrorAxis == 2);
						mirrorWorker.elementMirror = 2;
						mirrorWorker.elementA = 0;
						mirrorWorker.elementB = 1;
					}

					mirrorWorker.axisMirror = VectorInt3::zero;
					mirrorWorker.axisA = VectorInt3::zero;
					mirrorWorker.axisB = VectorInt3::zero;

					mirrorWorker.axisMirror.access_element(mirrorWorker.elementMirror) = 1;
					mirrorWorker.axisA.access_element(mirrorWorker.elementA) = 1;
					mirrorWorker.axisB.access_element(mirrorWorker.elementB) = 1;


					mirrorWorker.rangeA = srcRange.access_element(mirrorWorker.elementA);
					mirrorWorker.rangeB = srcRange.access_element(mirrorWorker.elementB);

					RangeInt srcRangeMirror = srcRange.access_element(mirrorWorker.elementMirror);
					if (mirrorWorker.axis)
					{
						if (mirrorWorker.dir == 0)
						{
							// at = 2	0123 -> 01:2:3 -> 1:2:34 -> 1234
							mirrorWorker.rangeMirror.min = mirrorWorker.at - (srcRangeMirror.max - mirrorWorker.at);
							mirrorWorker.rangeMirror.max = mirrorWorker.at + (mirrorWorker.at - srcRangeMirror.min);
						}
						else if (mirrorWorker.dir < 0)
						{
							// at = 2	0123 -> 01:2:3 -> 1:2:3 -> 123
							mirrorWorker.rangeMirror.min = mirrorWorker.at - (srcRangeMirror.max - mirrorWorker.at);
							mirrorWorker.rangeMirror.max = srcRangeMirror.max; // keep it
						}
						else
						{
							// at = 2	0123 -> 01:2:3 -> 01:2:34 -> 01234
							mirrorWorker.rangeMirror.min = srcRangeMirror.min; // keep it
							mirrorWorker.rangeMirror.max = mirrorWorker.at + (mirrorWorker.at - srcRangeMirror.min);
						}
					}
					else
					{
						if (mirrorWorker.dir == 0)
						{
							// at = 3	0123 -> 012:3 -> 2:345 -> 2345
							mirrorWorker.rangeMirror.min = mirrorWorker.at - ((srcRangeMirror.max + 1) - mirrorWorker.at);
							mirrorWorker.rangeMirror.max = mirrorWorker.at + ((mirrorWorker.at - srcRangeMirror.min) - 1);
						}
						else if (mirrorWorker.dir < 0)
						{
							// at = 3	01234 -> 012:34 -> 12:34 -> 1234
							mirrorWorker.rangeMirror.min = mirrorWorker.at - ((srcRangeMirror.max + 1) - mirrorWorker.at);
							mirrorWorker.rangeMirror.max = srcRangeMirror.max; // keep it
						}
						else
						{
							// at = 3	01234 -> 012:34 -> 012:345 -> 012345
							mirrorWorker.rangeMirror.min = srcRangeMirror.min; // keep it
							mirrorWorker.rangeMirror.max = mirrorWorker.at + ((mirrorWorker.at - srcRangeMirror.min) - 1);
						}
					}

					for_range(int, a, mirrorWorker.rangeA.min, mirrorWorker.rangeA.max)
					{
						for_range(int, b, mirrorWorker.rangeB.min, mirrorWorker.rangeB.max)
						{
							VectorInt3 coordAB = VectorInt3::zero;
							coordAB.access_element(mirrorWorker.elementA) = a;
							coordAB.access_element(mirrorWorker.elementB) = b;
							for_range(int, dm, mirrorWorker.rangeMirror.min, mirrorWorker.rangeMirror.max)
							{
								VectorInt3 dstABM = coordAB;
								dstABM.access_element(mirrorWorker.elementMirror) = dm;
								VectorInt3 srcABM = coordAB;
								{
									int sm = dm;
									if (mirrorWorker.axis)
									{
										if (mirrorWorker.dir == 0)
										{
											// at = 2	0123 -> 01:2:3 -> 1:2:34 -> 1234
											sm = mirrorWorker.at - (sm - mirrorWorker.at);
										}
										else if (mirrorWorker.dir < 0)
										{
											// at = 2	0123 -> 01:2:3 -> 1:2:3 -> 123
											if (sm < mirrorWorker.at)
											{
												sm = mirrorWorker.at - (sm - mirrorWorker.at);
											}
											// otherwise keep it
										}
										else
										{
											// at = 2	0123 -> 01:2:3 -> 01:2:34 -> 01234
											if (sm > mirrorWorker.at)
											{
												sm = mirrorWorker.at - (sm - mirrorWorker.at);
											}
											// otherwise keep it
										}
									}
									else
									{
										if (mirrorWorker.dir == 0)
										{
											// at = 3	0123 -> 012:3 -> 2:345 -> 2345
											sm = mirrorWorker.at - (sm - mirrorWorker.at) - 1;
										}
										else if (mirrorWorker.dir < 0)
										{
											// at = 3	01234 -> 012:34 -> 12:34 -> 1234
											if (sm < mirrorWorker.at)
											{
												sm = mirrorWorker.at - (sm - mirrorWorker.at) - 1;
											}
											// otherwise keep it
										}
										else
										{
											// at = 3	01234 -> 012:34 -> 012:345 -> 012345
											if (sm >= mirrorWorker.at)
											{
												sm = mirrorWorker.at - (sm - mirrorWorker.at) - 1;
											}
											// otherwise keep it
										}
									}
									srcABM.access_element(mirrorWorker.elementMirror) = sm;
								}
								auto& dstV = dst->access_at(dstABM);
								if (srcRange.does_contain(srcABM))
								{
									auto const & srcV = src->access_at(srcABM);
									dstV = srcV;
								}
							}
						}
					}
					// swap and prune
					swap(src, dst);	dst->clear();
					src->prune();
				}
			}
		}

		// offset
		{
			if (!src->range.is_empty())
			{
				src->range.offset(offset);
			}
			// no swap, no need to prune
		}
	}

	// combine into result layer
	if (!src->range.is_empty())
	{
		_resultLayer->access_at(src->range.near_bottom_left());
		_resultLayer->access_at(src->range.far_top_right());

		_resultLayer->combine_data_from(src->range, src->voxels);
	}

	return result;
}

void VoxelMoldTransformLayer::rescale(Optional<int> const& _moreVoxels, Optional<int> const& _lessVoxels)
{
	base::rescale(_moreVoxels, _lessVoxels);

	if (_moreVoxels.is_set())
	{
		int scale = _moreVoxels.get();
		offset *= scale;
		mirror.at *= scale;
		if (mirror.axis)
		{
			mirror.at += scale / 2;
			mirror.axis = mod(scale, 2) == 1;
		}
	}
	if (_lessVoxels.is_set())
	{
		int scale = _lessVoxels.get();
		offset /= scale;
		if (mod(mirror.at, scale) >= scale / 2)
		{
			mirror.axis = true;
		}
		mirror.at /= scale;
	}
}

VectorInt3 VoxelMoldTransformLayer::to_local(VoxelMold* _partOf, VectorInt3 const& _world, bool _ignoreMirrors) const
{
	VectorInt3 coord = base::to_local(_partOf, _world, _ignoreMirrors);

	if (is_visible(_partOf))
	{
		// in reverse
	
		// offset
		{
			coord = coord - offset;
		}

		// mirror
		if (!_ignoreMirrors)
		{
			if (mirror.mirrorAxis != NONE)
			{
				int& c = coord.access_element(mirror.mirrorAxis);

				if (mirror.axis)
				{
					if (mirror.dir == 0)
					{
						// at = 2	0123 -> 01:2:3 -> 1:2:34 -> 1234
						c = mirror.at - (c - mirror.at);
					}
					else if (mirror.dir < 0)
					{
						// at = 2	0123 -> 01:2:3 -> 1:2:3 -> 123
						if (c < mirror.at)
						{
							c = mirror.at - (c - mirror.at);
						}
						// otherwise keep it
					}
					else
					{
						// at = 2	0123 -> 01:2:3 -> 01:2:34 -> 01234
						if (c > mirror.at)
						{
							c = mirror.at - (c - mirror.at);
						}
						// otherwise keep it
					}
				}
				else
				{
					if (mirror.dir == 0)
					{
						// at = 3	0123 -> 012:3 -> 2:345 -> 2345
						c = mirror.at - (c - mirror.at) - 1;
					}
					else if (mirror.dir < 0)
					{
						// at = 3	01234 -> 012:34 -> 12:34 -> 1234
						if (c < mirror.at)
						{
							c = mirror.at - (c - mirror.at) - 1;
						}
						// otherwise keep it
					}
					else
					{
						// at = 3	01234 -> 012:34 -> 012:345 -> 012345
						if (c >= mirror.at)
						{
							c = mirror.at - (c - mirror.at) - 1;
						}
						// otherwise keep it
					}
				}
			}
		}

		// rotate
		{
			VectorInt3 xAxis, yAxis, zAxis;
			rotate.prepare_axes(OUT_ xAxis, OUT_ yAxis, OUT_ zAxis);

			VectorInt3 xAxisInv, yAxisInv, zAxisInv;
			xAxisInv.x = xAxis.x;
			xAxisInv.y = yAxis.x;
			xAxisInv.z = zAxis.x;
			yAxisInv.x = xAxis.y;
			yAxisInv.y = yAxis.y;
			yAxisInv.z = zAxis.y;
			zAxisInv.x = xAxis.z;
			zAxisInv.y = yAxis.z;
			zAxisInv.z = zAxis.z;

			coord = xAxisInv * coord.x + yAxisInv * coord.y + zAxisInv * coord.z;
		}
	}

	return coord;
}

VectorInt3 VoxelMoldTransformLayer::to_world(VoxelMold* _partOf, VectorInt3 const& _local, bool _ignoreMirrors) const
{
	VectorInt3 coord = base::to_world(_partOf, _local, _ignoreMirrors);

	if (is_visible(_partOf))
	{
		// rotate
		{
			VectorInt3 xAxis, yAxis, zAxis;
			rotate.prepare_axes(OUT_ xAxis, OUT_ yAxis, OUT_ zAxis);

			coord = xAxis * coord.x + yAxis * coord.y + zAxis * coord.z;
		}

		// mirror
		if (!_ignoreMirrors)
		{
			if (mirror.mirrorAxis != NONE)
			{
				int& c = coord.access_element(mirror.mirrorAxis);

				if (mirror.axis)
				{
					if (mirror.dir == 0)
					{
						// at = 2	0123 -> 01:2:3 -> 1:2:34 -> 1234
						c = mirror.at - (c - mirror.at);
					}
					// otherwise keep it (can be both, keep original)
				}
				else
				{
					if (mirror.dir == 0)
					{
						// at = 3	0123 -> 012:3 -> 2:345 -> 2345
						c = mirror.at - (c - mirror.at) - 1;
					}
					// otherwise keep it (can be both, keep original)
				}
			}
		}

		// offset
		{
			coord = coord + offset;
		}
	}

	return coord;
}

IVoxelMoldLayer* VoxelMoldTransformLayer::create_for_copy() const
{
	return new VoxelMoldTransformLayer();
}

void VoxelMoldTransformLayer::fill_copy(IVoxelMoldLayer* _copy) const
{
	base::fill_copy(_copy);
	auto* copy = fast_cast<VoxelMoldTransformLayer>(_copy);
	an_assert(copy);
	copy->rotate = rotate;
	copy->mirror = mirror;
	copy->offset = offset;
}

//--

void VoxelMoldTransformLayer::Rotate::prepare_axes(OUT_ VectorInt3& xAxis, OUT_ VectorInt3& yAxis, OUT_ VectorInt3& zAxis) const
{
	xAxis = VectorInt3(1, 0, 0);
	yAxis = VectorInt3(0, 1, 0);
	zAxis = VectorInt3(0, 0, 1);
	int usePitch = mod(pitch, 4);
	int useYaw = mod(yaw, 4);
	int useRoll = mod(roll, 4);
	struct RotateAxis
	{
		static VectorInt3 pitch(VectorInt3 axis)
		{
			swap(axis.y, axis.z); axis.y *= -1;
			return axis;
		}
		static VectorInt3 yaw(VectorInt3 axis)
		{
			swap(axis.x, axis.y); axis.y *= -1;
			return axis;
		}
		static VectorInt3 roll(VectorInt3 axis)
		{
			swap(axis.x, axis.z); axis.z *= -1;
			return axis;
		}
	};
	{
		an_assert(useRoll >= 0);
		while (useRoll > 0)
		{
			xAxis = RotateAxis::roll(xAxis);
			yAxis = RotateAxis::roll(yAxis);
			zAxis = RotateAxis::roll(zAxis);
			--useRoll;
		}
	}
	{
		an_assert(usePitch >= 0);
		while (usePitch > 0)
		{
			xAxis = RotateAxis::pitch(xAxis);
			yAxis = RotateAxis::pitch(yAxis);
			zAxis = RotateAxis::pitch(zAxis);
			--usePitch;
		}
	}
	{
		an_assert(useYaw >= 0);
		while (useYaw > 0)
		{
			xAxis = RotateAxis::yaw(xAxis);
			yAxis = RotateAxis::yaw(yAxis);
			zAxis = RotateAxis::yaw(zAxis);
			--useYaw;
		}
	}
}

bool VoxelMoldTransformLayer::Rotate::load_from_xml_child_node(IO::XML::Node const* _node, tchar const* _childName)
{
	bool result = true;
	for_every(n, _node->children_named(_childName))
	{
		pitch = n->get_int_attribute(TXT("pitch"), pitch);
		yaw = n->get_int_attribute(TXT("yaw"), yaw);
		roll = n->get_int_attribute(TXT("roll"), roll);
	}
	return result;
}

bool VoxelMoldTransformLayer::Rotate::save_to_xml_child_node(IO::XML::Node * _node, tchar const* _childName) const
{
	bool result = true;
	if (auto* n = _node->add_node(_childName))
	{
		n->set_int_attribute(TXT("pitch"), pitch);
		n->set_int_attribute(TXT("yaw"), yaw);
		n->set_int_attribute(TXT("roll"), roll);
	}
	return result;
}

//--

bool VoxelMoldTransformLayer::Mirror::load_from_xml_child_node(IO::XML::Node const* _node, tchar const* _childName)
{
	bool result = true;
	for_every(n, _node->children_named(_childName))
	{
		mirrorAxis = n->get_int_attribute(TXT("mirrorAxis"), mirrorAxis);
		at = n->get_int_attribute(TXT("at"), at);
		axis = n->get_bool_attribute(TXT("axis"), axis);
		dir = n->get_int_attribute(TXT("dir"), dir);
	}
	return result;
}

bool VoxelMoldTransformLayer::Mirror::save_to_xml_child_node(IO::XML::Node * _node, tchar const* _childName) const
{
	bool result = true;
	if (auto* n = _node->add_node(_childName))
	{
		n->set_int_attribute(TXT("mirrorAxis"), mirrorAxis);
		n->set_int_attribute(TXT("at"), at);
		n->set_bool_attribute(TXT("axis"), axis);
		n->set_int_attribute(TXT("dir"), dir);
	}
	return result;
}

//--

REGISTER_FOR_FAST_CAST(VoxelMoldFullTransformLayer);

VoxelMoldFullTransformLayer::VoxelMoldFullTransformLayer()
{
	tempALayer = new VoxelMoldLayer();
	tempBLayer = new VoxelMoldLayer();
}

VoxelMoldFullTransformLayer::~VoxelMoldFullTransformLayer()
{
}

bool VoxelMoldFullTransformLayer::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	// clear
	//
	rotate = Rotator3();
	offset = Vector3::zero;

	// load
	//
	result &= rotate.load_from_xml_child_node(_node, TXT("rotate"));
	result &= offset.load_from_xml_child_node(_node, TXT("offset"));

	return result;
}

bool VoxelMoldFullTransformLayer::save_to_xml(IO::XML::Node* _node) const
{
	bool result = base::save_to_xml(_node);

	result &= rotate.save_to_xml_child_node(_node, TXT("rotate"));
	result &= offset.save_to_xml_child_node(_node, TXT("offset"));

	return result;
}

bool VoxelMoldFullTransformLayer::combine(VoxelMold* _for, VoxelMoldLayer* _resultLayer, Array<RefCountObjectPtr<IVoxelMoldLayer>> const& _allLayers, CombineVoxelsContext const& _context) const
{
	if (!is_visible(_for))
	{
		return base::combine(_for, _resultLayer, _allLayers, _context);
	}

	tempALayer->clear();
	tempBLayer->clear();

	auto* dst = tempALayer.get();
	auto* src = tempBLayer.get();
	bool result = true;

	// read children result
	{
		result &= base::combine(_for, dst, _allLayers, _context);
		// swap and prune
		swap(src, dst);	dst->clear();
		src->prune();
	}

	// actual transform
	{
		// read all voxels and put them into dst
		{
			RangeInt3 srcRange = src->get_range();
			if (!srcRange.is_empty())
			{
				Range3 srcRangeFull = _for->to_full_range(srcRange);

				Range3 dstRangeFull = Range3::empty;

				{
					Transform transform(offset * _for->get_voxel_size(), rotate.to_quat());

					dstRangeFull.include(transform.location_to_world(srcRangeFull.get_at(Vector3(0.0f, 0.0f, 0.0f))));
					dstRangeFull.include(transform.location_to_world(srcRangeFull.get_at(Vector3(0.0f, 0.0f, 1.0f))));
					dstRangeFull.include(transform.location_to_world(srcRangeFull.get_at(Vector3(0.0f, 1.0f, 0.0f))));
					dstRangeFull.include(transform.location_to_world(srcRangeFull.get_at(Vector3(0.0f, 1.0f, 1.0f))));
					dstRangeFull.include(transform.location_to_world(srcRangeFull.get_at(Vector3(1.0f, 0.0f, 0.0f))));
					dstRangeFull.include(transform.location_to_world(srcRangeFull.get_at(Vector3(1.0f, 0.0f, 1.0f))));
					dstRangeFull.include(transform.location_to_world(srcRangeFull.get_at(Vector3(1.0f, 1.0f, 0.0f))));
					dstRangeFull.include(transform.location_to_world(srcRangeFull.get_at(Vector3(1.0f, 1.0f, 1.0f))));
				}
				
				RangeInt3 dstRange = _for->to_voxel_range(dstRangeFull, true);
				dstRange.expand_by(VectorInt3::one);

				Transform transform(offset, rotate.to_quat());

				for_range(int, z, dstRange.z.min, dstRange.z.max)
				{
					for_range(int, y, dstRange.y.min, dstRange.y.max)
					{
						for_range(int, x, dstRange.x.min, dstRange.x.max)
						{
							VectorInt3 dstXYZ(x, y, z);
							// this is not full, this is float (!)
							Vector3 dstXYZf = dstXYZ.to_vector3() + Vector3::half;
							Vector3 srcXYZf = transform.location_to_local(dstXYZf);
							Voxel const* srcV = nullptr;
							if (!srcV)
							{
								VectorInt3 srcXYZ = TypeConversions::Normal::f_i_cells(srcXYZf);
								srcV = src->get_at(srcXYZ);
								if (srcV && ! Voxel::is_anything(srcV->voxel))
								{
									srcV = nullptr;
								}
							}
							/*
							if (!srcV)
							{
								ArrayStatic< Voxel const*, 10> srcvs;
								float radius = 0.2f;
								Vector3 offsets[] = {
									Vector3( 1.0f,  0.0f,  0.0f),
									Vector3(-1.0f,  0.0f,  0.0f),
									Vector3( 0.0f,  1.0f,  0.0f),
									Vector3( 0.0f, -1.0f,  0.0f),
									Vector3( 0.0f,  0.0f,  1.0f),
									Vector3( 0.0f,  0.0f, -1.0f),
								};
								for_count(int, i, 6)
								{
									VectorInt3 srcXYZ = TypeConversions::Normal::f_i_cells(srcXYZf + offsets[i] * radius);
									auto* sv = src->get_at(srcXYZ);
									if (sv && ! Voxel::is_anything(sv->voxel))
									{
										srcvs.push_back(sv);
									}
								}
								int amount = 0;
								for_count(int, i, srcvs.get_size())
								{
									int amountThis = 0;
									auto* checksv = srcvs[i];
									for_every_ptr(sv, srcvs)
									{
										if (checksv && checksv->voxel == sv->voxel)
										{
											++amountThis;
										}
									}
									if (amountThis > amount || !srcV)
									{
										amount = amountThis;
										srcV = checksv;
									}
								}
							}
							*/
							if (srcV)
							{
								dst->access_at(dstXYZ) = *srcV;
							}
						}
					}
				}
			}
		}
		// swap and prune
		swap(src, dst);	dst->clear();
		src->prune();
	}

	// combine into result layer
	if (!src->range.is_empty())
	{
		_resultLayer->access_at(src->range.near_bottom_left());
		_resultLayer->access_at(src->range.far_top_right());

		_resultLayer->combine_data_from(src->range, src->voxels);
	}

	return result;
}

void VoxelMoldFullTransformLayer::rescale(Optional<int> const& _moreVoxels, Optional<int> const& _lessVoxels)
{
	base::rescale(_moreVoxels, _lessVoxels);

	if (_moreVoxels.is_set())
	{
		int scale = _moreVoxels.get();
		offset *= (float)scale;
	}
	if (_lessVoxels.is_set())
	{
		int scale = _lessVoxels.get();
		offset /= (float)scale;
	}
}

VectorInt3 VoxelMoldFullTransformLayer::to_local(VoxelMold* _partOf, VectorInt3 const& _world, bool _ignoreMirrors) const
{
	VectorInt3 coord = base::to_local(_partOf, _world, _ignoreMirrors);

	if (is_visible(_partOf))
	{
		Transform transform(offset, rotate.to_quat());

		Vector3 worldF = coord.to_vector3() + Vector3::half;
		
		Vector3 localF = transform.location_to_local(worldF);

		VectorInt3 localV = TypeConversions::Normal::f_i_cells(localF);

		coord = localV;
	}

	return coord;
}

VectorInt3 VoxelMoldFullTransformLayer::to_world(VoxelMold* _partOf, VectorInt3 const& _local, bool _ignoreMirrors) const
{
	VectorInt3 coord = base::to_world(_partOf, _local, _ignoreMirrors);

	if (is_visible(_partOf))
	{
		Transform transform(offset, rotate.to_quat());

		Vector3 localF = coord.to_vector3() + Vector3::half;

		Vector3 worldF = transform.location_to_world(localF);

		VectorInt3 worldV = TypeConversions::Normal::f_i_cells(worldF);

		coord = worldV;
	}

	return coord;
}

IVoxelMoldLayer* VoxelMoldFullTransformLayer::create_for_copy() const
{
	return new VoxelMoldFullTransformLayer();
}

void VoxelMoldFullTransformLayer::fill_copy(IVoxelMoldLayer* _copy) const
{
	base::fill_copy(_copy);
	auto* copy = fast_cast<VoxelMoldFullTransformLayer>(_copy);
	an_assert(copy);
	copy->rotate = rotate;
	copy->offset = offset;
}

//--

REGISTER_FOR_FAST_CAST(IncludeVoxelMoldLayer);

IncludeVoxelMoldLayer::IncludeVoxelMoldLayer()
{
	includedLayer = new VoxelMoldLayer();
}

IncludeVoxelMoldLayer::~IncludeVoxelMoldLayer()
{
}

bool IncludeVoxelMoldLayer::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	// clear
	//
	included.clear();

	// load
	//
	result &= included.load_from_xml(_node, TXT("voxelMold"), _lc);
	
	return result;
}

bool IncludeVoxelMoldLayer::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	result &= included.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);

	IF_PREPARE_LEVEL(_pfgContext, LibraryPrepareLevel::CombineVoxelMoldLayers)
	{
		if (auto* ivm = included.get())
		{
			if (ivm->resultLayer->get_range().is_empty()) // most likely not combined, combine now
			{
				CONTENT_ACCESS::combine_voxels(ivm);
			}
		}
	}

	return result;
}

bool IncludeVoxelMoldLayer::save_to_xml(IO::XML::Node* _node) const
{
	bool result = base::save_to_xml(_node);

	result &= included.save_to_xml(_node, TXT("voxelMold"));

	return result;
}

bool IncludeVoxelMoldLayer::combine(VoxelMold* _for, VoxelMoldLayer* _resultLayer, Array<RefCountObjectPtr<IVoxelMoldLayer>> const& _allLayers, CombineVoxelsContext const& _context) const
{
	bool result = base::combine(_for, _resultLayer, _allLayers, _context);

	includedLayer->clear();

	if (is_visible(_for))
	{
		if (auto* ivm = included.get())
		{
			if (auto* l = fast_cast<VoxelMoldLayer>(ivm->resultLayer.get()))
			{
				// make space
				includedLayer->access_at(l->range.near_bottom_left());
				includedLayer->access_at(l->range.far_top_right());

				includedLayer->combine_data_from(l->range, l->voxels);

				// and into the actual _resultLayer
				_resultLayer->access_at(l->range.near_bottom_left());
				_resultLayer->access_at(l->range.far_top_right());

				_resultLayer->combine_data_from(l->range, l->voxels);
			}
		}
	}

	return result;
}

IVoxelMoldLayer* IncludeVoxelMoldLayer::create_for_copy() const
{
	return new IncludeVoxelMoldLayer();
}

void IncludeVoxelMoldLayer::fill_copy(IVoxelMoldLayer* _copy) const
{
	base::fill_copy(_copy);
	auto* copy = fast_cast<IncludeVoxelMoldLayer>(_copy);
	an_assert(copy);
	copy->included = included;
}

//--

#define ACCESS(what) auto& what = _this->what;
#define GET(what) auto const & what = _this->what;

void VoxelMoldContentAccess::prune(VoxelMold * _this)
{
	ACCESS(layers);

	for_every_ref(layer, layers)
	{
		if (auto* vml = fast_cast<VoxelMoldLayer>(layer))
		{
			vml->prune();
		}
	}
}

RangeInt3 VoxelMoldContentAccess::get_combined_range(VoxelMold* _this)
{
	return _this->resultLayer->get_range();
}

void VoxelMoldContentAccess::combine_voxels(VoxelMold* _this)
{
	CombineVoxelsContext context;

	combine_voxels(_this, context);
}

void VoxelMoldContentAccess::combine_voxels(VoxelMold* _this, CombineVoxelsContext const& _context)
{
	ACCESS(layers);
	ACCESS(resultLayer);

	resultLayer->reset();

	if (layers.is_empty())
	{
		layers.push_back();
		layers.get_last() = new VoxelMoldLayer();
	}

	IVoxelMoldLayer::for_every_child_combining(nullptr, layers, [_this, &_context](IVoxelMoldLayer* _layer)
		{
			return _layer->combine(_this, _this->resultLayer.get(), _this->layers, _context);
		});
}

void VoxelMoldContentAccess::fill_at(VoxelMold* _this, VoxelMoldLayer * _inLayer, VectorInt3 const& _worldAt, std::function<bool(Voxel& _voxel)> _doForVoxel)
{
	_inLayer->fill_at(_this, _this->resultLayer.get(), _worldAt, _doForVoxel);
}

bool VoxelMoldContentAccess::merge_temporary_layers(VoxelMold* _this, IVoxelMoldLayer* _exceptLayer)
{
	ACCESS(layers);
	ACCESS(editedLayerIndex);

	bool anyMerged = false;

	int keepEditedLayerIndex = editedLayerIndex;

	for (int i = 0; i < layers.get_size() - 1; ++i)
	{
		auto* l = layers[i].get();
		auto* nl = layers[i + 1].get();
		if (l->is_temporary() &&
			l != _exceptLayer &&
			nl != _exceptLayer)
		{
			anyMerged = true;
			editedLayerIndex = i;
			String unused;
			merge_with_next(_this, unused);
			if (i < keepEditedLayerIndex)
			{
				--keepEditedLayerIndex;
			}
			--i;
		}
	}

	editedLayerIndex = keepEditedLayerIndex;

	return anyMerged;
}

bool VoxelMoldContentAccess::merge_with_next(VoxelMold* _this, OUT_ String& _resultInfo)
{
	ACCESS(layers);
	ACCESS(editedLayerIndex);

	if (!layers.is_index_valid(editedLayerIndex) ||
		!layers.is_index_valid(editedLayerIndex + 1))
	{
		_resultInfo = TXT("no next layer to merge");
		return false;
	}

	auto* l = layers[editedLayerIndex].get();
	auto* nl = layers[editedLayerIndex + 1].get();
	if (l->get_depth() != nl->get_depth())
	{
		_resultInfo = TXT("no next layer to merge");
		return false;
	}

	if (!l->is_visible(_this) || !nl->is_visible(_this))
	{
		_resultInfo = TXT("both layers should be visible");
		return false;
	}

	auto* vl = fast_cast<VoxelMoldLayer>(l);
	auto* nvl = fast_cast<VoxelMoldLayer>(nl);
	if (!vl || !nvl)
	{
		_resultInfo = TXT("can't merge these two layers");
		return false;
	}

	vl->merge(nvl);
	
	// keep valid name
	if (!vl->get_id().is_valid() ||
		vl->is_temporary())
	{
		vl->set_id(nvl->get_id());
	}
	vl->be_temporary(vl->is_temporary() && nvl->is_temporary());

	layers.remove_at(editedLayerIndex + 1);

	return true;
}

bool VoxelMoldContentAccess::merge_with_children(VoxelMold* _this, OUT_ String& _resultInfo)
{
	ACCESS(layers);
	ACCESS(editedLayerIndex);

	if (!layers.is_index_valid(editedLayerIndex))
	{
		_resultInfo = TXT("no next layer to merge");
		return false;
	}

	auto* l = layers[editedLayerIndex].get();

	if (!l->is_visible(_this))
	{
		_resultInfo = TXT("layer not visible");
		return false;
	}

	Name newId = l->get_id();

	int childCount = 0;
	int lDepth = l->get_depth();
	for (int i = editedLayerIndex + 1; i < layers.get_size(); ++i)
	{
		if (layers[i]->get_depth() <= lDepth)
		{
			break;
		}
		if (!newId.is_valid())
		{
			newId = layers[i]->get_id();
		}
		++childCount;
	}
	
	an_assert(!l->is_temporary());

	// always create a replacement VoxelMoldLayer that will be filled with "combine" result
	RefCountObjectPtr<IVoxelMoldLayer> replacementLayer;
	VoxelMoldLayer* replacementLayerVML = new VoxelMoldLayer();
	replacementLayer = replacementLayerVML;
	l->fill_copy_basics(replacementLayer.get());
	
	// use new id that could be the original one or taken from a child
	replacementLayer->set_id(newId);

	// combine now
	CombineVoxelsContext context;
	l->combine(_this, replacementLayerVML, layers, context);

	// put it where we are and remove old us and children
	layers.insert_at(editedLayerIndex, replacementLayer);
	layers.remove_at(editedLayerIndex + 1, 1 + childCount);

	return true;
}

bool VoxelMoldContentAccess::ray_cast(VoxelMold const * _this, Vector3 const& _start, Vector3 const& _dir, OUT_ VoxelHitInfo& _hitInfo, bool _onlyActualContent)
{
	ACCESS(zeroOffset);
	ACCESS(voxelSize);
	ACCESS(resultLayer);

	Vector3 fullOffset = zeroOffset * voxelSize;
	Vector3 start = _start + fullOffset;
	start /= voxelSize;

	Vector3 dir = _dir.normal();

	if (auto* rl = fast_cast<VoxelMoldLayer>(resultLayer.get()))
	{
		RangeInt3 range = rl->range;
		Vector3 rangeMin = range.near_bottom_left().to_vector3();
		Vector3 rangeMax = range.far_top_right().to_vector3() + Vector3::one;

		struct Utils
		{
			static bool is_possible_to_hit(float startC, float dirC, float toC, float dirAwayC)
			{
				float diff = toC - startC;
				if (diff * dirC < 0.0f &&
					dirAwayC * dirC > 0.0f)
				{
					return false;
				}
				return true;
			}
			static Vector3 drag_start_to(Vector3 const& _start, Vector3 const& _dir, float startC, float dirC, float toC, float dragC)
			{
				float diff = toC - startC;
				if (diff * dirC > 0.0f &&
					dragC * dirC > 0.0f)
				{
					float t = (toC - startC) / dirC;
					Vector3 start = _start + _dir * t;
					return start;
				}
				return _start;
			}
			static void update_in_cell(float startInCellC, float dirC, REF_ float & mint, REF_ Optional<VectorInt3> & changeCell, VectorInt3 const & cAxis)
			{
				if (dirC < 0.0f)
				{
					float t = startInCellC / -dirC;
					if (mint > t)
					{
						mint = t;
						changeCell = -cAxis;
					}
				}
				else if (dirC > 0.0f)
				{
					float t = (1.0f - startInCellC) / dirC;
					if (mint > t)
					{
						mint = t;
						changeCell = cAxis;
					}
				}
			}
		};


		{
			start = Utils::drag_start_to(start, dir, start.x, dir.x, rangeMin.x - 1.0f,  1.0f);
			start = Utils::drag_start_to(start, dir, start.x, dir.x, rangeMax.x + 1.0f, -1.0f);
			start = Utils::drag_start_to(start, dir, start.y, dir.y, rangeMin.y - 1.0f,  1.0f);
			start = Utils::drag_start_to(start, dir, start.y, dir.y, rangeMax.y + 1.0f, -1.0f);
			start = Utils::drag_start_to(start, dir, start.z, dir.z, rangeMin.z - 1.0f,  1.0f);
			start = Utils::drag_start_to(start, dir, start.z, dir.z, rangeMax.z + 1.0f, -1.0f);
		}

		if (!Utils::is_possible_to_hit(start.x, dir.x, rangeMin.x, -1.0f) ||
			!Utils::is_possible_to_hit(start.x, dir.x, rangeMax.x,  1.0f) ||
			!Utils::is_possible_to_hit(start.y, dir.y, rangeMin.y, -1.0f) ||
			!Utils::is_possible_to_hit(start.y, dir.y, rangeMax.y,  1.0f) ||
			!Utils::is_possible_to_hit(start.z, dir.z, rangeMin.z, -1.0f) ||
			!Utils::is_possible_to_hit(start.z, dir.z, rangeMax.z,  1.0f))
		{
			return false;
		}

		RangeInt3 rangeWider = range;
		rangeWider.expand_by(VectorInt3(1, 1, 1));

		VectorInt3 cell = TypeConversions::Normal::f_i_cells(start);
		rangeWider.include(cell);

		Vector3 startInCell;
		{
			Vector3 cellAt = cell.to_vector3();
			startInCell = start - cellAt;
		}
		VectorInt3 hitNormal = VectorInt3::zero;

		bool wasInRange = false;
		while (rangeWider.does_contain(cell))
		{
			int voxelIdx = voxel_index(cell, range);
			if (rl->voxels.is_index_valid(voxelIdx))
			{
				auto& voxel = rl->voxels[voxelIdx];
				bool hit = Voxel::is_solid(voxel.voxel);
				if (!_onlyActualContent)
				{
					hit |= Voxel::is_anything(voxel.voxel);
				}
				if (hit)
				{
					_hitInfo.hit = cell;
					_hitInfo.normal = hitNormal;
					return true;
				}
				wasInRange = true;
			}
			else if (wasInRange)
			{
				// we already left
				break;
			}
			float mint = 1000.0f;
			Optional<VectorInt3> changeCell;
			Utils::update_in_cell(startInCell.x, dir.x, REF_ mint, REF_ changeCell, VectorInt3(1, 0, 0));
			Utils::update_in_cell(startInCell.y, dir.y, REF_ mint, REF_ changeCell, VectorInt3(0, 1, 0));
			Utils::update_in_cell(startInCell.z, dir.z, REF_ mint, REF_ changeCell, VectorInt3(0, 0, 1));
			an_assert(changeCell.is_set());
			cell += changeCell.get();
			startInCell += dir * mint;
			startInCell -= changeCell.get().to_vector3();
			hitNormal = -changeCell.get();
		}
	}

	return false;
}

bool VoxelMoldContentAccess::ray_cast_xy(VoxelMold const* _this, Vector3 const& _start, Vector3 const& _dir, OUT_ VoxelHitInfo& _hitInfo)
{
	return ray_cast_plane(_this, _start, _dir, _hitInfo, 2, 0.0f);
}

bool VoxelMoldContentAccess::ray_cast_plane(VoxelMold const* _this, Vector3 const& _start, Vector3 const& _dir, OUT_ VoxelHitInfo& _hitInfo, int axisPlane, float _planeT)
{
	ACCESS(zeroOffset);
	ACCESS(voxelSize);

	Vector3 fullOffset = zeroOffset * voxelSize;
	Vector3 start = _start + fullOffset;
	int axisA = axisPlane == 0 ? 1 : 0;
	int axisB = axisPlane == 2 ? 1 : 2;
	if ((start.get_element(axisPlane) - _planeT) * _dir.get_element(axisPlane) <= 0.0f && _dir.get_element(axisPlane) != 0.0f)
	{
		float t = (_planeT - start.get_element(axisPlane)) / _dir.get_element(axisPlane);
		if (t >= 0.0f && t < 10000.0f)
		{
			Vector3 end = start + t * _dir;
			end /= voxelSize;
			end -= fullOffset;
			VectorInt3 cell = TypeConversions::Normal::f_i_cells(end);

			int atPlane = TypeConversions::Normal::f_i_cells((_planeT / voxelSize) - fullOffset.get_element(axisPlane));
			if (_dir.get_element(axisPlane) < 0.0f)
			{
				_hitInfo.hit = VectorInt3::zero;
				_hitInfo.hit.access().access_element(axisA) = cell.get_element(axisA);
				_hitInfo.hit.access().access_element(axisB) = cell.get_element(axisB);
				_hitInfo.hit.access().access_element(axisPlane) = atPlane;
				_hitInfo.normal = VectorInt3::zero;
				_hitInfo.normal.access().access_element(axisPlane) = 1;
			}
			else 
			{
				_hitInfo.hit = VectorInt3::zero;
				_hitInfo.hit.access().access_element(axisA) = cell.get_element(axisA);
				_hitInfo.hit.access().access_element(axisB) = cell.get_element(axisB);
				_hitInfo.hit.access().access_element(axisPlane) = atPlane;
				_hitInfo.normal = VectorInt3::zero;
				_hitInfo.normal.access().access_element(axisPlane) = -1;
			}
			return true;
		}
	}
	return false;
}

IVoxelMoldLayer* VoxelMoldContentAccess::access_edited_layer(VoxelMold * _this)
{
	ACCESS(layers);
	ACCESS(editedLayerIndex);

	if (!layers.is_index_valid(editedLayerIndex))
	{
		editedLayerIndex = 0;
	}
	if (layers.is_index_valid(editedLayerIndex))
	{
		return layers[editedLayerIndex].get();
	}
	return nullptr;
}

int VoxelMoldContentAccess::get_edited_layer_index(VoxelMold const* _this)
{
	return _this->editedLayerIndex;
}

void VoxelMoldContentAccess::set_edited_layer_index(VoxelMold* _this, int _index)
{
	ACCESS(layers);
	ACCESS(editedLayerIndex);

	bool visibilityOnly = false;

	if (layers.is_index_valid(editedLayerIndex))
	{
		visibilityOnly = layers[editedLayerIndex]->get_visibility_only();
	}

	editedLayerIndex = clamp(_index, 0, layers.get_size() - 1);

	if (layers.is_index_valid(editedLayerIndex))
	{
		auto* l = layers[editedLayerIndex].get();
		if (visibilityOnly)
		{
			l->set_visibility_only(visibilityOnly, _this);
		}
	}
}

void VoxelMoldContentAccess::move_edited_layer(VoxelMold* _this, int _by)
{
	ACCESS(layers);
	ACCESS(editedLayerIndex);

	while (_by < 0 && editedLayerIndex > 0)
	{
		swap(layers[editedLayerIndex - 1], layers[editedLayerIndex]);
		--editedLayerIndex;
		++_by;
	}
	int maxLayer = layers.get_size() - 1;
	while (_by > 0 && editedLayerIndex < maxLayer)
	{
		swap(layers[editedLayerIndex + 1], layers[editedLayerIndex]);
		++editedLayerIndex;
		--_by;
	}
}

void VoxelMoldContentAccess::change_depth_of_edited_layer(VoxelMold* _this, int _by)
{
	ACCESS(layers);
	ACCESS(editedLayerIndex);

	if (layers.is_index_valid(editedLayerIndex))
	{
		if (auto* l = layers[editedLayerIndex].get())
		{
			l->set_depth(l->get_depth() + _by);
		}
	}
}

void VoxelMoldContentAccess::change_depth_of_edited_layer_and_its_children(VoxelMold* _this, int _by)
{
	ACCESS(layers);
	ACCESS(editedLayerIndex);

	if (layers.is_index_valid(editedLayerIndex))
	{
		int depth = layers[editedLayerIndex]->get_depth();
		for (int i = editedLayerIndex; i < layers.get_size(); ++i)
		{
			auto* l = layers[i].get();
			if (i > editedLayerIndex && l->get_depth() <= depth)
			{
				break;
			}
			l->set_depth(l->get_depth() + _by);
		}
	}
}

void VoxelMoldContentAccess::change_depth_of_edited_layers_children(VoxelMold* _this, int _by)
{
	ACCESS(layers);
	ACCESS(editedLayerIndex);

	if (layers.is_index_valid(editedLayerIndex))
	{
		int depth = layers[editedLayerIndex]->get_depth();
		for (int i = editedLayerIndex; i < layers.get_size(); ++i)
		{
			auto* l = layers[i].get();
			if (i > editedLayerIndex)
			{
				if (l->get_depth() <= depth)
				{
					break;
				}
				l->set_depth(l->get_depth() + _by);
			}
		}
	}
}

void VoxelMoldContentAccess::remove_edited_layer(VoxelMold* _this)
{
	ACCESS(layers);
	ACCESS(editedLayerIndex);

	if (layers.is_index_valid(editedLayerIndex))
	{
		layers.remove_at(editedLayerIndex);
	}
}

void VoxelMoldContentAccess::add_layer(VoxelMold* _this, IVoxelMoldLayer* _layer, int _offset)
{
	ACCESS(layers);
	ACCESS(editedLayerIndex);

	if (layers.is_empty())
	{
		layers.push_back();
		layers.get_last() = _layer;
		editedLayerIndex = 0;
	}
	else
	{
		editedLayerIndex = clamp(editedLayerIndex, 0, layers.get_size() - 1);
		RefCountObjectPtr<IVoxelMoldLayer> nl;
		nl = _layer;
		nl->set_depth(layers[editedLayerIndex]->get_depth());
		layers.insert_at(editedLayerIndex + _offset, nl);
		editedLayerIndex += _offset;
	}
}

void VoxelMoldContentAccess::duplicate_edited_layer(VoxelMold* _this, int _offset)
{
	ACCESS(layers);
	ACCESS(editedLayerIndex);

	if (layers.is_index_valid(editedLayerIndex))
	{
		add_layer(_this, layers[editedLayerIndex]->create_copy(), _offset);
	}
}

void VoxelMoldContentAccess::rescale_more_voxels(VoxelMold* _this, int _scale)
{
	ACCESS(layers);

	set_voxel_size(_this, _this->get_voxel_size() / (float)_scale);

	for_every_ref(layer, layers)
	{
		layer->rescale(_scale, NP);
	}
}

void VoxelMoldContentAccess::rescale_less_voxels(VoxelMold* _this, int _scale)
{
	set_voxel_size(_this, _this->get_voxel_size() * (float)_scale);

	for_every_ref(layer, _this->layers)
	{
		layer->rescale(NP, _scale);
	}
}

void VoxelMoldContentAccess::set_colour_palette(VoxelMold* _this, ColourPalette const* _colourPalette, bool _convertToNewOne)
{
	ACCESS(colourPalette);
	ACCESS(layers);
	
	if (_convertToNewOne && colourPalette.get() && _colourPalette && colourPalette.get() != _colourPalette)
	{
		UsedLibraryStored<ColourPalette> prevPalette = colourPalette;
		Array<int> prevToNew;
		for_every(pc, prevPalette->get_colours())
		{
			int best = NONE;
			float bestDiff = 0.0f;
			for_every(nc, _colourPalette->get_colours())
			{
				float diff = abs(nc->r - pc->r) + abs(nc->g - pc->g) + abs(nc->b - pc->b);
				if (diff < bestDiff || best == NONE)
				{
					bestDiff = diff;
					best = for_everys_index(nc);
				}
			}
			prevToNew.push_back(best);
		}
		for_every_ref(l, layers)
		{
			if (auto* vml = fast_cast<VoxelMoldLayer>(l))
			{
				for_every(v, vml->voxels)
				{
					if (prevToNew.is_index_valid(v->voxel))
					{
						v->voxel = prevToNew[v->voxel];
					}
				}
			}
		}
	}
	colourPalette = _colourPalette;

	combine_voxels(_this);
}

bool VoxelMoldContentAccess::can_include(VoxelMold const * _this, VoxelMold const* _other)
{
	struct IncludeCheck
	{
		Array<VoxelMold const*> used;

		bool setup_for(VoxelMold const* _vm)
		{
			used.clear();
			used.push_back(_vm);
			int i = 0;
			while (i < used.get_size())
			{
				auto* vm = used[i];
				for_every_ref(l, vm->layers)
				{
					if (auto* ivml = fast_cast<IncludeVoxelMoldLayer>(l))
					{
						auto* ivm = ivml->get_included();
						used.push_back_unique(ivm);
					}
				}

				if (used.get_size() > 10000)
				{
					an_assert(false, TXT("that's too many?"));
					return false;
				}
				++i;
			}
			return true;
		}
	} includeCheck;

	if (!includeCheck.setup_for(_other))
	{
		return false;
	}
	if (includeCheck.used.does_contain(_this))
	{
		// can't include someone who already included us
		return false;
	}

	return true;
}
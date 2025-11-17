#include "displayHardwareMeshGenerator.h"

#include "..\..\core\mesh\mesh3d.h"
#include "..\..\core\system\video\vertexFormatUtils.h"

#include "..\appearance\mesh.h"
#include "..\library\library.h"
#include "..\library\usedLibraryStored.inl"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

//

// library name
DEFINE_STATIC_NAME(displays);
DEFINE_STATIC_NAME_STR(hardwareDisplayDevice, TXT("hardware display device"));

// global references
DEFINE_STATIC_NAME_STR(grDisplayVRSceneMeshGenerator, TXT("display vr scene mesh generator"));
DEFINE_STATIC_NAME_STR(grDisplayVRSceneMaterial, TXT("display vr scene material"));

// variables
DEFINE_STATIC_NAME(radius);

// vertex format elements
#ifdef USE_DISPLAY_CASING_POST_PROCESS
DEFINE_STATIC_NAME(inIsCasing);
#endif

//

DisplayHardwareMeshSetup::DisplayHardwareMeshSetup()
{
	material.set_name(LibraryName(NAME(displays), NAME(hardwareDisplayDevice)));
	if (!material.find_may_fail(Library::get_current()))
	{
		// as we have material hardcoded here, we will have to have such material anyway
		// that's why this error is commented out
		//error(TXT("couldn't find material \"%S\""), material.get_name().to_string().to_char());
	}
}

#define LOAD_FLOAT(_name) \
	_name = _node->get_float_attribute(TXT(#_name), _name);

#define LOAD_INT(_name) \
	_name = _node->get_int_attribute(TXT(#_name), _name);

bool DisplayHardwareMeshSetup::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;

	if (!_node)
	{
		return true;
	}

#ifdef USE_DISPLAY_MESH
	// loading paremeters
	if (IO::XML::Attribute const * attr = _node->get_attribute(TXT("casingInside")))
	{
		casingInside.x = casingInside.y = attr->get_as_float();
	}
	casingInside.load_from_xml_child_node(_node, TXT("casingInside"));
	LOAD_FLOAT(innerCornerInside);
	LOAD_FLOAT(outerCornerInside);
	LOAD_FLOAT(diagonal);
	LOAD_FLOAT(depth);
	LOAD_FLOAT(casingDepth);
	LOAD_FLOAT(innerBorderTakes);
	LOAD_FLOAT(reflectUVDistanceCoef);
	LOAD_FLOAT(reflectUVPerpendicularCoef);
	LOAD_FLOAT(darkerReflectionInCornersCoef);

	LOAD_INT(xCurvePointCount);
	LOAD_INT(yCurvePointCount);
	LOAD_INT(innerRingCount);
	LOAD_INT(outerRingCount);

	screenColour.load_from_xml_child_node(_node, TXT("screenColour"));
	casingColour.load_from_xml_child_node(_node, TXT("casingColour"));

	result &= material.load_from_xml(_node, TXT("material"), _lc);
#endif

	return result;
}

#undef LOAD_FLOAT
#undef LOAD_INT

bool DisplayHardwareMeshSetup::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;

#ifdef USE_DISPLAY_MESH
	result &= material.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);
#endif

	return result;
}

//

Mesh* DisplayHardwareMeshGenerator::create_2d_mesh(DisplayHardwareMeshSetup const & _setup, Library* _library, LibraryName const & _useLibraryName)
{
	return create_mesh(_setup, true, _library, _useLibraryName);
}

Mesh* DisplayHardwareMeshGenerator::create_3d_mesh(DisplayHardwareMeshSetup const & _setup, Library* _library, LibraryName const & _useLibraryName, OPTIONAL_ OUT_ Vector2 * _meshSize)
{
	return create_mesh(_setup, false, _library, _useLibraryName, OPTIONAL_ OUT_ _meshSize);
}

static BezierCurve<Vector2> prepare_curve(int curveIndex, Vector2 const & halfSize, Vector2 const & corner, float const tangent)
{
	BezierCurve<Vector2> curve;
	if (curveIndex == 0) curve = BezierCurve<Vector2>(Vector2(-corner.x, corner.y), Vector2(-corner.x + tangent, corner.y + tangent), Vector2(-corner.x + tangent, halfSize.y), Vector2(0.0f, halfSize.y));
	if (curveIndex == 1) curve = BezierCurve<Vector2>(Vector2(0.0f, halfSize.y), Vector2(corner.x - tangent, halfSize.y), Vector2(corner.x - tangent, corner.y + tangent), Vector2(corner.x, corner.y));
	if (curveIndex == 2) curve = BezierCurve<Vector2>(Vector2(corner.x, corner.y), Vector2(corner.x + tangent, corner.y - tangent), Vector2(halfSize.x, corner.y - tangent), Vector2(halfSize.x, 0.0f));
	if (curveIndex == 3) curve = BezierCurve<Vector2>(Vector2(halfSize.x, 0.0f), Vector2(halfSize.x, -corner.y + tangent), Vector2(corner.x + tangent, -corner.y + tangent), Vector2(corner.x, -corner.y));
	if (curveIndex == 4) curve = BezierCurve<Vector2>(Vector2(corner.x, -corner.y), Vector2(corner.x - tangent, -corner.y - tangent), Vector2(corner.x - tangent, -halfSize.y), Vector2(0.0f, -halfSize.y));
	if (curveIndex == 5) curve = BezierCurve<Vector2>(Vector2(0.0f, -halfSize.y), Vector2(-corner.x + tangent, -halfSize.y), Vector2(-corner.x + tangent, -corner.y - tangent), Vector2(-corner.x, -corner.y));
	if (curveIndex == 6) curve = BezierCurve<Vector2>(Vector2(-corner.x, -corner.y), Vector2(-corner.x - tangent, -corner.y + tangent), Vector2(-halfSize.x, -corner.y + tangent), Vector2(-halfSize.x, 0.0f));
	if (curveIndex == 7) curve = BezierCurve<Vector2>(Vector2(-halfSize.x, 0.0f), Vector2(-halfSize.x, corner.y - tangent), Vector2(-corner.x - tangent, corner.y - tangent), Vector2(-corner.x, corner.y));
	return curve;
}

static float calculate_radius_of_sphere(float _diagonal, float _depth)
{
	_depth = max(_depth, 0.0f);
	if (_depth == 0.0f) // flat
	{
		return 0.0f;
	}

	// find radius
	// x^2 + y^2 + z^2 = r^2
	// z = r - d
	// x^2 + y^2 = r^2 - (r - d)^2
	// x^2 + y^2 = r^2 - (r^2 - 2dr + d^2)
	// x^2 + y^2 = 2dr - d^2
	// 2dr = x^2 + y^2 + d^2
	// r = (x^2 + y^2 + d^2 ) / 2d
	// r = (x^2 + y^2) / 2d + d/2
	return sqr(_diagonal * 0.5f) / (2.0f * _depth) + _depth / 2.0f;
}

static float calculate_depth_at(Vector2 const & loc, float r, float _diagonal, Vector2 const & _diagonalPoint)
{
	if (r <= 0.0f) // flat
	{
		return 0.0f;
	}

	// find z
	// x^2 + y^2 + z^2 = r^2

	return r - sqrt(max(0.0f, sqr(r) - (_diagonal * 0.5f * (loc / _diagonalPoint)).length_squared()));
}

Mesh* DisplayHardwareMeshGenerator::create_mesh(DisplayHardwareMeshSetup const & _setup, bool _2d, Library* _library, LibraryName const & _useLibraryName, OPTIONAL_ OUT_ Vector2 * _meshSize)
{
	an_assert(!_setup.size2d.is_zero());
	Meshes::Mesh3D* mesh = new Meshes::Mesh3D();

#ifdef AN_SYSTEM_INFO_DRAW_DETAILS
	mesh->set_debug_mesh_name(String(TXT("DisplayHardwareMeshGenerator")));
#endif

	System::IndexFormat indexFormat;
	indexFormat.with_element_size(System::IndexElementSize::FourBytes);
	indexFormat.calculate_stride();

	System::VertexFormat vertexFormat;
	vertexFormat.with_padding();
	vertexFormat.with_colour_rgb();
	vertexFormat.with_texture_uv().with_texture_uv_type(::System::DataFormatValueType::Float);
#ifdef USE_DISPLAY_CASING_POST_PROCESS
	vertexFormat.with_custom(NAME(inIsCasing), System::DataFormatValueType::Float, System::VertexFormatAttribType::Float, 1);
#endif
	vertexFormat.calculate_stride_and_offsets();

	int aroundPointCount = (_setup.xCurvePointCount - 1) * 2 * 2 +
						   (_setup.yCurvePointCount - 1) * 2 * 2; // on each curve we do not have last point - it is shared with first on next curve
	int numberOfRings = _setup.innerRingCount + _setup.outerRingCount; // they do not share ring!

	Array<int32> elements;

	Array<int8> vertexData;
	vertexData.set_size(vertexFormat.get_stride() * (aroundPointCount * numberOfRings + 1)); // plus 1 for centre one
	int8* currentVertexData = vertexData.get_data();

	Vector2 useSize = _2d ? _setup.size2d : (_setup.size3d.is_zero() ? _setup.size2d : _setup.size3d);

	// calculate points to be used with bezier curves for inner and outer
	Vector2 outerHalfSize = (useSize * _setup.sizeMultiplier * 0.5f);
	Vector2 innerHalfSize = (useSize * _setup.sizeMultiplier * 0.5f * (Vector2::one - _setup.casingInside));
	float outerTangent = min(outerHalfSize.x, outerHalfSize.y) * (_setup.outerCornerInside);
	float innerTangent = min(innerHalfSize.x, innerHalfSize.y) * (_setup.innerCornerInside);
	Vector2 outerCorner = outerHalfSize - Vector2(outerTangent, outerTangent);
	Vector2 innerCorner = innerHalfSize - Vector2(innerTangent, innerTangent);

	// for 3d scale to match diagonal
	if (!_2d && _setup.size3d.is_zero())
	{
		float innerCornerLength = innerCorner.length();
		float innerCornerDiagonal = innerCornerLength * 2.0f;
		float scale = _setup.diagonal / innerCornerDiagonal;
		outerHalfSize *= scale;
		innerHalfSize *= scale;
		outerTangent *= scale;
		innerTangent *= scale;
		outerCorner *= scale;
		innerCorner *= scale;
	}

	assign_optional_out_param(_meshSize, outerHalfSize * 2.0f);

	float sphereRadius = calculate_radius_of_sphere(_setup.diagonal, _setup.depth);

	// add centre
	{
		Vector3* location = (Vector3*)(currentVertexData + vertexFormat.get_location_offset());
		*location = Vector3::zero;
		
		::System::VertexFormatUtils::store(currentVertexData + vertexFormat.get_texture_uv_offset(), vertexFormat.get_texture_uv_type(), Vector2::half);
		
		Colour* colour = (Colour*)(currentVertexData + vertexFormat.get_colour_offset());
		colour->r = _setup.screenColour.r;
		colour->g = _setup.screenColour.g;
		colour->b = _setup.screenColour.b;
	}
	currentVertexData += vertexFormat.get_stride();

#ifdef USE_DISPLAY_CASING_POST_PROCESS
	System::VertexFormatUtils::FillCustomDataParams isCasingParams;
	isCasingParams.with_float(1.0f);
	System::VertexFormatUtils::FillCustomDataParams isScreenParams;
	isScreenParams.with_float(0.0f);
	auto* isCasingCustomData = vertexFormat.get_custom_data(NAME(inIsCasing));		
#endif

	// start with top left corner, go clockwise, never add last point
	//		0 1
	//	  7		2
	//	  6		3
	//		5 4	
	for (int ringIndex = 0; ringIndex < numberOfRings; ++ringIndex)
	{
		Vector2 corner = innerCorner;
		Vector2 halfSize = innerHalfSize;
		float tangent = innerTangent;
		float ringT = 0.0f;
		bool actualScreen = true;
		if (ringIndex < _setup.innerRingCount)
		{
			actualScreen = true;
			float t = (float)(ringIndex + 1) / (float)(_setup.innerRingCount);
			t = (1.0f - sqr(1.0f - t)) * 0.9f + 0.1f * t; // to have more density at the edge
			t *= (1.0f - _setup.innerBorderTakes); // don't touch edge?
			ringT = t;
			corner = innerCorner * t;
			halfSize = innerHalfSize * t;
			tangent = innerTangent * t;
		}
		else
		{
			actualScreen = false;
			float t = clamp((float)(ringIndex - _setup.innerRingCount) / (float)(_setup.outerRingCount - 1), 0.0f, 1.0f);
			t = sqr(t) * 0.8f + 0.2f * t; // more density closer to edge
			t = 1.0f - ((1.0f - _setup.innerBorderTakes) * (1.0f - t)); // don't touch edge
			ringT = t;
			corner = innerCorner * (1.0f - t) + outerCorner * t;
			halfSize = innerHalfSize * (1.0f - t) + outerHalfSize * t;
			tangent = innerTangent * (1.0f - t) + outerTangent * t;
		}

		for (int curveIndex = 0; curveIndex < 8; ++curveIndex)
		{
			int curvePointCount = (curveIndex < 2 || curveIndex == 4 || curveIndex == 5) ? _setup.xCurvePointCount : _setup.yCurvePointCount;
			// choose curve around
			BezierCurve<Vector2> curve = prepare_curve(curveIndex, halfSize, corner, tangent);
			BezierCurve<Vector2> innerCurve = prepare_curve(curveIndex, innerHalfSize, innerCorner, innerTangent);
			for (int pointIndex = 0; pointIndex < curvePointCount - 1; ++pointIndex)
			{
				// t along curve
				float t = (float)pointIndex / (float)(curvePointCount - 1);
				
				Vector3* location = (Vector3*)(currentVertexData + vertexFormat.get_location_offset());
				Vector2 loc = curve.calculate_at(t);
				float z = 0.0f;
				Vector2 uv = Vector2::zero;
				Colour colour = Colour::white;

				if (actualScreen)
				{
					uv = loc / outerHalfSize;
					colour = _setup.screenColour * (1.0f - 0.2f * ringT); // lighter in the centre
					colour *= clamp(((1.0f - _setup.innerBorderTakes) - ringT) / 0.01f, 0.0f, 1.0f); // border (check above)
					z = calculate_depth_at(loc, sphereRadius, _setup.diagonal, innerCorner);
				}
				else
				{
					// calculate relative/normalised corner placement (as it would be square)
					// we need that to know where we are and to calculate colours
					Vector2 corLoc = Vector2(loc.x * halfSize.y / halfSize.x, loc.y).normal();
					corLoc.x = abs(corLoc.x);
					corLoc.y = abs(corLoc.y);

					// calculate depth - from point on screen's edge (inner) to end of screen/casing using ringT
					Vector2 locAtInnerCurve = innerCurve.calculate_at(t);
					z = calculate_depth_at(locAtInnerCurve, sphereRadius, _setup.diagonal, innerCorner) * (1.0f - ringT) + _setup.casingDepth * ringT;

					// calculate uv - mirror from point on screen's edge (inner) with small adjustments
					Vector2 uvInnerCurve = locAtInnerCurve / outerHalfSize;
					uv = loc / outerHalfSize;
					uv = uvInnerCurve - (uv - uvInnerCurve) * _setup.reflectUVDistanceCoef * (corLoc.x > corLoc.y ? Vector2(1.0f, _setup.reflectUVPerpendicularCoef) : Vector2(_setup.reflectUVPerpendicularCoef, 1.0f));

					// colour should fade out when close to corner and very close to screen
					colour = _setup.casingColour;
					colour *= clamp(-1.0f + abs(corLoc.x - corLoc.y) * _setup.darkerReflectionInCornersCoef, 0.0f, 1.0f); // darker in corners
					colour *= clamp(1.0f - 1.2f * ringT, 0.0f, 1.0f); // darker on outer ridge
					colour *= clamp(0.6f + ringT * 2.0f, 0.0f, 1.0f); // lighten slighlty when going away
					colour *= clamp((ringT - (_setup.innerBorderTakes) * 2.5f) * 20.0f, 0.0f, 1.0f); // border
				}

				*location = Vector3(loc.x, loc.y, z);

				::System::VertexFormatUtils::store(currentVertexData + vertexFormat.get_texture_uv_offset(), vertexFormat.get_texture_uv_type(), uv* Vector2::half + Vector2::half);

				Colour* vertexColourRGB = (Colour*)(currentVertexData + vertexFormat.get_colour_offset());
				vertexColourRGB->r = colour.r;
				vertexColourRGB->g = colour.g;
				vertexColourRGB->b = colour.b;

#ifdef USE_DISPLAY_CASING_POST_PROCESS
				System::VertexFormatUtils::fill_custom_data(vertexFormat, currentVertexData, vertexFormat.get_stride(), *isCasingCustomData, actualScreen? isScreenParams : isCasingParams);
#endif

				currentVertexData += vertexFormat.get_stride();
			}
		}
	}

	// elements
	// inner with centre
	int ringStartsAt = 1;
	for (int pointIndex = 0; pointIndex < aroundPointCount; ++pointIndex)
	{
		elements.push_back(0);
		elements.push_back(ringStartsAt + (pointIndex));
		elements.push_back(ringStartsAt + ((pointIndex + 1) % aroundPointCount));
	}
	int prevRingStartsAt = ringStartsAt;
	ringStartsAt += aroundPointCount;
	
	for (int ringType = 0; ringType < 2; ++ringType)
	{
		int ringCount = ringType == 0 ? _setup.innerRingCount - 1 : _setup.outerRingCount - 1;
		for (int ringIndex = 0; ringIndex < ringCount; ++ringIndex)
		{
			for (int pointIndex = 0; pointIndex < aroundPointCount; ++pointIndex)
			{
				elements.push_back(prevRingStartsAt + ((pointIndex + 1) % aroundPointCount));
				elements.push_back(prevRingStartsAt + (pointIndex));
				elements.push_back(ringStartsAt + (pointIndex));

				elements.push_back(ringStartsAt + (pointIndex));
				elements.push_back(ringStartsAt + ((pointIndex + 1) % aroundPointCount));
				elements.push_back(prevRingStartsAt + ((pointIndex + 1) % aroundPointCount));
			}
			prevRingStartsAt = ringStartsAt;
			ringStartsAt += aroundPointCount;
		}
		prevRingStartsAt = ringStartsAt;
		ringStartsAt += aroundPointCount;
	}

	if (_2d)
	{
		int count = vertexData.get_size() / vertexFormat.get_stride();
		int8* currentVertexData = vertexData.get_data();
		for (int i = 0; i < count; ++i, currentVertexData += vertexFormat.get_stride())
		{
			Vector3* location = (Vector3*)(currentVertexData + vertexFormat.get_location_offset());
			float z = location->z;
			// we add 1.0 to z, as z is relative to front plane of display
			// and we only affect z component with camera angle to have display always taking same space on screen (at 0 plane)
			float coef = 1.0f / (1.0f + z * tan_deg(clamp(_setup.cameraAngle, 1.0f, 175.0f) / 2.0f)); 
			location->x *= coef;
			location->y *= coef;
			location->z = 0.0f;
		}
	}
	else
	{
		int count = vertexData.get_size() / vertexFormat.get_stride();
		int8* currentVertexData = vertexData.get_data();
		for (int i = 0; i < count; ++i, currentVertexData += vertexFormat.get_stride())
		{
			Vector3* location = (Vector3*)(currentVertexData + vertexFormat.get_location_offset());
			swap(location->y, location->z);
		}
	}

	mesh->load_part_data(vertexData.get_data(), elements.get_data(), Meshes::Primitive::Triangle, vertexData.get_size() / vertexFormat.get_stride(), elements.get_size(), vertexFormat, indexFormat);
	
	if (!_library)
	{
		_library = Library::get_current();
	}

	// create framework mesh and setup material
	Mesh* frameworkMesh;
	if (_useLibraryName.is_valid())
	{
		frameworkMesh = _library->get_meshes_static().find_or_create(_useLibraryName);
	}
	else
	{
		frameworkMesh = new Mesh(_library, _useLibraryName);
	}
	frameworkMesh->be_temporary(true);
	frameworkMesh->replace_mesh(mesh);
	frameworkMesh->set_material(0, _setup.material.get());

	return frameworkMesh;
}

Mesh* DisplayHardwareMeshGenerator::create_vr_scene_mesh(float _radius, Library* _library, LibraryName const & _useLibraryName, LibraryName const & _useMeshGenerator)
{
	MeshGenerator* useMeshGenerator = nullptr;
	Material* useMaterial = nullptr;
	if (_library)
	{
		if (_useMeshGenerator.is_valid())
		{
			useMeshGenerator = _library->get_mesh_generators().find(_useMeshGenerator);
		}
		if (!useMeshGenerator)
		{
			useMeshGenerator = _library->get_global_references().get<MeshGenerator>(NAME(grDisplayVRSceneMeshGenerator));
		}
		if (!useMaterial)
		{
			useMaterial = _library->get_global_references().get<Material>(NAME(grDisplayVRSceneMaterial));
		}
	}
	if (!useMeshGenerator)
	{
		todo_note(TXT("some fallback?"));
		return nullptr;
	}

	todo_note(TXT("create mesh"));
	SimpleVariableStorage parameters;
	parameters.access<float>(NAME(radius)) = _radius;
	auto * mesh = useMeshGenerator->generate_mesh(MeshGeneratorRequest().using_parameters(parameters).no_lods());

	if (!mesh)
	{
		return nullptr;
	}

	Mesh* frameworkMesh;
	if (_useLibraryName.is_valid())
	{
		frameworkMesh = _library->get_meshes_static().find_or_create(_useLibraryName);
	}
	else
	{
		frameworkMesh = new Mesh(_library, _useLibraryName);
	}

	frameworkMesh->replace_mesh(mesh);
	if (useMaterial)
	{
		frameworkMesh->set_material(0, useMaterial);
	}

	return frameworkMesh;
}

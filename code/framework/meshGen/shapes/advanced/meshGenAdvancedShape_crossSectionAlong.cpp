#include "..\meshGenAdvancedShapes.h"

#include "..\..\meshGenGenerationContext.h"
#include "..\..\meshGenCreateCollisionInfo.h"
#include "..\..\meshGenCreateSpaceBlockerInfo.h"
#include "..\..\meshGenUtils.h"
#include "..\..\meshGenValueDef.h"
#include "..\..\meshGenValueDefImpl.inl"
#include "..\..\meshGenSubStepDef.h"
#include "..\..\customData\meshGenSpline.h"
#include "..\..\customData\meshGenSplineRef.h"

#include "..\..\..\library\customLibraryDatas.h"
#include "..\..\..\library\customLibraryDataLookup.h"
#include "..\..\..\library\customLibraryDataLookup.inl"

#include "..\..\..\..\core\collision\model.h"
#include "..\..\..\..\core\mesh\mesh3d.h"
#include "..\..\..\..\core\mesh\builders\mesh3dBuilder_IPU.h"
#include "..\..\..\..\core\mesh\helpers\mesh3dHelper_bezierSurface.h"
#include "..\..\..\..\core\wheresMyPoint\wmp.h"

#ifdef AN_CLANG
DEFINE_STATIC_NAME(zero);
#include "..\..\customData\meshGenSplineImpl.inl"
#include "..\..\customData\meshGenSplineRefImpl.inl"
#endif

using namespace Framework;
using namespace MeshGeneration;
using namespace AdvancedShapes;

//

DEFINE_STATIC_NAME(csStartsAt);
DEFINE_STATIC_NAME(csEndsAt);
DEFINE_STATIC_NAME(csUp);
DEFINE_STATIC_NAME(csHeight);
DEFINE_STATIC_NAME(csNominalHeight);
DEFINE_STATIC_NAME(csWidth);

//

class CrossSectionAlongData
: public ElementShapeData
{
	FAST_CAST_DECLARE(CrossSectionAlongData);
	FAST_CAST_BASE(ElementShapeData);
	FAST_CAST_END();

	typedef ElementShapeData base;

public:
	override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
	override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

public:
	CreateCollisionInfo createMovementCollisionMesh; // will create mesh (that might be broken into convex hulls)
	CreateCollisionInfo createPreciseCollisionMesh; // will create mesh
	CreateSpaceBlockerInfo createSpaceBlocker;
	bool noMesh = false;
	bool ignoreForCollision = false;

	/**
	 *	Requires refPoints for spline:
	 *		zero
	 *		height (along y axis)
	 *		length (along x axis)
	 *		compress min/max (optional) brackets for compression if at 0/length it will compress/stretch only points between 0 and length leaving other ones unaffected (well, height affects them)
	 *
	 *	Requires parameters:
	 *		startsAt
	 *		endsAt
	 *		up (optional)
	 *		height (optional)
	 *		width (optional - to scale existing)
	 */

	// using lists instead of arrays to make it easier to add elements for the memory + not copy actual stuff all around
	struct Layer
	: public RefCountObject
	{
		struct Sub
		{
			GenerationCondition generationCondition;
			ValueDef<float> min;
			ValueDef<float> max;
			Optional<float> placeAtOfLength; // align at % of length, don't compress
			bool capMin = false;
			bool capMax = false;
			bool capsOnly = false;
		};
		GenerationCondition generationCondition;
		bool reverseCrossSection = false;
		bool reverseSurfaces = false;
		CustomData::SplineRef<Vector2> crossSection;
		SubStepDef crossSectionSubStep;
		List<Sub> subs;
		UVDef uAtMin;
		UVDef uAtMax;
	};
	List<Layer> layers;
};

//

ElementShapeData* AdvancedShapes::create_cross_section_along_data()
{
	return new CrossSectionAlongData();
}

//

static void decompress_recompress_x(float & _x, float _below, float _above, float _decompress, float _addAboveDecompressed, float _recompress, float _addAboveRecompressed)
{
	if (_x < _below)
	{
		_x *= _decompress;
	}
	else if (_x > _above)
	{
		_x = _x * _decompress + _addAboveDecompressed;
	}
	else
	{
		_x = _x * _recompress + _addAboveRecompressed;
	}
}

bool AdvancedShapes::cross_section_along(GenerationContext & _context, ElementInstance & _instance, ::Framework::MeshGeneration::ElementShapeData const * _data)
{
	Array<int8> elementsData;
	Array<int8> vertexData;

	::Meshes::Builders::IPU ipu;

	MeshGeneration::load_ipu_parameters(ipu, _context);

	//

	if (CrossSectionAlongData const * csData = fast_cast<CrossSectionAlongData>(_data))
	{
		Vector3 useStartsAt = Vector3::zero;
		Vector3 useEndsAt = Vector3::zero;
		Vector3 useUp = Vector3::zero;
		float useHeight = 1.0f;
		float useNominalHeight = 1.0f;
		float useWidth = 0.0f;

		bool startsAtPresent = false;
		bool endsAtPresent = false;
		bool heightPresent = false;
		bool nominalHeightPresent = false;
		bool widthPresent = false;
		{
			FOR_PARAM(_context, Vector3, csStartsAt)
			{
				useStartsAt = *csStartsAt;
				startsAtPresent = true;
			}
			FOR_PARAM(_context, Vector3, csEndsAt)
			{
				useEndsAt = *csEndsAt;
				endsAtPresent = true;
			}
			FOR_PARAM(_context, Vector3, csUp) // perpendicular
			{
				useUp = *csUp;
			}
			FOR_PARAM(_context, float, csHeight)
			{
				useHeight = *csHeight;
				heightPresent = true;
			}
			FOR_PARAM(_context, float, csNominalHeight)
			{
				useNominalHeight = *csNominalHeight; // nominal height works as scale
				nominalHeightPresent = true;
			}
			FOR_PARAM(_context, float, csWidth)
			{
				useWidth = *csWidth;
				widthPresent = true;
			}
		}

		if (!startsAtPresent)
		{
			error_generating_mesh(_instance, TXT("no \"startsAt\" defined"));
		}
		if (!endsAtPresent)
		{
			error_generating_mesh(_instance, TXT("no \"endsAt\" defined"));
		}
		if (startsAtPresent && endsAtPresent)
		{
			for_every(layer, csData->layers)
			{
				if (!layer->generationCondition.check(_instance))
				{
					continue;
				}
				float uAtMinValue = layer->uAtMin.get_u(_context);
				float uAtMaxValue = layer->uAtMax.get_u(_context);
				// get all required parameters
				Matrix33 from = Matrix33::identity;
				DEFINE_STATIC_NAME(zero);
				if (Vector2 const * point = layer->crossSection.get()->find_ref_point(NAME(zero)))
				{
					from.set_translation(*point);
				}
				DEFINE_STATIC_NAME(length);
				if (Vector2 const * point = layer->crossSection.get()->find_ref_point(NAME(length)))
				{
					from.set_x_axis(*point);
				}
				DEFINE_STATIC_NAME(height);
				if (Vector2 const * point = layer->crossSection.get()->find_ref_point(NAME(height)))
				{
					from.set_y_axis(*point);
				}
				else
				{
					from.set_y_axis(-from.get_x_axis_2().rotated_right());
				}

				Optional<float> fromCompressMin;
				Optional<float> fromCompressMax;
				DEFINE_STATIC_NAME(compressMin);
				if (Vector2 const * point = layer->crossSection.get()->find_ref_point(NAME(compressMin)))
				{
					fromCompressMin = point->x;
				}
				DEFINE_STATIC_NAME(compressMax);
				if (Vector2 const * point = layer->crossSection.get()->find_ref_point(NAME(compressMax)))
				{
					fromCompressMax = point->x;
				}
				float fromLength = from.get_x_axis_2().length();

				// calculate values for end mesh
				float length = (useEndsAt - useStartsAt).length();
				float height = useHeight;
				if (nominalHeightPresent)
				{
					height = useNominalHeight * from.get_y_axis_2().length();
				}
				else if (!heightPresent)
				{
					height = from.get_y_axis_2().length();
				}

				// adjust width
				Range totalWidth = Range::empty;
				for_every(sub, layer->subs)
				{
					if (!sub->generationCondition.check(_instance))
					{
						continue;
					}
					totalWidth.include(Range(sub->min.get(_context), sub->max.get(_context)));
				}

				float widthMultiplier = 1.0f;
				if (widthPresent && totalWidth.length() != 0.0f)
				{
					widthMultiplier = useWidth / totalWidth.length();
				}

				// process (sub) layers
				for_every(sub, layer->subs)
				{
					if (!sub->generationCondition.check(_instance))
					{
						continue;
					}
					// ready transform so we will have cross section properly scaled
					Matrix33 to = Matrix33::identity;
					if (sub->placeAtOfLength.is_set())
					{
						// place where it should be placed
						to.set_translation(Vector2(length * sub->placeAtOfLength.get(), 0.0f));
						to.set_y_axis(Vector2(0.0f, height));
						to.set_x_axis(Vector2((height / from.get_y_axis_2().length()) * fromLength, 0.0f));
					}
					else
					{
						// compress/stretch
						to.set_translation(Vector2::zero);
						to.set_y_axis(Vector2(0.0f, height));
						to.set_x_axis(Vector2(length, 0.0f));
					}


					Array<CustomData::Spline<Vector2>::Segment> csSegments;
					{
						CustomData::SplineContext<Vector2> csContext;
						csContext.transform = to.to_world(from.full_inverted());

						WheresMyPoint::Context wmpContext(&_instance);
						wmpContext.set_random_generator(_instance.access_context().access_random_generator());
						layer->crossSection.get()->get_segments_for(csSegments, wmpContext, csContext);

						if (!sub->placeAtOfLength.is_set())
						{
							// decompress points that are before below and after above (check compress min/max)
							// use height for that (first reverse length stretching and then stretch using height)
							// this is to keep circles etc at start and at end
							// for points between recompress them so they fill left space properly
							float decompressX = (to.get_y_axis_2().length() / from.get_y_axis_2().length()) * (fromLength / length);
							float addDecompressXAbove = length * (1.0f - decompressX);
							float below = fromCompressMin.is_set() ? length * (fromCompressMin.get() / fromLength) : 0.0f;
							float above = fromCompressMax.is_set() ? length * (fromCompressMax.get() / fromLength) : length;
							float clampedAbove = min(length, above);
							float clampedBelow = max(0.0f, below);
							float recompressX = length / (clampedAbove - clampedBelow);
							float addRecompressX = ((clampedAbove - clampedBelow) * 0.5f + clampedBelow) * (1.0f - recompressX); // that's centre of clamped "below->above"
							for_every(segment, csSegments)
							{
								decompress_recompress_x(segment->segment.p0.x, below, above, decompressX, addDecompressXAbove, recompressX, addRecompressX);
								decompress_recompress_x(segment->segment.p1.x, below, above, decompressX, addDecompressXAbove, recompressX, addRecompressX);
								decompress_recompress_x(segment->segment.p2.x, below, above, decompressX, addDecompressXAbove, recompressX, addRecompressX);
								decompress_recompress_x(segment->segment.p3.x, below, above, decompressX, addDecompressXAbove, recompressX, addRecompressX);
							}
						}
					}

					CustomData::Spline<Vector2>::make_sure_segments_are_clockwise(csSegments);

					// change segments into actual points
					Array<Vector2> points;
					Array<UVDef> us;
					CustomData::Spline<Vector2>::change_segments_into_points(csSegments, Vector2::one /* already scaled */, layer->crossSectionSubStep, _context, OUT_ points, OUT_ us, fast_cast<CrossSectionAlongData>(_data) && fast_cast<CrossSectionAlongData>(_data)->noMesh);

					if (layer->reverseCrossSection)
					{
						{
							int last = points.get_size() - 1;
							int count = points.get_size() / 2;
							for (int i = 0; i < count; ++i)
							{
								swap(points[i], points[last - i]);
							}
						}
						{
							int last = us.get_size() - 1;
							int count = us.get_size() / 2;
							for (int i = 0; i < count; ++i)
							{
								swap(us[i], us[last - i]);
							}
						}
					}

					// create transform matrix to place points in 3d
					//	x to sides (width, place layer at)
					//	y and z are in plane of each layer (in-layer's x&y, length&height), out of which y is from start to end along axis (if height, length not provided)
					if (useUp.is_zero())
					{
						// but first make sure we will have some fallback forward axis
						Vector3 diff = useEndsAt - useStartsAt;
						if (abs(diff.z) > abs(diff.y) &&
							abs(diff.z) > abs(diff.x))
						{
							useUp = Vector3::yAxis;
						}
						else
						{
							useUp = Vector3::zAxis;
						}
					}
					Matrix44 placePointsTransform = look_at_matrix(useStartsAt, useEndsAt, useUp);

					Range useWidth = Range(sub->min.get(_context), sub->max.get(_context));
					if (useWidth.min > useWidth.max)
					{
						swap(useWidth.min, useWidth.max);
					}
					useWidth *= widthMultiplier;

					int layerStartAt = ipu.get_point_count();
					int prevPointsStartAt = ipu.get_point_count();
					for (int i = 0; i <= 2; ++i)
					{
						int thisPointsStartAt = ipu.get_point_count();
						float x = i == 0 ? useWidth.min : useWidth.max;
						for_every(point, points)
						{
							Vector3 p3d = Vector3(x, point->x, point->y);
							ipu.add_point(placePointsTransform.location_to_world(p3d));
						}
						if (!sub->capsOnly &&
							i > 0 && useWidth.min != useWidth.max)
						{
							int thisIdx = thisPointsStartAt;
							int prevIdx = prevPointsStartAt;
							for_every(uvDef, us)
							{
								ipu.add_quad(uvDef->get_u(_context), prevIdx, prevIdx + 1, thisIdx + 1, thisIdx, layer->reverseSurfaces);
								++thisIdx;
								++prevIdx;
							}
						}
						prevPointsStartAt = thisPointsStartAt;
					}
			
					if (sub->capMin || sub->capMax)
					{
						// add sides
						int sideA = 1;
						int sideB = points.get_size() - 1;
						int pMin = layerStartAt;
						int pMax = layerStartAt + points.get_size();
						// go on each side and build triangle, assume that we have crosssection going clockwise
						// add initial triangle
						ipu.add_triangle(uAtMinValue, pMin, pMin + sideB, pMin + sideA, layer->reverseSurfaces);
						ipu.add_triangle(uAtMaxValue, pMax, pMax + sideA, pMax + sideB, layer->reverseSurfaces);
						bool moveA = true;
						while (sideA != sideB)
						{
							bool sideANextIsValid = true;
							bool sideBNextIsValid = true;
							// check if 'side A/B next' is valid
							// work it out at min width
							for (int checkSide = 0; checkSide < 2; ++checkSide)
							{
								ArrayStatic<int, 3> tri; SET_EXTRA_DEBUG_INFO(tri, TXT("AdvancedShapes::cross_section_along.tri"));
								if (checkSide == 0)
								{
									tri.push_back(sideA);
									tri.push_back(sideA + 1);
									tri.push_back(sideB);
								}
								else
								{
									tri.push_back(sideA);
									tri.push_back(sideB - 1);
									tri.push_back(sideB);
								}
								// build array of vectors pointing inside triangle from each vertex
								ArrayStatic<Vector2, 3> triVertex; SET_EXTRA_DEBUG_INFO(triVertex, TXT("AdvancedShapes::cross_section_along.triVertex"));
								ArrayStatic<Vector2, 3> triInto; SET_EXTRA_DEBUG_INFO(triInto, TXT("AdvancedShapes::cross_section_along.triInto"));
								for (int p = 0; p < 3; ++p)
								{
									triVertex.push_back(points[tri[p]]);
									triInto.push_back((points[tri[(p + 1) % 3]] - points[tri[p]]).rotated_right().normal());
								}
								// check if any other point would belong into new triangle
								for_every(point, points)
								{
									if (tri.does_contain(for_everys_index(point)))
									{
										// skip this point as it is part of tri
										continue;
									}
									int pInside = 0;
									for (int p = 0; p < 3; ++p)
									{
										if (Vector2::dot(*point - triVertex[p], triInto[p]) > MARGIN)
										{
											++pInside;
										}
									}
									if (pInside == 3)
									{
										// it is inside triangle, fail this path
										if (checkSide == 0)
										{
											sideANextIsValid = false;
										}
										else
										{
											sideBNextIsValid = false;
										}
										break;
									}
								}
							}
							an_assert(sideANextIsValid || sideBNextIsValid);
							// adjust moveA to test results
							if (!sideANextIsValid)
							{
								moveA = false;
							}
							if (!sideBNextIsValid)
							{
								moveA = true;
							}
							if (moveA)
							{
								if (sub->capMin)
								{
									ipu.add_triangle(uAtMinValue, pMin + sideA, pMin + sideB, pMin + sideA + 1, layer->reverseSurfaces);
								}
								if (sub->capMax)
								{
									ipu.add_triangle(uAtMaxValue, pMax + sideA, pMax + sideA + 1, pMax + sideB, layer->reverseSurfaces);
								}
								++sideA;
							}
							else
							{
								if (sub->capMin)
								{
									ipu.add_triangle(uAtMinValue, pMin + sideB, pMin + sideB - 1, pMin + sideA, layer->reverseSurfaces);
								}
								if (sub->capMax)
								{
									ipu.add_triangle(uAtMaxValue, pMax + sideB, pMax + sideA, pMax + sideB - 1, layer->reverseSurfaces);
								}
								--sideB;
							}
							// try to alter
							moveA = !moveA;
						}
					}
				}
			}
		}
	}
	
	//

	// prepare data for vertex and index formats, so further modifications may be applied and mesh can be imported
	ipu.prepare_data(_context.get_vertex_format(), _context.get_index_format(), vertexData, elementsData);

	int vertexCount = vertexData.get_size() / _context.get_vertex_format().get_stride();
	int elementsCount = elementsData.get_size() / _context.get_index_format().get_stride();

	//

	if (_context.does_require_movement_collision())
	{
		if (CrossSectionAlongData const * data = fast_cast<CrossSectionAlongData>(_data))
		{
			if (data->createMovementCollisionMesh.create)
			{
				_context.store_movement_collision_parts(create_collision_meshes_from(ipu, _context, _instance ,data->createMovementCollisionMesh));
			}
		}
	}

	if (_context.does_require_precise_collision())
	{
		if (CrossSectionAlongData const * data = fast_cast<CrossSectionAlongData>(_data))
		{
			if (data->createPreciseCollisionMesh.create)
			{
				_context.store_precise_collision_parts(create_collision_meshes_from(ipu, _context, _instance, data->createPreciseCollisionMesh));
			}
		}
	}

	if (_context.does_require_space_blockers())
	{
		if (CrossSectionAlongData const* data = fast_cast<CrossSectionAlongData>(_data))
		{
			if (data->createSpaceBlocker.create)
			{
				_context.store_space_blocker(create_space_blocker_from(ipu, _context, _instance, data->createSpaceBlocker));
			}
		}
	}

	//

	if (fast_cast<CrossSectionAlongData>(_data) && fast_cast<CrossSectionAlongData>(_data)->noMesh)
	{
		ipu.keep_polygons(0);
	}

	//

	if (!ipu.is_empty())
	{
		// create part
		Meshes::Mesh3DPart* part = _context.get_generated_mesh()->load_part_data(vertexData.get_data(), elementsData.get_data(), Meshes::Primitive::Triangle, vertexCount, elementsCount, _context.get_vertex_format(), _context.get_index_format());

#ifdef AN_DEVELOPMENT
		part->debugInfo = String::printf(TXT("cross section along @ %S"), _instance.get_element()->get_location_info().to_char());
#endif

		_context.store_part(part, _instance);

		if (CrossSectionAlongData const * data = fast_cast<CrossSectionAlongData>(_data))
		{
			if (data->ignoreForCollision)
			{
				_context.ignore_part_for_collision(part);
			}
		}
	}

	return true;
}

//

REGISTER_FOR_FAST_CAST(CrossSectionAlongData);

bool CrossSectionAlongData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	createMovementCollisionMesh.load_from_xml(_node, TXT("createMovementCollisionMesh"), _lc);
	createPreciseCollisionMesh.load_from_xml(_node, TXT("createPreciseCollisionMesh"), _lc);
	error_loading_xml_on_assert(!_node->first_child_named(TXT("createPreciseCollisionMeshSkinned")), result, _node, TXT("createPreciseCollisionMeshSkinned not handled"));
	createSpaceBlocker.load_from_xml(_node, TXT("createSpaceBlocker"));
	noMesh = _node->get_bool_attribute_or_from_child_presence(TXT("noMesh"), noMesh);
	ignoreForCollision = _node->get_bool_attribute_or_from_child_presence(TXT("ignoreForCollision"), ignoreForCollision);

	for_every(layersNode, _node->children_named(TXT("layers")))
	{
		for_every(node, layersNode->children_named(TXT("layer")))
		{
			layers.push_back(Layer());
			Layer& layer = layers.get_last();

			bool thisIsOk = true;

			thisIsOk &= layer.generationCondition.load_from_xml(node);
			thisIsOk &= layer.crossSection.load_from_xml(node, TXT("crossSectionSpline"), TXT("crossSectionSpline"), _lc);

			layer.reverseCrossSection = node->get_bool_attribute(TXT("reverseCrossSection"), layer.reverseCrossSection);
			layer.reverseSurfaces = node->get_bool_attribute(TXT("reverseSurfaces"), layer.reverseSurfaces);

			thisIsOk &= layer.crossSectionSubStep.load_from_xml_child_node(node, TXT("crossSectionSubStep"));
			for_every(atNode, node->children_named(TXT("at")))
			{
				layer.subs.push_back(Layer::Sub());
				Layer::Sub & sub = layer.subs.get_last();
				thisIsOk &= sub.generationCondition.load_from_xml(atNode);
				sub.placeAtOfLength.load_from_xml(atNode->get_attribute(TXT("placeAtOfLength")));
				sub.placeAtOfLength.load_from_xml(atNode->get_attribute(TXT("placeAt")));
				if (IO::XML::Attribute const * attr = atNode->get_attribute(TXT("width")))
				{
					Range width;
					width.load_from_string(attr->get_as_string());
					sub.min.set(width.min);
					sub.max.set(width.max);
				}
				sub.min.load_from_xml(atNode, TXT("min"));
				sub.max.load_from_xml(atNode, TXT("max"));
				sub.capsOnly = atNode->get_bool_attribute(TXT("capsOnly"), sub.capsOnly);
				if (IO::XML::Attribute const * attr = atNode->get_attribute(TXT("cap")))
				{
					if (attr->get_as_string() == TXT("both"))
					{
						sub.capMin = sub.capMax = true;
					}
					else if (attr->get_as_string() == TXT("none"))
					{
						sub.capMin = sub.capMax = false;
					}
					else if (attr->get_as_string() == TXT("min"))
					{
						sub.capMin = true;
						sub.capMax = false;
					}
					else if (attr->get_as_string() == TXT("max"))
					{
						sub.capMin = false;
						sub.capMax = true;
					}
					else
					{
						error_loading_xml(atNode, TXT("\"cap\" parameter \"%S\" not recognised"), attr->get_as_string().to_char());
						thisIsOk = false;
					}
				}
			}
			layer.uAtMin.load_from_xml(node);
			layer.uAtMax.load_from_xml(node);
			layer.uAtMin.load_from_xml(node, TXT("uAtMin"), TXT("uAtMinParam"));
			layer.uAtMax.load_from_xml(node, TXT("uAtMax"), TXT("uAtMaxParam"));
			if (thisIsOk)
			{
				// ok!
			}
			else
			{
				layers.pop_back();
				error_loading_xml(node, TXT("problem while loading layer"));
				result = false;
			}
		}
	}

	return result;
}

bool CrossSectionAlongData::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	for_every(layer, layers)
	{
		result &= layer->crossSection.prepare_for_game(_library, _pfgContext);
	}

	return result;
}
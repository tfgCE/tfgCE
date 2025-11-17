#include "meshGenAdvancedShape_edgesSurfaces.h"

#include "..\..\meshGenParamImpl.inl"
#include "..\..\meshGenValueDefImpl.inl"

#include "..\..\..\debugSettings.h"

#include "..\..\..\game\game.h"

#include "..\..\..\..\core\math\mathToString.h"
#include "..\..\..\..\core\other\parserUtils.h"
#include "..\..\..\..\core\other\parserUtilsImpl.inl"
#include "..\..\..\..\core\random\randomUtils.h"
#include "..\..\..\..\core\types\vectorPacked.h"
#include "..\..\..\..\core\wheresMyPoint\wmp_restore.h"

#include "..\..\..\..\core\debug\debugVisualiser.h"

#ifdef AN_CLANG
DEFINE_STATIC_NAME(zero);
#include "..\..\customData\meshGenSplineImpl.inl"
#include "..\..\customData\meshGenSplineRefImpl.inl"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

// all definitions and everything is in there

//#define LOG_USED_CSOCS
//#define LOG_CALCULATE_NORMAL
//#define LOG_POINTS_NUM

//#define ROUND_CORNER_DEBUG_VISUALISE

#ifdef ROUND_CORNER_DEBUG_VISUALISE
static int stopOnlyAt = 20080; // this is master control

// if any of those are hit, it stops
static int stopAt = 178000;
static int stopAtEvery = false;
static int stopAtEveryFirstIteration = false;
static int stopAtEveryLastIteration = true;
static int nowAt = -1;
#endif

//

// generation issues
DEFINE_STATIC_NAME(polygonNormalIssue);

//

bool AdvancedShapes::edges_and_surfaces(GenerationContext & _context, ElementInstance & _instance, ::Framework::MeshGeneration::ElementShapeData const * _data)
{
	bool result = true;

	::Meshes::Builders::IPU ipu;

	MeshGeneration::load_ipu_parameters(ipu, _context);

	//

	EdgesAndSurfacesData const * ensData = fast_cast<EdgesAndSurfacesData>(_data);

	EdgesAndSurfacesContext ensContext(_context, _instance, ensData, ipu);

	if (ensData)
	{
		if (ensData->prepare_for_generation(ensContext))
		{
			result &= ensData->process(ensContext);
		}
	}
	
	if (!ipu.is_empty())
	{
		Array<::Meshes::Builders::IPU> ipus;

		ipu.break_by_material_into(ipus);

		// get all ipus
		struct ByMaterialIPU
		{
			::Meshes::Builders::IPU* ipu;
			Optional<int> materialIdx;
			ByMaterialIPU() {}
			ByMaterialIPU(::Meshes::Builders::IPU* _ipu, Optional<int> const& _materialIdx) :ipu(_ipu), materialIdx(_materialIdx) {}
		};
		Array<ByMaterialIPU> allIpus;
		allIpus.push_back(ByMaterialIPU(&ipu, NP));
		for_every(ipu, ipus)
		{
			if (!ipu->is_empty())
			{
				allIpus.push_back(ByMaterialIPU(ipu, for_everys_index(ipu)));
			}
		}

		for_every(bmipu, allIpus)
		{
			auto* ipu = bmipu->ipu;
			if (ipu->is_empty())
			{
				continue;
			}

			Array<int8> elementsData;
			Array<int8> vertexData;

			//

			// prepare data for vertex and index formats, so further modifications may be applied and mesh can be imported
			ipu->prepare_data(_context.get_vertex_format(), _context.get_index_format(), vertexData, elementsData);

			int vertexCount = vertexData.get_size() / _context.get_vertex_format().get_stride();
			int elementsCount = elementsData.get_size() / _context.get_index_format().get_stride();

			//

			if (_context.does_require_movement_collision())
			{
				if (ensData && ensData->createMovementCollisionMesh.create)
				{
					_context.store_movement_collision_parts(create_collision_meshes_from(*ipu, _context, _instance, ensData->createMovementCollisionMesh));
				}
				if (ensData && ensData->createMovementCollisionBox.create)
				{
					_context.store_movement_collision_part(create_collision_box_from(*ipu, _context, _instance, ensData->createMovementCollisionBox));
				}
			}

			if (_context.does_require_precise_collision())
			{
				if (ensData && ensData->createPreciseCollisionMesh.create)
				{
					_context.store_precise_collision_parts(create_collision_meshes_from(*ipu, _context, _instance, ensData->createPreciseCollisionMesh));
				}
				if (ensData && ensData->createPreciseCollisionBox.create)
				{
					_context.store_precise_collision_part(create_collision_box_from(*ipu, _context, _instance, ensData->createPreciseCollisionBox));
				}
			}

			if (_context.does_require_space_blockers())
			{
				if (ensData && ensData->createSpaceBlocker.create)
				{
					_context.store_space_blocker(create_space_blocker_from(*ipu, _context, _instance, ensData->createSpaceBlocker));
				}
			}

			if (ensData && ensData->noMesh)
			{
				ipu->keep_polygons(0);
			}

			//

			// create part
			if (!ipu->is_empty())
			{

				Meshes::Mesh3DPart* part = _context.get_generated_mesh()->load_part_data(vertexData.get_data(), elementsData.get_data(), Meshes::Primitive::Triangle, vertexCount, elementsCount, _context.get_vertex_format(), _context.get_index_format());

#ifdef AN_DEVELOPMENT
				part->debugInfo = String::printf(TXT("edges and surfaces @ %S"), _instance.get_element()->get_location_info().to_char());
#endif

				_context.store_part(part, _instance, bmipu->materialIdx);

				if (ensData)
				{
					if (ensData->ignoreForCollision)
					{
						_context.ignore_part_for_collision(part);
					}
				}

#ifdef AN_DEVELOPMENT_OR_PROFILER
				if (_context.should_log_mesh_data())
				{
					LogInfoContext log;
					log.log(TXT("edges and surfaces output"));
					_context.get_generated_mesh()->log(log);
					log.output_to_output();
				}
#endif

			}
		}
	}

	// generate all sub elements
	result &= ensContext.generate_all_sub_elements(_context, _instance);

	// generate other elements
	if (ensData)
	{
		for_every_const(element, ensData->elements)
		{
			_context.access_random_generator().advance_seed(8235, 63508);
			result &= _context.process(element->get(), &_instance, for_everys_index(element));
		}
	}

#ifdef AN_HANDLE_GENERATION_ISSUES
	if (ipu.get_polygon_normal_issues() && Game::get())
	{
		Game::get()->add_generation_issue(NAME(polygonNormalIssue));
	}
#endif

	return result;
}

//

REGISTER_FOR_FAST_CAST(EdgesAndSurfacesContext);

EdgesAndSurfacesContext::EdgesAndSurfacesContext(GenerationContext & _generationContext, ElementInstance & _elementInstance, EdgesAndSurfacesData const * _edgesAndSurfaces, ::Meshes::Builders::IPU & _ipu)
: generationContext(_generationContext)
, elementInstance(_elementInstance)
, edgesAndSurfaces(_edgesAndSurfaces)
, ipu(_ipu)
{
}

EdgesAndSurfacesContext::~EdgesAndSurfacesContext()
{
	for_every_ptr(generateSubElement, generateSubElements)
	{
		delete generateSubElement;
	}
}

bool EdgesAndSurfacesContext::get_point_for_wheres_my_point(Name const & _byName, OUT_ Vector3 & _point) const
{
	if (Node const * node = edgesAndSurfaces->find_node(_byName))
	{
		_point = node->location;
		return true;
	}
	else
	{
		return false;
	}
}

bool EdgesAndSurfacesContext::store_value_for_wheres_my_point(Name const & _byName, WheresMyPoint::Value const & _value)
{
	return elementInstance.access_context().store_value_for_wheres_my_point(_byName, _value);
}

bool EdgesAndSurfacesContext::restore_value_for_wheres_my_point(Name const & _byName, OUT_ WheresMyPoint::Value & _value) const
{
	return elementInstance.access_context().restore_value_for_wheres_my_point(_byName, OUT_ _value);
}

bool EdgesAndSurfacesContext::store_convert_value_for_wheres_my_point(Name const& _byName, TypeId _to)
{
	return elementInstance.access_context().store_convert_value_for_wheres_my_point(_byName, _to);
}

bool EdgesAndSurfacesContext::rename_value_forwheres_my_point(Name const& _from, Name const& _to)
{
	return elementInstance.access_context().rename_value_forwheres_my_point(_from, _to);
}

bool EdgesAndSurfacesContext::store_global_value_for_wheres_my_point(Name const & _byName, WheresMyPoint::Value const & _value)
{
	return elementInstance.access_context().store_global_value_for_wheres_my_point(_byName, _value);
}

bool EdgesAndSurfacesContext::restore_global_value_for_wheres_my_point(Name const & _byName, OUT_ WheresMyPoint::Value & _value) const
{
	return elementInstance.access_context().restore_global_value_for_wheres_my_point(_byName, OUT_ _value);
}

bool EdgesAndSurfacesContext::generate_all_sub_elements(GenerationContext & _context, ElementInstance & _instance)
{
	bool result = true;
	for_every_const_ptr(generateSubElement, generateSubElements)
	{
		_context.access_random_generator().advance_seed(9436, 23562);

		Checkpoint::generate_with_checkpoint(_context, _instance, for_everys_index(generateSubElement), generateSubElement->element, &generateSubElement->parameters, generateSubElement->placement, generateSubElement->scale);
	}

	return result;
}

//

REGISTER_FOR_FAST_CAST(EdgesAndSurfacesData);

EdgesAndSurfacesData::EdgesAndSurfacesData()
{
}

EdgesAndSurfacesData::~EdgesAndSurfacesData()
{
	for_every(node, nodes)
	{
		delete_and_clear(*node);
	}
	for_every(edge, edges)
	{
		delete_and_clear(*edge);
	}
	for_every(surface, surfaces)
	{
		delete_and_clear(*surface);
	}
	for_every(edgeMesh, edgeMeshes)
	{
		delete_and_clear(*edgeMesh);
	}
	for_every(surfaceMesh, surfaceMeshes)
	{
		delete_and_clear(*surfaceMesh);
	}
	for_every(placeOnEdge, placeOnEdges)
	{
		delete_and_clear(*placeOnEdge);
	}
	for_every(polygonSet, polygons)
	{
		delete_and_clear(*polygonSet);
	}
}

template <typename Class>
struct LoadingTools
{
	static bool load_multiple_with_id_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc, Array<Class*> & _container,
												tchar const * const _nodeContainerName, tchar const * const _nodeName,
												std::function<bool(IO::XML::Node const * _node)> _onNode = nullptr,
												std::function<void(Class* _object)> _onCreation = nullptr)
	{
		bool result = true;
		for_every(containerNode, _node->children_named(_nodeContainerName))
		{
			for_every(node, containerNode->all_children())
			{
				if (_onNode)
				{
					if (_onNode(node))
					{
						continue;
					}
				}
				if (node->get_name() == _nodeName)
				{
					Class* newObject = new Class();
					if (_onCreation)
					{
						_onCreation(newObject);
					}
					if (newObject->load_from_xml(node, _lc))
					{
						bool found = false;
						for_every_const_ptr(object, _container)
						{
							if (object->id == newObject->id)
							{
								error_loading_xml(node, TXT("two or more %S with id \"%S\" exist"), _nodeContainerName, newObject->id.to_char());
								found = true;
								result = false;
								break;
							}
						}
						if (!found)
						{
							_container.push_back(newObject);
						}
					}
				}
			}
		}
		return result;
	}

	static bool load_multiple_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc, Array<Class*> & _container, tchar const * const _nodeContainerName, tchar const * const _nodeName)
	{
		bool result = true;
		for_every(containerNode, _node->children_named(_nodeContainerName))
		{
			for_every(node, containerNode->children_named(_nodeName))
			{
				Class* newObject = new Class();
				if (newObject->load_from_xml(node, _lc))
				{
					_container.push_back(newObject);
				}
			}
		}
		return result;
	}
};

bool EdgesAndSurfacesData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	createMovementCollisionMesh.load_from_xml(_node, TXT("createMovementCollisionMesh"), _lc);
	createMovementCollisionBox.load_from_xml(_node, TXT("createMovementCollisionBox"), _lc);
	createPreciseCollisionMesh.load_from_xml(_node, TXT("createPreciseCollisionMesh"), _lc);
	createPreciseCollisionBox.load_from_xml(_node, TXT("createPreciseCollisionBox"), _lc);
	error_loading_xml_on_assert(!_node->first_child_named(TXT("createPreciseCollisionMeshSkinned")), result, _node, TXT("createPreciseCollisionMeshSkinned not handled"));
	createSpaceBlocker.load_from_xml(_node, TXT("createSpaceBlocker"));
	noMesh = _node->get_bool_attribute_or_from_child_presence(TXT("noMesh"), noMesh);
	ignoreForCollision = _node->get_bool_attribute_or_from_child_presence(TXT("ignoreForCollision"), ignoreForCollision);

	debugDrawNodes = _node->get_bool_attribute_or_from_child_presence(TXT("debugDrawNodes"), debugDrawNodes);
	debugDrawEdges = _node->get_bool_attribute_or_from_child_presence(TXT("debugDrawEdges"), debugDrawEdges);
	debugDrawSurfaces = _node->get_bool_attribute_or_from_child_presence(TXT("debugDrawSurfaces"), debugDrawSurfaces);

	debugDrawNodesTextScale = _node->get_float_attribute_or_from_child(TXT("debugDrawNodesTextScale"), debugDrawNodesTextScale);
	if (auto* n = _node->first_child_named(TXT("debugDrawNodes")))
	{
		debugDrawNodesTextScale = n->get_float_attribute(TXT("textScale"), debugDrawNodesTextScale);
	}
	debugDrawEdgesTextScale = _node->get_float_attribute_or_from_child(TXT("debugDrawEdgesTextScale"), debugDrawEdgesTextScale);
	if (auto* n = _node->first_child_named(TXT("debugDrawEdges")))
	{
		debugDrawEdgesTextScale = n->get_float_attribute(TXT("textScale"), debugDrawEdgesTextScale);
	}
	debugDrawSurfacesTextScale = _node->get_float_attribute_or_from_child(TXT("debugDrawSurfacesTextScale"), debugDrawSurfacesTextScale);
	if (auto* n = _node->first_child_named(TXT("debugDrawSurfaces")))
	{
		debugDrawSurfacesTextScale = n->get_float_attribute(TXT("textScale"), debugDrawSurfacesTextScale);
	}

	result &= edgeRefs.load_from_xml(_node->first_child_named(TXT("edgeRefs")), TXT("edgeRef"), TXT("edgeCustomData"), TXT("edge"), _lc);

	{
		NodeDef nodeDef;
		result &= LoadingTools<Node>::load_multiple_with_id_from_xml(_node, _lc, nodes, TXT("nodes"), TXT("node"),
			[&nodeDef](IO::XML::Node const * _node)
			{
				if (_node->get_name() != TXT("node"))
				{
					nodeDef.sub_load_from_xml(_node);
					return true;
				}
				return false;
			},
			[&nodeDef](Node* _node)
			{
				_node->nodeDef = nodeDef;
			}
			);
	}
	{
		EdgeDef edgeDef;
		result &= LoadingTools<Edge>::load_multiple_with_id_from_xml(_node, _lc, edges, TXT("edges"), TXT("edge"),
			[&edgeDef](IO::XML::Node const * _node)
			{
				if (_node->get_name() != TXT("edge"))
				{
					edgeDef.sub_load_from_xml(_node);
					return true;
				}
				else
				{
					return false;
				}
			},
			[&edgeDef](Edge* _edge)
			{
				_edge->edgeDef = edgeDef;
			}
			);
	}
	{
		SurfaceDef surfaceDef;

		result &= LoadingTools<Surface>::load_multiple_with_id_from_xml(_node, _lc, surfaces, TXT("surfaces"), TXT("surface"),
			[&surfaceDef](IO::XML::Node const * _node)
			{
				if (_node->get_name() != TXT("surface"))
				{
					surfaceDef.sub_load_from_xml(_node);
					return true;
				}
				return false;
			},
			[&surfaceDef](Surface* _surface)
			{
				_surface->surfaceDef = surfaceDef;
			}
			);
	}
	result &= LoadingTools<EdgeMesh>::load_multiple_from_xml(_node, _lc, edgeMeshes, TXT("generateEdgeMeshes"), TXT("edgeMesh"));
	result &= LoadingTools<SurfaceMesh>::load_multiple_from_xml(_node, _lc, surfaceMeshes, TXT("generateSurfaceMeshes"), TXT("surfaceMesh"));
	result &= LoadingTools<EdgePlaceOnEdge>::load_multiple_from_xml(_node, _lc, placeOnEdges, TXT("placeOnEdges"), TXT("placeOnEdge"));
	result &= LoadingTools<Polygons>::load_multiple_from_xml(_node, _lc, polygons, TXT("generatePolygons"), TXT("polygons"));

	for_every(nodeContainer, _node->children_named(TXT("elements")))
	{
		for_every(node, nodeContainer->all_children())
		{
			if (Element* element = Element::create_from_xml(node, _lc))
			{
				if (element->load_from_xml(node, _lc))
				{
					elements.push_back(RefCountObjectPtr<Element>(element));
				}
				else
				{
					error_loading_xml(node, TXT("problem loading one element for edges & surfaces"));
					result = false;
				}
			}
		}
	}

	return result;
}

bool EdgesAndSurfacesData::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	result &= edgeRefs.prepare_for_game(_library, _pfgContext);

	for_every_ptr(edgeMesh, edgeMeshes)
	{
		result &= edgeMesh->prepare_for_game(_library, _pfgContext);
	}

	for_every_ptr(placeOnEdge, placeOnEdges)
	{
		result &= placeOnEdge->prepare_for_game(_library, _pfgContext);
	}

	for_every(element, elements)
	{
		result &= element->get()->prepare_for_game(_library, _pfgContext);
	}

	return result;
}

#ifdef AN_DEVELOPMENT
void EdgesAndSurfacesData::for_included_mesh_generators(std::function<void(MeshGenerator*)> _do) const
{
	for_every(element, elements)
	{
		element->get()->for_included_mesh_generators(_do);
	}
}
#endif


struct GatheredEdge
{
	Edge const * edge = nullptr;
	BezierCurve<Vector3> curveNotExtruded;
	BezierCurve<Vector3> curve;
	BezierCurve<Vector3> curveExtrusionFill;
	bool doExtrusionFill = false;
	bool useNotExtrudedCurveForSubStep = false;

	bool adjust_extrusion(EdgesAndSurfacesContext & _context, Node const * node_p0, Node const * node_p3, SurfaceMesh const * _surfaceMesh, OPTIONAL_ OUT_ float * _surfaceMeshExtrusion = nullptr, OPTIONAL_ Vector3 const & _useExtrusionNormal = Vector3::zero)
	{
		an_assert(node_p0 && node_p3);
		// adjust extrusion
		float surfaceMeshExtrusion = _surfaceMesh->extrusion.is_set() ? _surfaceMesh->extrusion.get(_context.access_element_instance().get_context()) : 0.0f;
		Optional<Vector3> surfaceMeshExtrusionTowards;
		if (_surfaceMesh->extrusionTowards.is_set())
		{
			surfaceMeshExtrusionTowards = _surfaceMesh->extrusionTowards.get(_context.access_element_instance().get_context());
		}
		Optional<Vector3> surfaceMeshExtrusionNormal;
		if (_surfaceMesh->extrusionNormal.is_set())
		{
			surfaceMeshExtrusionNormal = _surfaceMesh->extrusionNormal.get(_context.access_element_instance().get_context());
		}
		float surfaceMeshExtrusionFill = _surfaceMesh->extrusionFill.is_set() ? _surfaceMesh->extrusionFill.get(_context.access_element_instance().get_context()) : 0.0f;
		Vector3 extrude0 = Vector3::zero;
		Vector3 extrude3 = Vector3::zero;
		if ((surfaceMeshExtrusion != 0.0f || (surfaceMeshExtrusionFill != 0.0f && doExtrusionFill)) &&
			node_p0 && node_p0->has_extrude_normal(_surfaceMesh->extrusionSurfaceGroup) &&
			node_p3 && node_p3->has_extrude_normal(_surfaceMesh->extrusionSurfaceGroup))
		{
			if (surfaceMeshExtrusionTowards.is_set())
			{
				extrude0 = (surfaceMeshExtrusionTowards.get() - curve.p0).normal();
				extrude3 = (surfaceMeshExtrusionTowards.get() - curve.p3).normal();
			}
			else if (surfaceMeshExtrusionNormal.is_set())
			{
				extrude3 = extrude0 = (surfaceMeshExtrusionNormal.get()).normal();
			}
			else
			{
				extrude0 = *(node_p0->find_extrude_normal(_context.access_element_instance(), _surfaceMesh->extrusionSurfaceGroup));
				extrude3 = *(node_p3->find_extrude_normal(_context.access_element_instance(), _surfaceMesh->extrusionSurfaceGroup));
			}
		}
		else if (!_useExtrusionNormal.is_zero())
		{
			extrude0 = _useExtrusionNormal;
			extrude3 = _useExtrusionNormal;
		}

		curveNotExtruded = curve;
		curveExtrusionFill = curve;
		if (! extrude0.is_zero() && ! extrude3.is_zero())
		{
			// calculate 1 and 2
			float const third = 1.0f / 3.0f;
			float const rest = 1.0f - third;
			Vector3 const extrude1 = (extrude0 * rest + extrude3 * third).normal();
			Vector3 const extrude2 = (extrude0 * third + extrude3 * rest).normal();
			curve.p0 += extrude0 * surfaceMeshExtrusion;
			curve.p1 += extrude1 * surfaceMeshExtrusion;
			curve.p2 += extrude2 * surfaceMeshExtrusion;
			curve.p3 += extrude3 * surfaceMeshExtrusion;
			curveExtrusionFill.p0 += extrude0 * surfaceMeshExtrusionFill;
			curveExtrusionFill.p1 += extrude1 * surfaceMeshExtrusionFill;
			curveExtrusionFill.p2 += extrude2 * surfaceMeshExtrusionFill;
			curveExtrusionFill.p3 += extrude3 * surfaceMeshExtrusionFill;
			assign_optional_out_param(_surfaceMeshExtrusion, surfaceMeshExtrusion);
			return true;
		}
		assign_optional_out_param(_surfaceMeshExtrusion, 0.0f);
		return false;
	}

	static void setup_triangle(OUT_ GatheredEdge & gatheredEdge0, GatheredEdge & gatheredEdge, OUT_ GatheredEdge & gatheredEdge1, Vector3 const & extrudedCentre, bool reversed, REF_ ArrayStatic<GatheredEdge, 4> & gatheredEdges)
	{
		// add to centre
		gatheredEdge0.curve.p0 = extrudedCentre;
		gatheredEdge0.curve.p3 = gatheredEdge.curve.p0;
		gatheredEdge0.curve.make_linear();
		gatheredEdge1.curve.p0 = gatheredEdge.curve.p3;
		gatheredEdge1.curve.p3 = extrudedCentre;
		gatheredEdge1.curve.make_linear();

		// order it properly
		if (reversed)
		{
			gatheredEdge.curve = gatheredEdge.curve.reversed(true);
			gatheredEdge0.curve = gatheredEdge0.curve.reversed(true);
			gatheredEdge1.curve = gatheredEdge1.curve.reversed(true);
			gatheredEdges.push_back(gatheredEdge1);
			gatheredEdges.push_back(gatheredEdge);
			gatheredEdges.push_back(gatheredEdge0);
		}
		else
		{
			gatheredEdges.push_back(gatheredEdge0);
			gatheredEdges.push_back(gatheredEdge);
			gatheredEdges.push_back(gatheredEdge1);
		}
	}
};

bool EdgesAndSurfacesData::process(EdgesAndSurfacesContext & _context) const
{
	process_edge_meshes(_context);
	process_surface_meshes(_context);
	process_place_mesh_on_edges(_context);
	process_polygons(_context);

	todo_note(TXT("give proper result"));
	return true;
}

void EdgesAndSurfacesData::process_edge_meshes(EdgesAndSurfacesContext & _context) const
{
#ifdef AN_DEVELOPMENT
	for_every_const_ptr(node, nodes)
	{
		if (node->debugDraw || debugDrawNodes)
		{
			if (auto * drf = _context.access_generation_context().debugRendererFrame)
			{
				drf->add_sphere(true, true, Colour::purple, 0.3f, Sphere(node->location, 0.02f));
				if (node->normal.is_set())
				{
					drf->add_arrow(true, Colour::purple, node->location, node->location + node->normal.get());
				}
				drf->add_text(true, Colour::purple.blended_to(Colour::white, 0.5f), node->location, NP, true, 0.1f * debugDrawNodesTextScale, NP, node->id.to_char());
			}
		}
	}
	for_every_const_ptr(edge, edges)
	{
		if (edge->debugDraw || debugDrawEdges)
		{
			if (auto * drf = _context.access_generation_context().debugRendererFrame)
			{
				float length = edge->curve.length();
				if (length != 0.0f)
				{
					float const invLength = 1.0f / length;
					float const step = min(0.1f, length / 20.0f);
					Vector3 prev = edge->curve.calculate_at(0.0f);
					for (float t = step; t < length; t += step)
					{
						Vector3 curr = edge->curve.calculate_at(t * invLength);

						drf->add_line(true, Colour::orange, prev, curr);

						prev = curr;
					}
					Vector3 last = edge->curve.calculate_at(1.0f);
					drf->add_arrow(true, Colour::orange, prev, last, length * 0.05f);
					drf->add_text(true, Colour::orange.blended_to(Colour::white, 0.5f), edge->curve.calculate_at(0.5f), NP, true, 0.1f * debugDrawEdgesTextScale, NP, edge->id.to_char());
				}
			}
		}
	}
#endif
#ifdef LOG_POINTS_NUM
	output(TXT("[points num] : %04i : start"), _context.access_ipu().get_point_count());
	output(TXT("[points num] : edge meshes : %i"), edgeMeshes.get_size());
	for_every_const_ptr(edgeMesh, edgeMeshes)
	{
		output(TXT("[points num] :      :  %02i %S"), for_everys_index(edgeMesh), edgeMesh->id.to_char());
	}
#endif
	for_every_const_ptr(edgeMesh, edgeMeshes)
	{
		if (edgeMesh->storeCrossSectionVertexCountInVar.is_valid())
		{
			if (!edgeMesh->crossSections.is_empty())
			{
				_context.access_generation_context().set_parameter<int>(edgeMesh->storeCrossSectionVertexCountInVar, edgeMesh->crossSections.get_first().points.get_size());
			}
		}

#ifdef LOG_POINTS_NUM
		output(TXT("[points num] : %04i : next edge mesh"), _context.access_ipu().get_point_count());
#endif
		if (! edgeMesh->generationCondition.check(_context.access_element_instance()))
		{
			continue;
		}

		if (edgeMesh->useEdges.is_empty())
		{
			continue;
		}

		if (!edgeMesh->wmpToolSet.is_empty())
		{
			int idx = for_everys_index(edgeMesh);
			WheresMyPoint::Context wmpContext(&_context); // use this context as we will be accessing points!
			wmpContext.set_random_generator(_context.access_element_instance().access_context().access_random_generator());
			wmpContext.access_random_generator().advance_seed(idx, idx);
			edgeMesh->wmpToolSet.update(wmpContext);
		}

		int polygonsSoFar = _context.access_ipu().get_polygon_count();
		int pointsSoFar = _context.access_ipu().get_point_count();

		Vector2 const edgeMeshScale = Vector2::one; // cross sections are already scaled!
		Vector2 const edgeMeshOffset = edgeMesh->offsetCrossSection.get(_context.access_element_instance().get_context());

		// build from cross sections (and store on curve
		if (edgeMesh->cornersOnly)
		{
			if (edgeMesh->createMovementCollisionBoxes.create ||
				edgeMesh->createPreciseCollisionBoxes.create)
			{
				warn_generating_mesh(_context.access_element_instance(), TXT("generation of boxes on cornersOnly is not implemented"));
			}
			int firstPointsStartAt = NONE;
			int prevPointsStartAt = NONE;
			ShapeUtils::CrossSection const* firstCrossSection = nullptr;
			for_every_const_ptr(edge, edgeMesh->useEdges)
			{
#ifdef LOG_USED_CSOCS
				output(TXT("  +- edge \"%S\""), edge->edgeRef.id.to_char());
#endif
				if (edgeMesh->hasCrossSection)
				{
					//auto* useEdgeRef = edgeMesh->useEdgeRef->get(_context.access_generation_context());
					{
						Vector2 useCornerScale = edgeMeshScale;
						useCornerScale *= edge->cornerScaleFloat.get_with_default(_context.access_element_instance().get_context(), 1.0f);
						useCornerScale *= edge->cornerScale.get_with_default(_context.access_element_instance().get_context(), Vector2::one);

						Meshes::VertexSkinningInfo skinningInfo = Meshes::VertexSkinningInfo::blend(edge->calculate_skinning_info_for(edge->betweenCorners.max, edgeMesh, _context), edge->calculate_skinning_info_for(edge->betweenCorners.min, edgeMesh, _context), 0.5f);

						ShapeUtils::CrossSection const& crossSection = ShapeUtils::get_cross_section(edgeMesh->crossSections, ShapeUtils::get_alt_cross_section_id(edgeMesh->crossSections, edge->cornerCrossSectionId, edge->cornerCrossSectionAltIds));
#ifdef LOG_USED_CSOCS
						output(TXT("    add single cross section (corners only) %S"), crossSection.id.to_char());
#endif

						// sharp corners we use are only provided between edges, we need to use curves' start and end
						if (!edgeMesh->loopedEdgeChain &&
							prevPointsStartAt == NONE)
						{
							float t = 0.0f;
							Vector3 loc = edge->curve.calculate_at(t);
							Vector3 tangent = edge->curve.calculate_tangent_at(t);
							Vector3 normal = edge->calculate_normal_for(edge->curve, t, tangent, edgeMesh, _context);
							Vector3 right = Vector3::cross(tangent, normal).normal();
							prevPointsStartAt = ShapeUtils::add_single_cross_section_points(_context.access_ipu(), _context.access_generation_context(), edgeMesh->debugDraw, crossSection, /*edge->sharpCorner.scaleCrossSection * ignore for first */useCornerScale, edgeMeshOffset,
								loc, tangent, normal, right, &skinningInfo);
						}

						int currPointsStartAt;
						if (!edgeMesh->loopedEdgeChain &&
							for_everys_index(edge) == edgeMesh->useEdges.get_size() - 1)
						{
							float t = 1.0f;
							Vector3 loc = edge->curve.calculate_at(t);
							Vector3 tangent = edge->curve.calculate_tangent_at(t);
							Vector3 normal = edge->calculate_normal_for(edge->curve, t, tangent, edgeMesh, _context);
							Vector3 right = Vector3::cross(tangent, normal).normal();
							currPointsStartAt = ShapeUtils::add_single_cross_section_points(_context.access_ipu(), _context.access_generation_context(), edgeMesh->debugDraw, crossSection, edge->sharpCorner.scaleCrossSection * useCornerScale, edgeMeshOffset,
								loc, tangent, normal, right, &skinningInfo);
						}
						else
						{
							currPointsStartAt = ShapeUtils::add_single_cross_section_points(_context.access_ipu(), _context.access_generation_context(), edgeMesh->debugDraw, crossSection, edge->sharpCorner.scaleCrossSection * useCornerScale, edgeMeshOffset, edge->sharpCorner.location, edge->sharpCorner.tangent, edge->sharpCorner.normal, edge->sharpCorner.right, &skinningInfo);
						}

						// connect with previous
						if (prevPointsStartAt != NONE)
						{
							int prevPointIdx = prevPointsStartAt;
							int currPointIdx = currPointsStartAt;
							for_every_const(uvDef, crossSection.us)
							{
								float uValue = uvDef->get_u(_context.access_element_instance().get_context());
								_context.access_ipu().add_quad(uValue, prevPointIdx, currPointIdx, currPointIdx + 1, prevPointIdx + 1);
								++prevPointIdx;
								++currPointIdx;
							}
						}

						if (!firstCrossSection && edgeMesh->loopedEdgeChain)
						{
							firstCrossSection = &ShapeUtils::get_cross_section(edgeMesh->crossSections, ShapeUtils::get_alt_cross_section_id(edgeMesh->crossSections, edge->cornerCrossSectionId, edge->cornerCrossSectionAltIds));
						}
						if (firstPointsStartAt == NONE)
						{
							firstPointsStartAt = currPointsStartAt;
						}
						prevPointsStartAt = currPointsStartAt;
					}
				}
			}
			if (edgeMesh->loopedEdgeChain &&
				firstCrossSection &&
				prevPointsStartAt != NONE && firstPointsStartAt != NONE &&
				prevPointsStartAt != firstPointsStartAt)
			{
				// connect to have chain
				int prevPointIdx = prevPointsStartAt;
				int currPointIdx = firstPointsStartAt;
				for_every_const(uvDef, firstCrossSection->us)
				{
					float uValue = uvDef->get_u(_context.access_element_instance().get_context());
					_context.access_ipu().add_quad(uValue, prevPointIdx, currPointIdx, currPointIdx + 1, prevPointIdx + 1);
					++prevPointIdx;
					++currPointIdx;
				}
			}
		}
		else
		{
#ifdef LOG_USED_CSOCS
			output(TXT(" edges and surfaces add cross sections"));
#endif
			EdgeMeshEdge const* prev = edgeMesh->loopedEdgeChain ? edgeMesh->useEdges.get_last() : nullptr;
			ShapeUtils::CrossSection const* lastCrossSection = nullptr; // not for looped? this is used to fix holes - if it's still broken, we need a pass both ways
			int edgeMeshPointsStartAt = _context.access_ipu().get_point_count(); // to avoid welding with completely different meshes
			struct WeldInfo
			{
				int pointsAt = 0;
				int pointCount = 0;
				WeldInfo() {}
				WeldInfo(int _pointsAt, int _pointCount) : pointsAt(_pointsAt), pointCount(_pointCount) {}
			};
			Array<WeldInfo> weldSections;
			Optional<int> pointCountPrevEdge;
			for_every_const_ptr(edge, edgeMesh->useEdges)
			{
#ifdef LOG_POINTS_NUM
				output(TXT("[points num] : %04i : edge \"%S\""), _context.access_ipu().get_point_count(), edge->edgeRef.id.to_char());
#endif
#ifdef LOG_USED_CSOCS
				output(TXT("  +-edge %S"), edge->edgeRef.id.to_char());
#endif
				if (edge->curve.is_degenerated())
				{
					// don't update prev
					continue;
				}

				Optional<int> pointCountCorner;
				Optional<int> pointCountEdge;
				if (edgeMesh->hasCrossSection)
				{
#ifdef LOG_POINTS_NUM
					output(TXT("[points num] : %04i : edge mesh has cross section"), _context.access_ipu().get_point_count());
#endif
					auto* useEdgeRef = edgeMesh->useEdgeRef->get(_context.access_generation_context());

					int preCornerPointsStartAt = _context.access_ipu().get_point_count();
					// build corners first
					if (prev && ! prev->curve.is_degenerated() && ! edge->curve.is_degenerated())
					{
#ifdef LOG_POINTS_NUM
						output(TXT("[points num] : %04i : build a corner"), _context.access_ipu().get_point_count());
#endif
						if (prev->cornerType == EdgeMeshCorner::Sharp ||
							prev->cornerType == EdgeMeshCorner::Cut)
						{
#ifdef LOG_POINTS_NUM
							output(TXT("[points num] : %04i : corner sharp or cut"), _context.access_ipu().get_point_count());
#endif
							Vector2 useCornerScale = edgeMeshScale;
							useCornerScale *= prev->cornerScaleFloat.get_with_default(_context.access_element_instance().get_context(), 1.0f);
							useCornerScale *= prev->cornerScale.get_with_default(_context.access_element_instance().get_context(), Vector2::one);

							Matrix44 prevTransform;
							ShapeUtils::CrossSection const& crossSection = lastCrossSection? *lastCrossSection : ShapeUtils::get_cross_section(edgeMesh->crossSections, ShapeUtils::get_alt_cross_section_id(edgeMesh->crossSections, prev->cornerCrossSectionId, prev->cornerCrossSectionAltIds));
							pointCountCorner.set_if_not_set(crossSection.points.get_size());
#ifdef LOG_POINTS_NUM
							output(TXT("[points num] : %04i : pointCountCorner : %i"), _context.access_ipu().get_point_count(), pointCountCorner.get());
#endif
#ifdef LOG_USED_CSOCS
							output(TXT("    add single cross section (corner first) %S"), crossSection.id.to_char());
#endif
							int prevPointsStartAt = ShapeUtils::add_single_cross_section_points(_context.access_ipu(), _context.access_generation_context(), edgeMesh->debugDraw, crossSection,
								useCornerScale, edgeMeshOffset,
								prev->curve, prev->betweenCorners.max, true,
								[prev, edgeMesh, &_context](BezierCurve<Vector3> const& _curve, float _t, Vector3 const& _tangent) { return prev->calculate_normal_for(_curve, _t, _tangent, edgeMesh, _context); },
								[prev, edgeMesh, &_context](BezierCurve<Vector3> const& _curve, float _t) { return prev->calculate_skinning_info_for(_curve, _t, edgeMesh, _context); },
								&prevTransform
							);

							int middlePointsStartAt = 0;
							Matrix44 middleTransform;
							if (prev->cornerType == EdgeMeshCorner::Sharp)
							{
								Meshes::VertexSkinningInfo skinningInfo = Meshes::VertexSkinningInfo::blend(prev->calculate_skinning_info_for(prev->betweenCorners.max, edgeMesh, _context), edge->calculate_skinning_info_for(edge->betweenCorners.min, edgeMesh, _context), 0.5f);
#ifdef LOG_USED_CSOCS
								output(TXT("    add single cross section (corner mid) %S"), crossSection.id.to_char());
#endif
								middlePointsStartAt = ShapeUtils::add_single_cross_section_points(_context.access_ipu(), _context.access_generation_context(), edgeMesh->debugDraw, crossSection,
									prev->sharpCorner.scaleCrossSection * useCornerScale, edgeMeshOffset,
									prev->sharpCorner.location, prev->sharpCorner.tangent, prev->sharpCorner.normal, prev->sharpCorner.right, &skinningInfo, &middleTransform);
							}

							Matrix44 edgeTransform;
#ifdef LOG_USED_CSOCS
							output(TXT("    add single cross section (corner last) %S"), crossSection.id.to_char());
#endif
							int edgePointsStartAt = ShapeUtils::add_single_cross_section_points(_context.access_ipu(), _context.access_generation_context(), edgeMesh->debugDraw, crossSection,
								useCornerScale, edgeMeshOffset,
								edge->curve, edge->betweenCorners.min, true,
								[edge, edgeMesh, &_context](BezierCurve<Vector3> const& _curve, float _t, Vector3 const& _tangent) { return edge->calculate_normal_for(_curve, _t, _tangent, edgeMesh, _context); },
								[edge, edgeMesh, &_context](BezierCurve<Vector3> const& _curve, float _t) { return edge->calculate_skinning_info_for(_curve, _t, edgeMesh, _context); },
								&edgeTransform
							);

#ifdef AN_DEVELOPMENT
							if (edgeMesh->debugDraw)
							{
								if (auto* drf = _context.access_generation_context().debugRendererFrame)
								{
									Vector3 p = prev->curve.calculate_at(prev->betweenCorners.max);
									Vector3 m = prev->cornerType == EdgeMeshCorner::Sharp ? prev->sharpCorner.location : p;
									Vector3 e = edge->curve.calculate_at(edge->betweenCorners.min);
									drf->add_line(true, Colour::red, p, m);
									drf->add_line(true, Colour::red, m, e);
								}
							}
#endif
							todo_note(TXT("turn it into a function"));
							if (prev->cornerType == EdgeMeshCorner::Sharp)
							{
								{
									int prevPointIdx = prevPointsStartAt;
									int middlePointIdx = middlePointsStartAt;
									int polygonsSoFar = _context.access_ipu().get_polygon_count();
									for_every_const(uvDef, crossSection.us)
									{
										float uValue = uvDef->get_u(_context.access_element_instance().get_context());
										_context.access_ipu().add_quad(uValue, prevPointIdx, middlePointIdx, middlePointIdx + 1, prevPointIdx + 1);
										++prevPointIdx;
										++middlePointIdx;
									}
									if (middleTransform.get_translation() != prevTransform.get_translation())
									{
										if (edgeMesh->createMovementCollisionBoxes.create)
										{
											Matrix44 useTransform = look_at_matrix(prevTransform.get_translation(), middleTransform.get_translation(), prevTransform.get_z_axis());
											_context.access_generation_context().store_movement_collision_part(create_collision_box_at(useTransform, _context.access_ipu(), _context.access_generation_context(), _context.access_element_instance(), edgeMesh->createMovementCollisionBoxes, polygonsSoFar));
										}
										if (edgeMesh->createPreciseCollisionBoxes.create)
										{
											Matrix44 useTransform = look_at_matrix(prevTransform.get_translation(), middleTransform.get_translation(), prevTransform.get_z_axis());
											_context.access_generation_context().store_precise_collision_part(create_collision_box_at(useTransform, _context.access_ipu(), _context.access_generation_context(), _context.access_element_instance(), edgeMesh->createPreciseCollisionBoxes, polygonsSoFar));
										}
									}
								}
								{
									int middlePointIdx = middlePointsStartAt;
									int edgePointIdx = edgePointsStartAt;
									int polygonsSoFar = _context.access_ipu().get_polygon_count();
									for_every_const(uvDef, crossSection.us)
									{
										float uValue = uvDef->get_u(_context.access_element_instance().get_context());
										_context.access_ipu().add_quad(uValue, middlePointIdx, edgePointIdx, edgePointIdx + 1, middlePointIdx + 1);
										++middlePointIdx;
										++edgePointIdx;
									}
									if (middleTransform.get_translation() != edgeTransform.get_translation())
									{
										if (edgeMesh->createMovementCollisionBoxes.create)
										{
											Matrix44 useTransform = look_at_matrix(middleTransform.get_translation(), edgeTransform.get_translation(), middleTransform.get_z_axis());
											_context.access_generation_context().store_movement_collision_part(create_collision_box_at(useTransform, _context.access_ipu(), _context.access_generation_context(), _context.access_element_instance(), edgeMesh->createMovementCollisionBoxes, polygonsSoFar));
										}
										if (edgeMesh->createPreciseCollisionBoxes.create)
										{
											Matrix44 useTransform = look_at_matrix(middleTransform.get_translation(), edgeTransform.get_translation(), middleTransform.get_z_axis());
											_context.access_generation_context().store_precise_collision_part(create_collision_box_at(useTransform, _context.access_ipu(), _context.access_generation_context(), _context.access_element_instance(), edgeMesh->createPreciseCollisionBoxes, polygonsSoFar));
										}
									}
								}
							}
							else
							{
								int prevPointIdx = prevPointsStartAt;
								int edgePointIdx = edgePointsStartAt;
								int polygonsSoFar = _context.access_ipu().get_polygon_count();
								for_every_const(uvDef, crossSection.us)
								{
									float uValue = uvDef->get_u(_context.access_element_instance().get_context());
									_context.access_ipu().add_quad(uValue, prevPointIdx, edgePointIdx, edgePointIdx + 1, prevPointIdx + 1);
									++prevPointIdx;
									++edgePointIdx;
								}
								if (prevTransform.get_translation() != edgeTransform.get_translation())
								{
									if (edgeMesh->createMovementCollisionBoxes.create)
									{
										Matrix44 useTransform = look_at_matrix(prevTransform.get_translation(), edgeTransform.get_translation(), prevTransform.get_z_axis());
										_context.access_generation_context().store_movement_collision_part(create_collision_box_at(useTransform, _context.access_ipu(), _context.access_generation_context(), _context.access_element_instance(), edgeMesh->createMovementCollisionBoxes, polygonsSoFar));
									}
									if (edgeMesh->createPreciseCollisionBoxes.create)
									{
										Matrix44 useTransform = look_at_matrix(prevTransform.get_translation(), edgeTransform.get_translation(), prevTransform.get_z_axis());
										_context.access_generation_context().store_precise_collision_part(create_collision_box_at(useTransform, _context.access_ipu(), _context.access_generation_context(), _context.access_element_instance(), edgeMesh->createPreciseCollisionBoxes, polygonsSoFar));
									}
								}
							}

							lastCrossSection = &crossSection;
						}
						if (prev->cornerType == EdgeMeshCorner::Round)
						{
#ifdef LOG_POINTS_NUM
							output(TXT("[points num] : %04i : corner round"), _context.access_ipu().get_point_count());
#endif
							// setup with corner values
							// get from both prev and next csocs
							// use betweenCorners and curve lengths to calculate how much to we want to copy and what are the proportions to put here
							// we remap pos
							Array<ShapeUtils::CrossSectionOnCurve> csocs;
							if (prev->csOnCurveToUseAll.is_empty() || prev->csOnCurveToUseClamped.is_empty() ||
								edge->csOnCurveToUseAll.is_empty() || edge->csOnCurveToUseClamped.is_empty())
							{
								csocs.set_size(2);
								csocs[0].pt = 0.0f;
								csocs[0].id = ShapeUtils::get_alt_cross_section_id(edgeMesh->crossSections, prev->cornerCrossSectionId, prev->cornerCrossSectionAltIds);
								csocs[0].scaleFloat = prev->cornerScaleFloat;
								csocs[0].scale = prev->cornerScale;
								csocs[1] = csocs[0];
								csocs[1].pt = 1.0f;

								// calculate everything we need
								float roundCornerCurveLength = prev->roundCorner.curve.length();
								csocs[0].calculate_pos(Range(0.0f, 1.0f), roundCornerCurveLength);
								csocs[1].calculate_pos(Range(0.0f, 1.0f), roundCornerCurveLength);
							}
							else
							{
								csocs.make_space_for(prev->csOnCurveToUseAll.get_size() + edge->csOnCurveToUseAll.get_size() + 2); // way too much
								csocs.push_back(prev->csOnCurveToUseClamped.get_last()); // first is last clamped of prev
								csocs.get_last().set_pos(0.0f);
								float prevLength = prev->curve.length();
								float currLength = edge->curve.length();
								float prevLengthInUse = prevLength * max(0.0f, 1.0f - prev->betweenCorners.max);
								float currLengthInUse = prevLength * max(0.0f, edge->betweenCorners.min);
								float totalLengthInUse = prevLengthInUse + currLengthInUse;
								float roundCornerLength = prev->roundCorner.curve.length();
								{
									float const fromPos = prev->betweenCorners.max * prevLength;
									float const fromLength = prevLength - fromPos;
									float const tolength = (prevLengthInUse / totalLengthInUse) * roundCornerLength;
									float const fromToLength = tolength / fromLength;
									for_every(csoc, prev->csOnCurveToUseAll)
									{
										if (csoc->get_pos_not_clamped() >= fromPos)
										{
											csocs.push_back(*csoc);
											csocs.get_last().set_pos((csocs.get_last().get_pos_not_clamped() - fromPos) * fromToLength);
										}
									}
								}
								{
									float const upToPos = edge->betweenCorners.min * currLength;
									float const fromLength = upToPos;
									float const tolength = (currLengthInUse / totalLengthInUse) * roundCornerLength;
									float const fromToLength = tolength / fromLength;
									float const toPos = (prevLengthInUse / totalLengthInUse) * roundCornerLength;
									for_every(csoc, edge->csOnCurveToUseAll)
									{
										if (csoc->get_pos_not_clamped() <= upToPos)
										{
											csocs.push_back(*csoc);
											csocs.get_last().set_pos(csocs.get_last().get_pos_not_clamped() * fromToLength + toPos);
										}
									}
								}
								// last is first clamped of edge
								csocs.push_back(edge->csOnCurveToUseClamped.get_first());
								csocs.get_last().set_pos(roundCornerLength);
							}

							if (lastCrossSection &&
								csocs.get_first().id != lastCrossSection->id)
							{
								ShapeUtils::CrossSectionOnCurve match = csocs.get_first();
								match.id = lastCrossSection->id;
								match.altId.clear();
								match.uId.clear();
								csocs.push_front(match);
							}

							/*
							ShapeUtils::build_cross_section_mesh(_context.access_ipu(), _context.access_element_instance().access_context(), _context.access_element_instance(),
								edgeMesh->createMovementCollisionBoxes, edgeMesh->createPreciseCollisionBoxes,
								edgeMesh->debugDraw, edgeMesh->crossSections, edgeMeshScale, edgeMeshOffset,
								prev->roundCorner.curve, 0.0f, 1.0f, true, useEdgeRef->get_edge_sub_step(),
								&csocs,
								nullptr,
								[prev, edgeMesh, &_context](BezierCurve<Vector3> const & _curve, float _t, Vector3 const & _tangent) { return (prev->roundCorner.prevNormal * (1.0f - _t) + prev->roundCorner.nextNormal * _t).normal(); },
								[prev, edge, edgeMesh, &_context](BezierCurve<Vector3> const & _curve, float _t) { return Meshes::VertexSkinningInfo::blend(prev->calculate_skinning_info_for(prev->betweenCorners.max, edgeMesh, _context), edge->calculate_skinning_info_for(edge->betweenCorners.min, edgeMesh, _context), _t); }
								);
							*/
#ifdef LOG_USED_CSOCS
							output(TXT("    add single cross section (window corners using csocs)"));
							for_every(csoc, csocs)
							{
								output(TXT("       %S @%.3f"), csoc->id.to_char(), csoc->get_pos());
							}
#endif
							ShapeUtils::build_cross_section_mesh_using_window_corners(_context.access_ipu(), _context.access_element_instance().access_context(), _context.access_element_instance(),
								edgeMesh->createMovementCollisionBoxes, edgeMesh->createPreciseCollisionBoxes,
								edgeMesh->debugDraw, edgeMesh->crossSections, edgeMeshScale, edgeMeshOffset,
								prev->roundCorner.curveWindow, prev->roundCorner.curveWindowCorners,
								prev->roundCorner.curve, 0.0f, 1.0f, useEdgeRef->get_edge_sub_step(), edge->details,
								&csocs,
								[prev, edge, edgeMesh, &_context](BezierCurve<Vector3> const& _curve, float _t) { return Meshes::VertexSkinningInfo::blend(prev->calculate_skinning_info_for(prev->betweenCorners.max, edgeMesh, _context), edge->calculate_skinning_info_for(edge->betweenCorners.min, edgeMesh, _context), _t); }
							);

							{
								lastCrossSection = nullptr;
								for_every(altId, csocs.get_last().altId)
								{
									if (!lastCrossSection)
									{
										lastCrossSection = &ShapeUtils::get_cross_section(edgeMesh->crossSections, *altId);
									}
								}
								if (!lastCrossSection)
								{
									lastCrossSection = &ShapeUtils::get_cross_section(edgeMesh->crossSections, csocs.get_last().id);
								}
								if (lastCrossSection)
								{
									pointCountCorner.set_if_not_set(lastCrossSection->points.get_size());
#ifdef LOG_POINTS_NUM
									output(TXT("[points num] : %04i : pointCountCorner : %i"), _context.access_ipu().get_point_count(), pointCountCorner.get());
#endif
								}
							}
						}
					}
					int pointsStartAt = _context.access_ipu().get_point_count();
#ifdef LOG_POINTS_NUM
					output(TXT("[points num] : %04i : build cross section (actual edge)"), _context.access_ipu().get_point_count());
					output(TXT("[points num] :      : edge->betweenCorners.min : %.3f"), edge->betweenCorners.min);
					output(TXT("[points num] :      : edge->betweenCorners.max : %.3f"), edge->betweenCorners.max);
					output(TXT("[points num] :      : edge->curve.is_degenerated() : %S"), edge->curve.is_degenerated()? TXT("DEGENERATED") : TXT("NOT DEG"));
#endif
					// build cross section
					if (edge->betweenCorners.max > edge->betweenCorners.min && ! edge->curve.is_degenerated())
					{
						Array<ShapeUtils::CrossSectionOnCurve> csocs = edge->csOnCurveToUseClamped;

#ifdef LOG_USED_CSOCS
						output(TXT("    add single cross section (build using csOnCurveToUseClamped)"));
						for_every(csoc, csocs)
						{
							output(TXT("       %S @%.3f"), csoc->id.to_char(), csoc->get_pos());
						}
#endif

						if (lastCrossSection && 
							!csocs.is_empty() &&
							csocs.get_first().id != lastCrossSection->id)
						{
							ShapeUtils::CrossSectionOnCurve match = csocs.get_first();
							match.id = lastCrossSection->id;
							match.altId.clear();
							match.uId.clear();
							float atDist = edge->safeDistAt0.get(edge->betweenCorners.min * edge->curve.length());
							match.pt.clear();
							match.dist = atDist;
							match.set_pos(atDist);
							csocs.push_front(match);
#ifdef LOG_USED_CSOCS
							output(TXT("   lst %S @%.3f"), match.id.to_char(), match.get_pos());
#endif
						}

						//pointCountCorner.set_if_not_set(crossSection.points.get_size());
						Array<ShapeUtils::CrossSectionOnCurve> usedcsocs;
						ShapeUtils::build_cross_section_mesh(_context.access_ipu(), _context.access_element_instance().access_context(), _context.access_element_instance(),
							edgeMesh->createMovementCollisionBoxes, edgeMesh->createPreciseCollisionBoxes,
							edgeMesh->debugDraw, edgeMesh->crossSections, edgeMeshScale, edgeMeshOffset, edge->curve,
							edge->betweenCorners.min, edge->betweenCorners.max, true, useEdgeRef->get_edge_sub_step(), edge->details,
							&csocs, &usedcsocs,
							&edge->customOnCurveToUseClamped,
							[edge, edgeMesh, &_context](BezierCurve<Vector3> const& _curve, float _t, Vector3 const& _tangent) { return edge->calculate_normal_for(_curve, _t, _tangent, edgeMesh, _context); },
							[edge, edgeMesh, &_context](BezierCurve<Vector3> const& _curve, float _t) { return edge->calculate_skinning_info_for(_curve, _t, edgeMesh, _context); }
						);
#ifdef LOG_POINTS_NUM
						output(TXT("[points num] : %04i : added cross section (actual edge)"), _context.access_ipu().get_point_count());
#endif
#ifdef LOG_USED_CSOCS
						output(TXT("    used csocs?"));
						for_every(csoc, usedcsocs)
						{
							output(TXT("       %S @%.3f"), csoc->id.to_char(), csoc->get_pos());
						}
#endif
						{
							lastCrossSection = nullptr;
							if (!usedcsocs.is_empty())
							{
								for_every(altId, usedcsocs.get_last().altId)
								{
									if (!lastCrossSection)
									{
										lastCrossSection = &ShapeUtils::get_cross_section(edgeMesh->crossSections, *altId);
									}
								}
								if (!lastCrossSection)
								{
									lastCrossSection = &ShapeUtils::get_cross_section(edgeMesh->crossSections, usedcsocs.get_last().id);
								}
							}
							if (lastCrossSection)
							{
								pointCountEdge.set_if_not_set(lastCrossSection->points.get_size());
#ifdef LOG_POINTS_NUM
								output(TXT("[points num] : %04i : pointCountEdge : %i"), _context.access_ipu().get_point_count(), pointCountEdge.get());
#endif
							}
						}
					}
					
#ifdef LOG_POINTS_NUM
					output(TXT("[points num] : %04i : post build cross section, pre weld"), _context.access_ipu().get_point_count());
					output(TXT("[points num] :      : pointsStartAt : %i"), pointsStartAt);
					output(TXT("[points num] :      : preCornerPointsStartAt : %i"), preCornerPointsStartAt);
#endif
					// changed to always welding as for non explicitly welded we still want to have point in the same spot
					// but sometimes due to curve alignment they were slightly off
					// we weld both pre corner and post corner
					// if they're the same, we don't do that - due to averaging would give the same result anyway
					//if (/* prev && prev->cornerType == EdgeMeshCorner::Weld && */) // this ensure we have valid points
					{
						for_count(int, weldIdx, preCornerPointsStartAt == pointsStartAt? 1 : 2)
						{
							// preCornerPointsStartAt weld corner with previous edge
							// pointsStartAt weld edge mesh with corner
							// there might be two separate or just one (one of two)
							int usePointsStartAt = weldIdx == 1 ? pointsStartAt : preCornerPointsStartAt;
							int pointCountPre = weldIdx == 1 ? pointCountCorner.get(pointCountEdge.get(0)) : pointCountPrevEdge.get(pointCountCorner.get(pointCountEdge.get(0)));
							int pointCountPost = weldIdx == 1 ? pointCountEdge.get(pointCountCorner.get(0)) : pointCountCorner.get(pointCountEdge.get(0));
#ifdef LOG_POINTS_NUM
							output(TXT("[points num] :      : weldIdx : %i"), weldIdx);
							output(TXT("[points num] :      :   pointCountPre : %i"), pointCountPre);
							output(TXT("[points num] :      :   pointCountPost : %i"), pointCountPost);
							output(TXT("[points num] :      :   pointCountCorner : %i"), pointCountCorner.get(-1));
							output(TXT("[points num] :      :   pointCountEdge : %i"), pointCountEdge.get(-1));
							output(TXT("[points num] :      :   pointCountPrevEdge : %i"), pointCountPrevEdge.get(-1));
							output(TXT("[points num] :      :   usePointsStartAt : %i"), usePointsStartAt);
#endif
							if (pointCountPre == pointCountPost)
							{
								int pointCount = pointCountPre;
#ifdef LOG_POINTS_NUM
								output(TXT("[points num] :      :   _context.access_ipu().get_point_count() : %i"), _context.access_ipu().get_point_count());
#endif
								if (_context.access_ipu().get_point_count() >= usePointsStartAt + pointCount &&
									usePointsStartAt - pointCount >= 0)
								{
									int actualStartAt = usePointsStartAt - pointCount;

									bool alreadyAdded = false;
									for_every(wi, weldSections)
									{
										if (wi->pointsAt == actualStartAt && wi->pointCount == pointCount)
										{
											alreadyAdded = true;
											break;
										}
									}
									if (!alreadyAdded)
									{
#ifdef LOG_POINTS_NUM
										output(TXT("[points num] : %04i : store weld %i->%i"), _context.access_ipu().get_point_count(), usePointsStartAt - pointCount, usePointsStartAt + pointCount - 1);
#endif
										weldSections.push_back(WeldInfo(actualStartAt, pointCount));
									}
									else
									{
#ifdef LOG_POINTS_NUM
										output(TXT("[points num] : %04i : already added to weld %i->%i"), _context.access_ipu().get_point_count(), usePointsStartAt - pointCount, usePointsStartAt + pointCount - 1);
#endif
									}
								}
							}
						}
					}
				}

				if (pointCountEdge.is_set())
				{
					pointCountPrevEdge = pointCountEdge;
				}
				else if (pointCountCorner.is_set())
				{
					pointCountPrevEdge = pointCountCorner;
				}
				
				if (!edge->storeOnCurveToUse.is_empty())
				{
					float curveLength = edge->curve.length();
					float invCurveLength = curveLength != 0.0f ? 1.0f / curveLength : 0.0f;
					Vector2 edgeMeshScale = edgeMesh->scaleCrossSection.get(_context.access_element_instance().get_context());
					{
						float edgeMeshScaleFloat = edgeMesh->scaleCrossSectionFloat.get(_context.access_element_instance().get_context());
						if (edgeMeshScaleFloat > 0.0f)
						{
							edgeMeshScale = Vector2(edgeMeshScaleFloat, edgeMeshScaleFloat);
						}
					}
					Vector2 const edgeMeshOffset = edgeMesh->offsetCrossSection.get(_context.access_element_instance().get_context());
					for_every_const(soc, edge->storeOnCurveToUse)
					{
						Vector3 vector = soc->vector;
						Matrix44 atCurveMat;
						ShapeUtils::calculate_matrix_for(edge->curve, soc->onCurve.get_pos() * invCurveLength, true, atCurveMat,
							[edge, edgeMesh, &_context](BezierCurve<Vector3> const& _curve, float _t, Vector3 const& _tangent) { return edge->calculate_normal_for(_curve, _t, _tangent, edgeMesh, _context); });

						// this might not work as intended?
						if (soc->applyOffset)
						{
							// offset curve and modify axes using scale and offset cross section
							atCurveMat.set_translation(atCurveMat.get_translation() + atCurveMat.vector_to_world(Vector3(edgeMeshOffset.x, 0.0f, edgeMeshOffset.y)));
						}
						if (soc->applyScale)
						{
							atCurveMat.set_x_axis(atCurveMat.get_x_axis() * edgeMeshScale.x);
							atCurveMat.set_z_axis(atCurveMat.get_z_axis() * edgeMeshScale.y);
						}

						if (soc->type == EdgeMeshEdgeStoreType::Location)
						{
							vector = atCurveMat.location_to_world(vector);
						}
						else
						{
							vector = atCurveMat.vector_to_world(vector);
						}

						if (soc->globalStore)
						{
							_context.access_element_instance().access_context().set_global_parameter<Vector3>(soc->asParameter, vector);
						}
						else
						{
							_context.access_element_instance().access_context().set_parameter<Vector3>(soc->asParameter, vector);
						}
					}
				}

#ifdef LOG_POINTS_NUM
				output(TXT("[points num] : %04i : done edge"), _context.access_ipu().get_point_count());
#endif

				prev = edge;
			}

			// weld at the end
			for_every(wi, weldSections)
			{
				int pointCount = wi->pointCount;
				int prevPointIdx = wi->pointsAt;
				int edgePointIdx = wi->pointsAt + pointCount;
#ifdef LOG_POINTS_NUM
				output(TXT("[points num] : weld section %i : %i->%i [%i->%i], point count %i"), for_everys_index(wi), prevPointIdx, edgePointIdx, prevPointIdx, edgePointIdx + pointCount - 1, pointCount);
#endif
				if (prevPointIdx >= edgeMeshPointsStartAt &&
					wi->pointsAt + pointCount * 2 - 1< _context.access_ipu().get_point_count())
				{
#ifdef LOG_POINTS_NUM
					output(TXT("[points num] : %04i : weld! prev %i to edge %i, pointCount %i"), _context.access_ipu().get_point_count(), prevPointIdx, edgePointIdx, pointCount);
#endif
					for_count(int, i, pointCount)
					{
						Vector3 prevLoc, prevNormal;
						Vector3 edgeLoc, edgeNormal;
						_context.access_ipu().get_point(prevPointIdx, prevLoc, prevNormal);
						_context.access_ipu().get_point(edgePointIdx, edgeLoc, edgeNormal);

						edgeLoc = (prevLoc + edgeLoc) * 0.5f;
						edgeNormal = (prevNormal + edgeNormal).normal();

						_context.access_ipu().set_point(prevPointIdx, edgeLoc, edgeNormal);
						_context.access_ipu().set_point(edgePointIdx, edgeLoc, edgeNormal);
						_context.access_ipu().copy_skinning(prevPointIdx, edgePointIdx);

						++prevPointIdx;
						++edgePointIdx;
					}
				}

			}
		}

		// get end points
		edgeMesh->edgeEndPointsA.clear();
		edgeMesh->edgeEndPointsB.clear();
		if (!edgeMesh->crossSections.is_empty())
		{
			int csPointCount = edgeMesh->crossSections.get_first().points.get_size();
			int idx = pointsSoFar;
			auto const& ipu = _context.access_ipu();
			while (idx < ipu.get_point_count() - csPointCount + 1)
			{
				{
					Vector3 l, n;
					ipu.get_point(idx, l, n);
					edgeMesh->edgeEndPointsA.push_back(l);
				}
				{
					Vector3 l, n;
					ipu.get_point(idx + csPointCount - 1, l, n);
					edgeMesh->edgeEndPointsB.push_back(l);
				}
				idx += csPointCount;
			}
		}

		// build from use elements
		if (!edgeMesh->useElements.elements.is_empty() &&
			edgeMesh->useElements.startToEndCorridor.is_set())
		{
			// first we want to build an array of placements we will use to transform generated mesh to edge based

			struct Curve
			{
				float length; // rangeT taken into consideration
				BezierCurve<Vector3> curve;
				Range rangeT;
				EdgeMeshEdge::RoundCorner const * basedOnRoundCorner = nullptr; // optional
				EdgeMeshEdge const * basedOnEdge = nullptr;

				Curve() {}
				Curve(float _length, BezierCurve<Vector3> const & _curve, Range const & _rangeT = Range(0.0f, 1.0f))
				: length(_length)
				, curve(_curve)
				, rangeT(_rangeT)
				{}

				static void calculate_placement_and_scale(float _at, Array<Curve> const & _curves, EdgeMesh const * _edgeMesh, EdgesAndSurfacesContext & _context, OUT_ Transform & _placement, OUT_ float & _scale)
				{
					int idx = 0;
					for_every(curve, _curves)
					{
						if (_at <= curve->length && curve->length != 0.0f)
						{
							break;
						}
						_at -= curve->length;
						++idx;
					}
					if (idx >= _curves.get_size())
					{
						idx = _curves.get_size() - 1;
						_at = _curves.get_last().length;
					}
					auto const & curve = _curves[idx];
					_at = clamp(_at, 0.0f, curve.length);
					an_assert(curve.length != 0.0f);
					float inRangeT = clamp(_at / curve.length, 0.0f, 1.0f);
					float inCurveT = curve.rangeT.get_at(inRangeT);
					Vector3 loc = curve.curve.calculate_at(inCurveT);
					Vector3 normal;
					Vector3 right;
					if (curve.basedOnRoundCorner)
					{
						Vector3 corners[4];
						for_count(int, c, 4)
						{
							corners[c] = curve.basedOnRoundCorner->curveWindowCorners[c].calculate_at(inCurveT);
						}
						normal = ((corners[ShapeUtils::LT] - corners[ShapeUtils::LB]).normal() + (corners[ShapeUtils::RT] - corners[ShapeUtils::RB]).normal()).normal();
						right = ((corners[ShapeUtils::RB] - corners[ShapeUtils::LB]).normal() + (corners[ShapeUtils::RT] - corners[ShapeUtils::LT]).normal()).normal();
						_scale = 1.0f;
						todo_note(TXT("add scale support"));
						if (! curve.basedOnEdge->csOnCurveToUseAll.is_empty())
						{
							auto const & c = curve.basedOnEdge->csOnCurveToUseAll.get_first();
							Vector2 scaleV2 = c.scale.get(_context.access_generation_context());
							float scaleF = c.scaleFloat.get(_context.access_generation_context());
							if (c.scaleCurve.is_set())
							{
								scaleV2 *= c.scaleCurve.get().calculate_at(0.0f).p;
							}
							if (c.scaleFloatCurve.is_set())
							{
								scaleF *= c.scaleFloatCurve.get().calculate_at(0.0f).p;
							}
							_scale = scaleF * (scaleV2.x + scaleV2.y) * 0.5f;
						}
					}
					else
					{
						Vector3 tangent = curve.curve.calculate_tangent_at(inCurveT);
						normal = curve.basedOnEdge->calculate_normal_for(inCurveT, tangent, _edgeMesh, _context);
						right = Vector3::cross(tangent, normal).normal(); // right should be perpendicular to both tangent and normal
						normal = Vector3::cross(right, tangent).normal(); // fix normal
						_scale = 1.0f;
						todo_note(TXT("add scale support"));
						if (curve.basedOnEdge->csOnCurveToUseAll.get_size() >= 2) // we do not cover auto start and end here!
						{
							int idx = 0;
							float prevPos = 0.0f;
							float t = 0.0f;
							for_every(cs, curve.basedOnEdge->csOnCurveToUseAll)
							{
								if (_at <= cs->get_pos() && for_everys_index(cs) > 0)
								{
									int iCs = for_everys_index(cs);
									if (iCs < curve.basedOnEdge->csOnCurveToUseAll.get_size())
									{
										float currPos = curve.basedOnEdge->csOnCurveToUseAll[iCs].get_pos();
										t = clamp((_at - prevPos) / (currPos - prevPos), 0.0f, 1.0f);
									}
									else
									{
										t = 1.0f;
									}
									break;
								}
								++idx;
								prevPos = cs->get_pos();
							}
							if (idx >= curve.basedOnEdge->csOnCurveToUseAll.get_size())
							{
								idx = curve.basedOnEdge->csOnCurveToUseAll.get_size() - 1;
								t = 1.0f;
							}
							Vector2 scaleV2 = curve.basedOnEdge->csOnCurveToUseAll[idx].calculate_final_scale(_context.access_generation_context(), &curve.basedOnEdge->csOnCurveToUseAll[idx - 1], t, true);
							_scale = (scaleV2.x + scaleV2.y) * 0.5f;
						}
					}
					_placement = matrix_from_up_right(loc, normal, right).to_transform();
				}
			};

			// start with calculation of actual length and building array of curves
			float actualLength = 0.0f;
			Array<Curve> curves;
			EdgeMeshEdge const * prev = edgeMesh->loopedEdgeChain ? edgeMesh->useEdges.get_last() : nullptr;
			for_every_const_ptr(edge, edgeMesh->useEdges)
			{
				Range betweenCorners = edge->betweenCorners;
				if (prev)
				{
					if (prev->cornerType == EdgeMeshCorner::Sharp ||
						prev->cornerType == EdgeMeshCorner::Cut)
					{
						betweenCorners.min = 0.0f;
					}
					if (prev->cornerType == EdgeMeshCorner::Round)
					{
						float length = prev->roundCorner.curve.length();
						actualLength += length;
						curves.push_back(Curve(length, prev->roundCorner.curve));
						curves.get_last().basedOnRoundCorner = &prev->roundCorner;
					}
				}
				if (edge->cornerType == EdgeMeshCorner::Sharp ||
					edge->cornerType == EdgeMeshCorner::Cut)
				{
					betweenCorners.max = 1.0f;
				}
				{
					float length = edge->curve.length() * betweenCorners.length();
					actualLength += length;
					curves.push_back(Curve(length, edge->curve, betweenCorners));
					curves.get_last().basedOnEdge = edge;
				}
			}

			if (actualLength > 0.0f &&
				edgeMesh->useElements.startToEndCorridorLength > 0.0f)
			{
				struct Div
				{
					float t; // total/whole t!
					//
					float at; // t * length
					Transform from;
					Transform to;
					float scale;

					Div() {}
					explicit Div(float _t) : t(_t) {}

					static int compare(void const * _a, void const * _b)
					{
						Div const * a = (Div const *)(_a);
						Div const * b = (Div const *)(_b);
						if (a->t < b->t) { return A_BEFORE_B; }
						if (a->t > b->t) { return B_BEFORE_A; }
						return A_AS_B;
					}

					static void calculate_placement_and_scale(Vector3 const & loc, Array<Div> const & divs, OUT_ Transform & _placement, OUT_ Vector3 & _scale, bool _scaleIntoToPlacement = false)
					{
						int beforeIdx = 0;
						float t = 0.0f;
						for_every(div, divs)
						{
							if (loc.y <= div->at)
							{
								break;
							}
							++beforeIdx;
						}
						if (beforeIdx >= divs.get_size())
						{
							beforeIdx = divs.get_size() - 1;
							t = 1.0f;
						}
						else
						{
							if (beforeIdx > 0)
							{
								auto & curr = divs[beforeIdx];
								auto & prev = divs[beforeIdx - 1];
								t = (loc.y - prev.at) / (curr.at - prev.at);
							}
							else
							{
								t = 0.0f;
							}
						}
						auto & curr = divs[beforeIdx];
						auto & prev = divs[max(0, beforeIdx - 1)];
						Transform from = Transform::lerp(t, prev.from, curr.from);
						Transform to = Transform::lerp(t, prev.to, curr.to);
						float scale = prev.scale * (1.0f - t) + t * curr.scale;
						if (_scaleIntoToPlacement)
						{
							to.set_scale(to.get_scale() * scale);
						}
						_placement = to.to_world(from.inverted());
						_scale = Vector3(scale, scale, scale);
					}
				};

				Array<Div> divs;

				auto & generationContext = _context.access_generation_context();

				if (edgeMesh->useElements.divMeshNodeName.is_valid())
				{
					auto * cpRef = &generationContext.get_mesh_nodes()[edgeMesh->useElements.checkpointStart.meshNodesSoFarCount];
					for_range(int, i, edgeMesh->useElements.checkpointStart.meshNodesSoFarCount, edgeMesh->useElements.checkpointEnd.meshNodesSoFarCount - 1)
					{
						auto * cp = cpRef->get();
						if (cp->name == edgeMesh->useElements.divMeshNodeName)
						{
							divs.push_back(Div(edgeMesh->useElements.startToEndCorridor.get().location_to_local(cp->placement.get_translation()).y / edgeMesh->useElements.startToEndCorridorLength));
						}
						++cpRef;
					}
				}

				if (divs.is_empty())
				{
					int subStepCount = edgeMesh->useElements.subStep.calculate_sub_step_count_for(actualLength, generationContext, 1.0f /* for individual edges? :/ */, edgeMesh->noMesh);
					if (subStepCount > 1)
					{
						divs.make_space_for(subStepCount + 2);
						float tStep = 1.0f / (float)subStepCount;
						float at = tStep;
						float endAt = 1.0f - tStep * 0.5f;
						while (at < endAt)
						{
							divs.push_back(Div(at));
							at += tStep;
						}
					}
				}

				// start and end!
				divs.push_back(Div(0.0f));
				divs.push_back(Div(1.0f));

				// sort them
				sort(divs);

				// calculate placements
				for_every(div, divs)
				{
					// length
					div->at = div->t * edgeMesh->useElements.startToEndCorridorLength;
					// from placement
					div->from = edgeMesh->useElements.startToEndCorridor.get();
					div->from.set_translation(div->from.location_to_world(Vector3::yAxis * div->t * edgeMesh->useElements.startToEndCorridorLength));
					// to placement
					Curve::calculate_placement_and_scale(div->t * actualLength, curves, edgeMesh, _context, OUT_ div->to, OUT_ div->scale);
				}

				// drop mesh nodes (if we need to keep them, an option could be added)
				for (int idx = edgeMesh->useElements.checkpointStart.meshNodesSoFarCount; idx < edgeMesh->useElements.checkpointEnd.meshNodesSoFarCount; ++idx)
				{
					auto& mn = generationContext.get_mesh_nodes()[idx];
					if (mn->name == edgeMesh->useElements.startMeshNodeName ||
						mn->name == edgeMesh->useElements.endMeshNodeName ||
						mn->name == edgeMesh->useElements.divMeshNodeName)
					{
						mn->drop();
					}
				}

				// get all vertices and stuff
				if (generationContext.get_parts().is_index_valid(edgeMesh->useElements.checkpointStart.partsSoFarCount))
				{
					for (int idx = edgeMesh->useElements.checkpointStart.partsSoFarCount; idx < edgeMesh->useElements.checkpointEnd.partsSoFarCount; ++idx)
					{
						auto * part = generationContext.get_parts()[idx].get();
						::System::VertexFormat const & vf = part->get_vertex_format();
						char8 * vertexLocAt = &part->access_vertex_data()[vf.get_location_offset()];
						char8 * vertexNorAt = vf.get_normal() != ::System::VertexFormatNormal::No ? &part->access_vertex_data()[vf.get_normal_offset()] : nullptr;
						int stride = vf.get_stride();
						for_count(int, v, part->get_number_of_vertices())
						{
							Vector3 & loc = *((Vector3 *)vertexLocAt);
							Transform placement;
							Vector3 scale;
							Div::calculate_placement_and_scale(loc, divs, placement, scale, true);
							loc = placement.location_to_world(loc);
							vertexLocAt += stride;
							if (vertexNorAt)
							{
								if (vf.is_normal_packed())
								{
									Vector3 nor = ((VectorPacked*)vertexNorAt)->get_as_vertex_normal();
									nor = placement.vector_to_world(nor).normal();
									((VectorPacked*)vertexNorAt)->set_as_vertex_normal(nor);
								}
								else
								{
									Vector3 & nor = *((Vector3 *)vertexNorAt);
									nor = placement.vector_to_world(nor).normal();
								}
								vertexNorAt += stride;
							}
						}
					}
				}
				if (generationContext.get_movement_collision_parts().is_index_valid(edgeMesh->useElements.checkpointStart.movementCollisionPartsSoFarCount))
				{
					todo_important(TXT("not supported yet"));
					/*
					::Collision::Model* const * part = &generationContext.get_movement_collision_parts()[edgeMesh->useElements.checkpointStart.movementCollisionPartsSoFarCount];
					for (int idx = edgeMesh->useElements.checkpointStart.movementCollisionPartsSoFarCount; idx < edgeMesh->useElements.checkpointEnd.movementCollisionPartsSoFarCount; ++idx, ++part)
					{
						ApplyTo applyTo(*part);
						apply_scale(applyTo, generationContext, generateSubElement->scale);
						apply_placement(applyTo, generationContext, generateSubElement->placement);
					}
					*/
				}
				if (generationContext.get_precise_collision_parts().is_index_valid(edgeMesh->useElements.checkpointStart.preciseCollisionPartsSoFarCount))
				{
					todo_important(TXT("not supported yet"));
					/*
					::Collision::Model* const * part = &generationContext.get_precise_collision_parts()[edgeMesh->useElements.checkpointStart.preciseCollisionPartsSoFarCount];
					for (int idx = edgeMesh->useElements.checkpointStart.preciseCollisionPartsSoFarCount; idx < edgeMesh->useElements.checkpointEnd.preciseCollisionPartsSoFarCount; ++idx, ++part)
					{
						ApplyTo applyTo(*part);
						apply_scale(applyTo, generationContext, generateSubElement->scale);
						apply_placement(applyTo, generationContext, generateSubElement->placement);
					}
					*/
				}
				if (edgeMesh->useElements.checkpointStart.socketsGenerationIdSoFar != generationContext.get_current_sockets_generation_id())
				{
					for_every(socket, generationContext.access_sockets().access_sockets())
					{
						if (socket->get_generation_id() > edgeMesh->useElements.checkpointStart.socketsGenerationIdSoFar &&
							socket->get_generation_id() < edgeMesh->useElements.checkpointEnd.socketsGenerationIdSoFar)
						{
							Vector3 loc = edgeMesh->useElements.startToEndCorridor.get().get_translation();
							if (socket->get_placement_MS().is_set())
							{
								loc = socket->get_placement_MS().get().get_translation();								
							}
							else if (!socket->get_bone_name().is_valid() || generationContext.find_bone_index(socket->get_bone_name()) == NONE)
							{
								// allow manipulation with auto placement only if not added to bone yet or no bone specified at all - because if there is bone, our placement is settled
								// and allow setting for element socket
								if (socket->get_relative_placement().is_set())
								{
									loc = socket->get_relative_placement().get().get_translation();
								}
							}

							Transform placement;
							Vector3 scale;
							Div::calculate_placement_and_scale(loc, divs, placement, scale);

							ApplyTo applyTo(socket);
							apply_scale(applyTo, generationContext, scale);
							apply_placement(applyTo, generationContext, placement);
						}
					}
				}
				if (generationContext.get_mesh_nodes().is_index_valid(edgeMesh->useElements.checkpointStart.meshNodesSoFarCount))
				{
					MeshNodePtr* meshNode = &generationContext.access_mesh_nodes()[edgeMesh->useElements.checkpointStart.meshNodesSoFarCount];
					for (int idx = edgeMesh->useElements.checkpointStart.meshNodesSoFarCount; idx < edgeMesh->useElements.checkpointEnd.meshNodesSoFarCount; ++idx, ++meshNode)
					{
						Vector3 loc = meshNode->get()->placement.get_translation();
						Transform placement;
						Vector3 scale;
						Div::calculate_placement_and_scale(loc, divs, placement, scale);

						ApplyTo applyTo(meshNode->get());
						apply_placement(applyTo, generationContext, placement);
					}
				}
				if (generationContext.get_pois().is_index_valid(edgeMesh->useElements.checkpointStart.poisSoFarCount))
				{
					PointOfInterestPtr* poi = &generationContext.access_pois()[edgeMesh->useElements.checkpointStart.poisSoFarCount];
					for (int idx = edgeMesh->useElements.checkpointStart.poisSoFarCount; idx < edgeMesh->useElements.checkpointEnd.poisSoFarCount; ++idx, ++poi)
					{
						Vector3 loc = poi->get()->offset.get_translation();
						Transform placement;
						Vector3 scale;
						Div::calculate_placement_and_scale(loc, divs, placement, scale);

						ApplyTo applyTo(poi->get());
						apply_scale(applyTo, generationContext, scale);
						apply_placement(applyTo, generationContext, placement);
					}
				}
				if (generationContext.get_space_blockers().blockers.is_index_valid(edgeMesh->useElements.checkpointStart.spaceBlockersSoFarCount))
				{
					SpaceBlocker* sb = &generationContext.access_space_blockers().blockers[edgeMesh->useElements.checkpointStart.spaceBlockersSoFarCount];
					for (int idx = edgeMesh->useElements.checkpointStart.spaceBlockersSoFarCount; idx < edgeMesh->useElements.checkpointEnd.spaceBlockersSoFarCount; ++idx, ++sb)
					{
						Vector3 loc = sb->box.get_centre();
						Transform placement;
						Vector3 scale;
						Div::calculate_placement_and_scale(loc, divs, placement, scale);

						ApplyTo applyTo(sb);
						apply_scale(applyTo, generationContext, scale);
						apply_placement(applyTo, generationContext, placement);
					}
				}
			}
		}

		// caps
		if (edgeMesh->capStart || edgeMesh->capEnd)
		{
			EdgeMeshEdge const * edge[] = { edgeMesh->useEdges.get_first(), edgeMesh->useEdges.get_last() };
			float capStartsAt[] = { edgeMesh->capStartT, edgeMesh->capEndT };
			Transform cap[2];

			for (int i = 0; i < 2; ++i)
			{
				ShapeUtils::calculate_transform_for(edge[i]->curve, capStartsAt[i], i == 1 /* for start we want opposite direction! */, cap[i],
					[&edge, i, edgeMesh, &_context](BezierCurve<Vector3> const & _curve, float _t, Vector3 const & _tangent) { return edge[i]->calculate_normal_for(_curve, _t, _tangent, edgeMesh, _context); });
			}

			if (edgeMesh->capStart)
			{
				_context.add_sub_element_to_generate(GenerateSubElement::get_one(edgeMesh->capStart->get_element(), cap[0]));
			}

			if (edgeMesh->capEnd)
			{
				_context.add_sub_element_to_generate(GenerateSubElement::get_one(edgeMesh->capEnd->get_element(), cap[1]));
			}
		}

		if (_context.access_generation_context().does_require_movement_collision())
		{
			if (edgeMesh->createMovementCollisionMesh.create)
			{
				_context.access_generation_context().store_movement_collision_parts(create_collision_meshes_from(_context.access_ipu(), _context.access_generation_context(), _context.access_element_instance(), edgeMesh->createMovementCollisionMesh, polygonsSoFar));
			}
			if (edgeMesh->createMovementCollisionBox.create)
			{
				_context.access_generation_context().store_movement_collision_part(create_collision_box_from(_context.access_ipu(), _context.access_generation_context(), _context.access_element_instance(), edgeMesh->createMovementCollisionBox, polygonsSoFar));
			}
		}

		if (_context.access_generation_context().does_require_precise_collision())
		{
			if (edgeMesh->createPreciseCollisionMesh.create)
			{
				_context.access_generation_context().store_precise_collision_parts(create_collision_meshes_from(_context.access_ipu(), _context.access_generation_context(), _context.access_element_instance(), edgeMesh->createPreciseCollisionMesh, polygonsSoFar));
			}
			if (edgeMesh->createPreciseCollisionBox.create)
			{
				_context.access_generation_context().store_precise_collision_part(create_collision_box_from(_context.access_ipu(), _context.access_generation_context(), _context.access_element_instance(), edgeMesh->createPreciseCollisionBox, polygonsSoFar));
			}
		}

		if (_context.access_generation_context().does_require_space_blockers())
		{
			if (edgeMesh->createSpaceBlocker.create)
			{
				_context.access_generation_context().store_space_blocker(create_space_blocker_from(_context.access_ipu(), _context.access_generation_context(), _context.access_element_instance(), edgeMesh->createSpaceBlocker, polygonsSoFar));
			}
		}

		if (edgeMesh->noMesh)
		{
			_context.access_ipu().keep_polygons(polygonsSoFar);
		}

		if (auto * useEdgeRef = edgeMesh->useEdgeRef? edgeMesh->useEdgeRef->get(_context.access_generation_context()) : nullptr)
		{
			SurfaceDef surfaceDef;
			surfaceDef.access_sub_step_override() = useEdgeRef->get_edge_sub_step();

			for_every_const_ptr(surfaceMesh, edgeMesh->generateSurfaceMeshes)
			{
				if (!surfaceMesh->generationCondition.check(_context.access_element_instance()))
				{
					continue;
				}

				int polygonsSoFar = _context.access_ipu().get_polygon_count();

				if (surfaceMesh->useEdgeEndPointsA ||
					surfaceMesh->useEdgeEndPointsB)
				{
					for(int edgeEndIdx = 0; edgeEndIdx < 2; ++ edgeEndIdx)
					{
						if (edgeEndIdx == 0 && !surfaceMesh->useEdgeEndPointsA)
						{
							continue;
						}
						if (edgeEndIdx == 1 && !surfaceMesh->useEdgeEndPointsB)
						{
							continue;
						}
						auto const& pointsSource = edgeEndIdx == 0? edgeMesh->edgeEndPointsA : edgeMesh->edgeEndPointsB;
						if (!pointsSource.is_empty())
						{
							bool reverse = surfaceMesh->reversed;
							if (edgeEndIdx == 0)
							{
								reverse = !reverse;
							}
							ARRAY_PREALLOC_SIZE(Vector3, points, pointsSource.get_size());
							if (reverse)
							{
								for_every_reverse(point, pointsSource)
								{
									points.push_back(*point);
								}
							}
							else
							{
								for_every(point, pointsSource)
								{
									points.push_back(*point);
								}
							}
							if (surfaceMesh->useOnePointAsCommon)
							{
								float u = surfaceMesh->uvDef.get_u(_context.access_element_instance().get_context());
								int commonIdx = NONE;
								int prevIdx = NONE;
								for_every(point, points)
								{
									if (commonIdx == NONE)
									{
										commonIdx = _context.access_ipu().add_point(*point);
										continue;
									}
									if (prevIdx == NONE)
									{
										prevIdx = _context.access_ipu().add_point(*point);
										continue;
									}
									int currIdx = _context.access_ipu().add_point(*point);
									_context.access_ipu().add_triangle(u, commonIdx, prevIdx, currIdx);
									prevIdx = currIdx;
								}
							}
							else
							{
								Vector3 centre = Vector3::zero;
								float centreWeight = 0.0f;
								for_every(point, points)
								{
									centre += *point;
									centreWeight += 1.0f;
								}
								centre /= centreWeight;

								int centreIdx = _context.access_ipu().add_point(centre);
								int prevIdx = NONE;
								Optional<Vector3> prevPoint;
								if (edgeMesh->loopedEdgeChain)
								{
									prevPoint = points.get_last();
								}
								float u = surfaceMesh->uvDef.get_u(_context.access_element_instance().get_context());
								for_every(point, points)
								{
									if (prevPoint.is_set())
									{
										if (prevIdx == NONE)
										{
											prevIdx = _context.access_ipu().add_point(prevPoint.get());
										}
									}
									int currIdx = _context.access_ipu().add_point(*point);
									if (prevIdx != NONE && centreIdx != NONE)
									{
										_context.access_ipu().add_triangle(u, prevIdx, currIdx, centreIdx);
									}
									prevPoint = *point;
									prevIdx = currIdx;
								}
							}
						}
					}
				}
				else if (! edgeMesh->useEdges.is_empty())
				{
					Vector3 centre = Vector3::zero;
					Vector3 extrusionNormalAtCentre = Vector3::zero;
					{
						// first centre
						float centreWeight = 0.0f;
						for_every_const_ptr(edge, edgeMesh->useEdges)
						{
							centre += edge->node->location;
							centreWeight += 1.0f;
						}
						centre /= centreWeight;
						// now extrusionNormalAtCentre
						EdgeMeshEdge const * prev = edgeMesh->loopedEdgeChain ? edgeMesh->useEdges.get_last() : nullptr;
						for_every_const_ptr(edge, edgeMesh->useEdges)
						{
							if (prev)
							{
								extrusionNormalAtCentre += Vector3::cross(edge->node->location - centre, prev->node->location - centre);
							}
							prev = edge;
						}
						extrusionNormalAtCentre.normalise();
					}

					extrusionNormalAtCentre *= (surfaceMesh->reversed ? -1.0f : 1.0f);

					float tuckInwards = surfaceMesh->tuckInwards.get(_context.access_element_instance().get_context());
					if (surfaceMesh->reversed)
					{
						tuckInwards = -tuckInwards; // because we will be going in opposite direction, right will become left etc
					}

					// this method is just to add triangle based on edge, centre and normal
					// adjusts extrusion and builds proper triangles
					// uses "forSubStep" to use same division method when calculating sub count for edges
					std::function<bool(GatheredEdge & gatheredEdge, GatheredEdge & gatheredEdgeForSubStep)> generate_triangle = [this, &_context, surfaceMesh, surfaceDef, extrusionNormalAtCentre, centre](
						GatheredEdge & gatheredEdge, GatheredEdge & gatheredEdgeForSubStep)
					{
						ARRAY_STATIC(GatheredEdge, gatheredEdges, 4);
						ARRAY_STATIC(GatheredEdge, gatheredEdgesForSubStep, 4);

						float surfaceMeshExtrusion;
						gatheredEdge.adjust_extrusion(_context, gatheredEdge.edge->fromNode, gatheredEdge.edge->toNode, surfaceMesh, OUT_ &surfaceMeshExtrusion, extrusionNormalAtCentre);

						{
							Vector3 extrudedCentre = centre + surfaceMeshExtrusion * extrusionNormalAtCentre;
							GatheredEdge gatheredEdge0;
							GatheredEdge gatheredEdge1;
							GatheredEdge::setup_triangle(OUT_ gatheredEdge0, gatheredEdge, OUT_ gatheredEdge1, extrudedCentre, surfaceMesh->reversed, REF_ gatheredEdges);
						}

						{
							GatheredEdge gatheredEdgeForSubStep0;
							GatheredEdge gatheredEdgeForSubStep1;
							GatheredEdge::setup_triangle(OUT_ gatheredEdgeForSubStep0, gatheredEdgeForSubStep, OUT_ gatheredEdgeForSubStep1, Vector3::zero, surfaceMesh->reversed, REF_ gatheredEdgesForSubStep);
						}

						return generate_surface_mesh(_context, surfaceMesh, surfaceDef, BezierSurfaceCreationMethod::Roundly, gatheredEdges, &gatheredEdgesForSubStep);
					};

					EdgeMeshEdge const * prev = edgeMesh->loopedEdgeChain ? edgeMesh->useEdges.get_last() : nullptr;
					for_every_const_ptr(edge, edgeMesh->useEdges)
					{
						// note: because we affect edge with tuckInwards and extrusion, we use original edge to calculate substep count
						{	// do the edge
							GatheredEdge gatheredEdge;
							gatheredEdge.edge = edge->edgeRef.edge;
							gatheredEdge.curve = edge->edgeRef.edge->curve; // apply round corners!
							Range betweenCorners = edge->betweenCorners;
							GatheredEdge gatheredEdgeForSubStep = gatheredEdge;
							{
								BezierCurve<Vector3> cutCurve = gatheredEdge.curve;
								Vector3 p0Tangent = gatheredEdge.curve.calculate_tangent_at(betweenCorners.min).normal();
								Vector3 p3Tangent = gatheredEdge.curve.calculate_tangent_at(betweenCorners.max).normal();
								cutCurve.p0 = gatheredEdge.curve.calculate_at(betweenCorners.min);
								cutCurve.p3 = gatheredEdge.curve.calculate_at(betweenCorners.max);
								cutCurve.p1 = cutCurve.p0 + p0Tangent * (gatheredEdge.curve.p1 - gatheredEdge.curve.p0).length();
								cutCurve.p2 = cutCurve.p3 - p3Tangent * (gatheredEdge.curve.p2 - gatheredEdge.curve.p3).length();
								gatheredEdgeForSubStep.curve = cutCurve;
								//
								cutCurve.p0 += Vector3::cross(p0Tangent, extrusionNormalAtCentre).normal() * tuckInwards;
								cutCurve.p3 += Vector3::cross(p3Tangent, extrusionNormalAtCentre).normal() * tuckInwards;
								cutCurve.p1 = cutCurve.p0 + p0Tangent * (gatheredEdge.curve.p1 - gatheredEdge.curve.p0).length();
								cutCurve.p2 = cutCurve.p3 - p3Tangent * (gatheredEdge.curve.p2 - gatheredEdge.curve.p3).length();
								gatheredEdge.curve = cutCurve;
							}

							if (! generate_triangle(gatheredEdge, gatheredEdgeForSubStep))
							{
								error_generating_mesh(_context.access_element_instance(), TXT("couldn't generate surfaceMesh %i for edgeMesh \"%S\""), edgeMesh->id.to_char());
								continue;
							}
						}

						// build corner now!
						if (prev)
						{
							if (prev->cornerType == EdgeMeshCorner::Round)
							{
								GatheredEdge gatheredEdge;
								gatheredEdge.edge = edge->edgeRef.edge;

								GatheredEdge gatheredEdgeForSubStep = gatheredEdge;
								{
									BezierCurve<Vector3> cutCurve = prev->roundCorner.curve;
									gatheredEdgeForSubStep.curve = cutCurve;
									//
									Vector3 p0Tangent = cutCurve.calculate_tangent_at(0.0f);
									Vector3 p3Tangent = cutCurve.calculate_tangent_at(1.0f);
									cutCurve.p0 += Vector3::cross(p0Tangent, extrusionNormalAtCentre).normal() * tuckInwards;
									cutCurve.p3 += Vector3::cross(p3Tangent, extrusionNormalAtCentre).normal() * tuckInwards;
									cutCurve.p1 = cutCurve.p0 + p0Tangent;
									cutCurve.p2 = cutCurve.p3 - p3Tangent;
									gatheredEdge.curve = cutCurve;
									gatheredEdge.curve.make_roundly_separated();
								}

								if (!generate_triangle(gatheredEdge, gatheredEdgeForSubStep))
								{
									error_generating_mesh(_context.access_element_instance(), TXT("couldn't generate surfaceMesh %i for edgeMesh \"%S\""), edgeMesh->id.to_char());
									continue;
								}
							}
							else if (prev->cornerType == EdgeMeshCorner::Sharp ||
									 prev->cornerType == EdgeMeshCorner::Cut)
							{
								// define 
								ARRAY_STATIC(Vector3, cornerPointLocation, 3);
								ARRAY_STATIC(float, cornerPointScaleTuckInwards, 3);
								ARRAY_STATIC(Vector3, cornerPointTangent, 3);

								// start
								cornerPointLocation.push_back(prev->edgeRef.edge->curve.calculate_at(prev->betweenCorners.max));
								cornerPointTangent.push_back(prev->edgeRef.edge->curve.calculate_tangent_at(prev->betweenCorners.max));
								cornerPointScaleTuckInwards.push_back(1.0f);

								// sharp if required
								if (prev->cornerType == EdgeMeshCorner::Sharp)
								{
									cornerPointLocation.push_back(prev->sharpCorner.location);
									cornerPointTangent.push_back(prev->sharpCorner.tangent);
									Vector3 tuckInwardsDir = Vector3::cross(prev->sharpCorner.tangent, extrusionNormalAtCentre).normal();
									Vector2 tuckInwardsInSharpCorner;
									tuckInwardsInSharpCorner.x = abs(Vector3::dot(tuckInwardsDir, prev->sharpCorner.right));
									tuckInwardsInSharpCorner.y = abs(Vector3::dot(tuckInwardsDir, prev->sharpCorner.normal));
									cornerPointScaleTuckInwards.push_back(Vector2::dot(tuckInwardsInSharpCorner, prev->sharpCorner.scaleCrossSection));
								}

								// end
								cornerPointLocation.push_back(edge->edgeRef.edge->curve.calculate_at(edge->betweenCorners.min));
								cornerPointTangent.push_back(edge->edgeRef.edge->curve.calculate_tangent_at(edge->betweenCorners.min));
								cornerPointScaleTuckInwards.push_back(1.0f);

								for_count(int, i, cornerPointLocation.get_size() - 1)
								{
									GatheredEdge gatheredEdge;
									gatheredEdge.edge = edge->edgeRef.edge;

									GatheredEdge gatheredEdgeForSubStep = gatheredEdge;
									{
										BezierCurve<Vector3> cutCurve;
										cutCurve.p0 = cornerPointLocation[i];
										cutCurve.p3 = cornerPointLocation[i + 1];
										cutCurve.make_linear();
										gatheredEdgeForSubStep.curve = cutCurve;
										//
										cutCurve.p0 += Vector3::cross(cornerPointTangent[i + 0], extrusionNormalAtCentre) * cornerPointScaleTuckInwards[i + 0] * tuckInwards;
										cutCurve.p3 += Vector3::cross(cornerPointTangent[i + 1], extrusionNormalAtCentre) * cornerPointScaleTuckInwards[i + 1] * tuckInwards;
										cutCurve.make_linear();
										gatheredEdge.curve = cutCurve;
									}

									if (!generate_triangle(gatheredEdge, gatheredEdgeForSubStep))
									{
										error_generating_mesh(_context.access_element_instance(), TXT("couldn't generate surfaceMesh %i for edgeMesh \"%S\""), edgeMesh->id.to_char());
										continue;
									}
								}
							}
							else
							{
								todo_important(TXT("implement_ other corner cases"));
							}
						}

						prev = edge;
					}

				}

				if (_context.access_generation_context().does_require_movement_collision())
				{
					if (surfaceMesh->createMovementCollisionMesh.create)
					{
						_context.access_generation_context().store_movement_collision_parts(create_collision_meshes_from(_context.access_ipu(), _context.access_generation_context(), _context.access_element_instance(), surfaceMesh->createMovementCollisionMesh, polygonsSoFar));
					}
					if (surfaceMesh->createMovementCollisionBox.create)
					{
						_context.access_generation_context().store_movement_collision_part(create_collision_box_from(_context.access_ipu(), _context.access_generation_context(), _context.access_element_instance(), surfaceMesh->createMovementCollisionBox, polygonsSoFar));
					}
				}

				if (_context.access_generation_context().does_require_precise_collision())
				{
					if (surfaceMesh->createPreciseCollisionMesh.create)
					{
						_context.access_generation_context().store_precise_collision_parts(create_collision_meshes_from(_context.access_ipu(), _context.access_generation_context(), _context.access_element_instance(), surfaceMesh->createPreciseCollisionMesh, polygonsSoFar));
					}
				}

				if (_context.access_generation_context().does_require_space_blockers())
				{
					if (surfaceMesh->createSpaceBlocker.create)
					{
						_context.access_generation_context().store_space_blocker(create_space_blocker_from(_context.access_ipu(), _context.access_generation_context(), _context.access_element_instance(), surfaceMesh->createSpaceBlocker, polygonsSoFar));
					}
				}

				if (surfaceMesh->noMesh)
				{
					_context.access_ipu().keep_polygons(polygonsSoFar);
				}
			}
		}
	}
}

bool EdgesAndSurfacesData::generate_surface_mesh(EdgesAndSurfacesContext & _context, SurfaceMesh const * surfaceMesh, SurfaceDef const & surfaceDef, BezierSurfaceCreationMethod::Type surfaceCreationMethod, ArrayStatic<GatheredEdge,4> const & gatheredEdges, ArrayStatic<GatheredEdge,4> const * gatheredEdgesForSubStep) const
{
	// create bezier surface
	BezierSurface bezierSurface;
	if (gatheredEdges.get_size() == 3)
	{
		bezierSurface = BezierSurface::create(gatheredEdges[0].curve, gatheredEdges[1].curve, gatheredEdges[2].curve);
	}
	else if (gatheredEdges.get_size() == 4)
	{
		bezierSurface = BezierSurface::create(gatheredEdges[0].curve, gatheredEdges[1].curve, gatheredEdges[2].curve, gatheredEdges[3].curve, surfaceCreationMethod);
	}
	else
	{
		return false;
	}

	_context.access_ipu().force_material_idx(surfaceMesh->forceMaterialIdx);

	// generate mesh
	Meshes::Helpers::BezierSurfaceHelper bezierSurfaceHelper(&bezierSurface);
	Meshes::Helpers::BezierSurfaceRibbonHelper bezierSurfaceRibbonHelper;
	bool doSimpleFlat = surfaceDef.should_force_simple_flat();
	if (!doSimpleFlat && surfaceDef.should_allow_simple_flat())
	{
		int linearCount = 0;
		for (int i = 0; i < gatheredEdges.get_size(); ++i)
		{
			if (gatheredEdges[i].curve.is_linear())
			{
				SubStepDef const* useSubStep = &gatheredEdges[i].edge->edgeDef.get_sub_step();
				if (SubStepDef const* subStepOverride = surfaceDef.get_sub_step_override())
				{
					useSubStep = subStepOverride;
				}
				if (!useSubStep || !useSubStep->should_sub_divide_linear(_context.access_element_instance().get_context()))
				{
					++linearCount;
				}
			}
		}
		if (linearCount == gatheredEdges.get_size())
		{
			doSimpleFlat = true;
		}
	}
	{
		float u = surfaceMesh->uvDef.get_u(_context.access_element_instance().get_context());
		bezierSurfaceHelper.set_default_u(u);
		bezierSurfaceRibbonHelper.set_default_u(surfaceMesh->extrusionU.is_set()? surfaceMesh->extrusionU.get(_context.access_element_instance().get_context()) : u);
	}
	int maxSegmentsCount[2] = { 0, 0 };
	bool allowInnerPointsOnGrid = surfaceDef.should_add_inner_grid();
	bool addInnerGrid = allowInnerPointsOnGrid && ! doSimpleFlat;
	REMOVE_AS_SOON_AS_POSSIBLE_ String info(TXT("generate surface mesh"));
	for (int i = 0; i < gatheredEdges.get_size(); ++i)
	{
		SubStepDef const * useSubStep = &gatheredEdges[i].edge->edgeDef.get_sub_step();
		if (SubStepDef const * subStepOverride = surfaceDef.get_sub_step_override())
		{
			useSubStep = subStepOverride;
		}
		auto const & gatheredEdgeForSubStep = (gatheredEdgesForSubStep ? (*gatheredEdgesForSubStep)[i] : gatheredEdges[i]);
#ifdef AN_DEVELOPMENT
#ifndef AN_CLANG
		float len = gatheredEdgeForSubStep.curve.length();
#endif
#endif
		todo_note(TXT("maybe we should have option to allow dividing edges anyway? TN#7"));
		int segmentsCount;
		if (doSimpleFlat || (!allowInnerPointsOnGrid && gatheredEdgeForSubStep.curve.is_linear()))
		{
			segmentsCount = 1;
		}
		else
		{
			segmentsCount = useSubStep->calculate_sub_step_count_for(
				gatheredEdgeForSubStep.useNotExtrudedCurveForSubStep ? gatheredEdgeForSubStep.curveNotExtruded.length() : gatheredEdgeForSubStep.curve.length(),
				_context.access_element_instance().get_context(), NP, surfaceMesh->noMesh);
		}
		maxSegmentsCount[i % 2] = max(maxSegmentsCount[i % 2], segmentsCount);
		REMOVE_AS_SOON_AS_POSSIBLE_ info += String::printf(TXT("\nedge %i of %i: segCount %i curve %S"), i, gatheredEdges.get_size(), segmentsCount, gatheredEdges[i].curve.to_string().to_char());
		{
			REMOVE_AS_SOON_AS_POSSIBLE_ scoped_call_stack_info_str_printf(info.to_char());
			bezierSurfaceHelper.add_border_edge(gatheredEdges[i].curve,
				segmentsCount,
				Meshes::Helpers::BezierSurfaceEdgeFlags::Border | Meshes::Helpers::BezierSurfaceEdgeFlags::Undividable);
			if (gatheredEdges[i].doExtrusionFill)
			{
				bezierSurfaceRibbonHelper.turn_into_mesh(gatheredEdges[i].curve, gatheredEdges[i].curveExtrusionFill, segmentsCount, &_context.access_ipu());
			}
		}
	}
	bezierSurfaceHelper.set_max_edge_length(doSimpleFlat ? 0.0f : surfaceDef.get_max_edge_length());
	if (addInnerGrid)
	{
		if (gatheredEdges.get_size() < 4)
		{
			maxSegmentsCount[0] = maxSegmentsCount[1] = max(maxSegmentsCount[0], maxSegmentsCount[1]);
		}
		bezierSurfaceHelper.set_max_edge_length(0.0f);
		bezierSurfaceHelper.add_inner_grid(maxSegmentsCount[0] - 1, maxSegmentsCount[1] - 1, bezierSurfaceHelper.get_default_u()); // for two segments we want one inner point
	}
	bezierSurfaceHelper.process();
	
	// transfer to ipu
	_context.access_ipu().will_need_more_points(bezierSurfaceHelper.get_point_count());
	bezierSurfaceHelper.into_mesh(&_context.access_ipu());
	_context.access_ipu().force_material_idx(NP);

	return true;
}

void EdgesAndSurfacesData::process_surface_meshes(EdgesAndSurfacesContext & _context) const
{
#ifdef AN_DEVELOPMENT
	for_every_const_ptr(surface, surfaces)
	{
		if (surface->debugDraw || debugDrawSurfaces)
		{
			if (auto * drf = _context.access_generation_context().debugRendererFrame)
			{
				for_every(edge, surface->edges)
				{
					float length = edge->edge->curve.length();
					if (length != 0.0f)
					{
						float const invLength = 1.0f / length;
						float const step = min(0.1f, length / 20.0f);
						Vector3 prev = edge->edge->curve.calculate_at(edge->reversed? 1.0f : 0.0f);
						for (float t = step; t < length; t += step)
						{
							Vector3 curr = edge->edge->curve.calculate_at(edge->reversed ? 1.0f - t * invLength : t * invLength);

							drf->add_line(true, Colour::red, prev, curr);

							prev = curr;
						}
						Vector3 last = edge->edge->curve.calculate_at(edge->reversed? 0.0f : 1.0f);
						drf->add_arrow(true, Colour::red, prev, last, length * 0.05f);
					}
				}
			}
		}
	}
#endif
	for_every_const_ptr(surfaceMesh, surfaceMeshes)
	{
		if (!surfaceMesh->generationCondition.check(_context.access_element_instance()))
		{
			continue;
		}
		
		int polygonsSoFar = _context.access_ipu().get_polygon_count();
		
		bool doExtrusionFill = surfaceMesh->extrusionFill.is_set();

		// each edge is assumed to be extrusion fill edge, unless it is mentioned at least twice in the surfaces (we assume, by two or more surfaces), then it is no longer truly an external edge
		ARRAY_PREALLOC_SIZE(Name, extrusionFillEdges, 4 * surfaceMesh->surfaces.get_size());
		ARRAY_PREALLOC_SIZE(Name, nonExtrusionFillEdges, 4 * surfaceMesh->surfaces.get_size());

		if (doExtrusionFill)
		{
			for_every_const_ptr(surface, surfaceMesh->surfaces)
			{
				for_every_const(edgeRef, surface->edges)
				{
					Name edgeId = edgeRef->id;
					if (extrusionFillEdges.does_contain(edgeId))
					{
						extrusionFillEdges.remove_fast(edgeId);
						nonExtrusionFillEdges.push_back_unique(edgeId);
					}
					else if (!nonExtrusionFillEdges.does_contain(edgeId))
					{
						extrusionFillEdges.push_back(edgeId);
					}
				}
			}
		}

		for_every_const_ptr(surface, surfaceMesh->surfaces)
		{
			// gather edges/curves in proper order, we will be referring only to this array (and a little bit to definitions of edges through "edge" field)
			// but in general all adjustments to curves should be done here and those curves are guaranteed to be used later/below
			ARRAY_STATIC(GatheredEdge, gatheredEdges, 4);
			for_every_const(edgeRef, surface->edges)
			{
				GatheredEdge gatheredEdge;
				gatheredEdge.edge = edgeRef->edge;
				gatheredEdge.curve = edgeRef->edge->curve;
				gatheredEdge.adjust_extrusion(_context, gatheredEdge.edge->fromNode, gatheredEdge.edge->toNode, surfaceMesh);
				// order it properly
				gatheredEdge.curve = gatheredEdge.curve.reversed(edgeRef->reversed);
				gatheredEdge.curveExtrusionFill = gatheredEdge.curveExtrusionFill.reversed(edgeRef->reversed);
				// store info about extrusion fill
				gatheredEdge.doExtrusionFill = extrusionFillEdges.does_contain(edgeRef->id);
				if (surfaceMesh->useNotExtrudedCurveForSubStep.is_set())
				{
					gatheredEdge.useNotExtrudedCurveForSubStep = surfaceMesh->useNotExtrudedCurveForSubStep.get(_context.access_element_instance().get_context());
				}
				else
				{
					gatheredEdge.useNotExtrudedCurveForSubStep = surfaceMesh->extrusionFill.is_set();
				}
				gatheredEdges.push_back(gatheredEdge);
			}

			if (!generate_surface_mesh(_context, surfaceMesh, surface->surfaceDef, surface->surfaceCreationMethod, gatheredEdges))
			{
				error_generating_mesh(_context.access_element_instance(), TXT("couldn't generate surfaceMesh \"%S\", gatheredEdges count %i"), surface->id.to_char(), gatheredEdges.get_size());
				continue;
			}
		}

		if (_context.access_generation_context().does_require_movement_collision())
		{
			if (surfaceMesh->createMovementCollisionMesh.create)
			{
				_context.access_generation_context().store_movement_collision_parts(create_collision_meshes_from(_context.access_ipu(), _context.access_generation_context(), _context.access_element_instance(), surfaceMesh->createMovementCollisionMesh, polygonsSoFar));
			}
			if (surfaceMesh->createMovementCollisionBox.create)
			{
				_context.access_generation_context().store_movement_collision_part(create_collision_box_from(_context.access_ipu(), _context.access_generation_context(), _context.access_element_instance(), surfaceMesh->createMovementCollisionBox, polygonsSoFar));
			}
		}

		if (_context.access_generation_context().does_require_precise_collision())
		{
			if (surfaceMesh->createPreciseCollisionMesh.create)
			{
				_context.access_generation_context().store_precise_collision_parts(create_collision_meshes_from(_context.access_ipu(), _context.access_generation_context(), _context.access_element_instance(), surfaceMesh->createPreciseCollisionMesh, polygonsSoFar));
			}
		}

		if (_context.access_generation_context().does_require_space_blockers())
		{
			if (surfaceMesh->createSpaceBlocker.create)
			{
				_context.access_generation_context().store_space_blocker(create_space_blocker_from(_context.access_ipu(), _context.access_generation_context(), _context.access_element_instance(), surfaceMesh->createSpaceBlocker, polygonsSoFar));
			}
		}

		if (surfaceMesh->noMesh)
		{
			_context.access_ipu().keep_polygons(polygonsSoFar);
		}

	}
}

void EdgesAndSurfacesData::process_place_mesh_on_edges(EdgesAndSurfacesContext & _context) const
{
	for_every_ptr(placeOnEdge, placeOnEdges)
	{
		if (placeOnEdge->useEdges.is_empty() ||
			!placeOnEdge->generationCondition.check(_context.access_element_instance()))
		{
			continue;
		}

		if (placeOnEdge->onEdgeMesh.is_valid())
		{
			for_every(at, placeOnEdge->ats)
			{
				float const atStartElementsSize = placeOnEdge->atStartElements.size.get(_context.access_element_instance().get_context());
				float const atEndElementsSize = placeOnEdge->atEndElements.size.get(_context.access_element_instance().get_context());
				float mainElementsSize = placeOnEdge->mainElements.size.get(_context.access_element_instance().get_context());

				float wholeLength = 0.0f;
				for_every(edge, placeOnEdge->useEdges)
				{
					wholeLength += edge->curveLength;
				}

				Range atPT = at->pt.get(_context.access_element_instance().get_context());

				float startAtDist = wholeLength * atPT.min;
				float endAtDist = wholeLength * atPT.max;
				float startPlacingAtDist = wholeLength * atPT.min + atStartElementsSize;
				float endPlacingAtDist = wholeLength * atPT.max - atEndElementsSize;

				float wholeLengthAvailable = endPlacingAtDist - startPlacingAtDist; // wholeLength - atStartElementsSize - atEndElementsSize;

				EdgeMeshPlaceAlignment::Type alignment = at->alignment;

				float spacing = at->spacing.is_set() ? at->spacing.get(_context.access_element_instance().get_context()) : 0.0f;
				bool skipEnds = at->skipEnds.is_set() ? at->skipEnds.get(_context.access_element_instance().get_context()) : false;
				bool atLeastOne = at->atLeastOne.is_set() ? at->atLeastOne.get(_context.access_element_instance().get_context()) : false;

				int count = 0;
				if (startPlacingAtDist >= endPlacingAtDist)
				{
					// force setting just one
					count = 1;
					startPlacingAtDist = endPlacingAtDist = startAtDist; // just to make sure
				}
				else if (wholeLengthAvailable > 0.0f)
				{
					count = placeOnEdge->calculate_count_for(_context, at, wholeLengthAvailable, placeOnEdge->edgeMesh->loopedEdgeChain, nullptr, OUT_ alignment, OUT_ spacing, OUT_ skipEnds, OUT_ atLeastOne);
				}

				if (count < 0)
				{
					continue;
				}

				float spaceRequiredForOneObject = mainElementsSize + spacing;
				bool const dontAddLastOne = placeOnEdge->edgeMesh->loopedEdgeChain ||
											(at->dontAddLastOne.is_set() ? at->dontAddLastOne.get(_context.access_element_instance().get_context()) : false);
				bool const dontAddFirstOne = (at->dontAddFirstOne.is_set() ? at->dontAddFirstOne.get(_context.access_element_instance().get_context()) : false);

				if (count > 0)
				{
					if (spaceRequiredForOneObject == 0.0f && spacing == 0.0f && count > 1)
					{
						spaceRequiredForOneObject = wholeLengthAvailable / (float)(count - 1);
						spacing = spaceRequiredForOneObject;
					}
					float willUseLength = (float)count * spaceRequiredForOneObject - spacing;
					float moveBy = 0.0f;
					if (willUseLength > 0.0f)
					{
						if (alignment == EdgeMeshPlaceAlignment::Centre)
						{
							moveBy = max(0.0f, wholeLengthAvailable - willUseLength) * 0.5f;
						}
						else if (alignment == EdgeMeshPlaceAlignment::End)
						{
							moveBy = max(0.0f, wholeLengthAvailable - willUseLength);
						}
					}
					if (moveBy != 0.0f)
					{
						startPlacingAtDist += moveBy;
					}
					endPlacingAtDist = startPlacingAtDist + willUseLength;
					if (at->adjustStartAndEndElements)
					{
						if (moveBy != 0.0f)
						{
							startAtDist += moveBy;
						}
						endAtDist += startAtDist + willUseLength;
					}

					float const fromStartToEndDist = endPlacingAtDist - startPlacingAtDist;
					float const invCount = count > 1 ? 1.0f / (float)(count - 1) : 0.0f;
					float const invWillUseLength = willUseLength != 0.0f ? 1.0f / willUseLength : 0.0f;
					for (int i = 0; i < count; ++i)
					{
						float t = (float)i * invCount;
						if (invWillUseLength != 0.0f)
						{
							// because with spacing we won't be laying pieces so evenly
							t = (float)i * spaceRequiredForOneObject * invWillUseLength;
						}
						if (count == 1)
						{
							t = 0.5f;
						}
						float dist = t * fromStartToEndDist + startPlacingAtDist;
						float localT;

						EdgePlaceOnEdgeEdge* edge = placeOnEdge->find_edge_among_use_edges(REF_ dist, OUT_ localT);

						int lastOne = skipEnds ? count - 2 : count - 1;
						if (((!skipEnds || (i > 0 && i < count - 1)) &&
						 	 (!dontAddLastOne || i < lastOne) &&
							 (!dontAddFirstOne || i > 0)) ||
							(atLeastOne && count == 1))
						{
							place_elements_at(placeOnEdge, edge, localT, true, _context, placeOnEdge->mainElements, mainElementsSize, spacing, i, count);
						}
					}
				}

				{
					float dist = startAtDist;
					float localT;
					EdgePlaceOnEdgeEdge* edge = placeOnEdge->find_edge_among_use_edges(REF_ dist, OUT_ localT);
					place_elements_at(placeOnEdge, edge, localT, true, _context, placeOnEdge->atStartElements);
				}
				{
					float dist = startAtDist;
					float localT;
					EdgePlaceOnEdgeEdge* edge = placeOnEdge->find_edge_among_use_edges(REF_ dist, OUT_ localT);
					place_elements_at(placeOnEdge, edge, localT, true, _context, placeOnEdge->atStartElements);
				}
			}
		}
		else
		{
			for_every(edge, placeOnEdge->useEdges)
			{
				Array<EdgePlaceOnEdgeAt> const & useAts = !edge->ats.is_empty() ? edge->ats : placeOnEdge->ats;

				for_every(at, useAts)
				{
					// this ptRange is along edge (we don't consider reversing yet!)
					Range ptRange = Range(0.0f, 1.0f);
					{
						if (edge->edgeMeshEdge)
						{
							ptRange = edge->edgeMeshEdge->betweenCorners;
							if (edge->edgeMeshEdge->edgeRef.reversed)
							{
								// we need to reverse betweenCorners as it is made for reversed edge
								swap(ptRange.min, ptRange.max);
								ptRange.min = 1.0f - ptRange.min;
								ptRange.max = 1.0f - ptRange.max;
							}
						}
					}

					// those will be general start and end, for start/end elements
					float startAtT = ptRange.min;
					float endAtT = ptRange.max;

					an_assert(startAtT >= 0.0f && startAtT <= 1.0f);
					an_assert(endAtT >= 0.0f && endAtT <= 1.0f);

					bool forward = ! edge->edgeRef.reversed;
					if (!forward)
					{
						swap(startAtT, endAtT);
					}

					{	// put into range of at
						float wholeRangeLength = endAtT - startAtT;
						float wholeRangeAt = startAtT;
						Range atPT = at->pt.get(_context.access_element_instance().get_context());
						startAtT = wholeRangeAt + atPT.min * wholeRangeLength;
						endAtT = wholeRangeAt + atPT.max * wholeRangeLength;
					}

					an_assert(startAtT >= 0.0f && startAtT <= 1.0f);
					an_assert(endAtT >= 0.0f && endAtT <= 1.0f);

					float const atStartElementsSize = placeOnEdge->atStartElements.size.get(_context.access_element_instance().get_context());
					float const atEndElementsSize = placeOnEdge->atEndElements.size.get(_context.access_element_instance().get_context());
					float mainElementsSize = placeOnEdge->mainElements.size.get(_context.access_element_instance().get_context());

					float curveLength = edge->edgeRef.edge->curve.distance(ptRange.min, ptRange.max);
					float curveLengthAvailable = curveLength;
					curveLengthAvailable -= atStartElementsSize + atEndElementsSize;
					//float curveLengthAvailableNotScaled = curveLengthAvailable;

					curveLengthAvailable *= (endAtT - startAtT);

					// those will be for main elements
					float startPlacingAtT = edge->edgeRef.edge->curve.get_t_at_distance_ex(atStartElementsSize, startAtT, forward);
					float endPlacingAtT = edge->edgeRef.edge->curve.get_t_at_distance_ex(atEndElementsSize, endAtT, !forward);

					float spacing = at->spacing.is_set() ? at->spacing.get(_context.access_element_instance().get_context()) : 0.0f;
					bool skipEnds = at->skipEnds.is_set() ? at->skipEnds.get(_context.access_element_instance().get_context()) : false;
					bool atLeastOne = at->atLeastOne.is_set() ? at->atLeastOne.get(_context.access_element_instance().get_context()) : false;
					EdgeMeshPlaceAlignment::Type alignment = at->alignment;

					int count = 0;
					bool oneRequested = false;
					if (startAtT == endAtT && curveLengthAvailable >= -MARGIN)
					{
						// force setting just one
						count = 1;
						oneRequested = true;
						startPlacingAtT = endPlacingAtT = startAtT; // just to make sure
					}
					else if (curveLengthAvailable > 0.0f)
					{
						count = placeOnEdge->calculate_count_for(_context, at, curveLengthAvailable, false, edge->edgeRef.id.to_char(), OUT_ alignment, OUT_ spacing, OUT_ skipEnds, OUT_ atLeastOne);
					}

					if (count < 0)
					{
						continue;
					}

					float spaceRequiredForOneObject = mainElementsSize + spacing;
					bool const dontAddLastOne = at->dontAddLastOne.is_set() ? at->dontAddLastOne.get(_context.access_element_instance().get_context()) : false;
					bool const dontAddFirstOne = at->dontAddFirstOne.is_set() ? at->dontAddFirstOne.get(_context.access_element_instance().get_context()) : false;

					if (count > 0)
					{
						if (spaceRequiredForOneObject == 0.0f && spacing == 0.0f && count > 1)
						{
							spaceRequiredForOneObject = curveLengthAvailable / (float)(count - 1);
							spacing = spaceRequiredForOneObject;
						}
						float willUseLength = (float)count * spaceRequiredForOneObject - spacing;
						float moveBy = 0.0f;
						if (!oneRequested)
						{
							if (alignment == EdgeMeshPlaceAlignment::Centre)
							{
								moveBy = max(0.0f, curveLengthAvailable - willUseLength) * 0.5f;
							}
							else if (alignment == EdgeMeshPlaceAlignment::End)
							{
								moveBy = max(0.0f, curveLengthAvailable - willUseLength);
							}
						}
						if (moveBy != 0.0f)
						{
							startPlacingAtT = edge->edgeRef.edge->curve.get_t_at_distance_ex(moveBy, startPlacingAtT, forward);
						}
						endPlacingAtT = edge->edgeRef.edge->curve.get_t_at_distance_ex(willUseLength, startPlacingAtT, forward);
						if (at->adjustStartAndEndElements)
						{
							if (moveBy != 0.0f)
							{
								startAtT = edge->edgeRef.edge->curve.get_t_at_distance_ex(moveBy, startAtT, forward);
							}
							endAtT = edge->edgeRef.edge->curve.get_t_at_distance_ex(willUseLength, startAtT, forward);
							an_assert(endAtT >= -0.0001f && endAtT <= 1.0001f);
							endAtT = clamp(endAtT, 0.0f, 1.0f); // inaccuracy kicks in
						}

						float const fromStartToEndT = endPlacingAtT - startPlacingAtT;
						float const invCount = count > 1? 1.0f / (float)(count - 1) : 0.0f;
						float const invWillUseLength = willUseLength != 0.0f ? 1.0f / willUseLength : 0.0f;
						for (int i = 0; i < count; ++i)
						{
							float t = (float)i * invCount;
							if (count == 1)
							{
								t = 0.5f;
								t = t * fromStartToEndT + startPlacingAtT;
							}
							else if (invWillUseLength != 0.0f)
							{
								// because with spacing we won't be laying pieces so evenly
								t = (float)i * spaceRequiredForOneObject * invWillUseLength * fromStartToEndT + startPlacingAtT;
							}
							else
							{
								t = t * fromStartToEndT + startPlacingAtT;
							}

							int lastOne = skipEnds? count - 2 : count - 1;
							if (((!skipEnds || (i > 0 && i < count - 1)) &&
								 (!dontAddLastOne || i < lastOne) &&
								 (!dontAddFirstOne || i > 0)) ||
								(atLeastOne && count == 1))
							{
								an_assert(t >= -0.0001f && t <= 1.0001f);
								t = clamp(t, 0.0f, 1.0f); // inaccuracy kicks in
								place_elements_at(placeOnEdge, edge, t, forward, _context, placeOnEdge->mainElements, mainElementsSize, spacing, i, count);
							}
						}
					}

					place_elements_at(placeOnEdge, edge, startAtT, forward, _context, placeOnEdge->atStartElements);
					place_elements_at(placeOnEdge, edge, endAtT, forward, _context, placeOnEdge->atStartElements);
				}

			}
		}
	}
}

void EdgesAndSurfacesData::place_elements_at(EdgePlaceOnEdge const * _placeOnEdge, EdgePlaceOnEdgeEdge const * _edge, float _t, bool _forward, EdgesAndSurfacesContext & _context, EdgePlaceOnEdgeElements const & _elements, float _placedOnEdgeElementSize, float _placedOnEdgeSpacing, int _onEdgeIndex, int _onEdgeCount) const
{
	Transform placement = Transform::identity;

	Transform offset = _placeOnEdge->placement.is_set()? _placeOnEdge->placement.get(_context.access_generation_context()) : Transform::identity;

	if (_edge->useNormals)
	{
		ShapeUtils::calculate_transform_for(_edge->curve, _t, _forward, placement,
			[_edge](BezierCurve<Vector3> const & _curve, float _t, Vector3 const & _tangent) { return (_edge->startNormal * (1.0f - _t) + _edge->endNormal * _t).normal(); });
	}
	else if (_edge->edgeMesh && _edge->edgeMeshEdge)
	{
		ShapeUtils::calculate_transform_for(_edge->curve, _t, _forward, placement,
			[_edge, &_context](BezierCurve<Vector3> const & _curve, float _t, Vector3 const & _tangent) { return _edge->edgeMeshEdge->calculate_normal_for(_curve, _t, _tangent, _edge->edgeMesh, _context); });

		todo_note(TXT("process scale? through cross sections?..."));
	}
	else
	{
		CustomData::EdgeNormalAlignment::Type normalAlignment = _edge->surfacesContext.get_normal_alignment(_context, _placeOnEdge->surfacesContext.get_normal_alignment(_context));

		Vector3 refPoint = _edge->surfacesContext.get_ref_point_for_normal(_context, _placeOnEdge->surfacesContext.get_ref_point_for_normal(_context));

		ShapeUtils::calculate_transform_for(_edge->curve, _t, _forward, placement,
			[_edge, normalAlignment, refPoint, &_context](BezierCurve<Vector3> const & _curve, float _t, Vector3 const & _tangent) { return calculate_normal_for(_context.access_element_instance(), _curve, _t, _tangent, normalAlignment, _edge->surfacesOnSides, refPoint); });
	}


	placement = placement.to_world(offset);

	if (_placeOnEdge->alongDirAxis == Axis::Z)
	{
		Transform rotate;
		rotate.build_from_axes(Axis::Y, Axis::X, -Vector3::zAxis, Vector3::xAxis, Vector3::yAxis, Vector3::zero);
		placement = placement.to_world(rotate);
	}
	else if (_placeOnEdge->alongDirAxis == Axis::X)
	{
		Transform rotate;
		rotate.build_from_axes(Axis::Y, Axis::X, -Vector3::xAxis, Vector3::yAxis, Vector3::zAxis, Vector3::zero);
		placement = placement.to_world(rotate);
	}
	else if (_placeOnEdge->alongDirAxis != Axis::Y)
	{
		todo_important(TXT("implement_"));
	}

	for_every(element, _elements.elements)
	{
		GenerateSubElement* gse = GenerateSubElement::get_one(element->get(), placement);
		if (_elements.elementsSizeParamName.is_valid())
		{
			gse->parameters.access<float>(_elements.elementsSizeParamName) = _placedOnEdgeElementSize;
		}
		if (_elements.spacingParamName.is_valid())
		{
			gse->parameters.access<float>(_elements.spacingParamName) = _placedOnEdgeSpacing;
		}
		if (_onEdgeIndex != NONE &&
			_onEdgeCount > 0)
		{
			if (_elements.firstElementParamName.is_valid())
			{
				gse->parameters.access<bool>(_elements.firstElementParamName) = _onEdgeIndex == 0;
			}
			if (_elements.lastElementParamName.is_valid())
			{
				gse->parameters.access<bool>(_elements.lastElementParamName) = _onEdgeIndex == _onEdgeCount - 1;
			}
		}
		_context.add_sub_element_to_generate(gse);
	}
}

void EdgesAndSurfacesData::process_polygons(EdgesAndSurfacesContext & _context) const
{
	for_every_const_ptr(polygonSet, polygons)
	{
		if (!polygonSet->generationCondition.check(_context.access_element_instance()))
		{
			continue;
		}

		int polygonsSoFar = _context.access_ipu().get_polygon_count();

		_context.access_ipu().force_material_idx(polygonSet->forceMaterialIdx);

		float u = polygonSet->uvDef.get_u(_context.access_element_instance().get_context());

		if (! polygonSet->autoConvex.triangles.is_empty())
		{
			for_every(triangle, polygonSet->autoConvex.triangles)
			{
				Node const* nodeA = find_node(triangle->nodeA);
				Node const* nodeB = find_node(triangle->nodeB);
				Node const* nodeC = find_node(triangle->nodeC);
				if (nodeA && nodeB && nodeC)
				{
					int aIdx = _context.access_ipu().add_point(nodeA->location);
					int bIdx = _context.access_ipu().add_point(nodeB->location);
					int cIdx = _context.access_ipu().add_point(nodeC->location);
					_context.access_ipu().add_triangle(u, aIdx, bIdx, cIdx);
				}
				else
				{
					an_assert(false, TXT("how come? we stored IDs from actual nodes!"));
				}
			}
		}

		for_every_const(polygon, polygonSet->polygons)
		{
			if (polygon->nodeA && polygon->nodeB && polygon->nodeC)
			{
				Meshes::VertexSkinningInfo vsi;
				Meshes::VertexSkinningInfo* usevsi = nullptr;
				if (polygon->boneIdx != NONE)
				{
					vsi.add(Meshes::VertexSkinningInfo::Bone(polygon->boneIdx, 1.0f));
					usevsi = &vsi;
				}
				int a = _context.access_ipu().add_point(polygon->nodeA->location + polygon->offsetAct, NP, usevsi);
				int b = _context.access_ipu().add_point(polygon->nodeB->location + polygon->offsetAct, NP, usevsi);
				int c = _context.access_ipu().add_point(polygon->nodeC->location + polygon->offsetAct, NP, usevsi);
				if (polygon->nodeD)
				{
					int d = _context.access_ipu().add_point(polygon->nodeD->location + polygon->offsetAct, NP, usevsi);
					_context.access_ipu().add_quad(u, a, b, c, d);
				}
				else
				{
					_context.access_ipu().add_triangle(u, a, b, c);
				}
			}
		}

		if (_context.access_generation_context().does_require_movement_collision())
		{
			if (polygonSet->createMovementCollisionMesh.create)
			{
				_context.access_generation_context().store_movement_collision_parts(create_collision_meshes_from(_context.access_ipu(), _context.access_generation_context(), _context.access_element_instance(), polygonSet->createMovementCollisionMesh, polygonsSoFar));
			}
			if (polygonSet->createMovementCollisionBox.create)
			{
				_context.access_generation_context().store_movement_collision_part(create_collision_box_from(_context.access_ipu(), _context.access_generation_context(), _context.access_element_instance(), polygonSet->createMovementCollisionBox, polygonsSoFar));
			}
		}

		if (_context.access_generation_context().does_require_precise_collision())
		{
			if (polygonSet->createPreciseCollisionMesh.create)
			{
				_context.access_generation_context().store_precise_collision_parts(create_collision_meshes_from(_context.access_ipu(), _context.access_generation_context(), _context.access_element_instance(), polygonSet->createPreciseCollisionMesh, polygonsSoFar));
			}
		}

		if (_context.access_generation_context().does_require_space_blockers())
		{
			if (polygonSet->createSpaceBlocker.create)
			{
				_context.access_generation_context().store_space_blocker(create_space_blocker_from(_context.access_ipu(), _context.access_generation_context(), _context.access_element_instance(), polygonSet->createSpaceBlocker, polygonsSoFar));
			}
		}

		_context.access_ipu().force_material_idx(NP);
	}
}

Node const * EdgesAndSurfacesData::find_node(Name const & _id) const
{
	for_every_const_ptr(node, nodes)
	{
		if (node->id == _id)
		{
			return node;
		}
	}
	if (_id.is_valid())
	{
		error_generating_mesh(*this, TXT("could not find node \"%S\""), _id.to_char());
	}
	return nullptr;
}

Node const * EdgesAndSurfacesData::find_node(Vector3 const & _loc) const
{
	for_every_const_ptr(node, nodes)
	{
		if (node->location == _loc)
		{
			return node;
		}
	}
	error_generating_mesh(*this, TXT("could not find node with location %S"), _loc.to_string().to_char());
	return nullptr;
}

Edge const * EdgesAndSurfacesData::find_edge(Name const & _id) const
{
	for_every_const_ptr(edge, edges)
	{
		if (edge->id == _id)
		{
			return edge;
		}
	}
	if (_id.is_valid())
	{
		error_generating_mesh(*this, TXT("could not find edge \"%S\""), _id.to_char());
	}
	return nullptr;
}

Edge const* EdgesAndSurfacesData::find_edge_for_nodes(Name const& _nodeA, Name const& _nodeB, OPTIONAL_ OUT_ bool * _reversed, bool _quiet) const
{
	for_every_const_ptr(edge, edges)
	{
		if (edge->fromNodeId == _nodeA &&
			edge->toNodeId == _nodeB)
		{
			assign_optional_out_param(_reversed, false);
			return edge;
		}
		if (edge->fromNodeId == _nodeB &&
			edge->toNodeId == _nodeA)
		{
			assign_optional_out_param(_reversed, true);
			return edge;
		}
	}
	if (!_quiet && _nodeA.is_valid() && _nodeB.is_valid())
	{
		error_generating_mesh(*this, TXT("could not find edge for nodes \"%S\" and \"%S\""), _nodeA.to_char(), _nodeB.to_char());
	}
	return nullptr;
}

Surface const * EdgesAndSurfacesData::find_surface(Name const & _id) const
{
	for_every_const_ptr(surface, surfaces)
	{
		if (surface->id == _id)
		{
			return surface;
		}
	}
	if (_id.is_valid())
	{
		error_generating_mesh(*this, TXT("could not find surface \"%S\""), _id.to_char());
	}
	return nullptr;
}

EdgeMesh const * EdgesAndSurfacesData::find_edge_mesh(Name const & _id) const
{
	for_every_const_ptr(edgeMesh, edgeMeshes)
	{
		if (edgeMesh->id == _id)
		{
			return edgeMesh;
		}
	}
	if (_id.is_valid())
	{
		error_generating_mesh(*this, TXT("could not find edge mesh \"%S\""), _id.to_char());
	}
	return nullptr;
}

template <typename Class>
void remove_auto_generated(Array<Class*>& _array)
{
	for (int i = 0; i < _array.get_size(); ++i)
	{
		if (_array[i]->isAutoGenerated)
		{
			delete _array[i];
			_array.remove_at(i);
			--i;
		}
	}
}

bool EdgesAndSurfacesData::prepare_for_generation(EdgesAndSurfacesContext & _context) const
{
	bool result = true;

	autoConvexId = 0;

	remove_auto_generated(surfaces);
	remove_auto_generated(surfaceMeshes);

	for (int i = 0; i < PrepareForGenerationLevel::MAX; ++i)
	{
		PrepareForGenerationLevel::Type level = (PrepareForGenerationLevel::Type)i;

		for_every_const_ptr(node, nodes)
		{
			result &= node->prepare_for_generation(for_everys_index(node), _context, level);
		}

		for_every_const_ptr(edge, edges)
		{
			result &= edge->prepare_for_generation(_context, level);
		}

		for_every_const_ptr(surface, surfaces)
		{
			result &= surface->prepare_for_generation(_context, level);
		}

		for_every_const_ptr(edgeMesh, edgeMeshes)
		{
			result &= edgeMesh->prepare_for_generation(for_everys_index(edgeMesh), _context, level);
		}

		for_every_const_ptr(surfaceMesh, surfaceMeshes)
		{
			result &= surfaceMesh->prepare_for_generation(_context, level);
		}

		for_every_const_ptr(placeOnEdge, placeOnEdges)
		{
			result &= placeOnEdge->prepare_for_generation(_context, level);
		}

		for_every_const_ptr(polygonSet, polygons)
		{
			result &= polygonSet->prepare_for_generation(_context, level);
		}
	}

	return result;
}

Vector3 EdgesAndSurfacesData::calculate_normal_for(ElementInstance& _instance, BezierCurve<Vector3> const & _curve, float _t, Vector3 const & _tangent, CustomData::EdgeNormalAlignment::Type _normalAlignment, EdgeSurfacesOnSides const & _surfacesOnSides, Vector3 const & _refPoint)
{
	switch (_normalAlignment)
	{
		case CustomData::EdgeNormalAlignment::AlignWithLeftNormal:
		case CustomData::EdgeNormalAlignment::AlignWithRightNormal:
		case CustomData::EdgeNormalAlignment::AlignWithNormals:
			{
				Vector3 normalSum = Vector3::zero;
				if (_normalAlignment == CustomData::EdgeNormalAlignment::AlignWithLeftNormal ||
					_normalAlignment == CustomData::EdgeNormalAlignment::AlignWithNormals)
				{
					if (_surfacesOnSides.surfaceOnLeft)
					{
						normalSum += _surfacesOnSides.surfaceOnLeft->calculate_normal_at_uv(_surfacesOnSides.surfaceOnLeftEdgeUV.calculate_at(_t));
					}
					else
					{
						error_generating_mesh(_instance, TXT("no surface on left!"));
					}
				}
				if (_normalAlignment == CustomData::EdgeNormalAlignment::AlignWithRightNormal ||
					_normalAlignment == CustomData::EdgeNormalAlignment::AlignWithNormals)
				{
					if (_surfacesOnSides.surfaceOnRight)
					{
						normalSum += _surfacesOnSides.surfaceOnRight->calculate_normal_at_uv(_surfacesOnSides.surfaceOnRightEdgeUV.calculate_at(_t));
					}
					else
					{
						error_generating_mesh(_instance, TXT("no surface on right!"));
					}
				}
				return normalSum.normal();
			}
			break;
		case CustomData::EdgeNormalAlignment::AwayFromRefPoint:
		case CustomData::EdgeNormalAlignment::TowardsRefPoint:
			{
				Vector3 atCurve = _curve.calculate_at(_t);

				return _normalAlignment == CustomData::EdgeNormalAlignment::AwayFromRefPoint ? (atCurve - _refPoint).normal() : (_refPoint - atCurve).normal();
			}
			break;
		case CustomData::EdgeNormalAlignment::RefDir:
			{
				return _refPoint.normal();
			}
			break;
		default:
			todo_important(TXT("implement_ me!"));
			return Vector3::zero;
			break;
	}
}

//

bool Node::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;

	id = _node->get_name_attribute(TXT("id"), id);
	if (auto* attr = _node->get_attribute(TXT("fromVar")))
	{
		id = attr->get_as_name();
		locationFromVar.set_param(id);
	}
	if (!id.is_valid())
	{
		error_loading_xml(_node, TXT("node without id"));
		result = false;
	}

	mirrorXOfId = _node->get_name_attribute(TXT("mirrorOf"), mirrorXOfId);
	mirrorXOfId = _node->get_name_attribute(TXT("mirrorXOf"), mirrorXOfId);
	mirrorYOfId = _node->get_name_attribute(TXT("mirrorYOf"), mirrorYOfId);
	mirrorZOfId = _node->get_name_attribute(TXT("mirrorZOf"), mirrorZOfId);
	collapseXOfId = _node->get_name_attribute(TXT("collapseXOf"), collapseXOfId);
	addToId = _node->get_name_attribute(TXT("addTo"), addToId);

	result &= nodeDef.load_from_xml(_node);
	
	// allow to keep location in "location" child or in this node
	locationAsLoaded.load_from_xml_child_node(_node, TXT("location"));
	locationAsLoaded.load_from_xml(_node);

	if (IO::XML::Node const * node = _node->first_child_named(TXT("normal")))
	{
		normal = Vector3::zero;
		normal.access().load_from_xml(node);
	}

	// to replace
	// 		<node id="headTop"><wheresMyPoint><restore from="headTop"/></wheresMyPoint></node>
	// with
	// 		<node id="headTop" restore="headTop"/>
	if (IO::XML::Attribute const * attr = _node->get_attribute(TXT("restore")))
	{
		wmpToolSet.access_tools().push_back(new WheresMyPoint::Restore(attr->get_as_name()));
	}

	if (IO::XML::Node const * node = _node->first_child_named(TXT("wheresMyPoint")))
	{
		result &= wmpToolSet.load_from_xml(node);
	}

	debugDraw = _node->get_bool_attribute_or_from_child_presence(TXT("debugDraw"), debugDraw);

	return result;
}

bool Node::prepare_for_generation(int _idx, EdgesAndSurfacesContext & _context, PrepareForGenerationLevel::Type _level) const
{
	bool result = true;
	
	if (_level == PrepareForGenerationLevel::Resolve)
	{
		inSurfaces.clear(); // get ready for surface's prepare for generation "resolve" level
	}
	if (_level == PrepareForGenerationLevel::NodeLocation)
	{
		location = locationAsLoaded;
		if (locationFromVar.is_set())
		{
			location = locationFromVar.get(_context.get_wmp_owner(), location);
		}

		mirrorXOf = _context.get_edges_and_surfaces_data()->find_node(mirrorXOfId);
		if (mirrorXOf)
		{
			location = mirrorXOf->location;
			todo_note(TXT("add mirror axis?"));
			location.x = -location.x;
		}

		mirrorYOf = _context.get_edges_and_surfaces_data()->find_node(mirrorYOfId);
		if (mirrorYOf)
		{
			location = mirrorYOf->location;
			todo_note(TXT("add mirror axis?"));
			location.y = -location.y;
		}

		mirrorZOf = _context.get_edges_and_surfaces_data()->find_node(mirrorZOfId);
		if (mirrorZOf)
		{
			location = mirrorZOf->location;
			todo_note(TXT("add mirror axis?"));
			location.z = -location.z;
		}

		collapseXOf = _context.get_edges_and_surfaces_data()->find_node(collapseXOfId);
		if (collapseXOf)
		{
			location = collapseXOf->location;
			location.x = 0.0f;
		}

		addTo = _context.get_edges_and_surfaces_data()->find_node(addToId);
		if (addTo)
		{
			location = addTo->location;
		}

		if (!wmpToolSet.is_empty())
		{
			WheresMyPoint::Context wmpContext(&_context); // use this context as we will be accessing points!
			wmpContext.set_random_generator(_context.access_element_instance().access_context().access_random_generator()); 
			wmpContext.access_random_generator().advance_seed(_idx, _idx);
			Vector3 preWMPLocation = location;
			result &= wmpToolSet.update<Vector3>(location, wmpContext);
			if (addTo)
			{
				location = preWMPLocation + location;
			}
		}
	}
	if (_level == PrepareForGenerationLevel::FlatNormals)
	{
		totalNormal = Vector3::zero;
		for_every_const_ptr(inSurface, inSurfaces)
		{
			totalNormal += inSurface->calculate_flat_normal_for(this) * inSurface->calculate_size_for(this);
		}
		totalNormal.normalise();
		build_normals(_context.access_element_instance(), 
			flatNormals,
			[this](Name const & _surfaceGroup) { return has_flat_normal(_surfaceGroup); },
			[this](Surface const * _surface) { return _surface->calculate_flat_normal_for(this); },
			[](Plane & _plane) { /* no adjustments */ },
			[](Vector3 & _normal) { _normal.normalise(); }
		);
	}
	if (_level == PrepareForGenerationLevel::ExtrudeNormals)
	{
		build_normals(_context.access_element_instance(), 
			extrudeNormals,
			[this](Name const & _surfaceGroup) { return has_extrude_normal(_surfaceGroup); },
			[this](Surface const * _surface) { return _surface->calculate_extrude_normal_for(this); },
			[](Plane & _plane) { _plane = _plane.get_adjusted_along_normal(1.0f); },
			[](Vector3 & _normal) { /* do not normalise */ }
			);
	}

	return result;
}

void Node::build_normals(ElementInstance & _instance,
	Array<Normal> & _normals,
	std::function<bool(Name const & _surfaceGroup)> _has_normal,
	std::function<Vector3(Surface const * _surface)> _calculate_normal,
	std::function<void(Plane & _plane)> _adjust_planes_for_normal_calculation,
	std::function<void(Vector3 & _normal)> _final_normal_normalisation) const
{
	_normals.clear();

	// build normal map
	for_every_const_ptr(inSurface, inSurfaces)
	{
		for_every_const(surfaceGroup, inSurface->inSurfaceGroups)
		{
			if (!_has_normal(*surfaceGroup))
			{
				_normals.push_back(Normal(*surfaceGroup));
			}
		}
	}

	for_every(normal, _normals)
	{
		// try to find all unique normals (per each surface group)
		ARRAY_PREALLOC_SIZE(Vector3, uniqueNormals, inSurfaces.get_size());
		for_every_const_ptr(inSurface, inSurfaces)
		{
			if (!inSurface->inSurfaceGroups.does_contain(normal->surfaceGroup))
			{
				continue;
			}
			Vector3 calculatedNormal = _calculate_normal(inSurface);
			if (calculatedNormal.is_zero())
			{
				todo_note(TXT("[demo hack] commented out, check this later"));
				//error_generating_mesh(_instance, TXT("node %S surface %S has normal of 0"), id.to_char(), inSurface->id.to_char());
			}
			if (!calculatedNormal.is_zero())
			{
				bool unique = true;
				for_every(uniqueNormal, uniqueNormals)
				{
					if (Vector3::dot(*uniqueNormal, calculatedNormal) > 1.0f - MARGIN)
					{
						unique = false;
						break;
					}
				}
				if (unique)
				{
					uniqueNormals.push_back(calculatedNormal);
				}
			}
		}

		if (uniqueNormals.get_size() == 0)
		{
			// there's something really bad happening, just provide default fallback
			normal->normal = Vector3::zAxis;
		}
		else if (uniqueNormals.get_size() == 1)
		{
			// just one? use it then!
			normal->normal = uniqueNormals[0];
		}
		else if (uniqueNormals.get_size() == 2)
		{
			// if two, intersection of two planes gives line, we need to find point on that line that is closest to our location
			// and that would be also point on plane that has same normal as cross product of uniqueNormals
			Plane normalPlanes[2];
			normalPlanes[0] = Plane(uniqueNormals[0], location);
			normalPlanes[1] = Plane(uniqueNormals[1], location);
			_adjust_planes_for_normal_calculation(normalPlanes[0]);
			_adjust_planes_for_normal_calculation(normalPlanes[1]);
			if (Vector3::dot(normalPlanes[0].get_normal(), normalPlanes[1].get_normal()) > 1.0f - MARGIN)
			{
				// both are the same or almost the same, just use their normals
				normal->normal = (normalPlanes[0].get_normal() + normalPlanes[1].get_normal()).normal();
			}
			else if ((normalPlanes[0].get_normal() + normalPlanes[1].get_normal()).length() < MARGIN)
			{
#ifdef AN_DEVELOPMENT
				normal->unsafeReason = TXT("normals point in opposite directions");
#endif
				normal->normal = normalPlanes[0].get_normal();
			}
			else
			{
				// calculate third plane that will be perpendicular to 0 and 1 and will be going through our node
				// somewhere on this plane there is point where both planes cross
				// that point will help us finding normal
				Plane third = Plane(Vector3::cross(normalPlanes[0].get_normal(), normalPlanes[1].get_normal()).normal(), location);
				Vector3 meetingPoint = Plane::intersect(normalPlanes[0], normalPlanes[1], third);
				normal->normal = meetingPoint - location;
			}
		}
		else
		{
			normal->normal = Vector3::zero;
			float count = 0.0f;
			// if we have three, it will be perfect. but if more we have to use approximation - we will calculate for all consequetive threes and calculate average of results
			int iterationCount = uniqueNormals.get_size() == 3 ? 1 : uniqueNormals.get_size();
			for (int i = 0; i < iterationCount; ++i)
			{
				for (int j = 0; j < 3; ++j)
				{
					int p = (i + j) % uniqueNormals.get_size();
					Plane normalPlanes[3];
					for (int n = 0; n < 3; ++n)
					{
						int pn = (p + n) % uniqueNormals.get_size();
						normalPlanes[n] = Plane(uniqueNormals[pn], location);
						_adjust_planes_for_normal_calculation(normalPlanes[n]);
					}
					if (Plane::can_intersect(normalPlanes[0], normalPlanes[1], normalPlanes[2]))
					{
						Vector3 meetingPoint = Plane::intersect(normalPlanes[0], normalPlanes[1], normalPlanes[2]);
						normal->normal += (meetingPoint - location);
						count += 1.0f;
					}
					else
					{
#ifdef AN_DEVELOPMENT
						normal->unsafeReason = TXT("some planes (created with normals) can't intersect, that set is dropped");
#endif
					}
				}
			}
			if (count != 0.0f)
			{
				normal->normal = normal->normal / count;
			}
			else
			{
#ifdef AN_DEVELOPMENT
				normal->unsafeReason = TXT("no normal could be calculated, maybe normals couldn't intersect? but maybe that's on purpose as it could be avoid with surface groups");
#endif
			}
		}

		_final_normal_normalisation(normal->normal);
	}
}

bool Node::has_flat_normal(Name const & _surfaceGroup) const
{
	for_every_const(flatNormal, flatNormals)
	{
		if (flatNormal->surfaceGroup == _surfaceGroup)
		{
			return true;
		}
	}
	return false;
}
	
bool Node::has_extrude_normal(Name const & _surfaceGroup) const
{
	for_every_const(extrudeNormal, extrudeNormals)
	{
		if (extrudeNormal->surfaceGroup == _surfaceGroup)
		{
			return true;
		}
	}
	return false;
}

Vector3 const * Node::find_flat_normal(ElementInstance& _instance, Name const & _surfaceGroup) const
{
	for_every_const(flatNormal, flatNormals)
	{
		if (flatNormal->surfaceGroup == _surfaceGroup)
		{
#ifdef AN_DEVELOPMENT
			if (flatNormal->unsafeReason)
			{
				error_generating_mesh(_instance, TXT("unsafe flat normal for %S! reason: %S"), id.to_char(), flatNormal->unsafeReason);
			}
#endif
			return &flatNormal->normal;
		}
	}
	return nullptr;
}

Vector3 const * Node::find_extrude_normal(ElementInstance& _instance, Name const & _surfaceGroup) const
{
	for_every_const(extrudeNormal, extrudeNormals)
	{
		if (extrudeNormal->surfaceGroup == _surfaceGroup)
		{
#ifdef AN_DEVELOPMENT
			if (extrudeNormal->unsafeReason)
			{
				error_generating_mesh(_instance, TXT("unsafe extrude normal for %S! reason: %S"), id.to_char(), extrudeNormal->unsafeReason);
			}
#endif
			return &extrudeNormal->normal;
		}
	}
	return nullptr;
}

//

bool Edge::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;

	id = _node->get_name_attribute(TXT("id"), id);
	if (!id.is_valid())
	{
		error_loading_xml(_node, TXT("edge without id"));
		result = false;
	}

	mirrorOfId = _node->get_name_attribute(TXT("mirrorOf"), mirrorOfId);

	fromNodeId = _node->get_name_attribute(TXT("from"), fromNodeId);
	toNodeId = _node->get_name_attribute(TXT("to"), toNodeId);

	useNormals = _node->get_bool_attribute(TXT("useNormals"), useNormals);

	atFromTowardsNodeId = _node->get_name_attribute(TXT("bothTowards"), atFromTowardsNodeId);
	atToTowardsNodeId = _node->get_name_attribute(TXT("bothTowards"), atToTowardsNodeId);
	atFromTowardsNodeId = _node->get_name_attribute(TXT("atFromTowards"), atFromTowardsNodeId);
	atToTowardsNodeId = _node->get_name_attribute(TXT("atToTowards"), atToTowardsNodeId);

	atFromUseDirFromNodeId = _node->get_name_attribute(TXT("atFromUseDirFrom"), atFromUseDirFromNodeId);
	atFromUseDirToNodeId = _node->get_name_attribute(TXT("atFromUseDirTo"), atFromUseDirToNodeId);
	atToUseDirFromNodeId = _node->get_name_attribute(TXT("atToUseDirFrom"), atToUseDirFromNodeId);
	atToUseDirToNodeId = _node->get_name_attribute(TXT("atToUseDirTo"), atToUseDirToNodeId);

	roundSeparationAtFrom = _node->get_bool_attribute_or_from_child_presence(TXT("roundSeparationAtFrom"), roundSeparationAtFrom);
	roundSeparationAtTo = _node->get_bool_attribute_or_from_child_presence(TXT("roundSeparationAtTo"), roundSeparationAtTo);
	roundSeparationAtFrom = _node->get_bool_attribute_or_from_child_presence(TXT("roundSeparation"), roundSeparationAtFrom);
	roundSeparationAtTo = _node->get_bool_attribute_or_from_child_presence(TXT("roundSeparation"), roundSeparationAtTo);

	result &= edgeDef.load_from_xml(_node);

	// allow getting fromNodeId and toNodeId from id, parsing: "from -> to ; comment" or "from -> controlPoint | controlPoint <- to ; comment"
	if (!fromNodeId.is_valid() && !toNodeId.is_valid())
	{
		String rawId = id.to_string();

		// separate from comment (if present)
		List<String> tokens;
		rawId.split(String(TXT(";")), tokens, 1);
		String const & nodeToken = tokens.is_empty() ? rawId : tokens.get_first();

		// break into node names
		if (nodeToken.find_first_of('|') != NONE)
		{
			List<String> nodeTokens;
			nodeToken.split(String(TXT("|")), nodeTokens);
			if (nodeTokens.get_size() == 2)
			{
				{
					List<String> fromTokens;
					nodeTokens.get_first().split(String(TXT("->")), fromTokens);
					if (fromTokens.get_size() == 2)
					{
						fromNodeId = Name(fromTokens.get_first().trim());
						atFromTowardsNodeId = Name(fromTokens.get_last().trim());
					}
					else
					{
						fromNodeId = Name(nodeTokens.get_first().trim());
					}
				}
				{
					List<String> toTokens;
					nodeTokens.get_last().split(String(TXT("<-")), toTokens);
					if (toTokens.get_size() == 2)
					{
						toNodeId = Name(toTokens.get_last().trim());
						atToTowardsNodeId = Name(toTokens.get_first().trim());
					}
					else
					{
						toNodeId = Name(nodeTokens.get_last().trim());
					}
				}
			}
		}
		else
		{
			List<String> nodeTokens;
			nodeToken.split(String(TXT("->")), nodeTokens);
			if (nodeTokens.get_size() == 2)
			{
				fromNodeId = Name(nodeTokens.get_first().trim());
				toNodeId = Name(nodeTokens.get_last().trim());
			}
		}
	}

	if (!fromNodeId.is_valid())
	{
		error_loading_xml(_node, TXT("node without from"));
		result = false;
	}
	if (!toNodeId.is_valid())
	{
		error_loading_xml(_node, TXT("node without to"));
		result = false;
	}

	/*
	if (IO::XML::Node const * curveOffsetNode = _node->first_child_named(TXT("curveOffset")))
	{
		// make sure we have some default value set in them just in case we don't set anything!
		curveOffsetAtFrom.unset_with(Vector3::zero);
		curveOffsetAtFrom.load_from_xml(curveOffsetNode->first_child_named(TXT("from")));
		curveOffsetAtTo.unset_with(Vector3::zero);
		curveOffsetAtTo.load_from_xml(curveOffsetNode->first_child_named(TXT("to")));
	}
	*/

	result &= atFromDir.load_from_xml(_node, TXT("atFromDir"));
	result &= atFromDirNeg.load_from_xml(_node, TXT("atFromDirNeg"));
	result &= atFromNormal.load_from_xml(_node, TXT("atFromNormal"));
	result &= atToDir.load_from_xml(_node, TXT("atToDir"));
	result &= atToDirNeg.load_from_xml(_node, TXT("atToDirNeg"));
	result &= atToNormal.load_from_xml(_node, TXT("atToNormal"));

	debugDraw = _node->get_bool_attribute_or_from_child_presence(TXT("debugDraw"), debugDraw);

	return result;
}

bool Edge::prepare_for_generation(EdgesAndSurfacesContext & _context, PrepareForGenerationLevel::Type _level) const
{
	bool result = true;

	if (_level == PrepareForGenerationLevel::Resolve)
	{
		mirrorOf = _context.get_edges_and_surfaces_data()->find_edge(mirrorOfId);

		fromNode = _context.get_edges_and_surfaces_data()->find_node(fromNodeId);
		toNode = _context.get_edges_and_surfaces_data()->find_node(toNodeId);

		atFromTowardsNode = _context.get_edges_and_surfaces_data()->find_node(atFromTowardsNodeId);
		atToTowardsNode = _context.get_edges_and_surfaces_data()->find_node(atToTowardsNodeId);
		atFromUseDirFromNode = _context.get_edges_and_surfaces_data()->find_node(atFromUseDirFromNodeId);
		atFromUseDirToNode = _context.get_edges_and_surfaces_data()->find_node(atFromUseDirToNodeId);
		atToUseDirFromNode = _context.get_edges_and_surfaces_data()->find_node(atToUseDirFromNodeId);
		atToUseDirToNode = _context.get_edges_and_surfaces_data()->find_node(atToUseDirToNodeId);

		inSurfaces.clear(); // get ready for surface's prepare for generation "resolve" level
	}
	if (_level == PrepareForGenerationLevel::EdgeCurves)
	{
		if (fromNode && toNode)
		{
			curve.p0 = fromNode->location;
			curve.p3 = toNode->location;
			curve.make_linear();
			bool allowRoundSeparationAtFrom = roundSeparationAtFrom;
			bool allowRoundSeparationAtTo = roundSeparationAtTo;
			if (useNormals)
			{
				if (fromNode->normal.is_set())
				{
					Vector3 normal = fromNode->normal.get();
					Vector3 dir = (curve.p3 - curve.p0);
					Vector3 side = Vector3::cross(dir, normal);
					dir = (Vector3::cross(normal, side)).normal() * (dir.length() / 3.0f) * normal.length();
					curve.p1 = curve.p0 + dir;
				}
				if (toNode->normal.is_set())
				{
					Vector3 normal = toNode->normal.get();
					Vector3 dir = (curve.p0 - curve.p3);
					Vector3 side = Vector3::cross(dir, normal);
					dir = (Vector3::cross(normal, side)).normal() * (dir.length() / 3.0f) * normal.length();
					curve.p2 = curve.p3 + dir;
				}
			}
			if (atFromTowardsNode)
			{
				if (allowRoundSeparationAtFrom)
				{
					curve.p1 = curve.p0 + (atFromTowardsNode->location - curve.p0).normal();
				}
				else
				{
					curve.p1 = atFromTowardsNode->location;
				}
			}
			if (atToTowardsNode)
			{
				if (allowRoundSeparationAtTo)
				{
					curve.p2 = curve.p3 + (atToTowardsNode->location - curve.p3).normal();
				}
				else
				{
					curve.p2 = atToTowardsNode->location;
				}
			}
			if (atFromUseDirToNode)
			{
				if (atFromUseDirFromNode)
				{
					if (allowRoundSeparationAtFrom)
					{
						curve.p1 = curve.p0 + (atFromUseDirToNode->location - atFromUseDirFromNode->location).normal();
					}
					else
					{
						curve.p1 = curve.p0 + (atFromUseDirToNode->location - atFromUseDirFromNode->location);
					}
				}
				else
				{
					if (allowRoundSeparationAtFrom)
					{
						curve.p1 = curve.p0 + (atFromUseDirToNode->location - curve.p3).normal();
					}
					else
					{
						curve.p1 = atFromUseDirToNode->location;
					}
				}
			}
			if (atToUseDirToNode)
			{
				if (atToUseDirFromNode)
				{
					if (allowRoundSeparationAtTo)
					{
						curve.p2 = curve.p3 + (atToUseDirToNode->location - atToUseDirFromNode->location).normal();
					}
					else
					{
						curve.p2 = curve.p3 + (atToUseDirToNode->location - atToUseDirFromNode->location);
					}
				}
				else
				{
					if (allowRoundSeparationAtTo)
					{
						curve.p2 = curve.p3 + (atToUseDirToNode->location - curve.p3).normal();
					}
					else
					{
						curve.p2 = atToUseDirToNode->location;
					}
				}
			}
			if (atFromDir.is_set())
			{
				curve.p1 = curve.p0 + atFromDir.get(_context.get_wmp_owner(), curve.p1 - curve.p0);
			}
 			if (atFromDirNeg.is_set())
			{
				curve.p1 = curve.p0 - atFromDirNeg.get(_context.get_wmp_owner(), -(curve.p1 - curve.p0));
			}
			if (atToDir.is_set())
			{
				curve.p2 = curve.p3 + atToDir.get(_context.get_wmp_owner(), curve.p2 - curve.p3);
			}
			if (atToDirNeg.is_set())
			{
				curve.p2 = curve.p3 - atToDirNeg.get(_context.get_wmp_owner(), -(curve.p2 - curve.p3));
			}
			if (atFromNormal.is_set())
			{
				curve.p1 = curve.p0 + (curve.p1 - curve.p0).drop_using(atFromNormal.get(_context.get_wmp_owner(), Vector3::zero));
			}
			if (atToNormal.is_set())
			{
				curve.p2 = curve.p3 + (curve.p2 - curve.p3).drop_using(atToNormal.get(_context.get_wmp_owner(), Vector3::zero));
			}
			/*
			if (curveOffsetAtFrom.is_set())
			{
				curve.p1 = curve.p0 + curveOffsetAtFrom.get();
				allowRoundSeparationAtFrom = false;
			}
			if (curveOffsetAtTo.is_set())
			{
				curve.p2 = curve.p3 + curveOffsetAtTo.get();
				allowRoundSeparationAtTo = false;
			}
			*/
			float roundSeparation = curve.calculate_round_separation();
			if ((allowRoundSeparationAtFrom && (atFromTowardsNode || edgeDef.should_allow_round_separation())) || edgeDef.should_force_round_separation())
			{
				curve.p1 = curve.p0 + (curve.p1 - curve.p0).normal() * roundSeparation;
			}
			if ((allowRoundSeparationAtTo && (atToTowardsNode || edgeDef.should_allow_round_separation())) || edgeDef.should_force_round_separation())
			{
				curve.p2 = curve.p3 + (curve.p2 - curve.p3).normal() * roundSeparation;
			}
			if (curve.is_linear())
			{
				curve.make_linear();
			}
		}
		else
		{
			if (!fromNode)
			{
				error_generating_mesh(_context.access_element_instance(), TXT("could not find from node \"%S\""), fromNodeId.to_char());
				result = false;
			}
			if (!toNode)
			{
				error_generating_mesh(_context.access_element_instance(), TXT("could not find to node \"%S\""), toNodeId.to_char());
				result = false;
			}
		}

		if (mirrorOf)
		{
			edgeDef = mirrorOf->edgeDef;
			curve = mirrorOf->curve;
			todo_note(TXT("add mirror axis?"));
			curve.p0.x = -curve.p0.x;
			curve.p1.x = -curve.p1.x;
			curve.p2.x = -curve.p2.x;
			curve.p3.x = -curve.p3.x;
		}
	}
	if (_level == PrepareForGenerationLevel::AutoSmoothEdges)
	{
		float prevCurveLength = curve.length();
		for (int i = 0; i < 2; ++i)
		{
			/*
			if ((i == 0 && (curveOffsetAtFrom.is_set() || atFromTowardsNode)) ||
				(i == 1 && (curveOffsetAtTo.is_set() || atToTowardsNode)))
			{
				// set by hand, keep them as they are
				continue;
			}
			*/
			Node const * node = i == 0 ? fromNode : toNode;
			if (!node)
			{
				continue;
			}
			Vector3 firstNormalFound = Vector3::zero;
			Vector3 normal = Vector3::zero;
			float weight = 0.0f;
			bool doSmoothing = true;
			Name autoSmoothSurfaceGroup = edgeDef.get_auto_smooth_surface_group();
			float autoSmoothNormalDotLimit = max(node->nodeDef.get_auto_smooth_normal_dot_limit(), edgeDef.get_auto_smooth_normal_dot_limit());
			if (autoSmoothNormalDotLimit > 1.0f)
			{
				// won't smooth anything
				continue;
			}
			// go through all (2 tops!) surfaces for edge and find common normal (check if it is within dot limit!)
			for_every_const_ptr(inSurface, inSurfaces)
			{
				if (!inSurface->inSurfaceGroups.does_contain(autoSmoothSurfaceGroup))
				{
					continue;
				}
				Vector3 inSurfaceNormal = inSurface->calculate_flat_normal_for(node);
				float inSurfaceSize = inSurface->calculate_size_for(node);
				normal += inSurfaceNormal * inSurfaceSize;
				weight += inSurfaceSize;
				if (firstNormalFound.is_zero())
				{
					firstNormalFound = inSurfaceNormal;
				}
				else
				{
					if (Vector3::dot(firstNormalFound, inSurfaceNormal) < autoSmoothNormalDotLimit)
					{
						doSmoothing = false;
					}
				}
			}
			if (doSmoothing && weight > 0.0f)
			{
				Vector3 normalRef = (normal / weight).normal();
				normal = Vector3::zero;
				weight = 0.0f;
				// get normals from all surfaces for that node that are fine within dot limit
				for_every_const_ptr(inSurface, node->inSurfaces)
				{
					if (!inSurface->inSurfaceGroups.does_contain(autoSmoothSurfaceGroup))
					{
						continue;
					}
					Vector3 inSurfaceNormal = inSurface->calculate_flat_normal_for(node);
					float inSurfaceSize = inSurface->calculate_size_for(node);
					if (Vector3::dot(normalRef, inSurfaceNormal) >= autoSmoothNormalDotLimit)
					{
						normal += inSurfaceNormal * inSurfaceSize;
						weight += inSurfaceSize;
					}
				}
				if (weight > 0.0f)
				{
					// store direction in curve
					normal = (normal / weight).normal();
					Vector3 dir = (i == 0 ? curve.p1 - curve.p0 : curve.p2 - curve.p3).normal();
					Vector3 side = Vector3::cross(dir, normal);
					dir = (Vector3::cross(normal, side)).normal();
					if (i == 0)
					{
						curve.p1 = curve.p0 + dir * prevCurveLength * 0.33f;
					}
					else
					{
						curve.p2 = curve.p3 + dir * prevCurveLength * 0.33f;
					}
				}
			}
		}
		// apply round separation
		if (edgeDef.should_allow_round_separation() || edgeDef.should_force_round_separation())
		{
			float roundSeparation = curve.calculate_round_separation();
			if (roundSeparationAtFrom || edgeDef.should_force_round_separation())
			{
				curve.p1 = curve.p0 + (curve.p1 - curve.p0).normal() * roundSeparation;
			}
			if (roundSeparationAtTo || edgeDef.should_force_round_separation())
			{
				curve.p2 = curve.p3 + (curve.p2 - curve.p3).normal() * roundSeparation;
			}
		}
		if (mirrorOf)
		{
			curve = mirrorOf->curve;
			todo_note(TXT("add mirror axis?"));
			curve.p0.x = -curve.p0.x;
			curve.p1.x = -curve.p1.x;
			curve.p2.x = -curve.p2.x;
			curve.p3.x = -curve.p3.x;
		}
	}

	return result;
}

//

bool Edge::Ref::load_from_xml(IO::XML::Node const * _node)
{
	id = _node->get_name_attribute(TXT("id"));
	if (!id.is_valid())
	{
		error_loading_xml(_node, TXT("could not load id for edge ref in surface"));
		return false;
	}
	reversed = _node->get_bool_attribute(TXT("reversed"), false); // only important for first edge
	reversed = _node->get_bool_attribute(TXT("reverse"), reversed);
	return true;
}

bool Edge::Ref::prepare_for_generation(EdgesAndSurfacesContext & _context) const
{
	bool result = true;

	edge = _context.get_edges_and_surfaces_data()->find_edge(id);
	if (!edge)
	{
		error_generating_mesh(_context.access_element_instance(), TXT("could not find edge \"%S\" for edge ref"), id.to_char());
		result = false;
	}

	return result;
}

//

bool Surface::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;

	id = _node->get_name_attribute(TXT("id"), id);
	if (!id.is_valid())
	{
		error_loading_xml(_node, TXT("surface without id"));
		result = false;
	}

	debugDraw = _node->get_bool_attribute_or_from_child_presence(TXT("debugDraw"), debugDraw);

	for (int i = 0; i < 2; ++i)
	{
		tchar const * xmlName = i == 0 ? TXT("inSurfaceGroup") : TXT("inSurfaceGroups");
		String isg = _node->get_string_attribute(xmlName);
		if (!isg.is_empty())
		{
			List<String> tokens;
			isg.split(String::comma(), tokens);
			for_every_const(token, tokens)
			{
				inSurfaceGroups.push_back_unique(Name(token->trim()));
			}
		}
		for_every(node, _node->children_named(xmlName))
		{
			inSurfaceGroups.push_back_unique(Name(node->get_text().trim()));
		}
	}
	inSurfaceGroups.push_back(Name::invalid());

	surfaceDef.load_from_xml(_node);

	surfaceCreationMethod = BezierSurfaceCreationMethod::Roundly;
	if (_node->first_child_named(TXT("node")))
	{
		for_every(nodeNode, _node->children_named(TXT("node")))
		{
			Name nodeName = nodeNode->get_name_attribute(TXT("id"));
			if (nodeName.is_valid())
			{
				if (nodeNode->get_bool_attribute(TXT("bendAlongThis")))
				{
					surfaceCreationMethod = (nodesLoaded.get_size() % 2) == 0 ? BezierSurfaceCreationMethod::AlongU : BezierSurfaceCreationMethod::AlongV;
				}
				nodesLoaded.push_back(nodeName);
			}
		}

		if (nodesLoaded.get_size() > 4)
		{
			error_loading_xml(_node, TXT("too many nodes for surface \"%S\""), id.to_char());
			result = false;
		}
		if (nodesLoaded.get_size() < 3)
		{
			error_loading_xml(_node, TXT("not enough nodes for surface \"%S\""), id.to_char());
			result = false;
		}
	}
	else
	{
		for_every(edgeNode, _node->children_named(TXT("edge")))
		{
			Edge::Ref edgeRef;
			if (edgeRef.load_from_xml(edgeNode))
			{
				if (edgeNode->get_bool_attribute(TXT("bendAlongThis")))
				{
					surfaceCreationMethod = (edgesLoaded.get_size() % 2) == 0 ? BezierSurfaceCreationMethod::AlongU : BezierSurfaceCreationMethod::AlongV;
				}
				edgesLoaded.push_back(edgeRef);
			}
		}

		if (edgesLoaded.get_size() > 4)
		{
			error_loading_xml(_node, TXT("too many edges for surface \"%S\""), id.to_char());
			result = false;
		}
		if (edgesLoaded.get_size() < 3)
		{
			error_loading_xml(_node, TXT("not enough edges for surface \"%S\""), id.to_char());
			result = false;
		}
	}

	return result;
}

bool check_edge_chain(ElementInstance & _instance, Edge::Ref const * prevEdge, Edge::Ref const * edge, bool justCheck = false, bool silent = false)
{
	bool result = true;
	if (prevEdge &&
		prevEdge->edge &&
		edge->edge)
	{
		Node const * endedWithNode = prevEdge->reversed ? prevEdge->edge->fromNode : prevEdge->edge->toNode;
		if (endedWithNode == edge->edge->fromNode)
		{
			if (justCheck)
			{
				if (edge->reversed)
				{
					if (!silent)
					{
						error_generating_mesh(_instance, TXT("won't build chain with these edges (\"%S\" and \"%S\") - reverse issue!"), prevEdge->edge->id.to_char(), edge->edge->id.to_char());
					}
					result = false;
				}
			}
			else
			{
				edge->reversed = false;
			}
		}
		else if (endedWithNode == edge->edge->toNode)
		{
			if (justCheck)
			{
				if (! edge->reversed)
				{
					if (!silent)
					{
						error_generating_mesh(_instance, TXT("won't build chain with these edges (\"%S\" and \"%S\") - reverse issue!"), prevEdge->edge->id.to_char(), edge->edge->id.to_char());
					}
					result = false;
				}
			}
			else
			{
				edge->reversed = true;
			}
		}
		else
		{
			if (!silent)
			{
				error_generating_mesh(_instance, TXT("won't build chain with these edges (\"%S\" and \"%S\")!"), prevEdge->edge->id.to_char(), edge->edge->id.to_char());
			}
			result = false;
		}
	}

	return result;
}

BezierCurve<Vector2> Surface::calculate_uv_curve_for(Edge const * _edge) const
{
	BezierCurve<Vector2> result;
	result.p0 = Vector2::zero;
	result.p3 = Vector2::zero;
	if (edges.get_size() == 3)
	{
		if (edges[0].edge == _edge)
		{
			result.p0 = Vector2(0.0f, 0.0f);
			result.p3 = Vector2(1.0f, 0.0f);
			result = result.reversed(edges[0].reversed);
		}
		if (edges[1].edge == _edge)
		{
			result.p0 = Vector2(1.0f, 0.0f);
			result.p3 = Vector2(0.0f, 1.0f);
			result = result.reversed(edges[1].reversed);
		}
		if (edges[2].edge == _edge)
		{
			result.p0 = Vector2(0.0f, 1.0f);
			result.p3 = Vector2(0.0f, 0.0f);
			result = result.reversed(edges[2].reversed);
		}
	}
	else if (edges.get_size() == 4)
	{
		if (edges[0].edge == _edge)
		{
			result.p0 = Vector2(0.0f, 0.0f);
			result.p3 = Vector2(1.0f, 0.0f);
			result = result.reversed(edges[0].reversed);
		}
		if (edges[1].edge == _edge)
		{
			result.p0 = Vector2(1.0f, 0.0f);
			result.p3 = Vector2(1.0f, 1.0f);
			result = result.reversed(edges[1].reversed);
		}
		if (edges[2].edge == _edge)
		{
			result.p0 = Vector2(1.0f, 1.0f);
			result.p3 = Vector2(0.0f, 1.0f);
			result = result.reversed(edges[2].reversed);
		}
		if (edges[3].edge == _edge)
		{
			result.p0 = Vector2(0.0f, 1.0f);
			result.p3 = Vector2(0.0f, 0.0f);
			result = result.reversed(edges[3].reversed);
		}
	}
	else
	{
		todo_important(TXT("implement_ others"));
	}

	result.make_linear();
	return result;
}

Vector3 Surface::calculate_normal_at_uv(Vector2 const & _uv) const
{
	return bezierSurface.calculate_normal_at(_uv);
}

Vector3 Surface::calculate_flat_normal_for(Node const * _node) const
{
	// find both edges that this node belongs to
	// and calculate directions from this node towards other end of edge
	// use those directions to calculate normal of surface for that node
	// note: this makes sense for 4 vertices,
	//		 for triangles for each node it will give same result always
	Vector3 startsDir = Vector3::zero;
	Vector3 endsDir = Vector3::zero;
	for_every_const(edge, edges)
	{
		if (!edge->edge)
		{
			continue;
		}
		Node const * startingNode = edge->edge->fromNode;
		Node const * endingNode = edge->edge->toNode;
		if (!startingNode || !endingNode)
		{
			continue;
		}
		if (edge->reversed)
		{
			swap(startingNode, endingNode);
		}
		if (_node == startingNode)
		{
			startsDir = (endingNode->location - startingNode->location).normal();
		}
		if (_node == endingNode)
		{
			endsDir = (startingNode->location - endingNode->location).normal();
		}
	}

	return Vector3::cross(endsDir, startsDir).normal();
}

Vector3 Surface::calculate_extrude_normal_for(Node const * _node) const
{
	// find both edges that this node belongs to
	// and calculate directions from this node towards first point of curve
	// use those directions to calculate normal of surface for that node
	// note: contrary to calculate_flat_normal_for, this may give different results
	//		 even for surfaces with 3 edges
	Vector3 startsDir = Vector3::zero;
	Vector3 endsDir = Vector3::zero;
	for_every_const(edge, edges)
	{
		if (!edge->edge)
		{
			continue;
		}
		Node const * startingNode = edge->edge->fromNode;
		Node const * endingNode = edge->edge->toNode;
		if (!startingNode || !endingNode)
		{
			continue;
		}
		BezierCurve<Vector3> curve = edge->edge->curve.reversed(edge->reversed);
		if (edge->reversed)
		{
			swap(startingNode, endingNode);
		}
		if (_node == startingNode)
		{
			startsDir = (curve.p1 - curve.p0).normal();
		}
		if (_node == endingNode)
		{
			endsDir = (curve.p2 - curve.p3).normal();
		}
	}

	return Vector3::cross(endsDir, startsDir).normal();
}

float Surface::calculate_size_for(Node const * _node) const
{
	// calculate using fans for triangles starting at node
	float size = 0.0f;
	for_every_const(edge, edges)
	{
		if (!edge->edge)
		{
			continue;
		}
		Node const * startingNode = edge->edge->fromNode;
		Node const * endingNode = edge->edge->toNode;
		if (!startingNode || !endingNode)
		{
			continue;
		}
		if (startingNode == _node || endingNode == _node)
		{
			// calculate only for edges further away from node
			continue;
		}
		// note: use Heron's formula, https://en.wikipedia.org/wiki/Heron%27s_formula
		float a = (_node->location - startingNode->location).length();
		float b = (_node->location - endingNode->location).length();
		float c = (endingNode->location - startingNode->location).length();

		float part = max(0.0f, (a + b + c) * (-a + b +c) * (a - b + c) * (a + b - c));
		size += 0.25f * sqrt(part);
	}

	return size;
}

Plane Surface::calculate_plane_for(Node const * _node) const
{
	return Plane(calculate_flat_normal_for(_node), _node->location);
}

bool Surface::prepare_for_generation(EdgesAndSurfacesContext & _context, PrepareForGenerationLevel::Type _level) const
{
	bool result = true;

	if (PrepareForGenerationLevel::should_resolve_auto_generated(isAutoGenerated, _level))
	{
		edges.clear();
		if (!nodesLoaded.is_empty())
		{
			for_count(int, i, nodesLoaded.get_size())
			{
				Name const & startsAtNodeId = nodesLoaded[i];
				Name const & endsAtNodeId = nodesLoaded[(i + 1) % nodesLoaded.get_size()];
				Edge::Ref foundEdge;
				for_every_ptr(edge, _context.get_edges_and_surfaces_data()->edges)
				{
					if (edge->fromNodeId == startsAtNodeId &&
						edge->toNodeId == endsAtNodeId)
					{
						foundEdge.edge = edge;
						foundEdge.id = edge->id;
						foundEdge.reversed = false;
						break;
					}
					if (edge->fromNodeId == endsAtNodeId &&
						edge->toNodeId == startsAtNodeId)
					{
						foundEdge.edge = edge;
						foundEdge.id = edge->id;
						foundEdge.reversed = true;
						break;
					}
				}
				if (foundEdge.edge)
				{
					edges.push_back(foundEdge);
				}
				else
				{
					error_generating_mesh(_context.access_element_instance(), TXT("could not find edge for points \"%S\" and \"%S\""), startsAtNodeId.to_char(), endsAtNodeId.to_char());
					result = false;
				}
			};
		}
		else
		{
			edges = edgesLoaded;
		}

		for_every_const(edge, edges)
		{
			if (!edge->prepare_for_generation(_context))
			{
				result = false;
				continue;
			}
			if (edge->edge)
			{
				edge->edge->inSurfaces.push_back_unique(this);
				if (edge->edge->fromNode)
				{
					edge->edge->fromNode->inSurfaces.push_back_unique(this);
				}
				if (edge->edge->toNode)
				{
					edge->edge->toNode->inSurfaces.push_back_unique(this);
				}
			}
			else
			{
				error_generating_mesh(_context.access_element_instance(), TXT("could not find edge \"%S\""), edge->id.to_char());
				result = false;
			}
		}

		// order edges and check if that's right
		Edge::Ref const * prevEdge = nullptr;
		for_every_const(edge, edges)
		{
			result &= check_edge_chain(_context.access_element_instance(), prevEdge, edge);

			prevEdge = edge;
		}

		result &= check_edge_chain(_context.access_element_instance(), prevEdge, &edges.get_first(), true);
	}

	if (_level == PrepareForGenerationLevel::Surfaces)
	{
		ARRAY_STATIC(GatheredEdge, gatheredEdges, 4);
		for_every_const(edgeRef, edges)
		{
			if (edgeRef->edge)
			{
				GatheredEdge gatheredEdge;
				gatheredEdge.edge = edgeRef->edge;
				gatheredEdge.curve = edgeRef->edge->curve;
				// order it properly
				gatheredEdge.curve = gatheredEdge.curve.reversed(edgeRef->reversed);
				gatheredEdges.push_back(gatheredEdge);
			}
		}

		if (gatheredEdges.get_size() == 3)
		{
			bezierSurface = BezierSurface::create(gatheredEdges[0].curve, gatheredEdges[1].curve, gatheredEdges[2].curve);
		}
		else if (gatheredEdges.get_size() == 4)
		{
			bezierSurface = BezierSurface::create(gatheredEdges[0].curve, gatheredEdges[1].curve, gatheredEdges[2].curve, gatheredEdges[3].curve, surfaceCreationMethod);
		}
		else
		{
			error_generating_mesh(_context.access_element_instance(), TXT("couldn't generate surfaceMesh \"%S\", gatheredEdges count %i"), id.to_char(), gatheredEdges.get_size());
			result = false;
		}
	}

	return result;
}

//

bool EdgeMeshEdgeStoreOnCurve::load_from_xml(IO::XML::Node const * _node)
{
	bool result = true;
	asParameter = _node->get_name_attribute(TXT("as"), asParameter);
	if (!asParameter.is_valid())
	{
		error_loading_xml(_node, TXT("no \"as\" provided for store on curve"));
		result = false;
	}

	if (IO::XML::Node const * node = _node->first_child_named(TXT("location")))
	{
		type = EdgeMeshEdgeStoreType::Location;
		vector.load_from_xml(node);
	}
	else if (IO::XML::Node const * node = _node->first_child_named(TXT("vector")))
	{
		type = EdgeMeshEdgeStoreType::Vector;
		vector.load_from_xml(node);
	}
	// if nothing find, it will be just location of curve
	
	result &= onCurve.load_single_from_xml(_node);

	applyScale = _node->get_bool_attribute(TXT("applyScale"), applyScale);
	applyOffset = _node->get_bool_attribute(TXT("applyOffset"), applyOffset);

	return result;
}

//

int EdgeMeshSkinningInfo::compare(void const * _a, void const * _b)
{
	EdgeMeshSkinningInfo const * a = plain_cast<EdgeMeshSkinningInfo>(_a);
	EdgeMeshSkinningInfo const * b = plain_cast<EdgeMeshSkinningInfo>(_b);
	if (a->pt < b->pt)
	{
		return -1;
	}
	if (a->pt > b->pt)
	{
		return 1;
	}
	return 0;
}

float EdgeMeshSkinningInfo::at_to_pt(float _at, float _lengthSoFar, float _thisLength)
{
	return clamp((_at - _lengthSoFar) / max(SMALL_MARGIN, _thisLength), 0.0f, 1.0f);
}

EdgeMeshSkinningInfo EdgeMeshSkinningInfo::blend(EdgeMeshSkinningInfo const & _a, EdgeMeshSkinningInfo const & _b, float _pt)
{
	an_assert(_a.boneAWeight == 1.0f && _a.boneBWeight == 0.0f);
	an_assert(_b.boneAWeight == 1.0f && _b.boneBWeight == 0.0f);
	an_assert(_b.pt > _a.pt);

	float weight = clamp((_pt - _a.pt) / max(SMALL_MARGIN, _b.pt - _a.pt), 0.0f, 1.0f);

	EdgeMeshSkinningInfo result = _a;
	result.boneA = _a.boneA;
	result.boneB = _b.boneA;
	result.boneAWeight = 1.0f - weight;
	result.boneBWeight = weight;
	result.pt = _a.pt + _b.pt * weight;

	return result;
}

bool EdgeMeshSkinningInfo::load_single_from_xml(IO::XML::Node const * _node)
{
	bool result = true;

	pt = _node->get_float_attribute(TXT("pt"), pt);
	boneA.load_from_xml(_node, TXT("bone"));
	boneAWeight = 1.0f;
	boneB = Name::invalid();
	boneBWeight = 0.0f;

	return result;
}

bool EdgeMeshSkinningInfo::load_from_xml(IO::XML::Node const * _node, Array<EdgeMeshSkinningInfo> & _skinning, tchar const * const _childName)
{
	if (!_node)
	{
		return true;
	}
	bool result = true;
	for_every(node, _node->children_named(_childName))
	{
		EdgeMeshSkinningInfo si;
		result &= si.load_single_from_xml(node);
		_skinning.push_back(si);
	}
	return result;
}

//

bool EdgeMeshEdge::load_from_xml_as_edge(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;

	result &= edgeRef.load_from_xml(_node);
	result &= ShapeUtils::CrossSectionOnCurve::load_from_xml(_node, csOnCurve, TXT("crossSection"));
	result &= EdgeMeshSkinningInfo::load_from_xml(_node, skinning, TXT("skinning"));

	details = _node->get_float_attribute(TXT("details"), details);

	for (int i = 0; i < 2; ++i)
	{
		for_every(node, _node->children_named(i == 0 ? TXT("store") : TXT("storeGlobal")))
		{
			EdgeMeshEdgeStoreOnCurve store;
			store.globalStore = i == 1;
			if (store.load_from_xml(node))
			{
				storeOnCurve.push_back(store);
			}
			else
			{
				error_loading_xml(node, TXT("couldn't load store on curve"));
				result = false;
			}
		}
	}

	return result;
}

bool EdgeMeshEdge::load_from_xml_as_node(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;

	cornerRadius.load_from_xml(_node, TXT("cornerRadius"));
	cutCorner.load_from_xml(_node, TXT("cutCorner"));
	cornerCrossSectionId = _node->get_name_attribute(TXT("crossSectionId"), cornerCrossSectionId); // id conflicts with node's id
	ParserUtils::load_multiple_from_xml_into<ArrayStatic<Name, 8>, Name>(_node, TXT("crossSectionAltId"), cornerCrossSectionAltIds);
	cornerScaleFloat.load_from_xml(_node, TXT("scale"));
	cornerScale.load_from_xml(_node, TXT("scale"));

	cornerWeld = _node->get_bool_attribute_or_from_child_presence(TXT("weld"), cornerWeld);

	details = _node->get_float_attribute(TXT("details"), details);

	result &= surfacesContext.load_from_xml(_node);

	return result;
}

bool EdgeMeshEdge::prepare_for_generation(EdgesAndSurfacesContext & _context, PrepareForGenerationLevel::Type _level) const
{
	bool result = true;

	if (_level == PrepareForGenerationLevel::Resolve)
	{
	}

	return result;
}

Matrix44 EdgeMeshEdge::calculate_matrix_for(float _t, EdgeMesh const * _owner, EdgesAndSurfacesContext & _context) const
{
	Vector3 location = curve.calculate_at(_t);
	Vector3 tangent = curve.calculate_tangent_at(_t);
	Vector3 normal = calculate_normal_for(_t, tangent, _owner, _context);
	Vector3 right = Vector3::cross(tangent, normal).normal(); // right should be perpendicular to both tangent and normal
	normal = Vector3::cross(right, tangent).normal(); // normal we get from calculation could be not perpendicular to tangent, make it be

	return matrix_from_axes(right, tangent, normal, location);
}

Vector3 EdgeMeshEdge::calculate_normal_for(BezierCurve<Vector3> const & _curve, float _t, Vector3 const & _tangent, EdgeMesh const * _owner, EdgesAndSurfacesContext & _context) const
{
	an_assert(_curve == curve);
	return calculate_normal_for(_t, _tangent, _owner, _context);
}

Vector3 EdgeMeshEdge::calculate_normal_for(float _t, Vector3 const & _tangent, EdgeMesh const * _owner, EdgesAndSurfacesContext & _context) const
{
	auto * useEdgeRef = _owner->useEdgeRef? _owner->useEdgeRef->get(_context.access_generation_context()) : nullptr;
	CustomData::EdgeNormalAlignment::Type normalAlignment = surfacesContext.get_normal_alignment(_context, _owner->surfacesContext.get_normal_alignment(_context,
		_owner->useEdgeRef && useEdgeRef ? useEdgeRef->get_normal_alignment() : CustomData::EdgeNormalAlignment::AlignWithNormals));

	Vector3 refPoint = surfacesContext.get_ref_point_for_normal(_context, _owner->surfacesContext.get_ref_point_for_normal(_context));
	
#ifdef LOG_CALCULATE_NORMAL
	output(TXT("calculate_normal_for %.8f tangent: %S ref point: %S"), _t, _tangent.to_string().to_char(), refPoint.to_string().to_char());
#endif
		
	return EdgesAndSurfacesData::calculate_normal_for(_context.access_element_instance(), curve, _t, _tangent, normalAlignment, surfacesOnSides, refPoint);
}

Meshes::VertexSkinningInfo EdgeMeshEdge::calculate_skinning_info_for(BezierCurve<Vector3> const & _curve, float _t, EdgeMesh const * _owner, EdgesAndSurfacesContext & _context) const
{
	an_assert(_curve == curve);
	return calculate_skinning_info_for(_t, _owner, _context);
}

Meshes::VertexSkinningInfo EdgeMeshEdge::calculate_skinning_info_for(float _t, EdgeMesh const * _owner, EdgesAndSurfacesContext & _context) const
{
	if (skinningToUse.is_empty())
	{
		if (_owner->boneIdx != NONE)
		{
			Meshes::VertexSkinningInfo vsi;
			vsi.add(Meshes::VertexSkinningInfo::Bone(_owner->boneIdx, 1.0f));
			return vsi;
		}
		return Meshes::VertexSkinningInfo();
	}
	if (_owner->bone.is_set())
	{
		warn(TXT("doubled skinning info and bone setting"));
	}
	an_assert(_t >= 0.0f && _t <= 1.0f);
	for_range(int, idx, 1, skinningToUse.get_size())
	{
		if (_t >= skinningToUse[idx - 1].pt &&
			_t <= skinningToUse[idx].pt)
		{
			EdgeMeshSkinningInfo const & prev = skinningToUse[idx - 1];
			EdgeMeshSkinningInfo const & curr = skinningToUse[idx];
			Meshes::VertexSkinningInfo prevVSI;
			prevVSI.add(Meshes::VertexSkinningInfo::Bone(prev.boneAIdx, prev.boneAWeight));
			prevVSI.add(Meshes::VertexSkinningInfo::Bone(prev.boneBIdx, prev.boneBWeight));
			Meshes::VertexSkinningInfo currVSI;
			currVSI.add(Meshes::VertexSkinningInfo::Bone(curr.boneAIdx, curr.boneAWeight));
			currVSI.add(Meshes::VertexSkinningInfo::Bone(curr.boneBIdx, curr.boneBWeight));

			float pt = clamp((_t - prev.pt) / max(SMALL_MARGIN, curr.pt - prev.pt), 0.0f, 1.0f);
			return Meshes::VertexSkinningInfo::blend(prevVSI, currVSI, pt);
		}
	}
	an_assert(false, TXT("could not find skinning info for t = %.3f? how come?"), _t);
	return Meshes::VertexSkinningInfo();
}

//

void EdgeMeshEdge::RoundCorner::prepare(EdgeMesh const * _edgeMesh, EdgesAndSurfacesContext & _context, EdgeMeshEdge const * prev, EdgeMeshEdge const * edge)
{
	Vector3 prevMaxTangent = prev->curve.calculate_tangent_at(prev->betweenCorners.max);
	Vector3 edgeMinTangent = edge->curve.calculate_tangent_at(edge->betweenCorners.min);

	// calculate curve p0 and p3 lay on curve, p1 and p2 should use tangent at that point
	curve.p0 = prev->curve.calculate_at(prev->betweenCorners.max);
	curve.p3 = edge->curve.calculate_at(edge->betweenCorners.min);
	curve.p1 = curve.p0 + prevMaxTangent;
	curve.p2 = curve.p3 - edgeMinTangent;

	curve.make_roundly_separated();

	// create corner curves
	Matrix44 matP0 = prev->calculate_matrix_for(prev->betweenCorners.max, _edgeMesh, _context);
	Matrix44 matP3 = edge->calculate_matrix_for(edge->betweenCorners.min, _edgeMesh, _context);
	Vector3 corners[4] = { Vector3(curveWindow.x.min, 0.0f, curveWindow.y.min), /* LB */
		Vector3(curveWindow.x.max, 0.0f, curveWindow.y.min), /* RB */
		Vector3(curveWindow.x.min, 0.0f, curveWindow.y.max), /* LT */
		Vector3(curveWindow.x.max, 0.0f, curveWindow.y.max)  /* RT */ };
	for_count(int, c, 4)
	{
		auto & cwc = curveWindowCorners[c];
		auto const & corner = corners[c];
		cwc.p0 = matP0.location_to_world(corner);
		cwc.p3 = matP3.location_to_world(corner);
		cwc.p1 = cwc.p0 + prevMaxTangent;
		cwc.p2 = cwc.p3 - edgeMinTangent;
		cwc.make_roundly_separated();
	}

	prevNormal = prev->calculate_normal_for(prev->betweenCorners.max, prevMaxTangent, _edgeMesh, _context);
	nextNormal = edge->calculate_normal_for(edge->betweenCorners.min, edgeMinTangent, _edgeMesh, _context);
}

//

void EdgeSurfacesOnSides::build(Edge::Ref const & _edgeRef, Name const & _useSurfaceGroup, Array<Surface*> const & _surfaces)
{
	for_every_const_ptr(surface, _surfaces)
	{
		if (!surface->inSurfaceGroups.does_contain(_useSurfaceGroup))
		{
			continue;
		}
		for_every_const(surfaceEdge, surface->edges)
		{
			if (surfaceEdge->edge == _edgeRef.edge)
			{
				if (surfaceEdge->reversed != _edgeRef.reversed)
				{
					surfaceOnLeft = surface;
					surfaceOnLeftEdgeUV = surface->calculate_uv_curve_for(_edgeRef.edge).reversed();
				}
				else
				{
					surfaceOnRight = surface;
					surfaceOnRightEdgeUV = surface->calculate_uv_curve_for(_edgeRef.edge);
				}
			}
		}
	}
}

//

GenerateSubElement* GenerateSubElement::get_one(Element const * _element, Transform const & _placement, Vector3 const & _scale)
{
	GenerateSubElement* newOne = base::get_one();
	newOne->element = _element;
	newOne->placement = _placement;
	newOne->scale = _scale;
	return newOne;
}

void GenerateSubElement::on_release()
{
	base::on_release();

	parameters.clear();
}

//

Name EdgeSurfacesContext::get_use_surface_group(EdgesAndSurfacesContext & _context, Name const & _base) const
{
	if (useSurfaceGroup.is_set())
	{
		return useSurfaceGroup.get();
	}
	return _base;
}

CustomData::EdgeNormalAlignment::Type EdgeSurfacesContext::get_normal_alignment(EdgesAndSurfacesContext & _context, CustomData::EdgeNormalAlignment::Type const & _base) const
{
	if (normalAlignment.is_set())
	{
		return normalAlignment.get();
	}
	return _base;
}

Vector3 EdgeSurfacesContext::get_ref_point_for_normal(EdgesAndSurfacesContext & _context, Vector3 const & _base) const
{
	if (refPointForNormal.is_set())
	{
		return refPointForNormal.get(_context.access_element_instance().get_context());
	}
	return _base;
}

bool EdgeSurfacesContext::load_from_xml(IO::XML::Node const * _node)
{
	bool result = true;

	useSurfaceGroup.load_from_xml(_node, TXT("useSurfaceGroup"));
	if (IO::XML::Attribute const * attr = _node->get_attribute(TXT("normalAlignment")))
	{
		normalAlignment = CustomData::EdgeNormalAlignment::parse(attr->get_as_string());
	}
	refPointForNormal.load_from_xml_child_node(_node, TXT("refPointForNormal"));
	refPointForNormal.load_from_xml_child_node(_node, TXT("refDirForNormal"));

	return result;
}

//

bool EdgeMesh::UseElements::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;

	lengthParamName = _node->get_name_attribute(TXT("lengthParam"), lengthParamName);
	startMeshNodeName = _node->get_name_attribute(TXT("startMeshNode"), startMeshNodeName);
	endMeshNodeName = _node->get_name_attribute(TXT("endMeshNode"), endMeshNodeName);
	divMeshNodeName = _node->get_name_attribute(TXT("divMeshNode"), divMeshNodeName);
	subStep.load_from_xml_child_node(_node, TXT("subStep"));

	if (!startMeshNodeName.is_valid() ||
		!endMeshNodeName.is_valid())
	{
		error_loading_xml(_node, TXT("startMeshNode and endMeshNode required!"));
		result = false;
	}

	for_every(node, _node->all_children())
	{
		if (node->get_name() != TXT("subStep"))
		{
			if (Element* element = Element::create_from_xml(node, _lc))
			{
				if (element->load_from_xml(node, _lc))
				{
					elements.push_back(RefCountObjectPtr<Element>(element));
				}
				else
				{
					error_loading_xml(node, TXT("problem loading one element for edge mesh's elements (of edges and surfaces)"));
					result = false;
				}
			}
		}
	}

	return result;
}

bool EdgeMesh::UseElements::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	for_every_ref(element, elements)
	{
		result &= element->prepare_for_game(_library, _pfgContext);
	}

	return result;
}

//

EdgeMesh::~EdgeMesh()
{
	for_every(edge, edges)
	{
		delete_and_clear(*edge);
	}
	edges.clear();
	for_every(generateSurfaceMesh, generateSurfaceMeshes)
	{
		delete_and_clear(*generateSurfaceMesh);
	}
	generateSurfaceMeshes.clear();
	clear_cross_sections();
	delete_and_clear(ownEdgeRef);
}

void EdgeMesh::clear_cross_sections() const
{
	for_every(crossSection, crossSectionSegments)
	{
		delete_and_clear(*crossSection);
	}
	crossSectionSegments.clear();
	crossSections.clear();
}

float EdgeMesh::get_corner_radius(EdgesAndSurfacesContext & _context) const
{
	if (cornerRadius.is_set())
	{
		return cornerRadius.get(_context.access_element_instance().get_context());
	}
	if (useEdgeRef)
	{
		if (auto * uer = useEdgeRef->get(_context.access_generation_context()))
		{
			return uer->get_corner_radius();
		}
	}
	return 0.0f;
}

bool EdgeMesh::get_cut_corner(EdgesAndSurfacesContext & _context) const
{
	if (cutCorner.is_set())
	{
		return cutCorner.get(_context.access_element_instance().get_context());
	}
	if (useEdgeRef)
	{
		if (auto * uer = useEdgeRef->get(_context.access_generation_context()))
		{
			return uer->get_cut_corner();
		}
	}
	return 0.0f;
}

bool EdgeMesh::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;

	bone.load_from_xml(_node, TXT("bone"));

	createMovementCollisionMesh.load_from_xml(_node, TXT("createMovementCollisionMesh"), _lc);
	createMovementCollisionBox.load_from_xml(_node, TXT("createMovementCollisionBox"), _lc);
	createMovementCollisionBoxes.load_from_xml(_node, TXT("createMovementCollisionBoxes"), _lc);
	createPreciseCollisionBox.load_from_xml(_node, TXT("createPreciseCollisionBox"), _lc);
	createPreciseCollisionMesh.load_from_xml(_node, TXT("createPreciseCollisionMesh"), _lc);
	createPreciseCollisionBoxes.load_from_xml(_node, TXT("createPreciseCollisionBoxes"), _lc);
	error_loading_xml_on_assert(!_node->first_child_named(TXT("createPreciseCollisionMeshSkinned")), result, _node, TXT("createPreciseCollisionMeshSkinned not handled"));
	createSpaceBlocker.load_from_xml(_node, TXT("createSpaceBlocker"));
	noMesh = _node->get_bool_attribute_or_from_child_presence(TXT("noMesh"), noMesh);


	id = _node->get_name_attribute(TXT("id"), id);

	if (IO::XML::Node const * node = _node->first_child_named(TXT("wheresMyPoint")))
	{
		result &= wmpToolSet.load_from_xml(node);
	}

	result &= generationCondition.load_from_xml(_node);

	if (IO::XML::Attribute const * attr = _node->get_attribute(TXT("scaleCrossSection")))
	{
		float val = attr->get_as_float();
		scaleCrossSection.set(Vector2(val, val));
	}
	scaleCrossSectionFloat.load_from_xml(_node, TXT("scaleCrossSection"));
	scaleCrossSection.load_from_xml_child_node(_node, TXT("scaleCrossSection"));
	offsetCrossSection.load_from_xml_child_node(_node, TXT("offsetCrossSection"));

	shortenStart.load_from_xml(_node, TXT("shorten"));
	shortenEnd.load_from_xml(_node, TXT("shorten"));
	shortenStart.load_from_xml(_node, TXT("shortenStart"));
	shortenEnd.load_from_xml(_node, TXT("shortenEnd"));
	moveCapStartByShorten = _node->get_bool_attribute(TXT("moveCapsByShorten"), moveCapStartByShorten);
	moveCapEndByShorten = _node->get_bool_attribute(TXT("moveCapsByShorten"), moveCapEndByShorten);
	moveCapStartByShorten = _node->get_bool_attribute(TXT("moveCapStartByShorten"), moveCapStartByShorten);
	moveCapEndByShorten = _node->get_bool_attribute(TXT("moveCapEndByShorten"), moveCapEndByShorten);

	storeCrossSectionVertexCountInVar = _node->get_name_attribute(TXT("storeCrossSectionVertexCountInVar"), storeCrossSectionVertexCountInVar);

	cornerRadius.load_from_xml(_node, TXT("cornerRadius"));
	cutCorner.load_from_xml(_node, TXT("cutCorner"));
	cornersOnlyIfPossible.load_from_xml(_node, TXT("cornersOnlyIfPossible"));

	autoCSOCid.load_from_xml(_node, TXT("autoCSOCid"));
	autoCSOClod = _node->get_int_attribute(TXT("autoCSOClod"), autoCSOClod);

	allowCSOC.load_from_xml(_node, TXT("allowCSOC"));
	blockCSOC.load_from_xml(_node, TXT("blockCSOC"));
	csocOnlyIfMoreDetailsAllowed = _node->get_bool_attribute_or_from_child_presence(TXT("csocOnlyIfMoreDetailsAllowed"), csocOnlyIfMoreDetailsAllowed);
	autoCSOCOnlyIfMoreDetailsAllowed = _node->get_bool_attribute_or_from_child_presence(TXT("autoCSOCOnlyIfMoreDetailsAllowed"), autoCSOCOnlyIfMoreDetailsAllowed);

	result &= surfacesContext.load_from_xml(_node);

	if (IO::XML::Node const * node = _node->first_child_named(TXT("elements")))
	{
		result &= useElements.load_from_xml(node, _lc);
	}

	if (useElements.elements.is_empty())
	{
		useEdgeRefId.load_from_xml(_node, TXT("useEdgeRef"));
		if (!useEdgeRefId.is_set())
		{
			an_assert(ownEdgeRef == nullptr);
			ownEdgeRef = new CustomData::EdgeRef();
			if (ownEdgeRef->load_from_xml(_node, TXT("useEdgeRef"), TXT("useEdge"), _lc))
			{
				useEdgeRef = ownEdgeRef;
			}
			else
			{
				error_loading_xml(_node, TXT("couldn't load own edge"));
				result = false;
				delete_and_clear(ownEdgeRef);
			}
		}
	}

	if (!useEdgeRefId.is_set() && !ownEdgeRef && useElements.elements.is_empty())
	{
		error_loading_xml(_node, TXT("no edge nor elements defined for edge mesh"));
		result = false;
	}
		
	for_every(edgesNode, _node->children_named(TXT("edges")))
	{
		for_every(node, edgesNode->all_children())
		{
			if (node->get_name() == TXT("edge"))
			{
				EdgeMeshEdge* edge = new EdgeMeshEdge();
				if (edge->load_from_xml_as_edge(node, _lc))
				{
					edges.push_back(edge);
				}
				else
				{
					result = false;
					error_loading_xml(node, TXT("could not load edge"));
					delete edge;
				}
			}
			if (node->get_name() == TXT("node"))
			{
				if (edges.is_empty())
				{
					error_loading_xml(node, TXT("always start with edge"));
					result = false;
				}
				else
				{
					EdgeMeshEdge * lastEdge = edges.get_last();
					result &= lastEdge->load_from_xml_as_node(node, _lc);
				}
			}
		}
	}

	if (edges.is_empty())
	{
		error_loading_xml(_node, TXT("there were no edges defined, at least there was no \"edges\" node containing edges"));
		result = false;
	}

	for_every(node, _node->children_named(TXT("generateSurfaceMesh")))
	{
		SurfaceMeshForEdge *sm = new SurfaceMeshForEdge();
		if (sm->load_from_xml(node, _lc))
		{
			generateSurfaceMeshes.push_back(sm);
		}
		else
		{
			delete sm;
			result = false;
		}
	}

	debugDraw = _node->get_bool_attribute_or_from_child_presence(TXT("debugDraw"), debugDraw);

	return result;
}

bool EdgeMesh::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	if (ownEdgeRef)
	{
		result &= ownEdgeRef->prepare_for_game(_library, _pfgContext);
	}
	result &= useElements.prepare_for_game(_library, _pfgContext);

	return result;
}

bool EdgeMesh::prepare_for_generation(int _idx, EdgesAndSurfacesContext & _context, PrepareForGenerationLevel::Type _level) const
{
	bool result = true;

	if (!generationCondition.check(_context.access_element_instance()))
	{
		return true;
	}

	if (!wmpToolSet.is_empty()) // might be useful on all levels
	{
		WheresMyPoint::Context wmpContext(&_context); // use this context as we will be accessing points!
		wmpContext.set_random_generator(_context.access_element_instance().access_context().access_random_generator());
		wmpContext.access_random_generator().advance_seed(_idx, _idx);
		wmpToolSet.update(wmpContext);
	}

	for_every_ptr(generateSurfaceMesh, generateSurfaceMeshes)
	{
		result &= generateSurfaceMesh->prepare_for_generation(_context, _level);
	}

	if (_level == PrepareForGenerationLevel::Resolve)
	{
		boneIdx = bone.is_set() ? _context.access_element_instance().get_context().find_bone_index(bone.get(_context.access_generation_context())) : NONE;
		if (useEdgeRefId.is_set())
		{
			Name actualUseEdgeRef = useEdgeRefId.get(_context.access_generation_context());
			useEdgeRef = _context.get_edges_and_surfaces_data()->edgeRefs.get(actualUseEdgeRef);
			if (!useEdgeRef)
			{
				error_generating_mesh(_context.access_element_instance(), TXT("could not find edge ref \"%S\""), actualUseEdgeRef.to_char());
				result = false;
			}
		}

		hasCrossSection = false;
		auto * uer = useEdgeRef ? useEdgeRef->get(_context.access_generation_context()) : nullptr;
		if (uer)
		{
			hasCrossSection = uer->get_cross_section_spline().get() != nullptr;
		}

		// order edges and check if that's right
		Edge::Ref const * prevEdge = nullptr;
		for_every_const_ptr(edge, edges)
		{
			if (!edge->edgeRef.prepare_for_generation(_context))
			{
				result = false;
				continue;
			}

			result &= check_edge_chain(_context.access_element_instance(), prevEdge, &edge->edgeRef);

			edge->node = edge->edgeRef.edge->get_to_node(edge->edgeRef.reversed);


			prevEdge = &edge->edgeRef;
		}
		// check if they are looped
		loopedEdgeChain = !edges.is_empty() ? check_edge_chain(_context.access_element_instance(), prevEdge, &edges.get_first()->edgeRef, true, true) : false;
	
		// get info about caps
		capStart = loopedEdgeChain || !hasCrossSection || !uer ? nullptr : uer->get_cap_start();
		capEnd = loopedEdgeChain || !hasCrossSection || !uer ? nullptr : uer->get_cap_end();
	}

	if (_level == PrepareForGenerationLevel::BuildCrossSections)
	{
		clear_cross_sections();

		// create new cross sections
		if (useEdgeRef)
		{
			if (auto * edgeRef = useEdgeRef->get(_context.access_generation_context()))
			{
				Vector2 edgeMeshScale = scaleCrossSection.get(_context.access_element_instance().get_context());
				{
					float edgeMeshScaleFloat = scaleCrossSectionFloat.get(_context.access_element_instance().get_context());
					if (edgeMeshScaleFloat > 0.0f)
					{
						edgeMeshScale = Vector2(edgeMeshScaleFloat, edgeMeshScaleFloat);
					}
				}
				crossSectionSegments.make_space_for(edgeRef->get_all_cross_sections().get_size());
				crossSections.make_space_for(edgeRef->get_all_cross_sections().get_size());

				CustomData::SplineContext<Vector2> csContext;
				csContext.transform = Matrix33::identity;

				// get all segments for all cross sections and check if number of segments is same for each one
				int minCount = NONE;
				int maxCount = NONE;
				for_every_const(sourceCS, edgeRef->get_all_cross_sections())
				{
					if (auto * spline = sourceCS->spline.get())
					{
						crossSectionSegments.push_back(new EdgeMeshCrossSection());
						if (auto * cs = crossSectionSegments.get_last())
						{
							cs->id = sourceCS->id;
							WheresMyPoint::Context wmpContext(&_context); // use this context as we will be accessing points!
							wmpContext.set_random_generator(_context.access_element_instance().access_context().access_random_generator());
							spline->get_segments_for(cs->segments, wmpContext, csContext);
							CustomData::Spline<Vector2>::make_sure_segments_are_clockwise(cs->segments);
							int count = cs->segments.get_size();
							minCount = minCount == NONE ? count : min(minCount, count);
							maxCount = maxCount == NONE ? count : max(maxCount, count);
						}
					}
				}

				if (minCount != maxCount)
				{
					result = false;
					error_generating_mesh(_context.access_element_instance(), TXT("number of segments differ for edge mesh %i-%i"), minCount, maxCount);
				}

				// check into how many steps do they divide

				// prepare place to store number of points
				Array<int> numberOfSubStepsPerSegment;
				numberOfSubStepsPerSegment.set_size(maxCount);
				memory_zero(numberOfSubStepsPerSegment.get_data(), sizeof(int) * maxCount);

				// make place for temporary results
				Array<int> numberOfSubStepsPerSegmentTemp;
				numberOfSubStepsPerSegmentTemp.make_space_for(maxCount);

				// calculate
				for_every_const_ptr(css, crossSectionSegments)
				{
					CustomData::Spline<Vector2>::change_segments_into_number_of_points(css->segments, edgeMeshScale, edgeRef->get_cross_section_sub_step(), _context.access_element_instance().get_context(), OUT_ numberOfSubStepsPerSegmentTemp, noMesh);
					for_every(nop, numberOfSubStepsPerSegment)
					{
						int nopIndex = for_everys_index(nop);
						if (numberOfSubStepsPerSegmentTemp.is_index_valid(nopIndex))
						{
							*nop = max(*nop, numberOfSubStepsPerSegmentTemp[nopIndex]);
						}
					}
				}

				crossSections.set_size(crossSectionSegments.get_size());

				// build actual points and us
				for_every_const_ptr(css, crossSectionSegments)
				{
					auto & cs = crossSections[for_everys_index(css)];
					cs.id = css->id;
					CustomData::Spline<Vector2>::change_segments_into_points(css->segments, edgeMeshScale, numberOfSubStepsPerSegment, OUT_ cs.points, OUT_ cs.us);
				}
			}
		}
	}

	if (_level == PrepareForGenerationLevel::EdgeMeshes)
	{
		bool allowCSOCnow = allowCSOC.is_set() ? allowCSOC.get(_context.access_element_instance().get_context()) : true;
		if (blockCSOC.is_set())
		{
			allowCSOCnow = !blockCSOC.get(_context.access_element_instance().get_context());
		}
		if (csocOnlyIfMoreDetailsAllowed)
		{
			if (_context.access_element_instance().get_context().does_force_more_details())
			{
				allowCSOCnow = true;
			}
			else if (auto * g = Game::get())
			{
				allowCSOCnow = g->should_generate_more_details();
			}
		}
		bool allowAutoCSOCnow = true;
		if (autoCSOCOnlyIfMoreDetailsAllowed)
		{
			if (_context.access_element_instance().get_context().does_force_more_details())
			{
				allowAutoCSOCnow = true;
			}
			else if (auto* g = Game::get())
			{
				allowAutoCSOCnow = g->should_generate_more_details();
			}
		}

		// get edges and surfaces on both sides
		for_every_const_ptr(edge, edges)
		{
			if (!edge->edgeRef.edge)
			{
				// just break!
				return false;
			}
			edge->curve = edge->edgeRef.edge->curve.reversed(edge->edgeRef.reversed);

			Name const usg = edge->surfacesContext.get_use_surface_group(_context, surfacesContext.get_use_surface_group(_context));
			edge->surfacesOnSides.build(edge->edgeRef, usg, _context.get_edges_and_surfaces_data()->surfaces);
		}

		// build useEdges array
		useEdges.clear();
		for_every_ptr(edge, edges)
		{
			if (!edge->curve.is_degenerated())
			{
				useEdges.push_back(edge);
			}
		}

		// reset ranges
		float length = 0.0f;
		for_every_const_ptr(edge, useEdges)
		{
			edge->betweenCorners = Range(0.0f, 1.0f);
			edge->safeDistAt0.clear();
			edge->safeDistAt1.clear();
			length += edge->curve.length();
		}

		useElements.window = Range2::empty;
		if (!useElements.elements.is_empty())
		{
			bool generationResult = true;

			SimpleVariableStorage parameters;
			if (useElements.lengthParamName.is_valid())
			{
				parameters.access<float>(useElements.lengthParamName) = length;
			}
			auto & generationContext = _context.access_generation_context();
			useElements.checkpointStart.make(generationContext);
			Checkpoint checkpoint(generationContext);
			generationContext.push_stack();
			generationContext.push_checkpoint(checkpoint);
			generationContext.set_parameters(parameters);
			for_every_ref(element, useElements.elements)
			{
				generationResult &= generationContext.process(element, &_context.access_element_instance(), for_everys_index(element));
			}
			useElements.checkpointEnd.make(generationContext);
			generationContext.pop_checkpoint();
			generationContext.pop_stack();

			if (!generationResult)
			{
				error_generating_mesh(_context.access_element_instance(), TXT("error generating one of elements"));
			}

			result &= generationResult;

			useElements.startMeshNodePlacement.clear();
			useElements.endMeshNodePlacement.clear();
			useElements.startToEndCorridor.clear();
			useElements.startToEndCorridorLength = 0.0f;

			auto * cpRef = &generationContext.get_mesh_nodes()[useElements.checkpointStart.meshNodesSoFarCount];
			for_range(int, i, useElements.checkpointStart.meshNodesSoFarCount, useElements.checkpointEnd.meshNodesSoFarCount - 1)
			{
				auto * cp = cpRef->get();
				if (cp->name == useElements.startMeshNodeName)
				{
					useElements.startMeshNodePlacement = cp->placement;
				}
				if (cp->name == useElements.endMeshNodeName)
				{
					useElements.endMeshNodePlacement = cp->placement;
				}
				++cpRef;
			}

			useElements.window = Range2::zero;

			if (useElements.startMeshNodePlacement.is_set() &&
				useElements.endMeshNodePlacement.is_set())
			{
				useElements.startToEndCorridor = look_at_matrix(useElements.startMeshNodePlacement.get().get_translation(), useElements.endMeshNodePlacement.get().get_translation(), useElements.startMeshNodePlacement.get().get_axis(Axis::Up)).to_transform();
				useElements.startToEndCorridorLength = (useElements.endMeshNodePlacement.get().get_translation() - useElements.startMeshNodePlacement.get().get_translation()).length();

				for_range(int, i, useElements.checkpointStart.partsSoFarCount, useElements.checkpointEnd.partsSoFarCount - 1)
				{
					auto * part = generationContext.get_parts()[i].get();
					::System::VertexFormat const & vf = part->get_vertex_format();
					char8 const * vertexLocAt = &part->get_vertex_data()[vf.get_location_offset()];
					int stride = vf.get_stride();
					for_count(int, v, part->get_number_of_vertices())
					{
						Vector3 const & loc = *((Vector3 const *)vertexLocAt);
						Vector3 locInCorridor = useElements.startToEndCorridor.get().location_to_local(loc);
						useElements.window.include(Vector2(locInCorridor.x, locInCorridor.z));
						vertexLocAt += stride;
					}
				}
			}
			else
			{
				error_generating_mesh(_context.access_element_instance(), TXT("start or end mesh node missing!"));
				result = false;
			}
		}

		cornersOnly = cornersOnlyIfPossible.get_with_default(_context.access_element_instance().get_context(), false);

		// go through all edges and calculate corners
		EdgeMeshEdge const * prev = loopedEdgeChain ? edges.get_last() : nullptr;
		for_every_const_ptr(edge, useEdges)
		{
			if (prev && prev->cornerWeld)
			{
				prev->cornerType = EdgeMeshCorner::Weld;
			}
			else if (prev)
			{
				float cornerRadius = 0.0f;
				if (prev->cornerRadius.is_set())
				{
					cornerRadius = prev->cornerRadius.get(_context.access_element_instance().get_context());
				}
				else
				{
					cornerRadius = get_corner_radius(_context);
				}

				bool cutCorner = false;
				if (prev->cutCorner.is_set())
				{
					cutCorner = prev->cutCorner.get(_context.access_element_instance().get_context());
				}
				else
				{
					cutCorner = get_cut_corner(_context);
				}

				if (cornerRadius != 0.0f || cutCorner)
				{
					cornersOnly = false;
				}

				if ((cornerRadius != 0.0f && !cutCorner) || crossSections.is_empty()) // in case we wouldn't be able to do it, should we revert to sharp corner? (that may also fail?)
				{
					prev->cornerType = EdgeMeshCorner::Round; // we may switch to cut at some point in time

					EdgeMeshEdge::RoundCorner & roundCorner = prev->roundCorner;

					// get cross section at corner (scaled)
					ShapeUtils::CrossSection const * crossSection = nullptr;
					if (!crossSections.is_empty())
					{
						crossSection = &ShapeUtils::get_cross_section(crossSections, ShapeUtils::get_alt_cross_section_id(crossSections, prev->cornerCrossSectionId, prev->cornerCrossSectionAltIds));
					}
					
					Vector2 crossSectionScale = prev->cornerScale.get_with_default(_context.access_element_instance().get_context(), Vector2::one);
					crossSectionScale *= prev->cornerScaleFloat.get_with_default(_context.access_element_instance().get_context(), 1.0f);

					Vector2 const edgeMeshOffset = offsetCrossSection.get(_context.access_element_instance().get_context());

					if (crossSection)
					{	// calculate window
						roundCorner.curveWindow = Range2::empty;
						for_every(point, crossSection->points)
						{
							Vector2 p = *point * crossSectionScale + edgeMeshOffset;
							roundCorner.curveWindow.include(p);
						}
					}
					else if (! useElements.elements.is_empty())
					{
						roundCorner.curveWindow = useElements.window;
					}

					bool considerCorner = false;
					bool considerFlat = false;
					int iterationIdx = 0;
					int iterationCount = 10;
					// iterations help with situations when our edges are not straight
					// (which means that tangents are different and tangents are base for our calculations for corners)
					// - it should alter corner placement a bit
					while (iterationIdx < iterationCount)
					{
						++iterationIdx;
						// we will use tangents a lot
						Vector3 const prevTangent = prev->curve.calculate_tangent_at(prev->betweenCorners.max);
						Vector3 const edgeTangent = edge->curve.calculate_tangent_at(edge->betweenCorners.min);

						if (prevTangent == edgeTangent)
						{
							considerFlat = true;
						}
						else
						{
							considerFlat = false;
							an_assert(prevTangent != edgeTangent, TXT("implement_ such case"));

							float safeCornerRadius = 0.0f;
							{	// this is to calculate minimal corner radius
								// normals at corner and right
								Vector3 prevNormal = prev->calculate_normal_for(prev->betweenCorners.max, prevTangent, this, _context);
								Vector3 edgeNormal = edge->calculate_normal_for(edge->betweenCorners.min, edgeTangent, this, _context);
								Vector3 const prevRight = Vector3::cross(prevTangent, prevNormal).normal();
								Vector3 const edgeRight = Vector3::cross(edgeTangent, edgeNormal).normal();
								prevNormal = Vector3::cross(prevRight, prevTangent).normal(); // normal we get from calculation could be not perpendicular to tangent, make it be
								edgeNormal = Vector3::cross(edgeRight, edgeTangent).normal(); // normal we get from calculation could be not perpendicular to tangent, make it be

#ifdef AN_DEVELOPMENT
								// corner
								Vector3 const cornerLoc = prev->curve.calculate_at(1.0f);
								an_assert((cornerLoc - edge->curve.calculate_at(0.0f)).length() < 0.01f);
#endif
								// calculate direction in which centre of round corner lies
								Vector3 const cornerToCentre = (-prevTangent + edgeTangent).normal();

								// calculate cornerToCentre in cross section space (using right and normal dirs - first calculating at prev and edge and then normalising (averaging from both))
								Vector2 cornerToCentreInCrossSectionSpace = Vector2::zero;
								{
									cornerToCentreInCrossSectionSpace += Vector2(Vector3::dot(prevRight, cornerToCentre), Vector3::dot(prevNormal, cornerToCentre));
									cornerToCentreInCrossSectionSpace += Vector2(Vector3::dot(edgeRight, cornerToCentre), Vector3::dot(edgeNormal, cornerToCentre));
									cornerToCentreInCrossSectionSpace.normalise();
								}

								// calculate minimal radius - we should have point at corner axis
								float minimalCornerRadius = 0.0f;
								if (crossSection)
								{
									for_every(point, crossSection->points)
									{
										Vector2 p = *point * crossSectionScale + edgeMeshOffset;
										// as below
										float distTowardsCentre = Vector2::dot(cornerToCentreInCrossSectionSpace, p);
										minimalCornerRadius = max(minimalCornerRadius, distTowardsCentre);
									}
								}
								else if (! useElements.elements.is_empty())
								{
									for_range(int, i, useElements.checkpointStart.partsSoFarCount, useElements.checkpointEnd.partsSoFarCount - 1)
									{
										auto * part = _context.access_generation_context().get_parts()[i].get();
										::System::VertexFormat const & vf = part->get_vertex_format();
										char8 const * vertexLocAt = &part->get_vertex_data()[vf.get_location_offset()];
										for_count(int, v, part->get_number_of_vertices())
										{
											Vector3 const & loc = *((Vector3 const *)vertexLocAt);
											Vector3 locInCorridor = useElements.startToEndCorridor.get().location_to_local(loc);
											Vector2 p = Vector2(locInCorridor.x, locInCorridor.z);
											// as above
											float distTowardsCentre = Vector2::dot(cornerToCentreInCrossSectionSpace, p);
											minimalCornerRadius = max(minimalCornerRadius, distTowardsCentre);
										}
									}
								}

								float const increaseCornerRadius = 0.01f; // this is to avoid polygons with zero normals
								safeCornerRadius = clamp(minimalCornerRadius + increaseCornerRadius, minimalCornerRadius * 1.0f, minimalCornerRadius * 1.1f);
								cornerRadius = max(cornerRadius, safeCornerRadius); // add margin to make points not collapse
							}

							// calculate angle between curves at the point where they meet
							float const cosAngle = clamp(Vector3::dot(-prevTangent, edgeTangent), -1.0f, 1.0f);
							// cosAngle takes -1 if tangents are aligned in same direction, 1 if they are in opposite, note "-prevTangent"!
							float const angle = acos_deg(cosAngle);
							float const hangle = angle * 0.5f;
							/*
							*				A			A centre of round corner
							*			   /|\			B point at prev edge (prev)
							*			r / | \			C point at next/current edge (edge)
							*			 /  |  \		D point where one edge ends and other begins
							*			B_  |  _C		<DBA (and <DCA) are 90'
							*			 x`-D-`x		x is distance from D where normal curve should end
							*			prev edge		<CDB is our calculated angle (angle)
							*							<ADB is half of our angle (hangle)
							*
							*							therefore:
							*							tg(hangle) = r / x
							*							x = r / tg(hangle)
							*/

							float const dist = cornerRadius / max(SMALL_MARGIN, tan_deg(hangle));

							{
								prev->safeDistAt1 = clamp((safeCornerRadius / max(SMALL_MARGIN, tan_deg(hangle))) / prev->curve.length(), 0.0f, 1.0f);
								edge->safeDistAt0 = clamp((safeCornerRadius / max(SMALL_MARGIN, tan_deg(hangle))) / edge->curve.length(), 0.0f, 1.0f);
							}

							// check if it is possible to fit in using requested radius
							float distWithMargin = dist; // I commented out margin as I'd like to see if what I have now is enough
							if (distWithMargin < 0.01f)
							{
								// using margin again, but only for small values as for them it may get problematic a bit
								distWithMargin = min(dist * 1.2f, dist + 0.05f); // margin to "handle" cases when both edges are not straight lines
							}
							float prevBCMaxRequested = prev->curve.get_t_at_distance_from_end(distWithMargin);
							float edgeBCMinRequested = edge->curve.get_t_at_distance(distWithMargin);
							// this bit is quite important
							// allow for whole dist, we will solve cases when those overlap later
							float prevBCMaxAvailable = 0.0f;
							float edgeBCMinAvailable = 1.0f;

							float const threshold = 0.5f; // should be enough for some "almost there" cases
							{
								// use accuracy and step special for this case
								prev->betweenCorners.max = max(prevBCMaxAvailable, prevBCMaxRequested);
								edge->betweenCorners.min = min(edgeBCMinAvailable, edgeBCMinRequested);

								roundCorner.prepare(this, _context, prev, edge);
							}

#ifdef ROUND_CORNER_DEBUG_VISUALISE
							++nowAt;
							output(TXT("round corner debug visualise at %i (round corner adjust, iteration %i)"), nowAt, iterationIdx);
							if (nowAt >= stopOnlyAt &&
								(nowAt >= stopAt ||
									stopAtEvery ||
									(stopAtEveryFirstIteration && iterationIdx == 0) ||
									(stopAtEveryLastIteration && iterationIdx == iterationCount - 1)))
							{
								DebugVisualiserPtr dv(new DebugVisualiser(String::printf(TXT("round corner adjust, iteration %i"), iterationIdx)));
								Library::get_current()->setup(dv.get());
								dv->activate();
								dv->start_gathering_data();
								dv->clear();
								{
									Vector3 p = prev->curve.calculate_at(0.0f);
									float t = 0.0f;
									while (t <= 1.0f)
									{
										Vector3 n = prev->curve.calculate_at(t);
										dv->add_line(p.to_vector2(), n.to_vector2(), t < prev->safeDistAt0.get(0.0f) || t > 1.0f - prev->safeDistAt1.get(0.0f) ? Colour::blue : Colour::orange);
										p = n;
										t += 0.01f;
									}
								}

								{
									Vector3 p = edge->curve.calculate_at(0.0f);
									float t = 0.0f;
									while (t <= 1.0f)
									{
										Vector3 n = edge->curve.calculate_at(t);
										dv->add_line(p.to_vector2(), n.to_vector2(), t < edge->safeDistAt0.get(0.0f) || t > 1.0f - edge->safeDistAt1.get(0.0f) ? Colour::blue : Colour::yellow);
										p = n;
										t += 0.01f;
									}
								}

								{
									Vector3 p = roundCorner.curve.calculate_at(0.0f);
									float t = 0.0f;
									while (t <= 1.0f)
									{
										Vector3 n = roundCorner.curve.calculate_at(t);
										dv->add_line(p.to_vector2(), n.to_vector2(), Colour::blue);
										p = n;
										t += 0.01f;
									}
								}

								for_count(int, c, 4)
								{
									auto & cwc = roundCorner.curveWindowCorners[c];
									Vector3 p = cwc.calculate_at(0.0f);
									float t = 0.0f;
									while (t <= 1.0f)
									{
										Vector3 n = cwc.calculate_at(t);
										dv->add_line(p.to_vector2(), n.to_vector2(), Colour::black);
										p = n;
										t += 0.01f;
									}
									dv->add_arrow(cwc.calculate_at(0.0f).to_vector2(),
										(cwc.calculate_at(0.0f) + cwc.calculate_tangent_at(0.0f) * 0.2f).to_vector2(), Colour::white);
									dv->add_arrow(cwc.calculate_at(1.0f).to_vector2(),
										(cwc.calculate_at(1.0f) - cwc.calculate_tangent_at(1.0f) * 0.2f).to_vector2(), Colour::white);
								}

								dv->end_gathering_data();
								dv->show_and_wait_for_key();
								dv->deactivate();
							}
#endif

							if (prevBCMaxRequested >= prevBCMaxAvailable - threshold &&
								edgeBCMinRequested <= edgeBCMinAvailable + threshold)
							{
								considerCorner = false;
							}
							else
							{
								considerCorner = false;//!@# true;
							}
						}
					}
					if (considerCorner)
					{
						// switch to corner
						cornerRadius = 0.0f;
					}
					if (considerFlat)
					{
						cornerRadius = 0.0f;
					}
				}

				if ((cornerRadius == 0.0f || cutCorner) && ! crossSections.is_empty())
				{
					prev->cornerType = cutCorner ? EdgeMeshCorner::Cut : EdgeMeshCorner::Sharp;
					EdgeMeshEdge::SharpCorner & sharpCorner = prev->sharpCorner;

					// calculate separate directions
					// note: calculate right and recalculate normal to be perpendicular to right
					Vector3 const prevTangent = prev->curve.calculate_tangent_at(1.0f);
					Vector3 const edgeTangent = edge->curve.calculate_tangent_at(0.0f);
					Vector3 prevNormal = prev->calculate_normal_for(1.0f, prevTangent, this, _context);
					Vector3 edgeNormal = edge->calculate_normal_for(0.0f, edgeTangent, this, _context);
					Vector3 const prevRight = Vector3::cross(prevTangent, prevNormal).normal();
					Vector3 const edgeRight = Vector3::cross(edgeTangent, edgeNormal).normal();
					prevNormal = Vector3::cross(prevRight, prevTangent).normal();
					edgeNormal = Vector3::cross(edgeRight, edgeTangent).normal();

					// for actual corner
					sharpCorner.location = edge->curve.calculate_at(0.0f);
					sharpCorner.tangent = (prevTangent + edgeTangent).normal();
					sharpCorner.normal = (prevNormal + edgeNormal).normal();
					sharpCorner.right = Vector3::cross(sharpCorner.tangent, sharpCorner.normal).normal();
					sharpCorner.normal = Vector3::cross(sharpCorner.right, sharpCorner.tangent).normal();

					// calculate how cross section has to be scaled
					// the greater angle difference there is, the smaller dot there will be
					// taking inversion we will have proper scale
					// note: we calculate dot from both curves although results should be same for both!
					sharpCorner.scaleCrossSection = Vector2::one;
					{
						float coef = min(abs(Vector3::dot(prevRight, sharpCorner.right)), abs(Vector3::dot(edgeRight, sharpCorner.right)));
						an_assert(coef > 0.0001f);
						if (coef != 0.0f)
						{
							sharpCorner.scaleCrossSection.x = 1.0f / coef;
						}
					}
					{
						float coef = min(abs(Vector3::dot(prevNormal, sharpCorner.normal)), abs(Vector3::dot(edgeNormal, sharpCorner.normal)));
						an_assert(coef > 0.0001f);
						if (coef != 0.0f)
						{
							sharpCorner.scaleCrossSection.y = 1.0f / coef;
						}
					}

					// calculate distance in both directions to allow to keep corner inside
					ShapeUtils::CrossSection const & crossSection = ShapeUtils::get_cross_section(crossSections, ShapeUtils::get_alt_cross_section_id(crossSections, edge->cornerCrossSectionId, edge->cornerCrossSectionAltIds));
					Array<Vector2> const & csPoints = crossSection.points;

					// calculate window of cross section (and scale it using cross section scale, add offset)
					Range2 csRange = Range2::zero;
					for_every_const(csPoint, csPoints)
					{
						csRange.include(*csPoint
							* sharpCorner.scaleCrossSection
							/* no need for edgeMesh scaleCrossSection as cross sections are already scaled */
							+ offsetCrossSection.get(_context.access_element_instance().get_context()));
					}

					// now calculate how far each side goes inside curve
					float prevMin = 0.0f;
					prevMin = min(prevMin, Vector3::dot(prevTangent, sharpCorner.right * csRange.x.min));
					prevMin = min(prevMin, Vector3::dot(prevTangent, sharpCorner.right * csRange.x.max));
					prevMin = min(prevMin, Vector3::dot(prevTangent, sharpCorner.normal * csRange.y.min));
					prevMin = min(prevMin, Vector3::dot(prevTangent, sharpCorner.normal * csRange.y.max));

					float edgeMax = 0.0f;
					edgeMax = max(edgeMax, Vector3::dot(edgeTangent, sharpCorner.right * csRange.x.min));
					edgeMax = max(edgeMax, Vector3::dot(edgeTangent, sharpCorner.right * csRange.x.max));
					edgeMax = max(edgeMax, Vector3::dot(edgeTangent, sharpCorner.normal * csRange.y.min));
					edgeMax = max(edgeMax, Vector3::dot(edgeTangent, sharpCorner.normal * csRange.y.max));

					// helps a bit if there's some extra space for corners
					prevMin *= 1.2f;
					edgeMax *= 1.2f;

					// use accuracy and step special for this case
					prev->betweenCorners.max = prev->curve.get_t_at_distance_from_end(-prevMin);
					edge->betweenCorners.min = edge->curve.get_t_at_distance(edgeMax);
				}

			}
			prev = edge;
		}

		if (true)
		{	// check for betweenCorners that are incorrect and try to work it out
			bool fixRoundCorners = false;
			int triesLeft = 100;
			while (triesLeft--)
			{
				bool ok = true;
				EdgeMeshEdge const * prev = loopedEdgeChain ? edges.get_last() : nullptr;
				for_every_const_ptr(edge, useEdges)
				{
					EdgeMeshEdge const * next = edge;
					if (for_everys_index(edge) == edges.get_size() - 1)
					{
						next = loopedEdgeChain ? edges.get_first() : nullptr;
					}
					else
					{
						next = edges[for_everys_index(edge) + 1];
					}
					// sometimes the safe region is exactly as it is but float precision kicks in and messes things a bit
					float const threshold = 0.001f;
					if (edge->betweenCorners.min > edge->betweenCorners.max ||
						edge->betweenCorners.min < edge->safeDistAt0.get(0.0f) - threshold ||
						edge->betweenCorners.max > 1.0f - edge->safeDistAt1.get(0.0f) + threshold)
					{
						// fingers crossed!

						// let's try moving neighbours
						// smaller radius
						float moveApart = (edge->betweenCorners.min - edge->betweenCorners.max) * 0.5f;
						float offset = 0.0f;
						if (edge->betweenCorners.min < edge->safeDistAt0.get(0.0f) - threshold)
						{
							offset += 0.05f;
						}
						if (edge->betweenCorners.max > 1.0f - edge->safeDistAt1.get(0.0f) + threshold)
						{
							offset -= 0.05f;
						}
						if (prev) prev->betweenCorners.max = clamp(prev->betweenCorners.max - moveApart * 0.1f + offset * 0.1f, 0.0f, 1.0f - (prev->safeDistAt1.get(0.0f)));
						if (next) next->betweenCorners.min = clamp(next->betweenCorners.min + moveApart * 0.1f + offset * 0.1f, next->safeDistAt0.get(0.0f), 1.0f);
						edge->betweenCorners.min = clamp(edge->betweenCorners.min - moveApart * 0.15f + offset * 0.15f, 0.0f, 1.0f);
						edge->betweenCorners.max = clamp(edge->betweenCorners.max + moveApart * 0.15f + offset * 0.15f, 0.0f, 1.0f);
						todo_note(TXT("if this does not work, try to make radiee same on each corner"));
						fixRoundCorners = true;

						ok = false;
					}
					prev = edge;
				}
#ifdef ROUND_CORNER_DEBUG_VISUALISE
				if (fixRoundCorners)
				{
					++nowAt;
					output(TXT("round corner debug visualise at %i (fix round corners, tries left %i)"), nowAt, triesLeft);
					if (nowAt >= stopAt)
					{
						DebugVisualiserPtr dv(new DebugVisualiser(String::printf(TXT("fix round corners, tries left %i"), triesLeft)));
						Library::get_current()->setup(dv.get());
						dv->activate();
						dv->start_gathering_data();
						dv->clear();
						EdgeMeshEdge const * prev = loopedEdgeChain ? edges.get_last() : nullptr;
						for_every_const_ptr(edge, useEdges)
						{
							{
								Vector3 p = edge->curve.calculate_at(0.0f);
								float t = 0.0f;
								while (t <= 1.0f)
								{
									Vector3 n = edge->curve.calculate_at(t);
									dv->add_line(p.to_vector2(), n.to_vector2(), t < edge->safeDistAt0.get(0.0f) || t > 1.0f - edge->safeDistAt1.get(0.0f) ? Colour::blue : Colour::yellow);
									p = n;
									t += 0.01f;
								}
							}

							if (prev)
							{
								EdgeMeshEdge::RoundCorner & roundCorner = prev->roundCorner;

								if (prev && prev->cornerType == EdgeMeshCorner::Round)
								{
									roundCorner.prepare(this, _context, prev, edge);
								}

								{
									Vector3 p = roundCorner.curve.calculate_at(0.0f);
									float t = 0.0f;
									while (t <= 1.0f)
									{
										Vector3 n = roundCorner.curve.calculate_at(t);
										dv->add_line(p.to_vector2(), n.to_vector2(), Colour::blue);
										p = n;
										t += 0.01f;
									}
								}

								for_count(int, c, 4)
								{
									auto & cwc = roundCorner.curveWindowCorners[c];
									Vector3 p = cwc.calculate_at(0.0f);
									float t = 0.0f;
									while (t <= 1.0f)
									{
										Vector3 n = cwc.calculate_at(t);
										dv->add_line(p.to_vector2(), n.to_vector2(), Colour::black);
										p = n;
										t += 0.01f;
									}
									dv->add_arrow(cwc.calculate_at(0.0f).to_vector2(),
										(cwc.calculate_at(0.0f) + cwc.calculate_tangent_at(0.0f) * 0.2f).to_vector2(), Colour::white);
									dv->add_arrow(cwc.calculate_at(1.0f).to_vector2(),
										(cwc.calculate_at(1.0f) - cwc.calculate_tangent_at(1.0f) * 0.2f).to_vector2(), Colour::white);
								}
							}
							prev = edge;
						}

						dv->end_gathering_data();
						dv->show_and_wait_for_key();
						dv->deactivate();
					}
				}
#endif
				if (ok)
				{
					break;
				}
			}

			// fix round corners, calculate curves again
			EdgeMeshEdge const * prev = loopedEdgeChain ? edges.get_last() : nullptr;
			for_every_const_ptr(edge, useEdges)
			{
				if (prev && prev->cornerType == EdgeMeshCorner::Round)
				{
					EdgeMeshEdge::RoundCorner & roundCorner = prev->roundCorner;

					roundCorner.prepare(this, _context, prev, edge);
				}
				prev = edge;
			}
		}
		else
		{
			for_every_const_ptr(edge, useEdges)
			{
				if (edge->betweenCorners.min > edge->betweenCorners.max)
				{
					// fingers crossed!
					edge->betweenCorners.min = edge->betweenCorners.max = (edge->betweenCorners.min + edge->betweenCorners.max) * 0.5f;
				}
			}
		}

		// make both edges shorter because of caps or just because it was requested
		if (!edges.is_empty())
		{
			for (int i = 0; i < 2; ++i)
			{
				// gather settings
				CustomData::EdgeCap const * cap = (i == 0 ? capStart : capEnd);
				bool moveCapByShorten = i == 0 ? moveCapStartByShorten : moveCapEndByShorten;
				EdgeMeshEdge const * edge = i == 0 ? edges.get_first() : edges.get_last();
				float & capT = i == 0 ? capStartT : capEndT;

				capT = i == 0 ? 0.0f : 1.0f; // reset!

				// check how much do we have to shorten edge, by each reason
				float shortenEdgeFromEdgeMesh = i == 0 ? shortenStart.get(_context.access_element_instance().get_context()) : shortenEnd.get(_context.access_element_instance().get_context());
				float shortenEdgeFromCap = 0.0f;
				if (cap)
				{
					shortenEdgeFromCap = cap->get_shorten_edge().get(_context.access_element_instance().get_context());
				}
				float shortenEdge = moveCapByShorten ? shortenEdgeFromEdgeMesh + shortenEdgeFromCap : max(shortenEdgeFromEdgeMesh, shortenEdgeFromCap);
				{
					// now shorten our edge
					float edgeShortenedAtT = 0.0f;
					float startsAt = i == 0 ? 0.0f : 1.0f;
					edgeShortenedAtT = edge->edgeRef.reversed
						? 1.0f - edge->curve.get_t_at_distance_ex(shortenEdge, 1.0f - startsAt, i == 1)
						: edge->curve.get_t_at_distance_ex(shortenEdge, startsAt, i == 0);
					if (i == 0)
					{
						edgeShortenedAtT = min(0.5f, edgeShortenedAtT);
						edge->betweenCorners.min = max(edge->betweenCorners.min, edgeShortenedAtT);
					}
					else
					{
						edgeShortenedAtT = max(0.5f, edgeShortenedAtT);
						edge->betweenCorners.max = min(edge->betweenCorners.max, edgeShortenedAtT);
					}

					// check where cap should start, move by shorten from edge mesh (unless said not to move!)
					if (moveCapByShorten)
					{
						capT = edge->edgeRef.reversed
							? (1.0f - edge->curve.get_t_at_distance_ex(shortenEdgeFromEdgeMesh, 1.0f - capT, i == 1))
							: edge->curve.get_t_at_distance_ex(shortenEdgeFromEdgeMesh, capT, i == 0);
					}
					// move by value requested by cap (if actually requested!)
					if (cap && cap->should_move_by_shorten_edge())
					{
						capT = edge->edgeRef.reversed
							? (1.0f - edge->curve.get_t_at_distance_ex(shortenEdgeFromCap, 1.0f - capT, i == 1))
							: edge->curve.get_t_at_distance_ex(shortenEdgeFromCap, capT, i == 0);
					}
					// make capT sane
					if (i == 0)
					{
						capT = min(0.5f, capT);
					}
					else
					{
						capT = max(0.5f, capT);
					}
				}
			}
		}

		// calculate actual positions on curve
		for_every_const_ptr(edge, useEdges)
		{
			edge->customOnCurveToUseAll.clear();
			if (!allowCSOCnow)
			{
				edge->csOnCurveToUseAll.clear();
			}
			else
			{
				// create copy - in case we want to reverse and if we want to add autocsoc between
				edge->csOnCurveToUseAll = edge->csOnCurve;
			}

			edge->storeOnCurveToUse = edge->storeOnCurve;

			// reverse if needed as we want it to be in proper place
			if (edge->edgeRef.reversed)
			{
				float curveLength = edge->curve.length();

				// we need initial pos calculation to be placed properly to reverse in order
				for_every(cs, edge->csOnCurveToUseAll)
				{
					cs->calculate_pos(edge->betweenCorners, curveLength);
				}
				for_every(cs, edge->customOnCurveToUseAll)
				{
					cs->calculate_pos(edge->betweenCorners, curveLength);
				}
				ShapeUtils::CrossSectionOnCurve::reverse(edge->csOnCurveToUseAll, curveLength);
				ShapeUtils::CustomOnCurve::reverse(edge->customOnCurveToUseAll, curveLength);
				for_every(soc, edge->storeOnCurveToUse)
				{
					soc->onCurve.reverse_single(curveLength);
				}
			}
		}

		useAutoCSOCid = Name::invalid();
		if (useEdgeRef && autoCSOCid.is_set()
			&& (autoCSOClod == NONE ||
				(Game::get()->should_generate_at_lod(autoCSOClod) &&
				 _context.access_generation_context().should_generate_at_lod(autoCSOClod)))
			&& allowCSOCnow && allowAutoCSOCnow)
		{
			useAutoCSOCid = autoCSOCid.get(_context.access_element_instance().get_context());
			if (useAutoCSOCid.is_valid())
			{
				Random::Generator rg = _context.access_element_instance().access_context().access_random_generator().spawn();
				rg.advance_seed(29797245, 97254);
				auto* edge = useEdgeRef->get(_context.access_generation_context());
				if (!edge)
				{
					error(TXT("could not find edge \"%S\", \"%S\""), useEdgeRef->get_use_edge().get_value().to_string().to_char(), useEdgeRef->get_use_edge().get_value_param().get(Name::invalid()).to_char());
				}
				else if (auto* autoCSOC = edge->get_auto_csoc(useAutoCSOCid))
				{
					if (!autoCSOC->sequences.is_empty())
					{
						cornersOnly = false;

						float length = 0.0f;
						for_every_const_ptr(edge, useEdges)
						{
							float curveLength = edge->curve.length();
							length += curveLength;
							an_assert(curveLength < 100000.0f);
							an_assert(length < 100000.0f);
						}

						ARRAY_PREALLOC_SIZE(Range, csocSequenceRanges, autoCSOC->sequences.get_size());
						Range autoCSOCRange(0.0f);
						for_every(sequence, autoCSOC->sequences)
						{
							csocSequenceRanges.push_back(Range(0.0f));
							for_every(csoc, sequence->csocs)
							{
								float v = csoc->dist.get(0.0f);
								autoCSOCRange.include(v);
								csocSequenceRanges.get_last().include(v);
							}
						}

						float minLength = max(autoCSOCRange.length(), autoCSOC->minLength.get(0.0f));
						float borderStart = rg.get_float(autoCSOC->border.get(Range(max(abs(autoCSOCRange.min), abs(autoCSOCRange.max)))));
						float borderEnd = rg.get_float(autoCSOC->border.get(Range(max(abs(autoCSOCRange.min), abs(autoCSOCRange.max)))));
						Range spacingRange = autoCSOC->spacing.get(Range(autoCSOCRange.length() * 1.5f));

						if (minLength > 0.0f && length >= minLength)
						{
							if (loopedEdgeChain)
							{
								borderStart = 0.0f;
								borderEnd = 0.0f;
							}
							struct WhereToPut
							{
								EdgeMeshEdge const * edge;
								Range range;
								Range safeRange; // with extra border to tell where we can put things (we can't at cut/sharp/weld corners)
								float startsAt;
							};
							ARRAY_PREALLOC_SIZE(WhereToPut, whereToPut, edges.get_size());
							{
								float startsAt = 0.0f;
								EdgeMeshEdge* prevEdge = loopedEdgeChain? useEdges.get_last() : nullptr;
								for_every_const_ptr(edge, useEdges)
								{
									float curveLength = edge->curve.length();
									float endsAt = startsAt + curveLength;
									WhereToPut w;
									w.edge = edge;
									w.startsAt = startsAt;
									w.range = Range(startsAt, endsAt);
									w.safeRange = Range(startsAt - 100000.f, endsAt + 10000.0f); //
									if (autoCSOC->notAtCorners ||
										(prevEdge &&
										 (prevEdge->cornerType == EdgeMeshCorner::Sharp ||
										  prevEdge->cornerType == EdgeMeshCorner::Cut ||
										  prevEdge->cornerType == EdgeMeshCorner::Weld)))
									{
										w.safeRange.min = w.range.min + curveLength * edge->betweenCorners.min;
									}
									if (autoCSOC->notAtCorners ||
										(edge->cornerType == EdgeMeshCorner::Sharp ||
										 edge->cornerType == EdgeMeshCorner::Cut ||
										 edge->cornerType == EdgeMeshCorner::Weld))
									{
										w.safeRange.max = w.range.max - curveLength * (1.0f - edge->betweenCorners.max);
									}
									if (autoCSOC->cornerBorder.is_set())
									{
										float cornerBorderStart = rg.get_float(autoCSOC->cornerBorder.get());
										float cornerBorderEnd = rg.get_float(autoCSOC->cornerBorder.get());

										w.safeRange.min = max(w.safeRange.min, w.range.min + cornerBorderStart);
										w.safeRange.max = min(w.safeRange.max, w.range.max - cornerBorderEnd);
									}
									startsAt = endsAt;
									whereToPut.push_back(w);
								}

								if (!loopedEdgeChain && !whereToPut.is_empty())
								{
									whereToPut.get_first().safeRange.min = whereToPut.get_first().range.min + borderStart;
									whereToPut.get_last().safeRange.max = whereToPut.get_last().range.max - borderEnd;
								}
							}

							//

							if (autoCSOC->atEdgesCentreOnly || autoCSOC->addOnePerEdge)
							{
								ARRAY_PREALLOC_SIZE(CustomData::Edge::AutoCrossSectionOnCurve::Sequence const*, availableCSOCsequences, autoCSOC->sequences.get_size());
								for_every(w, whereToPut)
								{
									availableCSOCsequences.clear();
									for_every(seq, autoCSOC->sequences)
									{
										availableCSOCsequences.push_back(seq);
									}
									bool added = false;
									while (!added && !availableCSOCsequences.is_empty())
									{
										int idx = RandomUtils::ChooseFromContainer<Array<CustomData::Edge::AutoCrossSectionOnCurve::Sequence const *>, CustomData::Edge::AutoCrossSectionOnCurve::Sequence const *>::choose
											(rg, availableCSOCsequences, [](CustomData::Edge::AutoCrossSectionOnCurve::Sequence const* _seq) { return _seq->probCoef; });
										auto const* chosen = availableCSOCsequences[idx];
										availableCSOCsequences.remove_fast_at(idx);
										Range csocsAt = Range::empty;
										Range wActualRange = w->range;
										wActualRange.min = w->range.get_at(w->edge->betweenCorners.min);
										wActualRange.max = w->range.get_at(w->edge->betweenCorners.max);
										float effectiveLength = wActualRange.length();
										for_every(csoc, chosen->csocs)
										{
											float v = csoc->dist.get(0.0f) + csoc->pt.get(0.0f) * effectiveLength;
											csocsAt.include(v);
										}
										float csocsCentreAt = w->range.centre();
										if (autoCSOC->addOnePerEdge)
										{
											csocsCentreAt = rg.get_float(wActualRange.min - csocsAt.min, wActualRange.max - csocsAt.max);
										}
										// offset to the right location
										csocsAt.min += csocsCentreAt;
										csocsAt.max += csocsCentreAt;
										if (wActualRange.does_contain(csocsAt))
										{
											for_every(csoc, chosen->csocs)
											{
												float v = csoc->dist.get(0.0f) + csoc->pt.get(0.0f) * effectiveLength;
												w->edge->csOnCurveToUseAll.push_back(*csoc);
												w->edge->csOnCurveToUseAll.get_last().pt.clear();
												w->edge->csOnCurveToUseAll.get_last().dist = (csocsCentreAt+v) - w->startsAt;
											}
											if (chosen->createMeshNode.is_valid())
											{
												ShapeUtils::CustomOnCurve coc;
												coc.createMeshNode = chosen->createMeshNode;
												w->edge->customOnCurveToUseAll.push_back(coc);
											}
											added = true;
										}
									}
								}
							}
							else
							{
								Array<int> chosenCSOCsequences;
								Array<float> spacings;
								an_assert(length < 100000.0f);
								float lengthAvailable = max(0.0f, length + -borderStart - borderEnd);
								float lengthTaken = 0.0f;
								int count = 0;
								{
									float lengthLeft = lengthAvailable;

									{
										// enforce ending (first one) if has to be enforced
										for_every(seq, autoCSOC->sequences)
										{
											if (seq->enforceAtEnds)
											{
												int seqIdx = for_everys_index(seq);
												chosenCSOCsequences.push_back(seqIdx);
												{
													float sequenceLengthNow = csocSequenceRanges[seqIdx].length();
													lengthTaken += sequenceLengthNow;
													lengthLeft -= sequenceLengthNow;
												}
												{
													float spacingNow = rg.get_float(spacingRange);
													spacingNow = min(spacingNow, lengthLeft / 2.0f);
													spacings.push_back(spacingNow);
													lengthLeft -= spacingNow;
												}
												++count;
												break;
											}
										}
									}

									while (lengthLeft > 0.0f)
									{
										chosenCSOCsequences.push_back(RandomUtils::ChooseFromContainer<List<CustomData::Edge::AutoCrossSectionOnCurve::Sequence>, CustomData::Edge::AutoCrossSectionOnCurve::Sequence>::choose
											(rg, autoCSOC->sequences, [](CustomData::Edge::AutoCrossSectionOnCurve::Sequence const & _seq) { return _seq.probCoef; }));
										bool isOk = false;
										float sequenceLengthNow = csocSequenceRanges[chosenCSOCsequences.get_last()].length();
										an_assert(lengthLeft < 100000.0f);
										if (lengthLeft >= sequenceLengthNow)
										{
											lengthLeft -= sequenceLengthNow;
											an_assert(lengthLeft < 100000.0f);
											lengthTaken += sequenceLengthNow;
											++count;
											float spacingNow = rg.get_float(spacingRange);
											isOk = true;
											if (lengthLeft > spacingNow)
											{
												lengthLeft -= spacingNow;
												an_assert(lengthLeft < 100000.0f);
												spacings.push_back(spacingNow);
											}
											else
											{
												break;
											}
										}
										an_assert(lengthLeft < 100000.0f);
										if (!isOk)
										{
											chosenCSOCsequences.pop_back();
											break;
										}
									}

									if (count >= 2) // it has to have at least the first one (enforced ending) and the one added
									{
										// enforce ending if has to be enforced
										for_every(seq, autoCSOC->sequences)
										{
											if (seq->enforceAtEnds)
											{
												// reuse the last one
												auto& csLast = chosenCSOCsequences.get_last();
												lengthTaken += csocSequenceRanges[csLast].length();
												int seqIdx = for_everys_index(seq);
												csLast = seqIdx;
												float sequenceLengthNow = csocSequenceRanges[seqIdx].length();
												{
													lengthTaken += sequenceLengthNow;
													lengthLeft -= sequenceLengthNow;
												}
												break;
											}
										}
									}
								}
								if (count > 0)
								{
									if (spacings.get_size() >= chosenCSOCsequences.get_size())
									{
										spacings.set_size(chosenCSOCsequences.get_size() - 1);
									}

									for_every(spacing, spacings)
									{
										lengthTaken += *spacing;
									}

									// rescale spacings
									for_every(spacing, spacings)
									{
										*spacing *= lengthAvailable / lengthTaken;
									}

									float at = count > 1 ? borderStart : length * 0.5f + csocSequenceRanges[chosenCSOCsequences.get_first()].min;
									for_count(int, i, count)
									{
										int seqIdx = chosenCSOCsequences[i];
										auto const & sequence = autoCSOC->sequences[seqIdx];
										at += -csocSequenceRanges[seqIdx].min; // centre
										bool fitsSomewhere = false;
										{	// check if fits completely within one (safe) range, safe ranges are wider for joined curves, so we're just looking for cases of cut/sharp/weld corners to not place on them
											float startAt = at + csocSequenceRanges[seqIdx].min;
											float endAt = at + csocSequenceRanges[seqIdx].max;
											for_every(w, whereToPut)
											{
												if (w->safeRange.does_contain(startAt) &&
													w->safeRange.does_contain(endAt))
												{
													fitsSomewhere = true;
												}
											}
										}
										if (fitsSomewhere)
										{
											for_every(csoc, sequence.csocs)
											{
												float csocAt = at + csoc->dist.get(0.0f);
												for_every(w, whereToPut)
												{
													if (w->range.does_contain(csocAt))
													{
														w->edge->csOnCurveToUseAll.push_back(*csoc);
														w->edge->csOnCurveToUseAll.get_last().pt.clear();
														w->edge->csOnCurveToUseAll.get_last().dist = csocAt - w->startsAt;
														// this may add to multiple curves
													}
												}
											}
											if (sequence.createMeshNode.is_valid())
											{
												float csocAt = at;
												for_every(w, whereToPut)
												{
													if (w->range.does_contain(csocAt))
													{
														ShapeUtils::CustomOnCurve coc;
														coc.createMeshNode = sequence.createMeshNode;
														coc.dist = csocAt - w->startsAt;
														w->edge->customOnCurveToUseAll.push_back(coc);
														// just add in the first one we find
														break;
													}
												}
											}
										}
										at += csocSequenceRanges[seqIdx].max;
										if (spacings.is_index_valid(i))
										{
											at += spacings[i];
										}
									}
								}
							}
						}
					}
				}
			}
		}

		for_every_const_ptr(edge, useEdges)
		{
			float curveLength = edge->curve.length();

			edge->csOnCurveToUseClamped.clear();
			edge->csOnCurveToUseClamped.make_space_for(edge->csOnCurveToUseAll.get_size());

			edge->customOnCurveToUseClamped.clear();
			edge->customOnCurveToUseClamped.make_space_for(edge->customOnCurveToUseAll.get_size());

			{
				// now as we are properly ordered (reversed or not) we can finally
				// calculate pos for cross sections on curve and for store on curve
				Range betweenLengths = edge->betweenCorners * curveLength;
				Range included = Range::empty;
				int lastBelowMin = NONE; float lastBelowMinPos;
				int firstAboveMax = NONE; float firstAboveMaxPos;
				for_every(cs, edge->csOnCurveToUseAll)
				{
					if (cs->calculate_pos(edge->betweenCorners, curveLength))
					{
						edge->csOnCurveToUseClamped.push_back(*cs);
						included.include(cs->get_pos());
					}
					else
					{
						float posNotClamped = cs->get_pos_not_clamped();
						if (posNotClamped < betweenLengths.min &&
							(lastBelowMin == NONE || posNotClamped >= lastBelowMinPos))
						{
							lastBelowMin = for_everys_index(cs);
							lastBelowMinPos = posNotClamped;
						}
						if (posNotClamped > betweenLengths.max &&
							(firstAboveMax == NONE || posNotClamped < firstAboveMaxPos))
						{
							firstAboveMax = for_everys_index(cs);
							firstAboveMaxPos = posNotClamped;
						}
					}
				}
				// to have anything in, if clamped, not included is closer, add it
				// if we have discontinuity issue, an element will be added earlier
				{
					if (lastBelowMin != NONE && (included.is_empty() || betweenLengths.min - lastBelowMinPos < included.min - betweenLengths.min))
					{
						edge->csOnCurveToUseClamped.push_front(edge->csOnCurveToUseAll[lastBelowMin]);
					}
					if (firstAboveMax != NONE && (included.is_empty() || firstAboveMaxPos - betweenLengths.max < betweenLengths.max - included.max))
					{
						edge->csOnCurveToUseClamped.push_back(edge->csOnCurveToUseAll[firstAboveMax]);
					}
				}
			}
			{
				for_every(coc, edge->customOnCurveToUseAll)
				{
					if (coc->calculate_pos(edge->betweenCorners, curveLength))
					{
						edge->customOnCurveToUseClamped.push_back(*coc);
					}
				}
			}
			for_every(soc, edge->storeOnCurveToUse)
			{
				soc->onCurve.calculate_pos(edge->betweenCorners, curveLength);
			}
		}

		// reset skinning and check if there is skinning info available, use unified array to handle
		Array<EdgeMeshSkinningInfo> skinning;
		{
			float lengthSoFar = 0.0f;
			for_every_const_ptr(edge, useEdges)
			{
				edge->skinningToUse.clear();

				float curveLength = edge->curve.length();
				for_every_const(skinningElement, edge->skinning)
				{
					float actualPT = edge->edgeRef.reversed ? 1.0f - skinningElement->pt : skinningElement->pt;
					float atTotal = lengthSoFar + curveLength * actualPT;
					EdgeMeshSkinningInfo addSkinningElement = *skinningElement;
					addSkinningElement.pt = atTotal;
					skinning.push_back(addSkinningElement);
				}
				lengthSoFar += curveLength;
			}
		}

		if (!skinning.is_empty())
		{
			// sort them
			sort(skinning);
			float lengthSoFar = 0.0f;
			int idx = 0;
			for_every_const_ptr(edge, useEdges)
			{
				if (idx >= skinning.get_size())
				{
					// just use last available
					edge->skinningToUse.push_back(skinning.get_last());
					edge->skinningToUse.get_last().pt = 0.0f;
				}
				else if (skinning[idx].pt > lengthSoFar)
				{
					if (idx == 0)
					{
						// add very first
						edge->skinningToUse.push_back(skinning[idx]);
						edge->skinningToUse.get_last().pt = 0.0f;
					}
					else
					{
						// blend between last and current
						edge->skinningToUse.push_back(EdgeMeshSkinningInfo::blend(skinning[idx - 1], skinning[idx], lengthSoFar));
						edge->skinningToUse.get_last().pt = 0.0f;
					}
				}

				// add all for this edge
				float curveLength = edge->curve.length();
				while (idx < skinning.get_size())
				{
					if (skinning[idx].pt <= lengthSoFar + curveLength)
					{
						edge->skinningToUse.push_back(skinning[idx]);
						edge->skinningToUse.get_last().pt = EdgeMeshSkinningInfo::at_to_pt(edge->skinningToUse.get_last().pt, lengthSoFar, curveLength);
					}
					else
					{
						// won't be useful here
						break;
					}
					if (skinning[idx].pt == lengthSoFar + curveLength)
					{
						// just collect first one
						break;
					}
					++idx;
				}

				// add last one if we didn't close properly
				if (edge->skinningToUse.get_last().pt < 1.0f)
				{
					if (idx >= skinning.get_size())
					{
						// last available
						edge->skinningToUse.push_back(skinning.get_last());
						edge->skinningToUse.get_last().pt = 1.0f;
					}
					else
					{
						// blend between prev and current
						edge->skinningToUse.push_back(EdgeMeshSkinningInfo::blend(skinning[idx - 1], skinning[idx], lengthSoFar + curveLength));
						edge->skinningToUse.get_last().pt = 1.0f;
					}
				}
				lengthSoFar += curveLength;

				if (_context.access_element_instance().get_context().has_skeleton())
				{
					// sort ouf bones
					for_every(skinningInfo, edge->skinningToUse)
					{
						Name boneA = skinningInfo->boneA.get(_context.access_element_instance().get_context());
						Name boneB = skinningInfo->boneB.is_set() ? skinningInfo->boneB.get(_context.access_element_instance().get_context()) : Name::invalid();
						skinningInfo->boneAIdx = _context.access_element_instance().get_context().find_bone_index(boneA);
						skinningInfo->boneBIdx = _context.access_element_instance().get_context().find_bone_index(boneB.is_valid()? boneB : boneA);
					}
				}
				else
				{
					warn(TXT("no skeleton, what should we do with skinning then?"));
				}
			}
		}
	}

	return result;
}

//

bool SurfaceMeshForEdge::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	// <generateSurfaceMesh uParam="uParamName" reversed="false" useEdgeEndPointsB="true" useOnePointAsCommon="false"/>
	useEdgeEndPointsA = _node->get_bool_attribute_or_from_child_presence(TXT("useEdgeEndPointsA"), useEdgeEndPointsA);
	useEdgeEndPointsB = _node->get_bool_attribute_or_from_child_presence(TXT("useEdgeEndPointsB"), useEdgeEndPointsB);
	useOnePointAsCommon = _node->get_bool_attribute_or_from_child_presence(TXT("useOnePointAsCommon"), useOnePointAsCommon);
	useOnePointAsCommon = ! _node->get_bool_attribute_or_from_child_presence(TXT("useCentrePointAsCommon"), ! useOnePointAsCommon);

	result &= tuckInwards.load_from_xml(_node, TXT("tuckInwards"));
	
	return result;
}

//

bool SurfaceMesh::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;

	createMovementCollisionMesh.load_from_xml(_node, TXT("createMovementCollisionMesh"), _lc);
	createMovementCollisionBox.load_from_xml(_node, TXT("createMovementCollisionBox"), _lc);
	createPreciseCollisionMesh.load_from_xml(_node, TXT("createPreciseCollisionMesh"), _lc);
	error_loading_xml_on_assert(!_node->first_child_named(TXT("createPreciseCollisionMeshSkinned")), result, _node, TXT("createPreciseCollisionMeshSkinned not handled"));
	createSpaceBlocker.load_from_xml(_node, TXT("createSpaceBlocker"));
	noMesh = _node->get_bool_attribute_or_from_child_presence(TXT("noMesh"), noMesh);

	id = _node->get_name_attribute(TXT("id"), id);

	result &= generationCondition.load_from_xml(_node);

	forceMaterialIdx.load_from_xml(_node, TXT("forceMaterialIdx"));
	uvDef.load_from_xml_with_clearing(_node);

	result &= extrusion.load_from_xml(_node, TXT("extrusion"));
	result &= extrusionU.load_from_xml(_node, TXT("extrusionU"));
	result &= extrusionTowards.load_from_xml(_node, TXT("extrusionTowards"));
	result &= extrusionNormal.load_from_xml(_node, TXT("extrusionNormal"));
	result &= extrusionFill.load_from_xml(_node, TXT("extrusionFill"));
	result &= useNotExtrudedCurveForSubStep.load_from_xml(_node, TXT("useNotExtrudedCurveForSubStep"));
	extrusionSurfaceGroup = _node->get_name_attribute(TXT("extrusionSurfaceGroup"), extrusionSurfaceGroup);

	for_every(surfaceNode, _node->children_named(TXT("surface")))
	{
		Name id = surfaceNode->get_name_attribute(TXT("id"));
		if (id.is_valid())
		{
			surfaceIds.push_back(id);
		}
		else
		{
			error_loading_xml(surfaceNode, TXT("no \"id\" for surface"));
			result = false;
		}
	}

	reversed = _node->get_bool_attribute_or_from_child_presence(TXT("reversed"), reversed);

	return result;
}

bool SurfaceMesh::prepare_for_generation(EdgesAndSurfacesContext & _context, PrepareForGenerationLevel::Type _level) const
{
	bool result = true;

	if (!generationCondition.check(_context.access_element_instance()))
	{
		return true;
	}

	if (PrepareForGenerationLevel::should_resolve_auto_generated(isAutoGenerated, _level))
	{
		surfaces.clear();
		for_every_const(surfaceId, surfaceIds)
		{
			if (Surface const * surface = _context.get_edges_and_surfaces_data()->find_surface(*surfaceId))
			{
				surfaces.push_back(surface);
			}
			else
			{
				error_generating_mesh(_context.access_element_instance(), TXT("could not find surface \"%S\" for surface mesh"), surfaceId->to_char());
				result = false;
			}
		}
	}

	return result;
}

//

bool EdgePlaceOnEdgeAt::load_from_xml(IO::XML::Node const * _node)
{
	bool result = true;

	pt.load_from_xml(_node, TXT("pt"));
	spacing.load_from_xml(_node, TXT("spacing"));
	count.load_from_xml(_node, TXT("count"));
	coverWholeLength.load_from_xml(_node, TXT("coverWholeLength"));
	updateElementSize.load_from_xml(_node, TXT("updateElementSize"));
	skipEnds.load_from_xml(_node, TXT("skipEnds"));
	atLeastOne.load_from_xml(_node, TXT("atLeastOne"));
	dontAddLastOne.load_from_xml(_node, TXT("dontAddLastOne"));
	dontAddFirstOne.load_from_xml(_node, TXT("dontAddFirstOne"));

	adjustStartAndEndElements = _node->get_bool_attribute(TXT("adjustStartAndEndElements"), adjustStartAndEndElements);

	if (IO::XML::Attribute const * attr = _node->get_attribute(TXT("alignment")))
	{
		if (attr->get_as_string() == TXT("start"))
		{
			alignment = EdgeMeshPlaceAlignment::Start;
		}
		else if (attr->get_as_string() == TXT("end"))
		{
			alignment = EdgeMeshPlaceAlignment::End;
		}
		else if (attr->get_as_string() == TXT("centre"))
		{
			alignment = EdgeMeshPlaceAlignment::Centre;
		}
		else if (attr->get_as_string() == TXT("cover whole"))
		{
			alignment = EdgeMeshPlaceAlignment::Centre;
			coverWholeLength = true;
		}
		else
		{
			error_loading_xml(_node, TXT("not recognised alignment \"%S\""), attr->get_as_string().to_char());
			result = false;
		}
	}
	return result;
}

//

bool EdgePlaceOnEdgeEdge::load_from_xml(IO::XML::Node const * _node)
{
	bool result = true;

	edgeRef.load_from_xml(_node);
	asInEdgeMesh = _node->get_name_attribute(TXT("asInEdgeMesh"), asInEdgeMesh);
	result &= surfacesContext.load_from_xml(_node);

	for_every(container, _node->children_named(TXT("where")))
	{
		for_every(node, container->children_named(TXT("at")))
		{
			EdgePlaceOnEdgeAt at;
			if (at.load_from_xml(node))
			{
				ats.push_back(at);
			}
			else
			{
				error_loading_xml(node, TXT("couldn't load place-on-edge's info about placement"));
				result = false;
			}
		}
	}

	return result;
}

bool EdgePlaceOnEdgeEdge::prepare_for_generation(EdgePlaceOnEdge const * _owner, EdgesAndSurfacesContext & _context, PrepareForGenerationLevel::Type _level) const
{
	bool result = true;

	if (_level == PrepareForGenerationLevel::PlaceOnEdges)
	{
		result &= edgeRef.prepare_for_generation(_context);
		edgeMesh = nullptr;
		edgeMeshEdge = nullptr;
		EdgeMeshEdge const * prevEdgeMeshEdge = nullptr;
		if (asInEdgeMesh.is_valid())
		{
			edgeMesh = _context.get_edges_and_surfaces_data()->find_edge_mesh(asInEdgeMesh);
			if (edgeMesh)
			{
				for_every_ptr(edge, edgeMesh->useEdges)
				{
					if (edgeRef.id == edge->edgeRef.id)
					{
						edgeMeshEdge = edge;
						break;
					}
					prevEdgeMeshEdge = edge;
				}
				if (!edgeMeshEdge)
				{
					error_generating_mesh(_context.access_element_instance(), TXT("there's no edge \"%S\" in edge mesh \"%S\""), edgeRef.id, asInEdgeMesh.to_char());
					result = false;
				}
			}
			else
			{
				error_generating_mesh(_context.access_element_instance(), TXT("couldn't find edge mesh \"%S\""), asInEdgeMesh.to_char());
				result = false;
			}
		}
		if (edgeMeshEdge)
		{
			curve = edgeMeshEdge->curve;
			curveRange = edgeMeshEdge->betweenCorners;
			if (prevEdgeMeshEdge)
			{
				if (prevEdgeMeshEdge->cornerType == EdgeMeshCorner::Sharp ||
					prevEdgeMeshEdge->cornerType == EdgeMeshCorner::Cut ||
					prevEdgeMeshEdge->cornerType == EdgeMeshCorner::Weld)
				{
					curveRange.min = 0.0f;
				}
			}
			if (edgeMeshEdge->cornerType == EdgeMeshCorner::Sharp ||
				edgeMeshEdge->cornerType == EdgeMeshCorner::Cut ||
				edgeMeshEdge->cornerType == EdgeMeshCorner::Weld)
			{
				curveRange.max = 1.0f;
			}
		}
		else
		{
			curve = edgeRef.edge->curve.reversed(edgeRef.reversed);
			curveRange = Range(0.0f, 1.0f);
		}
		curveLength = curve.distance(curveRange.min, curveRange.max);
		Name const usg = surfacesContext.get_use_surface_group(_context, _owner->surfacesContext.get_use_surface_group(_context));
		surfacesOnSides.build(edgeRef, usg, _context.get_edges_and_surfaces_data()->surfaces);
	}

	return result;
}

//

bool EdgePlaceOnEdgeElements::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;

	if (!_node)
	{
		return true;
	}

	size.load_from_xml(_node, TXT("size"));

	elementsSizeParamName = _node->get_name_attribute(TXT("elementsSizeParamName"), elementsSizeParamName);
	spacingParamName = _node->get_name_attribute(TXT("spacingParamName"), spacingParamName);
	firstElementParamName = _node->get_name_attribute(TXT("firstElementParamName"), firstElementParamName);
	lastElementParamName = _node->get_name_attribute(TXT("lastElementParamName"), lastElementParamName);

	for_every(nodeContainer, _node->children_named(TXT("elements")))
	{
		for_every(node, nodeContainer->all_children())
		{
			if (Element* element = Element::create_from_xml(node, _lc))
			{
				if (element->load_from_xml(node, _lc))
				{
					elements.push_back(RefCountObjectPtr<Element>(element));
				}
				else
				{
					error_loading_xml(node, TXT("problem loading one element for edges & surfaces"));
					result = false;
				}
			}
		}
	}

	return result;
}

bool EdgePlaceOnEdgeElements::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	for_every(element, elements)
	{
		result &= element->get()->prepare_for_game(_library, _pfgContext);
	}

	return result;
}

#ifdef AN_DEVELOPMENT
void EdgePlaceOnEdgeElements::for_included_mesh_generators(std::function<void(MeshGenerator*)> _do) const
{
	for_every(element, elements)
	{
		element->get()->for_included_mesh_generators(_do);
	}
}
#endif

//

bool EdgePlaceOnEdge::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;

	id = _node->get_name_attribute(TXT("id"), id);

	result &= generationCondition.load_from_xml(_node);

	result &= surfacesContext.load_from_xml(_node);

	onEdgeMesh = _node->get_name_attribute(TXT("onEdgeMesh"), onEdgeMesh);

	result &= placement.load_from_xml_child_node(_node, TXT("placement"));

	for_every(container, _node->children_named(TXT("notOnEdges")))
	{
		for_every(node, container->children_named(TXT("edge")))
		{
			Name edgeId = node->get_name_attribute_or_from_child(TXT("id"), Name::invalid());
			if (edgeId.is_valid())
			{
				notOnEdges.push_back(edgeId);
			}
		}
	}

	for_every(container, _node->children_named(TXT("edges")))
	{
		for_every(node, container->children_named(TXT("edge")))
		{
			EdgePlaceOnEdgeEdge edge;
			if (edge.load_from_xml(node))
			{
				edges.push_back(edge);
			}
			else
			{
				error_loading_xml(node, TXT("couldn't load place-on-edge's info about edge"));
				result = false;
			}
		}
	}

	for_every(container, _node->children_named(TXT("where")))
	{
		for_every(node, container->children_named(TXT("at")))
		{
			EdgePlaceOnEdgeAt at;
			if (at.load_from_xml(node))
			{
				ats.push_back(at);
			}
			else
			{
				error_loading_xml(node, TXT("couldn't load place-on-edge's info about placement"));
				result = false;
			}
		}
	}

	if (ats.is_empty())
	{
		// auto add full pt - will have to depend on size
		EdgePlaceOnEdgeAt at;
		at.pt = Range(0.0f, 1.0f);
		ats.push_back(at);
	}

	result &= atStartElements.load_from_xml(_node->first_child_named(TXT("atStart")), _lc);
	result &= mainElements.load_from_xml(_node->first_child_named(TXT("main")), _lc);
	result &= atEndElements.load_from_xml(_node->first_child_named(TXT("atEnd")), _lc);

	alongDirAxis = Axis::parse_from(_node->get_string_attribute(TXT("alongDirAxis")), alongDirAxis);

	return result;
}

bool EdgePlaceOnEdge::prepare_for_generation(EdgesAndSurfacesContext & _context, PrepareForGenerationLevel::Type _level) const
{
	bool result = true;

	if (_level == PrepareForGenerationLevel::PlaceOnEdges)
	{
		if (onEdgeMesh.is_valid())
		{
			// gather all curves
			useEdges.clear();
			edgeMesh = _context.get_edges_and_surfaces_data()->find_edge_mesh(onEdgeMesh);
			if (edgeMesh)
			{
				if (! edgeMesh->useEdges.is_empty())
				{
					EdgeMeshEdge const * prevEdgeMeshEdge = edgeMesh->loopedEdgeChain ? edgeMesh->useEdges.get_last() : nullptr;
					for_every_ptr(edgeMeshEdge, edgeMesh->useEdges)
					{
						if (notOnEdges.does_contain(edgeMeshEdge->edgeRef.id))
						{
							break;
						}
						EdgePlaceOnEdgeEdge e;
						e.edgeMesh = edgeMesh;
						e.edgeMeshEdge = edgeMeshEdge;
						e.curve = edgeMeshEdge->curve;
						e.curveRange = edgeMeshEdge->betweenCorners;
						if (prevEdgeMeshEdge && ! notOnEdges.does_contain(prevEdgeMeshEdge->edgeRef.id))
						{
							if (prevEdgeMeshEdge->cornerType == EdgeMeshCorner::Round)
							{
								// add round as separate curve
								EdgePlaceOnEdgeEdge r;
								r.cornerScale = prevEdgeMeshEdge->cornerScale.is_set()? prevEdgeMeshEdge->cornerScale.get(_context.access_generation_context()) : Vector2::one;
								r.cornerScaleFloat = prevEdgeMeshEdge->cornerScaleFloat.is_set()? prevEdgeMeshEdge->cornerScaleFloat.get(_context.access_generation_context()) : 1.0f;
								r.curve = prevEdgeMeshEdge->roundCorner.curve;
								r.curveRange = Range(0.0f, 1.0f);
								r.curveLength = r.curve.distance();
								r.useNormals = true;
								r.startNormal = prevEdgeMeshEdge->roundCorner.prevNormal;
								r.endNormal = prevEdgeMeshEdge->roundCorner.nextNormal;
								useEdges.push_back(r);
							}
							if (prevEdgeMeshEdge->cornerType == EdgeMeshCorner::Sharp ||
								prevEdgeMeshEdge->cornerType == EdgeMeshCorner::Cut ||
								prevEdgeMeshEdge->cornerType == EdgeMeshCorner::Weld)
							{
								e.curveRange.min = 0.0f;
							}
						}
						if (edgeMeshEdge->cornerType == EdgeMeshCorner::Sharp ||
							edgeMeshEdge->cornerType == EdgeMeshCorner::Cut ||
							edgeMeshEdge->cornerType == EdgeMeshCorner::Weld)
						{
							e.curveRange.max = 1.0f;
						}
						e.curveLength = e.curve.distance(e.curveRange.min, e.curveRange.max);
						e.surfacesOnSides = edgeMeshEdge->surfacesOnSides; // copy them, we should be prepared later than edgeMeshEdge
						prevEdgeMeshEdge = edgeMeshEdge;
						useEdges.push_back(e);
					}
				}
			}
			else
			{
				error_generating_mesh(_context.access_element_instance(), TXT("couldn't find edge mesh \"%S\""), onEdgeMesh.to_char());
				result = false;
			}
		}
		else
		{
			useEdges = edges;
			for_every_const(edge, useEdges)
			{
				result &= edge->prepare_for_generation(this, _context, _level);
			}
		}
	}

	return result;
}

bool EdgePlaceOnEdge::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	result &= atStartElements.prepare_for_game(_library, _pfgContext);
	result &= mainElements.prepare_for_game(_library, _pfgContext);
	result &= atEndElements.prepare_for_game(_library, _pfgContext);

	return result;
}

#ifdef AN_DEVELOPMENT
void EdgePlaceOnEdge::for_included_mesh_generators(std::function<void(MeshGenerator*)> _do) const
{
	atStartElements.for_included_mesh_generators(_do);
	mainElements.for_included_mesh_generators(_do);
	atEndElements.for_included_mesh_generators(_do);
}
#endif

static float calculate_local_t(float dist, EdgePlaceOnEdgeEdge const * edge)
{
	float localT = (edge->curveLength != 0.0f ? dist / edge->curveLength : 0.0f) * (edge->curveRange.max - edge->curveRange.min) + edge->curveRange.min;
	an_assert(localT >= 0.0f && localT <= 1.0f);
	return clamp(localT, 0.0f, 1.0f);
}

EdgePlaceOnEdgeEdge* EdgePlaceOnEdge::find_edge_among_use_edges(REF_ float & _atDist, OUT_ float & _localT)
{
	if (_atDist < 0.0f)
	{
		// first one
		auto* edge = &useEdges.get_first();
		_atDist = 0.0f;
		an_assert(_atDist >= 0.0f && _atDist <= edge->curveLength);
		_localT = calculate_local_t(_atDist, edge);
		return edge;
	}
	for_every(edge, useEdges)
	{
		float ndist = _atDist - edge->curveLength;
		if (ndist < 0.0f)
		{
			// middle one
			an_assert(_atDist >= 0.0f && _atDist <= edge->curveLength);
			_localT = calculate_local_t(_atDist, edge);
			return edge;
		}
		_atDist = ndist;
	}
	{
		// last one
		auto* edge = &useEdges.get_last();
		_atDist = edge->curveLength;
		_localT = clamp((edge->curveLength != 0.0f ? _atDist / edge->curveLength : 1.0f) * (edge->curveRange.max - edge->curveRange.min) + edge->curveRange.min, 0.0f, 1.0f);
		_localT = calculate_local_t(_atDist, edge);
		return edge;
	}
}

int EdgePlaceOnEdge::calculate_count_for(EdgesAndSurfacesContext & _context, EdgePlaceOnEdgeAt const * _at, float _curveLengthAvailable, bool _looped, tchar const * _edgeId, OUT_ EdgeMeshPlaceAlignment::Type & _alignment, OUT_ float & _spacing, OUT_ bool & _skipEnds, OUT_ bool & _atLeastOne) const
{
	float mainElementsSize = mainElements.size.get(_context.access_element_instance().get_context());

	float & spacing = _spacing;
	bool & skipEnds = _skipEnds;
	bool & atLeastOne = _atLeastOne;

	spacing = _at->spacing.is_set() ? _at->spacing.get(_context.access_element_instance().get_context()) : 0.0f;
	float spaceRequiredForOneObject = mainElementsSize + spacing;
	bool const coverWholeLength = _at->coverWholeLength.is_set() ? _at->coverWholeLength.get(_context.access_element_instance().get_context()) : false;
	bool const updateElementSize = _at->updateElementSize.is_set() ? _at->updateElementSize.get(_context.access_element_instance().get_context()) : false;
	skipEnds = _at->skipEnds.is_set() ? _at->skipEnds.get(_context.access_element_instance().get_context()) : false;
	atLeastOne = _at->atLeastOne.is_set() ? _at->atLeastOne.get(_context.access_element_instance().get_context()) : false;

	// calculate how many should there be
	int count = 0;
	Optional<RangeInt> providedCount;
	if (_at->count.is_set())
	{
		if (_at->count.get_value_param().is_set())
		{
			if (int const * value = _context.access_element_instance().get_context().get_parameter<int>(_at->count.get_value_param().get()))
			{
				providedCount = RangeInt(*value);
			}
		}
		if (!providedCount.is_set())
		{
			providedCount = _at->count.get(_context.access_element_instance().get_context());
		}
		count = providedCount.get().min;
	}

	// calculate how many objects will fit here
	if (spaceRequiredForOneObject == 0.0f)
	{
		if (count == 0)
		{
			if (providedCount.is_set())
			{
				count = providedCount.get().max;
			}
			if (count == 0)
			{
				error_generating_mesh(_context.access_element_instance(), TXT("can't tell how many objects to place! \"%S\", edge \"%S\""), id.to_char(), _edgeId ? _edgeId : TXT("??"));
				return -1;
			}
		}
	}
	else
	{
		// l = c * (s + sp) - sp		l curve length available		c count
		// c * (s + sp) = l + sp		s size							sp spacing
		// c = (l + sp) / (s + sp)
		// divide to get result
		count = (int)(floor((_curveLengthAvailable + spacing) / spaceRequiredForOneObject + MARGIN));
		if (coverWholeLength && count > 1)
		{
			float fillLength = _curveLengthAvailable;
			if (skipEnds)
			{
				fillLength += mainElementsSize * 2.0f;
			}
			// c * (s + sp) = l + sp
			// s + sp = (l + sp) / c;
			// s = (l + sp) / c - sp
			if (updateElementSize)
			{
				float newMainElementsSize = (fillLength + spacing) / ((float)count) - spacing;
				an_assert(newMainElementsSize >= mainElementsSize);
				mainElementsSize = newMainElementsSize;
			}
			else
			{
				float newSpacing = (fillLength) / ((float)count - 1.0f);
				an_assert(newSpacing >= spacing - MARGIN || newSpacing >= spacing * 0.99f, TXT("%.3f >= %.3f ?"), newSpacing, spacing);
				spacing = newSpacing;
			}
			spaceRequiredForOneObject = mainElementsSize + spacing;
			if (skipEnds)
			{
				// we will just drop that ends and place them evenly through whole thing aligning to centre
				count -= 2;
				skipEnds = false;
			}
			_alignment = EdgeMeshPlaceAlignment::Centre;
		}
		count = max(count, 0);
	}

	if (providedCount.is_set())
	{
		if (!providedCount.get().does_contain(count))
		{
			warn(TXT("provided count for place on edge \"%S\", edge \"%S\" does not hold calculated count %i, clamping"), id.to_char(), _edgeId ? _edgeId : TXT("??"), count);
			count = providedCount.get().clamp(count);
		}
	}

	if (atLeastOne)
	{
		count = max(count, 1);
	}

	return count;
}

//

bool Poly::load_from_xml(IO::XML::Node const * _node)
{
	bool result = true;

	bone.load_from_xml(_node, TXT("bone"));
	offset.load_from_xml_child_node(_node, TXT("offset"));

	nodeAId = _node->get_name_attribute(TXT("a"), nodeAId);
	nodeBId = _node->get_name_attribute(TXT("b"), nodeBId);
	nodeCId = _node->get_name_attribute(TXT("c"), nodeCId);
	nodeDId = _node->get_name_attribute(TXT("d"), nodeDId);

	if (! nodeAId.is_valid())
	{
		error_loading_xml(_node, TXT("nodeAId is missing!"));
	}
	if (!nodeBId.is_valid())
	{
		error_loading_xml(_node, TXT("nodeBId is missing!"));
	}
	if (!nodeCId.is_valid())
	{
		error_loading_xml(_node, TXT("nodeCId is missing!"));
	}
	// note: nodeDId is not required

	return result;
}

bool Poly::prepare_for_generation(EdgesAndSurfacesContext & _context, PrepareForGenerationLevel::Type _level) const
{
	bool result = true;

	if (_level == PrepareForGenerationLevel::Resolve)
	{
		boneIdx = bone.is_set()? _context.access_element_instance().get_context().find_bone_index(bone.get(_context.access_generation_context())) : NONE;
		offsetAct = offset.is_set()? offset.get(_context.access_generation_context()) : Vector3::zero;
		nodeA = _context.get_edges_and_surfaces_data()->find_node(nodeAId);
		nodeB = _context.get_edges_and_surfaces_data()->find_node(nodeBId);
		nodeC = _context.get_edges_and_surfaces_data()->find_node(nodeCId);
		nodeD = _context.get_edges_and_surfaces_data()->find_node(nodeDId);
	}

	return result;
}

//

bool AutoConvex::load_from_xml(IO::XML::Node const* _node)
{
	bool result = true;

	autoConvexGenerateSurfaces = false; // clear, we will load it

	bool autoConvexAllExplicit = false;

	autoConvexNoMirrorX = _node->get_bool_attribute_or_from_child_presence(TXT("autoConvexNoMirrorX"), autoConvexNoMirrorX);
	if (_node->first_child_named(TXT("autoConvex")))
	{
		autoConvex = AutoConvex::All;
	}
	if (auto* attr = _node->get_attribute(TXT("autoConvex")))
	{
		if (attr->get_as_string() == TXT("noMirrorX"))
		{
			autoConvex = AutoConvex::All;
			autoConvexNoMirrorX = true;
		}
		else if (attr->get_as_string() == TXT("all"))
		{
			autoConvex = AutoConvex::All;
			autoConvexAllExplicit = true;
		}
		else if (attr->get_as_string() == TXT("no"))
		{
			autoConvex = AutoConvex::No;
		}
		else
		{
			error_loading_xml(_node, TXT("couldn't recognise autoConvex \"%S\""), attr->get_as_string().to_char());
			result = false;
		}
	}

	for_every(node, _node->all_children())
	{
		if (node->get_name() == TXT("autoConvexGenerateSurfaces"))
		{
			autoConvexGenerateSurfaces = true;
			autoConvexGenerateSurfacesOnly = node->get_bool_attribute_or_from_child_presence(TXT("surfacesOnly"), autoConvexGenerateSurfacesOnly);
			result &= surfacesUVDef.load_from_xml(_node); // share with polygons
			result &= surfacesUVDef.load_from_xml(node); // but allow to override
		}
		if (node->get_name() == TXT("addAutoConvex"))
		{
			Name id = node->get_name_attribute(TXT("node"));
			if (id.is_valid())
			{
				autoConvexNodes.push_back(id);
			}
			else
			{
				error_loading_xml(node, TXT("no \"node\" for addAutoConvex"));
				result = false;
			}
		}
		if (node->get_name() == TXT("excludeAutoConvex"))
		{
			Name id = node->get_name_attribute(TXT("node"));
			if (id.is_valid())
			{
				excludeAutoConvexNodes.push_back(id);
			}
			else
			{
				error_loading_xml(node, TXT("no \"node\" for excludeAutoConvex"));
				result = false;
			}
		}
		if (node->get_name() == TXT("excludeAutoConvexSet"))
		{
			excludeAutoConvexNodeSets.push_back();
			for_every(snode, node->children_named(TXT("exclude")))
			{
				Name id = snode->get_name_attribute(TXT("node"));
				if (id.is_valid())
				{
					excludeAutoConvexNodeSets.get_last().push_back(id);
				}
				else
				{
					error_loading_xml(snode, TXT("no \"node\" for exclude of exludeAutoConvexSet"));
					result = false;
				}
			}
		}
	}

	if (!autoConvexNodes.is_empty() || !excludeAutoConvexNodes.is_empty())
	{
		if (autoConvex != AutoConvex::Chosen &&
			autoConvex != AutoConvex::No &&
			(autoConvexAllExplicit || autoConvex != AutoConvex::All))
		{
			warn_loading_xml(_node, TXT("forcing auto convex to \"chosen\""));
		}
		autoConvex = AutoConvex::Chosen;
	}
	else if (autoConvexGenerateSurfaces ||
			 autoConvexNoMirrorX)
	{
		if (autoConvex != AutoConvex::No &&
			(autoConvexAllExplicit || autoConvex != AutoConvex::All))
		{
			warn_loading_xml(_node, TXT("forcing auto convex to \"all\""));
		}
		autoConvex = AutoConvex::All;
	}

	return result;
}

bool AutoConvex::prepare_for_generation(EdgesAndSurfacesContext& _context, PrepareForGenerationLevel::Type _level) const
{
	bool result = true;

	if (_level == PrepareForGenerationLevel::AutoConvexPolygons)
	{
		triangles.clear();
		if (autoConvex != AutoConvex::Type::No)
		{
			ConvexHull cv;
			if (autoConvex == AutoConvex::Type::Chosen)
			{
				for_every_ptr(node, _context.get_edges_and_surfaces_data()->nodes)
				{
					if (((autoConvexNodes.is_empty() && ! excludeAutoConvexNodes.is_empty()) || autoConvexNodes.does_contain(node->id)) &&
						!excludeAutoConvexNodes.does_contain(node->id))
					{
						cv.add(node->location);
					}
				}
			}
			else
			{
				for_every_ptr(node, _context.get_edges_and_surfaces_data()->nodes)
				{
					cv.add(node->location);
				}
			}
			cv.build();
			for_count(int, i, cv.get_triangle_count())
			{
				Vector3 a, b, c;
				if (cv.get_triangle(i, OUT_ a, OUT_ b, OUT_ c))
				{
					bool ok = true;
					if (autoConvexNoMirrorX)
					{
						if (a.x == 0.0f && b.x == 0.0f && c.x == 0.0f)
						{
							ok = false;
						}
					}
					if (ok)
					{
						Node const* nodeA = _context.get_edges_and_surfaces_data()->find_node(a);
						Node const* nodeB = _context.get_edges_and_surfaces_data()->find_node(b);
						Node const* nodeC = _context.get_edges_and_surfaces_data()->find_node(c);
						ok = false;
						if (nodeA && nodeB && nodeC)
						{
							ok = true;
							for_every(set, excludeAutoConvexNodeSets)
							{
								if (set->does_contain(nodeA->id) &&
									set->does_contain(nodeB->id) &&
									set->does_contain(nodeC->id))
								{
									ok = false;
								}
							}
						}
						else
						{
							if (!nodeA)
							{
								error_generating_mesh(_context.access_element_instance(), TXT("no node at %S"), a.to_string().to_char());
							}
							if (!nodeB)
							{
								error_generating_mesh(_context.access_element_instance(), TXT("no node at %S"), b.to_string().to_char());
							}
							if (!nodeC)
							{
								error_generating_mesh(_context.access_element_instance(), TXT("no node at %S"), c.to_string().to_char());
							}
							result = false;
						}
						if (ok)
						{
							triangles.push_back(Triangle(nodeA->id, nodeB->id, nodeC->id));
						}
					}
				}
			}
			if (autoConvexGenerateSurfaces)
			{
				SurfaceMesh* surfaceMesh = nullptr;

				for (int tIdx = 0; tIdx < triangles.get_size(); ++tIdx)
				{
					Triangle const triangle = triangles[tIdx];
					Node const* nodeA = _context.get_edges_and_surfaces_data()->find_node(triangle.nodeA);
					Node const* nodeB = _context.get_edges_and_surfaces_data()->find_node(triangle.nodeB);
					Node const* nodeC = _context.get_edges_and_surfaces_data()->find_node(triangle.nodeC);
					an_assert(nodeA && nodeB && nodeC);

					Edge const* firstEdge = nullptr;
					for (int tryIdx = 0; tryIdx < 3; ++tryIdx)
					{
						firstEdge = _context.get_edges_and_surfaces_data()->find_edge_for_nodes(nodeA->id, nodeB->id, nullptr, true);
						if (firstEdge)
						{
							break;
						}
						{
							// cycle to get at least one
							Node const* nodeTemp = nodeA;
							nodeA = nodeB;
							nodeB = nodeC;
							nodeC = nodeTemp;
						}
					}
					if (!firstEdge)
					{
						// skip this triangle, no chance to build a surface as we don't have any edge
						continue;
					}
					// try to find a triangle build with these edges
					bool reversed = false;
					Edge const* edgeA = _context.get_edges_and_surfaces_data()->find_edge_for_nodes(nodeA->id, nodeB->id, &reversed, true);
					Edge const* edgeB = _context.get_edges_and_surfaces_data()->find_edge_for_nodes(nodeB->id, nodeC->id, nullptr, true);
					Edge const* edgeC = _context.get_edges_and_surfaces_data()->find_edge_for_nodes(nodeC->id, nodeA->id, nullptr, true);
					Edge const* edgeD = nullptr;

					//if (reversed)
					//{
					//	swap(edgeB, edgeC);
					//	swap(nodeB, nodeC);
					//}

					if (edgeA && ((edgeB && ! edgeC) || (!edgeB && edgeC)))
					{
						// we are missing one edge, maybe there is a triangle that shares missing one
						Name missingA, missingB;
						Edge const** pEdgeMA;
						Edge const** pEdgeMB;
						if (!edgeB)
						{
							missingA = nodeB->id;
							missingB = nodeC->id;
							edgeD = edgeC;
							edgeC = nullptr;
							pEdgeMA = &edgeB;
							pEdgeMB = &edgeC;
						}
						else
						{
							an_assert(!edgeC);
							missingA = nodeC->id;
							missingB = nodeA->id;
							pEdgeMA = &edgeC;
							pEdgeMB = &edgeD;
						}
						for (int ntIdx = tIdx + 1; ntIdx < triangles.get_size(); ++ntIdx)
						{
							Triangle const nTriangle = triangles[ntIdx];
							if (nTriangle.has(missingA) && nTriangle.has(missingB))
							{
								bool nReversed = false;
								Edge const* nEdgeA = _context.get_edges_and_surfaces_data()->find_edge_for_nodes(nTriangle.nodeA, nTriangle.nodeB, &nReversed, true);
								Edge const* nEdgeB = _context.get_edges_and_surfaces_data()->find_edge_for_nodes(nTriangle.nodeB, nTriangle.nodeC, nullptr, true);
								Edge const* nEdgeC = _context.get_edges_and_surfaces_data()->find_edge_for_nodes(nTriangle.nodeC, nTriangle.nodeA, nullptr, true);
								int edgesAvailable = (nEdgeA ? 1 : 0) + (nEdgeB ? 1 : 0) + (nEdgeC ? 1 : 0);
								if (edgesAvailable == 2)
								{
									bool ok = false;
									Edge const* nEdge[2] = { nullptr, nullptr };
									// we need exactly two, the missing one has to be shared
									if (!nEdgeA && ((nTriangle.nodeA == missingA && nTriangle.nodeB == missingB) ||
													(nTriangle.nodeA == missingB && nTriangle.nodeB == missingA)))
									{
										ok = true;
										nEdge[0] = nEdgeB;
										nEdge[1] = nEdgeC;
									}
									if (!nEdgeB && ((nTriangle.nodeB == missingA && nTriangle.nodeC == missingB) ||
													(nTriangle.nodeB == missingB && nTriangle.nodeC == missingA)))
									{
										ok = true;
										nEdge[0] = nEdgeC;
										nEdge[1] = nEdgeA;
									}
									if (!nEdgeC && ((nTriangle.nodeC == missingA && nTriangle.nodeA == missingB) ||
													(nTriangle.nodeC == missingB && nTriangle.nodeA == missingA)))
									{
										ok = true;
										nEdge[0] = nEdgeA;
										nEdge[1] = nEdgeB;
									}
									if (ok)
									{
										ok = false;
										if (nEdge[0]->has_from_to(missingA) &&
											nEdge[1]->has_from_to(missingB))
										{
											ok = true;
											*pEdgeMA = nEdge[0];
											*pEdgeMB = nEdge[1];
										}
										else if (nEdge[0]->has_from_to(missingB) &&
												 nEdge[1]->has_from_to(missingA))
										{
											ok = true;
											*pEdgeMA = nEdge[1];
											*pEdgeMB = nEdge[0];
										}
										if (ok)
										{
											an_assert(edgeA&& edgeB&& edgeC&& edgeD);
											if (edgeA && edgeB && edgeC && edgeD)
											{
												// consume triangle
												triangles.remove_at(ntIdx);
												break;
											}
										}
									}
								}
							}
						}
					}

					if (edgeA && edgeB && edgeC)
					{
						{	// consume triangle
							triangles.remove_at(tIdx);
							tIdx--;
						}

						// build a triangle/quad surface!

						// surface first
						Surface* surface = new Surface();
						surface->isAutoGenerated = true;
						surface->id = Name(String::printf(TXT("autoConvex_%i"), _context.get_edges_and_surfaces_data()->autoConvexId++));
						surface->edgesLoaded.set_size(edgeD? 4 : 3);
						surface->edgesLoaded[0].id = edgeA->id;
						surface->edgesLoaded[1].id = edgeB->id;
						surface->edgesLoaded[2].id = edgeC->id;
						if (edgeD)
						{
							surface->edgesLoaded[3].id = edgeD->id;
						}
						surface->edgesLoaded[0].reversed = reversed;
						surface->surfaceCreationMethod = surfaceCreationMethod;
						_context.get_edges_and_surfaces_data()->surfaces.push_back(surface);

						// add to surface mesh now
						if (!surfaceMesh)
						{
							surfaceMesh = new SurfaceMesh();
							surfaceMesh->isAutoGenerated = true;
							surfaceMesh->id = Name(String::printf(TXT("autoConvex_%i"), _context.get_edges_and_surfaces_data()->autoConvexId++));
							surfaceMesh->uvDef = surfacesUVDef;
							_context.get_edges_and_surfaces_data()->surfaceMeshes.push_back(surfaceMesh);
						}
						surfaceMesh->surfaceIds.push_back(surface->id);
					}
				}
				if (autoConvexGenerateSurfacesOnly)
				{
					// remove rest of the triangles
					triangles.clear();
				}
			}
		}
	}

	return result;
}

//

bool Polygons::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;

	id = _node->get_name_attribute(TXT("id"), id);

	createMovementCollisionMesh.load_from_xml(_node, TXT("createMovementCollisionMesh"), _lc);
	createMovementCollisionBox.load_from_xml(_node, TXT("createMovementCollisionBox"), _lc);
	createPreciseCollisionMesh.load_from_xml(_node, TXT("createPreciseCollisionMesh"), _lc);
	createPreciseCollisionBox.load_from_xml(_node, TXT("createPreciseCollisionBox"), _lc);
	createSpaceBlocker.load_from_xml(_node, TXT("createSpaceBlocker"));

	result &= generationCondition.load_from_xml(_node);

	forceMaterialIdx.load_from_xml(_node, TXT("forceMaterialIdx"));
	uvDef.load_from_xml_with_clearing(_node);

	result &= autoConvex.load_from_xml(_node);

	for_every(node, _node->all_children())
	{
		if (node->get_name() == TXT("polygon") ||
			node->get_name() == TXT("tri") ||
			node->get_name() == TXT("triangle") ||
			node->get_name() == TXT("quad"))
		{
			Poly polygon;
			if (polygon.load_from_xml(node))
			{
				polygons.push_back(polygon);
			}
			else
			{
				error_loading_xml(node, TXT("couldn't load polygon"));
				result = false;
			}
		}
	}

	return result;
}

bool Polygons::prepare_for_generation(EdgesAndSurfacesContext & _context, PrepareForGenerationLevel::Type _level) const
{
	bool result = true;

	autoConvex.prepare_for_generation(_context, _level);

	for_every_const(polygon, polygons)
	{
		result &= polygon->prepare_for_generation(_context, _level);
	}

	return result;
}

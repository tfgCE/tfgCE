#include "fbxMesh3dExtractor.h"

#include "fbxScene.h"
#include "fbxUtils.h"

#include "fbxImporter.h"

#include "..\..\core\mainSettings.h"
#include "..\..\core\debug\extendedDebug.h"
#include "..\..\core\memory\memory.h"
#include "..\..\core\mesh\mesh3d.h"
#include "..\..\core\mesh\skeleton.h"
#include "..\..\core\mesh\mesh3dPart.h"
#include "..\..\core\system\video\vertexFormatUtils.h"
#include "..\..\core\types\vectorPacked.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

#ifdef USE_FBX
using namespace FBX;

#define SKINNING_ELEMENT_COUNT 4

#define SKINNING_32
//#define SKINNING_16
//#define SKINNING_8

#ifdef SKINNING_32
#define SKINNING_VERTEX_FORMAT_VALUE_TYPE System::DataFormatValueType::Uint32
#define SKINNING_INDEX_VARIABLE_TYPE Uint32
#endif
#ifdef SKINNING_16
#define SKINNING_VERTEX_FORMAT_VALUE_TYPE System::DataFormatValueType::Uint16
#define SKINNING_INDEX_VARIABLE_TYPE Uint16
#endif
#ifdef SKINNING_8
#define SKINNING_VERTEX_FORMAT_VALUE_TYPE System::DataFormatValueType::Uint8
#define SKINNING_INDEX_VARIABLE_TYPE Uint8
#endif

struct FbxExtractedVertexInfo
{
	int outVertexIndex; // output vertex index - here lies cached vertex

	// all indices in input "space"
	int controlPointIndex;
	int normalIndex;
	int colourIndex;
	int textureUVIndex;

	FbxExtractedVertexInfo()
	: outVertexIndex(NONE)
	, controlPointIndex(NONE)
	, normalIndex(NONE)
	, colourIndex(NONE)
	, textureUVIndex(NONE)
	{
	}

	bool operator==(FbxExtractedVertexInfo const & _other) const
	{
		return controlPointIndex == _other.controlPointIndex &&
			   normalIndex == _other.normalIndex &&
			   colourIndex == _other.colourIndex &&
			   textureUVIndex == _other.textureUVIndex;
	}
};

struct FbxCachedControlPoint
{
	Array<FbxExtractedVertexInfo> vertices;
};

struct FbxExtractorCache
{
	int vertexCount;
	Array<int8> verticesData;
	Array<int> elementsData;

	Array<int8> tempVertex;

	Array<FbxCachedControlPoint> cachedVertices; // first index is control point index, secone

	FbxExtractorCache(int _controlPointsCount)
	: vertexCount(0)
	{
		cachedVertices.set_size(_controlPointsCount);
	}

	int find_vertex_index(FbxExtractedVertexInfo const & _vertexInfo) const
	{
		FbxCachedControlPoint const & cachedVertex = cachedVertices[_vertexInfo.controlPointIndex];
		for_every(vertex, cachedVertex.vertices)
		{
			if (*vertex == _vertexInfo)
			{
				return vertex->outVertexIndex;
			}
		}
		return NONE;
	}

	int add_vertex_info(FbxExtractedVertexInfo const & _vertexInfo, Array<int8> const & _vertexData)
	{
		an_assert(find_vertex_index(_vertexInfo) == NONE);
		FbxExtractedVertexInfo vertexInfo = _vertexInfo;
		vertexInfo.outVertexIndex = vertexCount;
		cachedVertices[vertexInfo.controlPointIndex].vertices.push_back(vertexInfo);
		verticesData.add_from(_vertexData);
		++vertexCount;
		return vertexCount - 1;
	}

	void add_element(int _element)
	{
		elementsData.push_back(_element);
	}
};

//

void Mesh3DExtractor::initialise_static()
{
	Meshes::Mesh3D::importer().register_file_type_with_options(String(TXT("fbx")),
		[](String const & _fileName, Meshes::Mesh3DImportOptions const & _options)
		{
			Concurrency::ScopedSpinLock lock(FBX::Importer::importerLock);
			Meshes::Mesh3D* mesh = nullptr;
			if (FBX::Scene* scene = FBX::Importer::import(_fileName))
			{
				//scene->remove_all_animations();
				mesh = Mesh3DExtractor::extract_from(scene, _options);
			}
			else
			{
				error(TXT("problem importing fbx from file \"%S\""), _fileName.to_char());
			}
			return mesh;
		}
	);
}

void Mesh3DExtractor::close_static()
{
	Meshes::Mesh3D::importer().unregister(String(TXT("fbx")));
}

Meshes::Mesh3D* Mesh3DExtractor::extract_from(Scene* _scene, Meshes::Mesh3DImportOptions const & _options)
{
	Mesh3DExtractor extractor(_scene, _options);
	extractor.extract_mesh(_options.importFromNode);
	return extractor.mesh;
}

Mesh3DExtractor::Mesh3DExtractor(Scene* _scene, Meshes::Mesh3DImportOptions const & _options)
: options(_options)
, scene(_scene)
, mesh(nullptr)
, skeleton(_options.skeleton)
{
	an_assert(scene);
}

Mesh3DExtractor::~Mesh3DExtractor()
{
}

void Mesh3DExtractor::extract_mesh(String const & _rootNode)
{
	an_assert(scene);
	an_assert(!mesh);

	find_nodes_containing_mesh(scene->get_node(_rootNode));

	mesh = Meshes::Mesh3D::create_new();

#ifdef AN_SYSTEM_INFO_DRAW_DETAILS
	mesh->set_debug_mesh_name(String(TXT("Mesh3DExtractor")));
#endif

	// store digested source for skeleton
	if (skeleton)
	{
		mesh->set_digested_skeleton(skeleton->get_digested_source());
	}

	// extract nodes
	Array<Meshes::Mesh3DPart*> parts;
	MeshNode* rootMeshNode = !meshNodes.is_empty() ? &meshNodes.get_first() : nullptr;
	for_every(meshNode, meshNodes)
	{
		if (Meshes::Mesh3DPart* part = extract_node_to_mesh(meshNode, rootMeshNode))
		{
			parts.push_back(part);
		}
	}
	
	if (!options.combineParts.is_empty() || options.combineAllParts)
	{
		// build combine lists
		Array<Array<Meshes::Mesh3D::FuseMesh3DPart> > combineLists;

		combineLists.set_size(options.combineParts.get_size());
		int combineListIndex = 0;
		for_every(combineIntoPartList, options.combineParts)
		{
			auto & combineList = combineLists[combineListIndex];
			for_every(combineIntoPart, *combineIntoPartList)
			{
				if (parts.is_index_valid(combineIntoPart->byIndex))
				{
					// simplest case, add part by index ("byIndex")
					combineList.push_back(Meshes::Mesh3D::FuseMesh3DPart(parts[combineIntoPart->byIndex]));
				}
				else if (! combineIntoPart->byName.is_empty())
				{
					// add part of first found source mesh node if it's name is equal to "byName"
					int partIndex = NONE;
					for_every(meshNode, meshNodes)
					{
						if (combineIntoPart->byName == String::from_char8(meshNode->meshNode->GetName()))
						{
							break;
						}
						++ partIndex;
					}
					if (parts.is_index_valid(partIndex))
					{
combineList.push_back(Meshes::Mesh3D::FuseMesh3DPart(parts[partIndex]));
					}
				}
				else if (!combineIntoPart->ifNameContains.is_empty())
				{
				// add all parts that source mesh node's name contain "ifNameContains"
				int partIndex = NONE;
				for_every(meshNode, meshNodes)
				{
					if (String::from_char8(meshNode->meshNode->GetName()).find_first_of(combineIntoPart->ifNameContains) != NONE)
					{
						if (parts.is_index_valid(partIndex))
						{
							combineList.push_back(Meshes::Mesh3D::FuseMesh3DPart(parts[partIndex]));
						}
					}
					++partIndex;
				}
				}
			}
			++combineListIndex;
		}

		// combine parts basing on lists that we've built
		mesh->fuse_parts(combineLists, nullptr);// options.combineAllParts);
	}
}

void Mesh3DExtractor::find_nodes_containing_mesh(FbxNode* _inNode)
{
	if (!_inNode)
	{
		return;
	}

	// check if this is mesh
	MeshNode meshNode;
	meshNode.meshNode = _inNode;
	meshNode.globalMatrix = fbx_matrix_to_matrix_44(meshNode.meshNode->EvaluateGlobalTransform(FBXSDK_TIME_INFINITE));
	meshNode.localMatrix = fbx_matrix_to_matrix_44(meshNode.meshNode->EvaluateLocalTransform(FBXSDK_TIME_INFINITE));
	if (FbxNodeAttribute* attr = meshNode.meshNode->GetNodeAttribute())
	{
		if (attr->GetAttributeType() == FbxNodeAttribute::eMesh)
		{
			meshNodes.push_back(meshNode);
		}
	}

	for (int i = 0; i < _inNode->GetChildCount(); ++i)
	{
		find_nodes_containing_mesh(_inNode->GetChild(i));
	}
}

template <typename InfoClass>
int get_index_from(InfoClass* _source, int _controlPointId, int _polygonVertexId)
{
	if (_source->GetMappingMode() == FbxGeometryElement::eByControlPoint)
	{
		if (_source->GetReferenceMode() == FbxGeometryElement::eDirect)
		{
			return _controlPointId;
		}
		else if (_source->GetReferenceMode() == FbxGeometryElement::eIndexToDirect)
		{
			return _source->GetIndexArray().GetAt(_controlPointId);
		}
		else
		{
			todo_important(TXT("handle me!"));
		}
	}
	else if (_source->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
	{
		if (_source->GetReferenceMode() == FbxGeometryElement::eDirect)
		{
			return _polygonVertexId;
		}
		else if (_source->GetReferenceMode() == FbxGeometryElement::eIndexToDirect)
		{
			return _source->GetIndexArray().GetAt(_polygonVertexId);
		}
		else
		{
			todo_important(TXT("handle me!"));
		}
	}
	else
	{
		todo_important(TXT("handle me!"));
	}
	return NONE;
}

Meshes::Mesh3DPart* Mesh3DExtractor::extract_node_to_mesh(MeshNode* _meshNode, MeshNode* _rootNode)
{
	an_assert(mesh);
	an_assert(_meshNode->meshNode->GetNodeAttribute() && _meshNode->meshNode->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eMesh);

	if (!options.ignoreImportingFromNodesWithPrefix.is_empty())
	{
		String meshNodeName = String::from_char8(_meshNode->meshNode->GetName());
		for_every(prefix, options.ignoreImportingFromNodesWithPrefix)
		{
			if (meshNodeName.has_prefix(*prefix))
			{
#ifdef AN_ALLOW_EXTENDED_DEBUG
				IF_EXTENDED_DEBUG(FbxImport)
				{
					output(TXT("ignore importing node node \"%S\""), meshNodeName.to_char());
				}
#endif
				return nullptr;
			}
		}
	}

#ifdef AN_ALLOW_EXTENDED_DEBUG
	IF_EXTENDED_DEBUG(FbxImport)
	{
		String meshNodeName = String::from_char8(_meshNode->meshNode->GetName());
		output(TXT("importing from node \"%S\""), meshNodeName.to_char());
	}
#endif

	FbxMesh* fbxMesh = (FbxMesh*)_meshNode->meshNode->GetNodeAttribute();

	// access number of various data sources
	int polygonCount = fbxMesh->GetPolygonCount();
	Vector3 importOffset = Vector3::zero;
	Vector3 importScaleFit = Vector3::one;
	Transform importFromPlacement = Transform::identity;
	if (options.importFromPlacement.is_set())
	{
		importFromPlacement = options.importFromPlacement.get();
	}
	else if (! options.importFromNode.is_empty())
	{
		// by default import from same location as root node
		importFromPlacement.set_translation(_rootNode->globalMatrix.get_translation());
	}
	Transform placeAfterImport = Transform::identity;
	if (options.placeAfterImport.is_set())
	{
		placeAfterImport = options.placeAfterImport.get();
	}
	Vector3 const importScale = options.importScale;

	bool normalsPresent = fbxMesh->GetElementNormalCount() != 0;
	bool coloursPresent = fbxMesh->GetElementVertexColorCount() != 0 && ! options.skipColour;
	bool textureUVsPresent = fbxMesh->GetElementUVCount() != 0 && ! options.skipUV;
	bool skinningPresent = false;

	// setup formats and get pointers to data
	System::IndexFormat indexFormat;
	indexFormat.with_element_size(System::IndexElementSize::FourBytes);
	indexFormat.calculate_stride();

	System::VertexFormat vertexFormat;
	vertexFormat.with_padding();

	Array<FbxGeometryElementNormal*> normalInfos;
	if (normalsPresent)
	{
		normalInfos.push_back(fbxMesh->GetElementNormal(0)); // just first set of normals
		vertexFormat.with_normal();
	}

	Array<FbxGeometryElementVertexColor*> colourInfos;
	if (coloursPresent)
	{
		colourInfos.push_back(fbxMesh->GetElementVertexColor(0)); // just first set of colours
		vertexFormat.with_colour_rgb();
	}

	Array<FbxGeometryElementUV*> textureUVInfos;
	if (textureUVsPresent)
	{
		textureUVInfos.push_back(fbxMesh->GetElementUV(0)); // just first set of uvs
		vertexFormat.with_texture_uv();
	}

	Array<FbxSkin*> skinningInfos;
	if (skeleton && !options.skipSkinning)
	{
		// collect skinning only if skeleton present
		for (int deformerIdx = 0; deformerIdx < fbxMesh->GetDeformerCount(); ++deformerIdx)
		{
			FbxStatus status;
			if (FbxDeformer* deformer = fbxMesh->GetDeformer(deformerIdx, &status))
			{
				if (deformer->GetDeformerType() == FbxDeformer::eSkin)
				{
					FbxSkin* skinning = plain_cast<FbxSkin>(deformer);
					if (skinning->GetGeometry() == fbxMesh)
					{
						skinningPresent = true;
						skinningInfos.push_back(plain_cast<FbxSkin>(deformer)); // just first set/deformer of skinning
					}
				}
			}
		}
		if (skinningInfos.is_empty())
		{
			String meshNodeName = String::from_char8(_meshNode->meshNode->GetName());
			error(TXT("skeleton provided assuming that we want to collect skinning but no skinning info provided, will skin to root bone! node \"%S\""), meshNodeName.to_char());
			skinningPresent = true;
		}
		vertexFormat.with_skinning_data(SKINNING_VERTEX_FORMAT_VALUE_TYPE, SKINNING_ELEMENT_COUNT);
	}

	vertexFormat.calculate_stride_and_offsets();

	// load vertex data from mesh into pointsData
	FbxExtractorCache cache(fbxMesh->GetControlPointsCount());
	Array<int8> vertexData;
	vertexData.set_size(vertexFormat.get_stride());

	// cache control points data (location + optional skinning)
	struct ControlPoint
	{
		Vector3 location;
		float skinningWeights[SKINNING_ELEMENT_COUNT];
		SKINNING_INDEX_VARIABLE_TYPE skinningIndices[SKINNING_ELEMENT_COUNT];
		ControlPoint()
		: location(Vector3::zero)
		{
			memory_zero(skinningWeights, sizeof(float) * SKINNING_ELEMENT_COUNT);
			memory_zero(skinningIndices, sizeof(SKINNING_INDEX_VARIABLE_TYPE) * SKINNING_ELEMENT_COUNT);
		}
		void normalise_skinning()
		{
			float weightSum = 0.0f;
			for (int idx = 0; idx < SKINNING_ELEMENT_COUNT; ++idx)
			{
				weightSum += skinningWeights[idx];
			}
			an_assert(weightSum != 0.0f, TXT("point without any weights?"));
			float invWeightSum = weightSum != 0.0f ? 1.0f / weightSum : 0.0f;
			for (int idx = 0; idx < SKINNING_ELEMENT_COUNT; ++idx)
			{
				skinningWeights[idx] *= invWeightSum;
			}
		}
		void add_skinning(int _index, float _weight)
		{
			if ((uint)_index != (SKINNING_INDEX_VARIABLE_TYPE)_index)
			{
				error(TXT("consider index variable type with greater capacity"));
			}
			int lowestIndex = NONE;
			float lowestWeight = 2.0f;
			for (int idx = 0; idx < SKINNING_ELEMENT_COUNT; ++idx)
			{
				if (_weight > skinningWeights[idx] &&
					lowestWeight > skinningWeights[idx])
				{
					lowestWeight = skinningWeights[idx];
					lowestIndex = idx;
				}
			}
			if (lowestIndex != NONE)
			{
				skinningIndices[lowestIndex] = (SKINNING_INDEX_VARIABLE_TYPE)_index;
				skinningWeights[lowestIndex] = _weight;
			}
		}
	};
	Array<ControlPoint> controlPoints;
	{
		int controlPointsCount = fbxMesh->GetControlPointsCount();
		controlPoints.set_size(controlPointsCount);
		int controlPointIdx = 0;
		for_every(controlPoint, controlPoints)
		{
			FbxVector4 sourceLocation = fbxMesh->GetControlPoints()[controlPointIdx];
			controlPoint->location.x = (float)(sourceLocation.mData[0]);
			controlPoint->location.y = (float)(sourceLocation.mData[1]);
			controlPoint->location.z = (float)(sourceLocation.mData[2]);
			++controlPointIdx;
		}
	}

	// collect information about skinning
	if (skinningPresent)
	{
		if (! skinningInfos.is_empty())
		{
			int skinningInfoIdx = 0; // just first set of skinning
			FbxSkin* skinning = skinningInfos[skinningInfoIdx];
			for_count(int, clusterIdx, skinning->GetClusterCount())
			{
				if (FbxCluster const * cluster = skinning->GetCluster(clusterIdx))
				{
					if (cluster->GetLink())
					{
						an_assert(cluster->GetLinkMode() != FbxCluster::eAdditive, TXT("I will handle \"normalize\" and \"total one\" as I am going to normalise anyway. but what to do with additive?"));
						Name boneName = Name(String::from_char8(cluster->GetLink()->GetName()));
						int boneIndex = skeleton->find_bone_index(boneName);
#ifdef AN_ALLOW_EXTENDED_DEBUG
						IF_EXTENDED_DEBUG(FbxImport)
						{
							output(TXT("    bone %S -> %i"), boneName.to_char(), boneIndex);
						}
#endif
						if (boneIndex != NONE)
						{
							for_count(int, controlPointIndicesIndex, cluster->GetControlPointIndicesCount())
							{
								int controlPointIndex = cluster->GetControlPointIndices()[controlPointIndicesIndex];
								if (controlPoints.is_index_valid(controlPointIndex))
								{
#ifdef AN_ALLOW_EXTENDED_DEBUG
									IF_EXTENDED_DEBUG(Mesh3DExtractor)
									{
										output(TXT("        add control point %i with weight %.3f"), controlPointIndex, (float)(cluster->GetControlPointWeights()[controlPointIndicesIndex]));
									}
#endif
									controlPoints[controlPointIndex].add_skinning(boneIndex, (float)(cluster->GetControlPointWeights()[controlPointIndicesIndex]));
								}
								else
								{
									error(TXT("control point index invalid"));
								}
							}
						}
					}
				}
			}
		}
		else
		{
			// fake skinning - to root
			for_every(controlPoint, controlPoints)
			{
				controlPoint->skinningWeights[0] = 1.0f;
				controlPoint->skinningIndices[0] = 0;
			}
		}
		// normalise skinning data
		for_every(controlPoint, controlPoints)
		{
			controlPoint->normalise_skinning();
		}
	}

	{
		if (options.importFromAutoCentre.is_set() ||
			options.importFromAutoPt.is_set() ||
			options.importScaleToFit.is_set())
		{
			Range3 range = Range3::empty;
			for_every(controlPoint, controlPoints)
			{
				range.include(_meshNode->globalMatrix.location_to_world(controlPoint->location));
			}
			if (!range.is_empty())
			{
				if (options.importFromAutoCentre.is_set())
				{
					importOffset = -range.centre() * options.importFromAutoCentre.get();
				}
				if (options.importFromAutoPt.is_set())
				{
					importOffset = -range.get_at(options.importFromAutoPt.get());
				}
				if (options.importScaleToFit.is_set())
				{
					if (options.importScaleToFit.get().x != 0.0f) importScaleFit.x = options.importScaleToFit.get().x / max(0.001f, range.x.length());
					if (options.importScaleToFit.get().y != 0.0f) importScaleFit.y = options.importScaleToFit.get().y / max(0.001f, range.y.length());
					if (options.importScaleToFit.get().z != 0.0f) importScaleFit.z = options.importScaleToFit.get().z / max(0.001f, range.z.length());
				}
			}
		}
	}

	// go through each polygon
	int polygonVertexIdAnchor = 0; // this is base vertex id per polygon
	for (int polygonIdx = 0; polygonIdx < polygonCount; ++polygonIdx)
	{
		int polygonSize = fbxMesh->GetPolygonSize(polygonIdx);
		// turn each polygon into triangle
		for (int subPolygon = 3; subPolygon <= polygonSize; ++subPolygon)
		{
			int outTriangleVertex[] = { subPolygon - 1, subPolygon - 2, 0 };
			// for all three vertices...
			for (int inTriangleIdx = 0; inTriangleIdx < 3; ++inTriangleIdx)
			{
#ifdef AN_ALLOW_EXTENDED_DEBUG
				IF_EXTENDED_DEBUG(Mesh3DExtractor)
				{
					output(TXT("    polygon %i subpolygon %i vertex %i"), polygonIdx, subPolygon, inTriangleIdx);
				}
#endif
				int positionInPolygon = outTriangleVertex[inTriangleIdx];
				// calculate ids
				int controlPointId = fbxMesh->GetPolygonVertex(polygonIdx, positionInPolygon);
				int polygonVertexId = polygonVertexIdAnchor + positionInPolygon; // polygon vertex id 

				// store info about vertex (indices) to look it up in cache and to know which one to access when building data
				FbxExtractedVertexInfo vertexInfo;
				vertexInfo.controlPointIndex = controlPointId;
				if (normalsPresent)
				{
					int normalInfoIdx = 0; // just first set of normals
					vertexInfo.normalIndex = get_index_from(normalInfos[normalInfoIdx], controlPointId, polygonVertexId);
				}
				if (coloursPresent)
				{
					int colourInfoIdx = 0; // just first set of colours
					vertexInfo.colourIndex = get_index_from(colourInfos[colourInfoIdx], controlPointId, polygonVertexId);
				}
				if (textureUVsPresent)
				{
					int textureUVInfoIdx = 0; // just first set of uvs
					vertexInfo.textureUVIndex = get_index_from(textureUVInfos[textureUVInfoIdx], controlPointId, polygonVertexId);
				}
				todo_future(TXT("tangents"));
				todo_future(TXT("binormals"));

				// try finding cached vertex
				int existingVertexInfo = cache.find_vertex_index(vertexInfo);
				if (existingVertexInfo != NONE)
				{
					cache.add_element(existingVertexInfo);
				}
				else
				{
					// fill vertexData
					{	// load location
						Vector3* location = (Vector3*)(vertexData.get_data() + vertexFormat.get_location_offset());
						*location = controlPoints[vertexInfo.controlPointIndex].location;
						*location = _meshNode->globalMatrix.location_to_world(*location);
						*location += importOffset;
						*location *= importScaleFit;
						*location = importFromPlacement.location_to_local(*location);
						*location *= importScale;
						*location = placeAfterImport.location_to_world(*location);
					}

					if (normalsPresent)
					{
						int normalInfoIdx = 0; // just first normal
						FbxVector4 sourceNormal = normalInfos[normalInfoIdx]->GetDirectArray().GetAt(vertexInfo.normalIndex);
						if (MainSettings::global().is_vertex_data_normal_packed())
						{
							Vector3 normal;
							normal.x = (float)(sourceNormal.mData[0]);
							normal.y = (float)(sourceNormal.mData[1]);
							normal.z = (float)(sourceNormal.mData[2]);
							normal = _meshNode->globalMatrix.vector_to_world(normal).normal();
							normal = importFromPlacement.vector_to_local(normal);
							normal = placeAfterImport.vector_to_world(normal);
							normal.normalise();
							((VectorPacked*)(vertexData.get_data() + vertexFormat.get_normal_offset()))->set_as_vertex_normal(normal);
						}
						else
						{
							Vector3* normal = (Vector3*)(vertexData.get_data() + vertexFormat.get_normal_offset());
							normal->x = (float)(sourceNormal.mData[0]);
							normal->y = (float)(sourceNormal.mData[1]);
							normal->z = (float)(sourceNormal.mData[2]);
							*normal = _meshNode->globalMatrix.vector_to_world(*normal).normal();
							*normal = importFromPlacement.vector_to_local(*normal);
							*normal = placeAfterImport.vector_to_world(*normal);
							normal->normalise();
						}
					}

					if (coloursPresent)
					{
						int colourInfoIdx = 0; // just first colour
						Colour* colour = (Colour*)(vertexData.get_data() + vertexFormat.get_colour_offset());
						FbxColor sourceColour = colourInfos[colourInfoIdx]->GetDirectArray().GetAt(vertexInfo.colourIndex);
						// rgb assumed (not rgba!)
						colour->r = (float)(sourceColour.mRed);
						colour->g = (float)(sourceColour.mGreen);
						colour->b = (float)(sourceColour.mBlue);
					}

					if (textureUVsPresent)
					{
						int textureUVInfoIdx = 0; // just first uv
						Vector2* textureUV = (Vector2*)(vertexData.get_data() + vertexFormat.get_texture_uv_offset());
						FbxVector2 sourceTextureUV = textureUVInfos[textureUVInfoIdx]->GetDirectArray().GetAt(vertexInfo.textureUVIndex);
						Vector2 uv = Vector2((float)(sourceTextureUV.mData[0]), (float)(sourceTextureUV.mData[1]));
						// uv assumed (not 1d or 3d)
						if (options.setU.is_set())
						{
							uv.x = options.setU.get();
						}
						::System::VertexFormatUtils::store(vertexData.get_data() + vertexFormat.get_texture_uv_offset(), vertexFormat.get_texture_uv_type(), uv);
					}

					todo_future(TXT("tangents"));
					todo_future(TXT("binormals"));
					// don't forget about importFromPlacement, placeAfterImport

					if (skinningPresent)
					{
						ControlPoint* controlPoint = &controlPoints[vertexInfo.controlPointIndex];
						float* skinningWeight = (float*)(vertexData.get_data() + vertexFormat.get_skinning_weights_offset());
						memory_copy(skinningWeight, controlPoint->skinningWeights, sizeof(float) * SKINNING_ELEMENT_COUNT);
						SKINNING_INDEX_VARIABLE_TYPE* skinningIndex = (SKINNING_INDEX_VARIABLE_TYPE*)(vertexData.get_data() + vertexFormat.get_skinning_indices_offset());
						memory_copy(skinningIndex, controlPoint->skinningIndices, sizeof(SKINNING_INDEX_VARIABLE_TYPE)* SKINNING_ELEMENT_COUNT);
#ifdef AN_ALLOW_EXTENDED_DEBUG
						IF_EXTENDED_DEBUG(Mesh3DExtractor)
						{
							for (int i = 0; i < SKINNING_ELEMENT_COUNT; ++i)
							{
								if (controlPoint->skinningWeights[i] != 0.0f)
								{
									output(TXT("        skinning to bone %i weight %.3f"), controlPoint->skinningIndices[i], controlPoint->skinningWeights[i]);
								}
							}
						}
#endif
					}

					// add to cache
					cache.add_element(cache.add_vertex_info(vertexInfo, vertexData));
				}
			}
		}
		polygonVertexIdAnchor += polygonSize;
	}

	// load data into mesh
	return mesh->load_part_data(cache.verticesData.get_data(), cache.elementsData.get_data(), Meshes::Primitive::Triangle, cache.vertexCount, cache.elementsData.get_size(), vertexFormat, indexFormat);
}

#endif

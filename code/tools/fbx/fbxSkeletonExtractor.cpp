#include "fbxSkeletonExtractor.h"

#include "fbxScene.h"
#include "fbxUtils.h"

#include "fbxImporter.h"

#include "..\..\core\debug\extendedDebug.h"
#include "..\..\core\memory\memory.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

#ifdef USE_FBX

//

using namespace FBX;

//

void SkeletonExtractor::initialise_static()
{
	Meshes::Skeleton::importer().register_file_type_with_options(String(TXT("fbx")),
		[](String const & _fileName, Meshes::SkeletonImportOptions const & _options)
		{
			Concurrency::ScopedSpinLock lock(FBX::Importer::importerLock);
			Meshes::Skeleton* skeleton = nullptr;
			if (FBX::Scene* scene = FBX::Importer::import(_fileName))
			{
				//scene->remove_all_animations();
				skeleton = SkeletonExtractor::extract_from(scene, _options);
			}
			else
			{
				error(TXT("problem importing fbx from file \"%S\""), _fileName.to_char());
			}
			return skeleton;
		}
	);
}

void SkeletonExtractor::close_static()
{
	Meshes::Skeleton::importer().unregister(String(TXT("fbx")));
}

Meshes::Skeleton* SkeletonExtractor::extract_from(Scene* _scene, Meshes::SkeletonImportOptions const & _options)
{
	SkeletonExtractor extractor(_scene);
	extractor.extract_skeleton(_options);
	return extractor.skeleton;
}

SkeletonExtractor::SkeletonExtractor(Scene* _scene)
: scene(_scene)
, skeleton(nullptr)
{
	an_assert(scene);
}

SkeletonExtractor::~SkeletonExtractor()
{
}

void SkeletonExtractor::extract_skeleton(Meshes::SkeletonImportOptions const & _options)
{
	an_assert(scene);
	an_assert(!skeleton);

	skeleton = Meshes::Skeleton::create_new();

	if (FbxNode* rootNode = scene->get_node(_options.importFromNode))
	{
		boneExtractor.extract_from_root_node(rootNode, _options);

		for_every(boneNodeInfo, boneExtractor.extracted)
		{
			process_bone(*boneNodeInfo, _options);
		}
	}

	skeleton->prepare_to_use();
}

void SkeletonExtractor::process_bone(BoneExtractor::BoneNodeInfo const & _boneNodeInfo, Meshes::SkeletonImportOptions const & _options)
{
	Meshes::BoneDefinition* bone = skeleton->find_or_create_bone(_boneNodeInfo.boneName);

	// add default placement that will be also used for static (root space becomes model space)
	bone->set_placement_MS(_boneNodeInfo.placementRS);
	bone->set_parent_name(_boneNodeInfo.parentName);
}

//--

BoneExtractor::BoneNodeInfoToProcess::BoneNodeInfoToProcess()
: rootNode(nullptr)
, parentNode(nullptr)
, node(nullptr)
{
}

BoneExtractor::BoneNodeInfoToProcess::BoneNodeInfoToProcess(FbxNode* _rootNode, FbxNode* _parentNode, FbxNode* _node)
: rootNode(_rootNode)
, parentNode(_parentNode)
, node(_node)
{
}

//--

void BoneExtractor::extract_from_root_node(FbxNode* rootNode, Meshes::SkeletonImportOptions const& _options)
{
	// you may ask: what in the world is happening here?
	// we add first bone to array "to process"
	// then for each bone in the array we add it to actual skeleton
	// and then we add its children to array "to process"
	// thanks to this approach we have bones in order from closest to the root to the furthest
	toProcess.push_back(BoneNodeInfoToProcess(rootNode, nullptr, rootNode));
	extract_bones_from_array(_options);

	// use pose as otherwise we will get bones from animation or whatever else and we want to get the...
	// I HAVE NO IDEA WHY
	// I just had to use pose otherwise the mesh is imported as mesh is but the nodes with evaluate get animation? You can't evaluate the default/t-pose
	{
		auto* scene = rootNode->GetScene();
		if (_options.importFromPoseIdx >= 0 &&
			_options.importFromPoseIdx < scene->GetPoseCount())
		{
			if (auto* pose = scene->GetPose(_options.importFromPoseIdx))
			{
				String name = String::from_char8(pose->GetName());
#ifdef AN_ALLOW_EXTENDED_DEBUG
				IF_EXTENDED_DEBUG(FbxImport)
				{
					output(TXT("pose \"%S\""), name.to_char());
					output(TXT("pose has %i item(s)"), pose->GetCount());
					output(TXT("pose is bind pose %i"), pose->IsBindPose());
				}
#endif
				Transform rootPlacementXS = Transform::identity;
				for_count(int, itemIdx, pose->GetCount())
				{
					String nodeName = String::from_char8(pose->GetNodeName(itemIdx).GetCurrentName());
#ifdef AN_ALLOW_EXTENDED_DEBUG
					IF_EXTENDED_DEBUG(FbxImport)
					{
						output(TXT("  %2i : %S"), itemIdx, nodeName.to_char());
					}
#endif
					if (nodeName == _options.importFromNode)
					{
						rootPlacementXS = fbx_matrix_to_matrix_44(pose->GetMatrix(itemIdx)).to_transform();
					}
				}
				for_count(int, itemIdx, pose->GetCount())
				{
					String nodeName = String::from_char8(pose->GetNodeName(itemIdx).GetCurrentName());
					for_every(boneNodeInfo, extracted)
					{
						if (boneNodeInfo->boneName.to_string() == nodeName)
						{
							Transform placementXS = fbx_matrix_to_matrix_44(pose->GetMatrix(itemIdx)).to_transform();
							boneNodeInfo->placementRS = rootPlacementXS.to_local(placementXS);
						}
					}
				}
			}
		}
	}
}

void BoneExtractor::extract_bones_from_array(Meshes::SkeletonImportOptions const& _options)
{
	while (!toProcess.is_empty())
	{
		BoneNodeInfoToProcess boneNodeInfo = toProcess.get_first();
		toProcess.remove_at(0);
		extract_bone(boneNodeInfo, _options);
		extract_children_bones_to_process(boneNodeInfo, _options);
	}
}

void BoneExtractor::extract_children_bones_to_process(BoneNodeInfoToProcess const& _boneNodeInfo, Meshes::SkeletonImportOptions const& _options)
{
	FbxNode* parentNodeForChildren = get_parent_node_for_children_of(_boneNodeInfo.parentNode, _boneNodeInfo.node, _options);

	// go through children
	int childCount = _boneNodeInfo.node->GetChildCount();
	for (int childIdx = 0; childIdx < childCount; ++childIdx)
	{
		FbxNode* childNode = _boneNodeInfo.node->GetChild(childIdx);
		toProcess.push_back(BoneNodeInfoToProcess(_boneNodeInfo.rootNode, parentNodeForChildren, childNode));
	}
}

FbxNode* BoneExtractor::get_parent_node_for_children_of(FbxNode* _parentNode, FbxNode* _node, Meshes::SkeletonImportOptions const& _options)
{
	FbxNode* parentNode = _parentNode ? _parentNode : _node->GetParent();

	if (FbxNodeAttribute* attr = _node->GetNodeAttribute())
	{
		if (attr->GetAttributeType() == FbxNodeAttribute::eSkeleton)
		{
			if (_options.socketNodePrefix.is_empty() || !String::from_char8(_node->GetName()).has_prefix(_options.socketNodePrefix))
			{
				parentNode = _node;
			}
		}
	}
	return parentNode;
}

Transform BoneExtractor::get_placement_rs(Matrix44 const& _rootGlobal, Matrix44 const& _nodeGlobal, Meshes::SkeletonImportOptions const& _options)
{
	Matrix44 importFromPlacement = Matrix44::identity;
	if (_options.importFromPlacement.is_set())
	{
		importFromPlacement = _options.importFromPlacement.get().to_matrix();
	}
	else if (!_options.importFromNode.is_empty())
	{
		// by default import from same location as root node
		importFromPlacement.set_translation(_rootGlobal.get_translation());
	}
	Matrix44 placeAfterImport = Matrix44::identity;
	if (_options.placeAfterImport.is_set())
	{
		placeAfterImport = _options.placeAfterImport.get().to_matrix();
	}
	Vector3 const importScale = _options.importScale;

	// we're only interested in node relative to root
	Matrix44 rootMatrixWS = _rootGlobal;
	Matrix44 nodeMatrixWS = _nodeGlobal;

	// remove scale
	// explanation[1]: when we export from Blender, we have to scale it up by 100 because exporter doesn't provide info about proper units used (cm)
	// explanation[2]: we don't care about scales for bones - we want pure location and placement
	rootMatrixWS.remove_scale();
	nodeMatrixWS.remove_scale();

	// apply "import from placement"
	rootMatrixWS = importFromPlacement.to_local(rootMatrixWS);
	nodeMatrixWS = importFromPlacement.to_local(nodeMatrixWS);
	rootMatrixWS.set_translation(rootMatrixWS.get_translation() * importScale);
	nodeMatrixWS.set_translation(nodeMatrixWS.get_translation() * importScale);
	rootMatrixWS = placeAfterImport.to_world(rootMatrixWS);
	nodeMatrixWS = placeAfterImport.to_world(nodeMatrixWS);

	// move node to root space
	Matrix44 nodeMatrixRS = rootMatrixWS.to_local(nodeMatrixWS);

	// rotate using import bone matrix info
	nodeMatrixRS.set_orientation_matrix(_options.importBoneMatrix.to_world(nodeMatrixRS.get_orientation_matrix()));

	// we will be storing transform
	Transform nodeRS = nodeMatrixRS.to_transform();

	return nodeRS;
}

void BoneExtractor::extract_bone(BoneExtractor::BoneNodeInfoToProcess const& _boneNodeInfo, Meshes::SkeletonImportOptions const& _options)
{
	FbxNode* rootNode = _boneNodeInfo.rootNode;
	FbxNode* parentNode = _boneNodeInfo.parentNode;
	FbxNode* node = _boneNodeInfo.node;

	if (_options.importFromNode == String::from_char8(node->GetName()) &&
		_options.skipImportFromNode)
	{
		// do not import this one
		return;
	}

	parentNode = parentNode ? parentNode : node->GetParent();

	if (parentNode &&
		_options.skipImportFromNode &&
		_options.importFromNode == String::from_char8(parentNode->GetName()))
	{
		parentNode = nullptr;
	}

	if (parentNode && !_options.importBonesFromMeshNodesWithPrefix.is_empty())
	{
		String nodeName = String::from_char8(parentNode->GetName());

		bool parentOk = false;
		for_every(prefix, _options.importBonesFromMeshNodesWithPrefix)
		{
			if (nodeName.has_prefix(*prefix))
			{
				parentOk = true;
				break;
			}
		}
		if (!parentOk)
		{
			parentNode = nullptr;
		}
	}

	if (FbxNodeAttribute* attr = node->GetNodeAttribute())
	{
		if ((_options.importBonesFromMeshNodesWithPrefix.is_empty() && attr->GetAttributeType() == FbxNodeAttribute::eSkeleton) ||
			(!_options.importBonesFromMeshNodesWithPrefix.is_empty() && attr->GetAttributeType() == FbxNodeAttribute::eMesh))
		{
			String nodeName = String::from_char8(node->GetName());
			bool okToImport = true;
			if (!_options.socketNodePrefix.is_empty() && nodeName.has_prefix(_options.socketNodePrefix))
			{
				okToImport = false;
			}
			if (okToImport && !_options.importBonesFromMeshNodesWithPrefix.is_empty())
			{
				okToImport = false;
				for_every(prefix, _options.importBonesFromMeshNodesWithPrefix)
				{
					if (nodeName.has_prefix(*prefix))
					{
						okToImport = true;
						break;
					}
				}
			}
			if (okToImport)
			{
				// we will be storing transform
				// use FBXSDK_TIME_INFINITE to extract without animation applied
				FbxTime time;
				time.SetSecondDouble(0);
				Transform nodeRS = get_placement_rs(
					fbx_matrix_to_matrix_44(rootNode->EvaluateGlobalTransform(time)),
					fbx_matrix_to_matrix_44(node->EvaluateGlobalTransform(time)),
					_options);

				extracted.push_back();
				auto& b = extracted.get_last();
				b.rootNode = rootNode;
				b.node = node;
				b.boneName = Name(String::from_char8(node->GetName()));
				b.parentNode = parentNode;
				b.parentName = parentNode ? Name(String::from_char8(parentNode->GetName())) : Name::invalid();
				b.placementRS = nodeRS;
			}
		}
	}
}

#endif

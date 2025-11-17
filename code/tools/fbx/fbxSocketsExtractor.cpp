#include "fbxSocketsExtractor.h"

#include "fbxScene.h"
#include "fbxUtils.h"

#include "fbxImporter.h"

#include "..\..\core\memory\memory.h"

#ifdef USE_FBX
using namespace FBX;

void SocketsExtractor::initialise_static()
{
	Meshes::SocketDefinitionSet::importer().register_file_type_with_options(String(TXT("fbx")),
		[](String const & _fileName, Meshes::SocketDefinitionSetImportOptions const & _options)
		{
			Concurrency::ScopedSpinLock lock(FBX::Importer::importerLock);
			Meshes::SocketDefinitionSet* sockets = nullptr;
			if (FBX::Scene* scene = FBX::Importer::import(_fileName))
			{
				//scene->remove_all_animations();
				sockets = SocketsExtractor::extract_from(scene, _options);
			}
			else
			{
				error(TXT("problem importing fbx from file \"%S\""), _fileName.to_char());
			}
			return sockets;
		}
	);
}

void SocketsExtractor::close_static()
{
	Meshes::SocketDefinitionSet::importer().unregister(String(TXT("fbx")));
}

Meshes::SocketDefinitionSet* SocketsExtractor::extract_from(Scene* _scene, Meshes::SocketDefinitionSetImportOptions const & _options)
{
	SocketsExtractor extractor(_scene);
	extractor.extract_sockets(_options);
	return extractor.sockets;
}

SocketsExtractor::SocketsExtractor(Scene* _scene)
: scene(_scene)
, sockets(nullptr)
{
	an_assert(scene);
}

SocketsExtractor::~SocketsExtractor()
{
}

void SocketsExtractor::extract_sockets(Meshes::SocketDefinitionSetImportOptions const & _options)
{
	an_assert(scene);
	an_assert(!sockets);

	sockets = Meshes::SocketDefinitionSet::create_new();

	if (FbxNode* rootNode = scene->get_node(_options.importFromNode))
	{
		extract_sockets(rootNode, nullptr, rootNode, _options);
	}
}

void SocketsExtractor::extract_sockets(FbxNode* _rootNode, FbxNode* _parentNode, FbxNode* _node, Meshes::SocketDefinitionSetImportOptions const & _options)
{
	FbxNode* parentNode = _parentNode ? _parentNode : _node->GetParent();

	Matrix44 importFromPlacement = Matrix44::identity;
	if (_options.importFromPlacement.is_set())
	{
		importFromPlacement = _options.importFromPlacement.get().to_matrix();
	}
	else if (!_options.importFromNode.is_empty())
	{
		// by default import from same location as root node
		importFromPlacement.set_translation(fbx_matrix_to_matrix_44(_rootNode->EvaluateGlobalTransform(FBXSDK_TIME_INFINITE)).get_translation());
	}
	Matrix44 placeAfterImport = Matrix44::identity;
	if (_options.placeAfterImport.is_set())
	{
		placeAfterImport = _options.placeAfterImport.get().to_matrix();
	}
	Vector3 const importScale = _options.importScale;

	if (FbxNodeAttribute* attr = _node->GetNodeAttribute())
	{
		if (attr->GetAttributeType() == FbxNodeAttribute::eSkeleton)
		{
			bool thisNodeIsSocket = false;
			if (_options.socketNodePrefix.is_empty() || String::from_char8(_node->GetName()).has_prefix(_options.socketNodePrefix))
			{
				thisNodeIsSocket = true;

				// we're only interested in node relative to root
				Matrix44 rootMatrixWS = fbx_matrix_to_matrix_44(_rootNode->EvaluateGlobalTransform(FBXSDK_TIME_INFINITE));
				Matrix44 nodeMatrixWS = fbx_matrix_to_matrix_44(_node->EvaluateGlobalTransform(FBXSDK_TIME_INFINITE));

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

				// extract name and find existing socket or find new
				Name socketName = Name(String::from_char8(_node->GetName()).without_prefix(_options.socketNodePrefix));
				Meshes::SocketDefinition* socket = sockets->find_or_create_socket(socketName);

				// add default placement that will be also used for static (root space becomes model space)
				socket->set_placement_MS(nodeRS);

				todo_note(TXT("extract sockets for skeleton"));
			}
			if (!thisNodeIsSocket)
			{
				parentNode = _node;
			}
		}
	}

	// go through children
	int childCount = _node->GetChildCount();
	for (int childIdx = 0; childIdx < childCount; ++childIdx)
	{
		FbxNode* childNode = _node->GetChild(childIdx);
		extract_sockets(_rootNode, parentNode, childNode, _options);
	}
}

#endif

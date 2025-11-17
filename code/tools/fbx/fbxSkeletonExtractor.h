#pragma once

#include "fbxLib.h"

#include "..\..\core\mesh\skeleton.h"

#ifdef USE_FBX
namespace FBX
{
	class Scene;

	struct BoneExtractor
	{
		struct BoneNodeInfo
		{
			FbxNode* rootNode; 
			FbxNode* node;
			FbxNode* parentNode;

			Name boneName;
			Name parentName;
			Transform placementRS = Transform::identity; // root space
		};

		Array<BoneNodeInfo> extracted;

		void extract_from_root_node(FbxNode* _rootNode, Meshes::SkeletonImportOptions const& _options);

		static Transform get_placement_rs(Matrix44 const& _rootGlobal, Matrix44 const& _nodeGlobal, Meshes::SkeletonImportOptions const& _options);

	private:
		struct BoneNodeInfoToProcess
		{
			FbxNode* rootNode;
			FbxNode* parentNode;
			FbxNode* node;

			BoneNodeInfoToProcess();
			BoneNodeInfoToProcess(FbxNode* _rootNode, FbxNode* _parentNode, FbxNode* _node);
		};

		Array<BoneNodeInfoToProcess> toProcess;

		void extract_bones_from_array(Meshes::SkeletonImportOptions const& _options);
		void extract_children_bones_to_process(BoneNodeInfoToProcess const& _boneNodeInfoToProcess, Meshes::SkeletonImportOptions const& _options);
		FbxNode* get_parent_node_for_children_of(FbxNode* _parentNode, FbxNode* _node, Meshes::SkeletonImportOptions const& _options);
		void extract_bone(BoneNodeInfoToProcess const& _boneNodeInfoToProcess, Meshes::SkeletonImportOptions const& _options);
	};

	class SkeletonExtractor
	{
	public:
		static void initialise_static();
		static void close_static();

		static Meshes::Skeleton* extract_from(Scene* _scene, Meshes::SkeletonImportOptions const & _options = Meshes::SkeletonImportOptions());

	protected:
		SkeletonExtractor(Scene* _scene);
		~SkeletonExtractor();

		void extract_skeleton(Meshes::SkeletonImportOptions const & _options);

	private:
		Scene* scene = nullptr;

		Meshes::Skeleton* skeleton = nullptr;

		BoneExtractor boneExtractor;

		void process_bone(BoneExtractor::BoneNodeInfo const& _extractedBoneNodeInfo, Meshes::SkeletonImportOptions const& _options);
	};

};
#endif
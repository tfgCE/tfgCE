#pragma once

#include "remapBoneArray.h"
#include "boneDefinition.h"
#include "pose.h"
#include "..\serialisation\serialisableResource.h"
#include "..\serialisation\importer.h"

namespace Meshes
{
	struct SkeletonImportOptions
	{
		String importFromNode; // fbx node
		Matrix33 importBoneMatrix;
		Optional<Transform> importFromPlacement;
		Optional<Transform> placeAfterImport;
		Vector3 importScale;
		bool skipImportFromNode; // do not import "importFromNode" as bone - true by default because it works only as a container
		Array<String> importBonesFromMeshNodesWithPrefix; // if this is set, skeleton bones will be skipped and mesh nodes' placements will be used
		int importFromPoseIdx = 0; // grab pose (if exists)

		String socketNodePrefix;

		SkeletonImportOptions();

		bool load_from_xml(IO::XML::Node const * _node);
	};

	class Skeleton
	: public SerialisableResource
	{
	public:
		static Skeleton* create_new();

		Skeleton();
		virtual ~Skeleton();

		static bool are_compatible(Skeleton const* _a, Skeleton const* _b); // same bone names

		static Importer<Skeleton, SkeletonImportOptions> & importer() { return s_importer; }

		bool load_from_xml(IO::XML::Node const * _node);
		static IO::XML::Node const * find_xml_bone_node(IO::XML::Node const * _node, Name const & _boneName);

		int get_num_bones() const { return bones.get_size(); }
		Array<BoneDefinition> const & get_bones() const { return bones; }
		Array<BoneDefinition> & access_bones() { return bones; }
		int get_parent_of(int _boneIdx) const;
		bool is_descendant(int _ancestorBoneIdx, int _descendantBoneIdx, bool _mightBeSameIdx = false) const;
		Array<Transform> const & get_bones_default_placement_LS() const { an_assert_immediate(preparedToUse);  return bonesDefaultPlacementLS; }
		Array<Transform> const & get_bones_default_placement_MS() const { an_assert_immediate(preparedToUse);  return bonesDefaultPlacementMS; }
		Array<Transform> const & get_bones_default_inverted_placement_MS() const { an_assert_immediate(preparedToUse);  return bonesDefaultInvertedPlacementMS; }
		Array<Matrix44> const & get_bones_default_matrix_MS() const { an_assert_immediate(preparedToUse);  return bonesDefaultMatrixMS; }
		Array<Matrix44> const & get_bones_default_inverted_matrix_MS() const { an_assert_immediate(preparedToUse);  return bonesDefaultInvertedMatrixMS; }
		Pose const * get_default_pose_ls() const { return defaultPoseLS; }

		int32 find_bone_index(Name const & _boneName) const;
		int32 find_bone_index(String const & _boneName) const;
		BoneDefinition const * find_bone(Name const & _boneName) const;
		BoneDefinition* find_bone(Name const & _boneName);
		BoneDefinition* find_or_create_bone(Name const & _boneName);

		bool prepare_to_use();

		void change_bones_ls_to_ms(Array<Transform> const & _bonesLS, OUT_ Array<Transform> & _bonesMS) const;
		void change_bones_ls_to_ms(Array<Transform> const & _bonesLS, OUT_ Array<Transform> & _bonesMS, Transform const & _placementMS) const;
		void change_bones_ls_to_ms(Array<Transform> const & _bonesLS, OUT_ Array<Matrix44> & _bonesMS) const;
		void change_bones_ls_to_ms(Array<Transform> const & _bonesLS, OUT_ Array<Matrix44> & _bonesMS, Transform const & _placementMS) const;

		void fuse(Skeleton const * _skeleton, RemapBoneArray & _remapBones); // remap in skeleton being fused
		Skeleton* create_copy() const;

	public: // SerialisableResource
		virtual bool serialise(Serialiser & _serialiser);

	private:
		static Importer<Skeleton, SkeletonImportOptions> s_importer;

		Array<BoneDefinition> bones;

		// this is filled when preparing to use
		bool preparedToUse;
		Array<int> bonesParents;
		Array<Transform> bonesDefaultPlacementLS;
		Array<Transform> bonesDefaultPlacementMS;
		Array<Transform> bonesDefaultInvertedPlacementMS;
		Array<Matrix44> bonesDefaultMatrixMS;
		Array<Matrix44> bonesDefaultInvertedMatrixMS;

		Pose* defaultPoseLS = nullptr;

	private:
		// this is just for loading
		struct ExtractedBoneFromXML
		{
			IO::XML::Node const * boneNode;
			IO::XML::Node const * parentBoneNode;
			ExtractedBoneFromXML();
			ExtractedBoneFromXML(IO::XML::Node const * _boneNode, IO::XML::Node const * _parentBoneNode);
		};
		Array<ExtractedBoneFromXML> extractedBonesToProcess;

		void add_bone_xml_node_to_process(IO::XML::Node const * _boneNode, IO::XML::Node const * _parentBoneNode);
		bool process_next_bone_xml_node();
	};
};

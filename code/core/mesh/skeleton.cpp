#include "skeleton.h"

#include "..\serialisation\serialiser.h"

using namespace Meshes;

Importer<Skeleton, SkeletonImportOptions> Skeleton::s_importer;

SkeletonImportOptions::SkeletonImportOptions()
: importBoneMatrix(Matrix33::identity)
, importScale(Vector3::one)
, skipImportFromNode(true)
{
}

bool SkeletonImportOptions::load_from_xml(IO::XML::Node const * _node)
{
	bool result = true;
	if (IO::XML::Attribute const * attr = _node->get_attribute(TXT("importFromNode")))
	{
		importFromNode = attr->get_as_string();
	}
	if (IO::XML::Node const * node = _node->first_child_named(TXT("importBoneMatrix")))
	{
		result &= importBoneMatrix.load_axes_from_xml(node);
	}
	if (IO::XML::Attribute const * attr = _node->get_attribute(TXT("skipImportFromNode")))
	{
		skipImportFromNode = attr->get_as_bool();
	}
	if (IO::XML::Node const * childNode = _node->first_child_named(TXT("importFromPlacement")))
	{
		importFromPlacement.set_if_not_set(Transform::identity);
		importFromPlacement.access().load_from_xml(childNode);
	}
	if (IO::XML::Node const * childNode = _node->first_child_named(TXT("placeAfterImport")))
	{
		placeAfterImport.set_if_not_set(Transform::identity);
		placeAfterImport.access().load_from_xml(childNode);
	}
	{
		float const isDef = -1000.0f;
		float is = _node->get_float_attribute_or_from_child(TXT("importScale"), isDef);
		if (is != isDef)
		{
			importScale = is * Vector3::one;
		}
		importScale.load_from_xml_child_node(_node, TXT("importScale"), TXT("x"), TXT("y"), TXT("z"), TXT("axis"), true);
	}
	socketNodePrefix = _node->get_string_attribute(TXT("socketNodePrefix"), socketNodePrefix);
	if (auto* attr = _node->get_attribute(TXT("importBonesFromMeshNodesWithPrefix")))
	{
		importBonesFromMeshNodesWithPrefix.push_back(attr->get_as_string());
	}
	for_every(node, _node->children_named(TXT("importBonesFromMeshNodes")))
	{
		if (auto* attr = node->get_attribute(TXT("withPrefix")))
		{
			importBonesFromMeshNodesWithPrefix.push_back(attr->get_as_string());
		}
	}	
	importFromPoseIdx = _node->get_int_attribute(TXT("importFromPoseIdx"), importFromPoseIdx);

	return result;
}

//

Skeleton::ExtractedBoneFromXML::ExtractedBoneFromXML()
: boneNode(nullptr)
, parentBoneNode(nullptr)
{
}

Skeleton::ExtractedBoneFromXML::ExtractedBoneFromXML(IO::XML::Node const * _boneNode, IO::XML::Node const * _parentBoneNode)
: boneNode(_boneNode)
, parentBoneNode(_parentBoneNode)
{
}

//

Skeleton* Skeleton::create_new()
{
	return new Skeleton();
}

Skeleton::Skeleton()
: preparedToUse(false)
{
}

Skeleton::~Skeleton()
{
	if (defaultPoseLS)
	{
		defaultPoseLS->release();
		defaultPoseLS = nullptr;
	}
}

bool Skeleton::are_compatible(Skeleton const* _a, Skeleton const* _b)
{
	if (_a && _b &&
		_a->bones.get_size() == _b->bones.get_size())
	{
		auto bb = _b->bones.begin();
		for_every(ab, _a->bones)
		{
			if (ab->get_name() != bb->get_name())
			{
				return false;
			}
			++bb;
		}

		return true;
	}
	else
	{
		return false;
	}
}

bool Skeleton::load_from_xml(IO::XML::Node const * _node)
{
	bool result = true;

	for_every(boneNode, _node->children_named(TXT("bone")))
	{
		add_bone_xml_node_to_process(boneNode, nullptr);
	}

	while (!extractedBonesToProcess.is_empty())
	{
		result &= process_next_bone_xml_node();
	}
	memory_leak_suspect;
	extractedBonesToProcess.prune();
	forget_memory_leak_suspect;

	prepare_to_use();

	return result;
}

void Skeleton::add_bone_xml_node_to_process(IO::XML::Node const * _boneNode, IO::XML::Node const * _parentBoneNode)
{
	extractedBonesToProcess.push_back(ExtractedBoneFromXML(_boneNode, _parentBoneNode));
}

bool Skeleton::process_next_bone_xml_node()
{
	ExtractedBoneFromXML extractedBone = extractedBonesToProcess.get_first();
	extractedBonesToProcess.remove_at(0);

	for_every(childBoneNode, extractedBone.boneNode->children_named(TXT("bone")))
	{
		add_bone_xml_node_to_process(childBoneNode, extractedBone.boneNode);
	}

	bool result = true;
	Name boneName = BoneDefinition::load_name_from_xml(extractedBone.boneNode);
	BoneDefinition* bone = find_or_create_bone(boneName);
	if (extractedBone.parentBoneNode)
	{
		Name parentBoneName = BoneDefinition::load_name_from_xml(extractedBone.parentBoneNode);
		bone->set_parent_name(parentBoneName);
	}
	result &= bone->load_from_xml(extractedBone.boneNode);

	return result;
}

IO::XML::Node const * Skeleton::find_xml_bone_node(IO::XML::Node const * _node, Name const & _boneName)
{
	for_every(boneNode, _node->children_named(TXT("bone")))
	{
		if (BoneDefinition::load_name_from_xml(boneNode) == _boneName)
		{
			return boneNode;
		}
	}
	return nullptr;
}

int32 Skeleton::find_bone_index(Name const & _boneName) const
{
	for_every(bone, bones)
	{
		if (bone->get_name() == _boneName)
		{
			return (int32)(bone - bones.get_data());
		}
	}
	return NONE;
}

int32 Skeleton::find_bone_index(String const& _boneName) const
{
	for_every(bone, bones)
	{
		if (_boneName == bone->get_name().to_char())
		{
			return (int32)(bone - bones.get_data());
		}
	}
	return NONE;
}

BoneDefinition const * Skeleton::find_bone(Name const & _boneName) const
{
	for_every(bone, bones)
	{
		if (bone->get_name() == _boneName)
		{
			return bone;
		}
	}
	return nullptr;
}

BoneDefinition * Skeleton::find_bone(Name const & _boneName)
{
	return cast_to_nonconst(cast_to_const(this)->find_bone(_boneName));
}

BoneDefinition* Skeleton::find_or_create_bone(Name const & _boneName)
{
	if (BoneDefinition* bone = find_bone(_boneName))
	{
		return bone;
	}
	BoneDefinition bone;
	bone.set_name(_boneName);
	bones.push_back(bone);
	return &(bones.get_last());
}

bool Skeleton::prepare_to_use()
{
	bool everythingOk = false;
	while (! everythingOk)
	{
		everythingOk = true;
		int32 boneIndex = 0;
		for_every(bone, bones)
		{
			bone->parentIndex = NONE;
			if (bone->parentName.is_valid())
			{
				bone->parentIndex = find_bone_index(bone->parentName);
				if (bone->parentIndex == NONE)
				{
					// reset to invalid
					bone->parentName = Name::invalid();
				}
				if (boneIndex < bone->parentIndex)
				{
					// check for circular
					{
						Name startedAtBone = bone->name;
						int circularCheckIndex = boneIndex;
						while (circularCheckIndex != NONE)
						{
							circularCheckIndex = find_bone_index(bones[circularCheckIndex].parentName);
							if (circularCheckIndex != NONE && bones[circularCheckIndex].name == startedAtBone)
							{
								error(TXT("circular dependency detected"));
								return false;
							}
						}
					}
					// change order - put this bone where parent currently is (when removing bone, parent will move up)
					int parentIndex = bone->parentIndex;
					BoneDefinition bone = bones[boneIndex];
					bones.remove_at(boneIndex);
					bones.insert_at(parentIndex, bone);
					everythingOk = false;
					break;
				}
			}
			++boneIndex;
		}
	}

	memory_leak_suspect;
	bones.prune();
	forget_memory_leak_suspect;

	// calculate all placements
	bonesParents.set_size(bones.get_size());
	bonesDefaultPlacementLS.set_size(bones.get_size());
	bonesDefaultPlacementMS.set_size(bones.get_size());
	bonesDefaultInvertedPlacementMS.set_size(bones.get_size());
	bonesDefaultMatrixMS.set_size(bones.get_size());
	bonesDefaultInvertedMatrixMS.set_size(bones.get_size());
	auto iBoneParent = bonesParents.begin();
	auto iBoneDefaultPlacementLS = bonesDefaultPlacementLS.begin();
	auto iBoneDefaultPlacementMS = bonesDefaultPlacementMS.begin();
	auto iBoneDefaultInvertedPlacementMS = bonesDefaultInvertedPlacementMS.begin();
	auto iBoneDefaultMatrixMS = bonesDefaultMatrixMS.begin();
	auto iBoneDefaultInvertedMatrixMS = bonesDefaultInvertedMatrixMS.begin();
	for_every(bone, bones)
	{
		Transform parentMS = Transform::identity;
		if (bone->parentIndex != NONE)
		{
			parentMS = bones[bone->parentIndex].placementMS;
		}
		bone->placementMS.normalise_orientation();
		bone->placementInvMS = bone->placementMS.inverted();
		bone->placementLS = parentMS.to_local(bone->placementMS);
		bone->placementLS.normalise_orientation();
		bone->placementMatMS = bone->placementMS.to_matrix();
		bone->placementMatLS = bone->placementLS.to_matrix();
		bone->placementInvMatMS = bone->placementMatMS.inverted();
		// store in arrays
		*iBoneParent = bone->parentIndex;
		*iBoneDefaultPlacementLS = bone->placementLS;
		*iBoneDefaultPlacementMS = bone->placementMS;
		*iBoneDefaultInvertedPlacementMS = bone->placementInvMS;
		*iBoneDefaultMatrixMS = bone->placementMatMS;
		*iBoneDefaultInvertedMatrixMS = bone->placementInvMatMS;
		++iBoneParent;
		++iBoneDefaultPlacementLS;
		++iBoneDefaultPlacementMS;
		++iBoneDefaultInvertedPlacementMS;
		++iBoneDefaultMatrixMS;
		++iBoneDefaultInvertedMatrixMS;
	}
	
	if (defaultPoseLS)
	{
		defaultPoseLS->release();
	}
	defaultPoseLS = Pose::get_one(this);
	defaultPoseLS->fill_with(bonesDefaultPlacementLS);

	preparedToUse = true;

	return true;
}

void Skeleton::change_bones_ls_to_ms(Array<Transform> const & _bonesLS, OUT_ Array<Transform> & _bonesMS) const
{
	auto boneLS = _bonesLS.begin();
	auto boneMS = _bonesMS.begin();
	for_every(boneParent, bonesParents)
	{
		int const boneParentIdx = *boneParent;
		*boneMS = boneParentIdx == NONE ? *boneLS : _bonesMS[boneParentIdx].to_world(*boneLS);
		++boneLS;
		++boneMS;
	}
}

void Skeleton::change_bones_ls_to_ms(Array<Transform> const & _bonesLS, OUT_ Array<Transform> & _bonesMS, Transform const & _placementMS) const
{
	auto boneLS = _bonesLS.begin();
	auto boneMS = _bonesMS.begin();
	for_every(boneParent, bonesParents)
	{
		int const boneParentIdx = *boneParent;
		*boneMS = boneParentIdx == NONE ? _placementMS.to_world(*boneLS) : _bonesMS[boneParentIdx].to_world(*boneLS);
		++boneLS;
		++boneMS;
	}
}

void Skeleton::change_bones_ls_to_ms(Array<Transform> const & _bonesLS, OUT_ Array<Matrix44> & _bonesMS) const
{
	auto boneLS = _bonesLS.begin();
	auto boneMS = _bonesMS.begin();
	for_every(boneParent, bonesParents)
	{
		int const boneParentIdx = *boneParent;
		*boneMS = boneParentIdx == NONE ? boneLS->to_matrix() : _bonesMS[boneParentIdx].to_world(boneLS->to_matrix());
		++boneLS;
		++boneMS;
	}
}

void Skeleton::change_bones_ls_to_ms(Array<Transform> const & _bonesLS, OUT_ Array<Matrix44> & _bonesMS, Transform const & _placementMS) const
{
	auto boneLS = _bonesLS.begin();
	auto boneMS = _bonesMS.begin();
	for_every(boneParent, bonesParents)
	{
		int const boneParentIdx = *boneParent;
		*boneMS = boneParentIdx == NONE ? _placementMS.to_world(*boneLS).to_matrix() : _bonesMS[boneParentIdx].to_world(boneLS->to_matrix());
		++boneLS;
		++boneMS;
	}
}

#define VER_BASE 0
#define CURRENT_VERSION VER_BASE

bool Skeleton::serialise(Serialiser & _serialiser)
{
	version = CURRENT_VERSION;
	bool result = SerialisableResource::serialise(_serialiser);
	if (version > CURRENT_VERSION)
	{
		error(TXT("invalid bone definition set version"));
		return false;
	}
	result &= serialise_data(_serialiser, bones);
	if (_serialiser.is_reading())
	{
		prepare_to_use();
	}
	return result;
}

int Skeleton::get_parent_of(int _boneIdx) const
{
	an_assert(bones.is_index_valid(_boneIdx));
	an_assert(bones[_boneIdx].get_parent_index() < _boneIdx);
	return bones[_boneIdx].get_parent_index();
}

bool Skeleton::is_descendant(int _ancestorBoneIdx, int _descendantBoneIdx, bool _mightBeSameIdx) const
{
	an_assert(bones.is_index_valid(_ancestorBoneIdx));
	an_assert(bones.is_index_valid(_descendantBoneIdx));
	if (!_mightBeSameIdx)
	{
		_descendantBoneIdx = bones[_descendantBoneIdx].get_parent_index();
	}
	while (_descendantBoneIdx != NONE)
	{
		if (_ancestorBoneIdx == _descendantBoneIdx)
		{
			return true;
		}
		_descendantBoneIdx = bones[_descendantBoneIdx].get_parent_index();
	}

	return false;
}

Skeleton* Skeleton::create_copy() const
{
	Skeleton* copy = new Skeleton();

	copy->bones = bones;
	copy->prepare_to_use();

	return copy;
}

void Skeleton::fuse(Skeleton const * _skeleton, RemapBoneArray & _remapBones)
{
	an_assert(_remapBones.is_empty());
	_remapBones.make_space_for(_skeleton->bones.get_size());
	for_every(bone, _skeleton->bones)
	{
		int exists = NONE;
		for_every(b, bones)
		{
			if (b->get_name() == bone->get_name() &&
				b->get_parent_name() == bone->get_parent_name())
			{
				exists = for_everys_index(b);
				break;
			}
		}
		if (exists == NONE)
		{
			exists = bones.get_size();
			bones.push_back(*bone);
		}
		_remapBones.push_back(exists);
	}

	prepare_to_use();
}

#include "meshGenBones.h"

#include "..\meshGenGenerationContext.h"
#include "..\meshGenUtils.h"
#include "..\meshGenElementBone.h"
#include "..\meshGenValueDef.h"

#include "..\..\..\core\containers\arrayStack.h"

using namespace Framework;
using namespace MeshGeneration;
using namespace Bones;

/**
 *	Allows:
 *		mirror using mirror axis and point
 */
class SkelBoneData
: public ElementBoneData
{
	FAST_CAST_DECLARE(SkelBoneData);
	FAST_CAST_BASE(ElementBoneData);
	FAST_CAST_END();

	typedef ElementBoneData base;
public:
	override_ bool is_xml_node_handled(IO::XML::Node const * _node) const;
	override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);

public: // we won't be exposed anywhere else, so we may be public here!
	// not one is set as some are exclusive
	bool mayAlreadyExist = false;
	ValueDef<Name> name = ValueDef<Name>();
	ValueDef<Name> parent = ValueDef<Name>();
	ValueDef<bool> pointAtChild = ValueDef<bool>();
	ValueDef<bool> keepParentOrientation = ValueDef<bool>();
	ValueDef<Transform> placement = ValueDef<Transform>();
	ValueDef<Transform> relativePlacement = ValueDef<Transform>();
	ValueDef<Vector3> location = ValueDef<Vector3>();
	ValueDef<Vector3> relativeLocation = ValueDef<Vector3>(); // relative to parent's placement, this may not work properly if pointAtChild is used!
	ValueDef<Vector3> relativePureLocation = ValueDef<Vector3>(); // relative to parent's location (ignores orientation)
	ValueDef<Vector3> forward = ValueDef<Vector3>();
	ValueDef<Vector3> up = ValueDef<Vector3>();
	ValueDef<Vector3> right = ValueDef<Vector3>();

	void build_orientation_matrix(REF_ Matrix33 & _mat, GenerationContext & _context, Optional<Vector3> const & _forwardOverride = NP) const;
};

//

bool Bones::bone_start(GenerationContext & _context, ElementInstance & _instance, ::Framework::MeshGeneration::ElementBoneData const * _data, int _boneIdx)
{
	bool result = true;

	if (SkelBoneData const * data = fast_cast<SkelBoneData>(_data))
	{
		if (data->name.is_set())
		{
			Name boneName = data->name.get(_context);
			boneName = _context.resolve_bone(boneName);

			int boneIdx;
			if (data->mayAlreadyExist && _context.get_generated_bone_if_exists(boneName, &boneIdx))
			{
				Bone& bone = _context.access_generated_bones()[boneIdx];
				// will be placed wherever it is placed
				_context.set_current_parent_bone(bone.boneName);
			}
			else if (!_context.add_generated_bone(boneName, _instance, &boneIdx))
			{
				result = false;
			}
			else
			{
				Bone& bone = _context.access_generated_bones()[boneIdx];
				if (data->parent.is_set())
				{
					bone.parentName = _context.resolve_bone(data->parent.get(_context));
				}
				if (data->placement.is_set())
				{
					bone.placement = data->placement.get(_context);
				}
				if (data->location.is_set())
				{
					bone.placement.set_translation(data->location.get(_context));
				}
				Bone const * parentBone = _context.get_generated_bone(bone.parentName);
				if (data->relativePlacement.is_set())
				{
					if (!parentBone)
					{
						error_generating_mesh(_instance, TXT("no parent bone \"%S\" found"), bone.parentName.to_char());
						result = false;
					}
					else
					{
						bone.placement = parentBone->placement.to_world(data->relativePlacement.get(_context));
					}
				}
				if (data->relativePureLocation.is_set())
				{
					if (!parentBone)
					{
						error_generating_mesh(_instance, TXT("no parent bone \"%S\" found"), bone.parentName.to_char());
						result = false;
					}
					else
					{
						bone.placement.set_translation(parentBone->placement.get_translation() + data->relativePureLocation.get(_context));
					}
				}

				// orientation
				if (data->forward.is_set() || data->right.is_set() || data->up.is_set())
				{
					Matrix33 mat = bone.placement.get_orientation().to_matrix_33();
					data->build_orientation_matrix(REF_ mat, _context);
					bone.placement.set_orientation(mat.to_quat());
				}

				// at the end, when we have bones set too
				if (data->relativeLocation.is_set())
				{
					if (!parentBone)
					{
						error_generating_mesh(_instance, TXT("no parent bone \"%S\" found"), bone.parentName.to_char());
						result = false;
					}
					else
					{
						bone.placement.set_translation(parentBone->placement.location_to_world(data->relativeLocation.get(_context)));
					}
				}

				if (data->keepParentOrientation.is_set())
				{
					if (!parentBone)
					{
						error_generating_mesh(_instance, TXT("no parent bone \"%S\" found"), bone.parentName.to_char());
						result = false;
					}
					else
					{
						bone.placement.set_orientation(parentBone->placement.get_orientation());
					}
				}

				if (result)
				{
					_context.set_current_parent_bone(bone.boneName);
				}
			}
		}
	}

	return result;
}

bool Bones::bone_end(GenerationContext & _context, ElementInstance & _instance, ::Framework::MeshGeneration::ElementBoneData const * _data, int _boneIdx)
{
	bool result = true;

	if (SkelBoneData const * data = fast_cast<SkelBoneData>(_data))
	{
		if (_context.access_generated_bones().is_index_valid(_boneIdx))
		{
			Bone& bone = _context.access_generated_bones()[_boneIdx];
			if (data->pointAtChild.is_set() && data->pointAtChild.get(_context))
			{
				bool found = false;
				for_every(childBone, _context.get_generated_bones())
				{
					if (childBone->parentName == bone.boneName)
					{
						Matrix33 mat = bone.placement.get_orientation().to_matrix_33();
						data->build_orientation_matrix(REF_ mat, _context, childBone->placement.get_translation() - bone.placement.get_translation());
						bone.placement.set_orientation(mat.to_quat());
						found = true;
						break;
					}
				}
				if (!found)
				{
					error_generating_mesh(_instance, TXT("can't point at a child, as there is no child for bone \"%S\""), bone.boneName.to_char());
				}
			}
		}
		else
		{
			error_generating_mesh(_instance, TXT("no bone generated? check previous error messages"));
		}
	}

	return result;
}

::Framework::MeshGeneration::ElementBoneData* Bones::create_bone_data()
{
	return new SkelBoneData();
}

//

REGISTER_FOR_FAST_CAST(SkelBoneData);

bool SkelBoneData::is_xml_node_handled(IO::XML::Node const * _node) const
{
	if (_node->get_name() == TXT("pointAtChild")) return true;
	if (_node->get_name() == TXT("keepParentOrientation")) return true;
	if (_node->get_name() == TXT("placement")) return true;
	if (_node->get_name() == TXT("relativePlacement")) return true;
	if (_node->get_name() == TXT("location")) return true;
	if (_node->get_name() == TXT("relativeLocation")) return true;
	if (_node->get_name() == TXT("relativePureLocation")) return true;
	if (_node->get_name() == TXT("forward")) return true;
	if (_node->get_name() == TXT("up")) return true;
	if (_node->get_name() == TXT("right")) return true;

	return base::is_xml_node_handled(_node);
}

bool SkelBoneData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;

	mayAlreadyExist = _node->get_bool_attribute_or_from_child_presence(TXT("mayAlreadyExist"), mayAlreadyExist);

	name.load_from_xml(_node, TXT("name"));
	parent.load_from_xml(_node, TXT("parent"));
	pointAtChild.load_from_xml(_node, TXT("pointAtChild"));
	if (_node->first_child_named(TXT("pointAtChild")))
	{
		pointAtChild.set(true);
	}
	keepParentOrientation.load_from_xml(_node, TXT("keepParentOrientation"));
	if (_node->first_child_named(TXT("keepParentOrientation")))
	{
		keepParentOrientation.set(true);
	}
	placement.load_from_xml_child_node(_node, TXT("placement"));
	relativePlacement.load_from_xml_child_node(_node, TXT("relativePlacement"));
	location.load_from_xml_child_node(_node, TXT("location"));
	relativeLocation.load_from_xml_child_node(_node, TXT("relativeLocation"));
	relativePureLocation.load_from_xml_child_node(_node, TXT("relativePureLocation"));
	forward.load_from_xml_child_node(_node, TXT("forward"));
	up.load_from_xml_child_node(_node, TXT("up"));
	right.load_from_xml_child_node(_node, TXT("right"));

	return result;
}

void SkelBoneData::build_orientation_matrix(REF_ Matrix33 & _mat, GenerationContext & _context, Optional<Vector3> const & _forwardOverride) const
{
	Vector3 forwardDir = forward.is_set() ? forward.get(_context) : Vector3::yAxis;
	Vector3 rightDir = right.is_set() ? right.get(_context) : Vector3::xAxis;
	Vector3 upDir = up.is_set() ? up.get(_context) : Vector3::zAxis;
	Axis::Type firstAxis = Axis::Y; // forward
	Axis::Type secondAxis = Axis::Z; // up
	if (_forwardOverride.is_set())
	{
		forwardDir = _forwardOverride.get().normal();
	}
	if (forward.is_set() || _forwardOverride.is_set())
	{
		if (!up.is_set())
		{
			if (right.is_set())
			{
				secondAxis = Axis::X;
			}
		}
	}
	else
	{
		firstAxis = Axis::Z; // up
		secondAxis = Axis::X; // right
		if (!up.is_set())
		{
			firstAxis = Axis::X; // up
			secondAxis = Axis::Z; // right
		}
	}
	_mat.build_orthogonal_from_axes(firstAxis, secondAxis, forwardDir, rightDir, upDir);
}

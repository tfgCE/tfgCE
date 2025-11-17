#include "boneRenamer.h"

using namespace Meshes;

void BoneRenamer::clear()
{
	renames.clear();
	usePrefix = Name::invalid();
	useSuffix = Name::invalid();
}

void BoneRenamer::add_from(BoneRenamer const & _renamer)
{
	usePrefix = _renamer.usePrefix;
	useSuffix = _renamer.useSuffix;
	for_every(rename, _renamer.renames)
	{
		renames.push_back(*rename);
	}
}

void BoneRenamer::use_rename_bone(Name const & _from, Name const & _to)
{
	renames.push_back(Rename(_from, _to, Rename::Anything));
}

void BoneRenamer::use_rename_bone_prefix(Name const & _from, Name const & _to)
{
	renames.push_back(Rename(_from, _to, Rename::Prefix));
}

void BoneRenamer::use_rename_bone_suffix(Name const & _from, Name const & _to)
{
	renames.push_back(Rename(_from, _to, Rename::Suffix));
}

void BoneRenamer::use_bone_prefix(Name const & _bonePrefix)
{
	usePrefix = _bonePrefix;
}

void BoneRenamer::use_bone_suffix(Name const & _boneSuffix)
{
	useSuffix = _boneSuffix;
}

Name BoneRenamer::apply(Name const & _bone) const
{
	Name bone = _bone;

	// check if we can rename
	if (!renames.is_empty())
	{
		for_every(rename, renames)
		{
			if (bone == rename->renameFrom)
			{
				bone = rename->renameTo;
			}
			int at = bone.to_string().find_first_of(rename->renameFrom.to_string());
			if (rename->what == Rename::Anything && at != NONE)
			{
				bone = Name(bone.to_string().replace(rename->renameFrom.to_string(), rename->renameTo.to_string()));
			}
			if (rename->what == Rename::Prefix && at == 0)
			{
				bone = Name(rename->renameTo.to_string() + bone.to_string().get_sub(rename->renameFrom.to_string().get_length()));
			}
			if (rename->what == Rename::Suffix && at == bone.to_string().get_length() - rename->renameFrom.to_string().get_length())
			{
				bone = Name(bone.to_string().get_left(at) + rename->renameTo.to_string());
			}
		}
	}

	// prefix
	if (usePrefix.is_valid())
	{
		int newBoneNameLength = usePrefix.to_string().get_length() + bone.to_string().get_length() + 1;
		allocate_stack_var(tchar, newBoneName, newBoneNameLength);
#ifdef AN_CLANG
		tsprintf(newBoneName, newBoneNameLength, TXT("%S%S"), usePrefix.to_char(), bone.to_char());
#else
		tsprintf(newBoneName, newBoneNameLength, TXT("%s%s"), usePrefix.to_char(), bone.to_char());
#endif
		bone = Name(newBoneName);
	}

	// suffix
	if (useSuffix.is_valid())
	{
		int newBoneNameLength = useSuffix.to_string().get_length() + bone.to_string().get_length() + 1;
		allocate_stack_var(tchar, newBoneName, newBoneNameLength);
#ifdef AN_CLANG
		tsprintf(newBoneName, newBoneNameLength, TXT("%S%S"), bone.to_char(), useSuffix.to_char());
#else
		tsprintf(newBoneName, newBoneNameLength, TXT("%s%s"), bone.to_char(), useSuffix.to_char());
#endif
		bone = Name(newBoneName);
	}

	return bone;
}

bool BoneRenamer::load_from_xml(IO::XML::Node const * _node)
{
	bool result = true;

	if (!_node)
	{
		return result;
	}

	usePrefix = _node->get_name_attribute(TXT("addPrefix"), usePrefix);
	useSuffix = _node->get_name_attribute(TXT("addSuffix"), useSuffix);
	usePrefix = _node->get_name_attribute(TXT("usePrefix"), usePrefix);
	useSuffix = _node->get_name_attribute(TXT("useSuffix"), useSuffix);

	for_every(renameNode, _node->children_named(TXT("rename")))
	{
		Name renameFrom = renameNode->get_name_attribute(TXT("from"));
		Name renameTo = renameNode->get_name_attribute(TXT("to"));
		if (renameFrom.is_valid() && renameTo.is_valid())
		{
			renames.push_back(Rename(renameFrom, renameTo, Rename::Anything));
		}
		else
		{
			error_loading_xml(renameNode, TXT("rename node doesn't have both \"from\" and \"to\""));
			result = false;
		}
	}

	for_every(renameNode, _node->children_named(TXT("renamePrefix")))
	{
		Name renameFrom = renameNode->get_name_attribute(TXT("from"));
		Name renameTo = renameNode->get_name_attribute(TXT("to"));
		if (renameFrom.is_valid() && renameTo.is_valid())
		{
			renames.push_back(Rename(renameFrom, renameTo, Rename::Prefix));
		}
		else
		{
			error_loading_xml(renameNode, TXT("renamePrefix node doesn't have both \"from\" and \"to\""));
			result = false;
		}
	}

	for_every(renameNode, _node->children_named(TXT("renameSuffix")))
	{
		Name renameFrom = renameNode->get_name_attribute(TXT("from"));
		Name renameTo = renameNode->get_name_attribute(TXT("to"));
		if (renameFrom.is_valid() && renameTo.is_valid())
		{
			renames.push_back(Rename(renameFrom, renameTo, Rename::Suffix));
		}
		else
		{
			error_loading_xml(renameNode, TXT("renameSuffix node doesn't have both \"from\" and \"to\""));
			result = false;
		}
	}

	return result;
}

bool BoneRenamer::load_from_xml_child_node(IO::XML::Node const * _node, tchar const * _childName)
{
	bool result = true;

	for_every(node, _node->children_named(_childName))
	{
		result &= load_from_xml(node);
	}

	return result;
}

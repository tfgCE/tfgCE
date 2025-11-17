#pragma once

#include "boneDefinition.h"
#include "..\serialisation\serialisableResource.h"
#include "..\serialisation\importer.h"

namespace Meshes
{
	struct BoneRenamer
	{
		typedef std::function<Name(Name const & _name)> RenameFunc;

		bool is_empty() const { return !usePrefix.is_valid() && !useSuffix.is_valid() && renames.is_empty(); }

		Name apply(Name const & _bone) const;

		void clear();

		void add_from(BoneRenamer const & _renamer);

		void use_rename_bone(Name const & _from, Name const & _to);
		void use_rename_bone_prefix(Name const & _from, Name const & _to);
		void use_rename_bone_suffix(Name const & _from, Name const & _to);
		void use_bone_prefix(Name const & _bonePrefix);
		void use_bone_suffix(Name const & _boneSuffix);

		bool load_from_xml(IO::XML::Node const * _node);
		bool load_from_xml_child_node(IO::XML::Node const * _node, tchar const * _childName);

	private:
		struct Rename // from code point of view, this is inconvenient when compared to map, but map will throw memory all over the place!
		{
			enum What
			{
				Anything,
				Prefix,
				Suffix,
			};
			Name renameFrom;
			Name renameTo;
			What what = Anything;

			Rename() {}
			Rename(Name const & _from, Name const & _to, What _what) : renameFrom(_from), renameTo(_to), what(_what) {}
		};
		Array<Rename> renames; // this allows renaming, and renaming couple of times (finger -> finger_1 -> lf_finger_1) but all bones have to be defined on the way
		
		// this example can be done with prefix and suffix too!
		Name usePrefix; // adds prefix to finding bone name - done after renaming
		Name useSuffix; // adds suffix to finding bone name - done after renaming
	};

};

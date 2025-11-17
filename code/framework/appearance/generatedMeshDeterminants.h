#pragma once

#include "..\..\core\containers\arrayStatic.h"
#include "..\..\core\other\simpleVariableStorage.h"
#include "..\..\core\types\name.h"

#include "..\library\usedLibraryStored.h"

//

namespace Framework
{
	class DoorInRoom;
	interface_class IModulesOwner;

	// has two usages
	//	1. what variables are determinants + their default values
	//	2. determinants for a specific object
	struct GeneratedMeshDeterminants
	{
		struct DeterminantValue
		{
			int32 value = 0; // can be used as any other 4 byte value
#ifdef AN_NAT_VIS
			void* natvis = nullptr; // this is for Name's nat vis
#endif
			bool operator == (DeterminantValue const& _other) const
			{
				return value == _other.value;
			}
		};
		struct Determinant
		{
			Name name;
			TypeId type;
			DeterminantValue value;
		};
		ArrayStatic<Determinant, 10> determinants;

		GeneratedMeshDeterminants()
		{
			SET_EXTRA_DEBUG_INFO(determinants, TXT("GeneratedMeshDeterminants.determinants"));
		}

		void clear();

		bool load_from_xml(IO::XML::Node const* _node);
		bool load_from_xml_child_node(IO::XML::Node const* _node, tchar const* _childName);

		void gather_for(IModulesOwner const* _imo, GeneratedMeshDeterminants const& _determinantsToGet);
		void gather_for(IModulesOwner const* _imo, SimpleVariableStorage const * _variables, GeneratedMeshDeterminants const& _determinantsToGet);
		void gather_for(DoorInRoom const* _doorInRoom, GeneratedMeshDeterminants const& _determinantsToGet);

		void apply_to(SimpleVariableStorage& _variables) const;

		bool operator==(GeneratedMeshDeterminants const& _other) const;

	private:
		void set(Name const& _name, TypeId _type, void const* _value);
	};
};
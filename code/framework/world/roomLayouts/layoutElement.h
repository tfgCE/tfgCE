#pragma once

#include "..\..\library\libraryStored.h"
#include "..\..\library\usedLibraryStoredOrTagged.h"

namespace Framework
{
	class Mesh;
	class ObjectLayout;
	struct LayoutElement;
	struct LayoutElementMeshInstance;

	struct LayoutElementCommonInfo
	{
		Name id;
		Name placeOnId; // placed on the last object with this id
		Name placeOnSocket; // target object
		Name placeUsingSocket; // own socket
	};

	struct LayoutElement
	: public LayoutElementCommonInfo
	{
		float chance = 1.0f;

		Transform placement = Transform::identity;
		Range3 offsetLocationRelative = Range3::empty;
		RangeRotator3 offsetRotation = RangeRotator3::empty;
		UsedLibraryStoredOrTagged<Mesh> mesh;

		bool load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc);
		bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

		// returns NP if can't be placed
		Optional<Transform> prepare_placement(Transform const& _mainPlacement, Array<LayoutElementMeshInstance> const& _alreadyPlacedMeshes, REF_ Random::Generator & _rg) const;
		Optional<Transform> alter_placement_with_place_using_socket(Transform const& _placement, Mesh const * _mesh) const;
		Optional<Transform> alter_placement_with_place_using_socket(Transform const& _placement, ObjectLayout const * _layout) const;
	};

	// all "includes" are changed into pure mesh instances
	struct LayoutElementMeshInstance
	: public LayoutElementCommonInfo
	{
		Transform placement = Transform::identity;
		Mesh* mesh = nullptr;
		Range3 spaceOccupied = Range3::empty; // not relative to placement but to the space
		int flags = 0; // any user flags that might be useful for particular thing

		void copy_ids_from(LayoutElementCommonInfo const& _src)
		{
			id = _src.id;
			placeOnId = _src.placeOnId;
			placeOnSocket = _src.placeOnSocket;
			placeUsingSocket = _src.placeUsingSocket;
		}
	};
};

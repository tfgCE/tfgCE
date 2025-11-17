#pragma once

#include "moduleMovement.h"

namespace Framework
{
	class ModuleMovementInteractivePartData;

	/**
	 *	Base for switches, dials
	 */
	class ModuleMovementInteractivePart
	: public ModuleMovement
	{
		FAST_CAST_DECLARE(ModuleMovementInteractivePart);
		FAST_CAST_BASE(ModuleMovement);
		FAST_CAST_END();

		typedef ModuleMovement base;
	public:
		ModuleMovementInteractivePart(IModulesOwner* _owner);

	protected:	// Module
		override_ void setup_with(ModuleData const * _moduleData);
		override_ void activate();

	protected:
		ModuleMovementInteractivePartData const * moduleMovementInteractivePartData = nullptr;

		SafePtr<IModulesOwner> baseObject;

		enum PlacementType
		{
			InWorldSpace,
			UsingOwnSocketRelativeToInitialPlacementWS,
			RelativeToInitialPlacementWS, // without socket
			RelativeToBaseObject, // not using any socket
			RelativeToBaseObjectUsingSocket,
			RelativeToBaseObjectUsingParentSocket, // when parent socket is used
			RelativeToAttachmentPlacement, // bone, socket (if attached, using relative placements, should be changed to RelativeToBaseObject)
		};
		PlacementType placementType;

		Optional<Transform> basePlacement; // this is in WS, always
		bool basePlacementValid = true;
		int relativeInitialPlacementAttachedToBoneIdx = NONE; // useful for relative
		int relativeInitialPlacementAttachedToSocketIdx = NONE;
		Transform relativeInitialPlacement = Transform::identity;
		Transform initialPlacementWS = Transform::identity;

		void update_base_object();
		void update_base_placement();

		virtual void mark_requires_movement(bool _bigChange = false) {}
	};

	class ModuleMovementInteractivePartData
	: public ModuleMovementData
	{
		FAST_CAST_DECLARE(ModuleMovementInteractivePartData);
		FAST_CAST_BASE(ModuleMovementData);
		FAST_CAST_END();
		typedef ModuleMovementData base;
	public:
		ModuleMovementInteractivePartData(LibraryStored* _inLibraryStored);

	public: // ModuleData
		override_ bool read_parameter_from(IO::XML::Attribute const * _attr, LibraryLoadingContext & _lc);
		override_ bool read_parameter_from(IO::XML::Node const * _node, LibraryLoadingContext & _lc);

	protected:

		friend class ModuleMovementInteractivePart;
	};
};


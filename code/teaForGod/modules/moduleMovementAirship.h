#pragma once

#include "..\..\framework\appearance\socketID.h"
#include "..\..\framework\module\moduleMovement.h"

namespace TeaForGodEmperor
{
	// actual airships don't really move!
	// this module is used only to store where the proxy should be
	class ModuleMovementAirship
	: public Framework::ModuleMovement
	{
		FAST_CAST_DECLARE(ModuleMovementAirship);
		FAST_CAST_BASE(Framework::ModuleMovement);
		FAST_CAST_END();

		typedef ModuleMovement base;
	public:
		static Framework::RegisteredModule<Framework::ModuleMovement> & register_itself();

		ModuleMovementAirship(Framework::IModulesOwner* _owner);
	
		void be_at(Name const& _zone, Optional<VectorInt2> const & _mapCell, Transform const& _placement);

		Name const& get_at_zone() const { return airshipZone; }
		Optional<VectorInt2> const& get_at_map_cell() const { return airshipMapCell; }
		Transform const& get_at_placement() const { return airshipPlacement; }

	public:
		Name airshipZone;
		Optional<VectorInt2> airshipMapCell;
		Transform airshipPlacement;
	};
};


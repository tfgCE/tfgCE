#include "moduleMovementAirshipProxy.h"

#include "moduleMovementAirship.h"

#include "..\pilgrimage\pilgrimageInstanceOpenWorld.h"

#include "..\..\framework\module\moduleAppearance.h"
#include "..\..\framework\module\modulePresence.h"
#include "..\..\framework\module\modules.h"
#include "..\..\framework\module\registeredModule.h"
#include "..\..\framework\modulesOwner\modulesOwner.h"
#include "..\..\framework\world\room.h"

#include "..\..\framework\advance\advanceContext.h"

#include "..\..\core\performance\performanceUtils.h"

using namespace TeaForGodEmperor;

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

DEFINE_STATIC_NAME(speed);
DEFINE_STATIC_NAME_STR(environmentAnchor, TXT("environment anchor"));

// rare advance for move
DEFINE_STATIC_NAME(raMoveAirshipProxy);

//

REGISTER_FOR_FAST_CAST(ModuleMovementAirshipProxy);

static Framework::ModuleMovement* create_module(Framework::IModulesOwner* _owner)
{
	return new ModuleMovementAirshipProxy(_owner);
}

Framework::RegisteredModule<Framework::ModuleMovement> & ModuleMovementAirshipProxy::register_itself()
{
	return Framework::Modules::movement.register_module(String(TXT("airship proxy")), create_module);
}

ModuleMovementAirshipProxy::ModuleMovementAirshipProxy(Framework::IModulesOwner* _owner)
: base(_owner)
{
	requiresPrepareMove = false;
}

void ModuleMovementAirshipProxy::set_movement_source(Framework::IModulesOwner* _source)
{
	movementSource = _source;
}

void ModuleMovementAirshipProxy::setup_with(Framework::ModuleData const * _moduleData)
{
	base::setup_with(_moduleData);
}

void ModuleMovementAirshipProxy::apply_changes_to_velocity_and_displacement(float _deltaTime, VelocityAndDisplacementContext & _context)
{
	{
		auto* inRoom = get_owner()->get_presence()->get_in_room();
		bool rareAdvanceNow = ! inRoom->was_recently_seen_by_player();
		if (rareAdvance != rareAdvanceNow)
		{
			rareAdvance = rareAdvanceNow;
			get_owner()->allow_rare_move_advance(rareAdvanceNow, NAME(raMoveAirshipProxy)); // every frame if in a recently seen room
		}
	}

	if (!inMapCell.is_set())
	{
		auto* inRoom = get_owner()->get_presence()->get_in_room();
		if (auto* piow = PilgrimageInstanceOpenWorld::get())
		{
			inMapCell = piow->find_cell_at(inRoom);
			cellSize = piow->get_cell_size();
		}
	}

	Transform const isAt = get_owner()->get_presence()->get_placement();
	Transform shouldBeAt = isAt;

	if (auto* msImo = movementSource.get())
	{
		if (auto* msMov = fast_cast<ModuleMovementAirship>(msImo->get_movement()))
		{
			Name zone = msMov->get_at_zone();
			Optional<VectorInt2> atMapCell = msMov->get_at_map_cell();
			Transform placement = msMov->get_at_placement();

			if (!anchorPlacement.is_set())
			{
				Name anchorPOI = zone.is_valid() ? zone : NAME(environmentAnchor);
				{
					Framework::PointOfInterestInstance* foundPOI = nullptr;
					if (get_owner()->get_presence()->get_in_room()->find_any_point_of_interest(anchorPOI, foundPOI))
					{
						anchorOwner = foundPOI->get_owner();
						anchorPlacement = foundPOI->calculate_placement();
						if (auto* imo = anchorOwner.get())
						{
							anchorRelativePlacement = imo->get_presence()->get_placement().to_local(anchorPlacement.get());
						}
					}
					else if (auto* ap = get_owner()->get_presence()->get_in_room()->get_variables().get_existing<Transform>(anchorPOI))
					{
						anchorPlacement = *ap;
					}
					else
					{
						warn(TXT("could not find POI or param/variable \"%S\" for airship proxy"), anchorPOI.to_char());
						anchorPlacement = get_owner()->get_presence()->get_in_room()->get_value<Transform>(anchorPOI, Transform::identity);
					}
				}
			}

			if (anchorRelativePlacement.is_set())
			{
				if (auto* imo = anchorOwner.get())
				{
					anchorPlacement = imo->get_presence()->get_placement().to_world(anchorRelativePlacement.get());
				}
			}

			if (! zone.is_valid() && atMapCell.is_set() && inMapCell.is_set())
			{
				VectorInt2 offsetBy = atMapCell.get() - inMapCell.get();
				placement.set_translation(placement.get_translation() + cellSize * offsetBy.to_vector2().to_vector3());
			}
			placement = anchorPlacement.get().to_world(placement);

			shouldBeAt = placement;
		}
	}

	// force displacements
	{
		_context.currentDisplacementLinear = shouldBeAt.get_translation() - isAt.get_translation();
		_context.currentDisplacementRotation = isAt.get_orientation().to_local(shouldBeAt.get_orientation());
		_context.velocityLinear = _deltaTime != 0.0f? _context.currentDisplacementLinear / _deltaTime : Vector3::zero;
		get_owner()->get_presence()->set_scale(shouldBeAt.get_scale());
	}

	// ignore base base::apply_changes_to_velocity_and_displacement(_deltaTime, _context);
}

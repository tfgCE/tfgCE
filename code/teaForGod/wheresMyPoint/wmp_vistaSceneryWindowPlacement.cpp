#include "wmp_vistaSceneryWindowPlacement.h"

#include "..\pilgrimage\pilgrimageInstanceOpenWorld.h"

#include "..\..\framework\world\room.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;

//

// region variables
DEFINE_STATIC_NAME(openWorldCellContext);

// POI name
DEFINE_STATIC_NAME(vistaWindow);
DEFINE_STATIC_NAME(vistaWindowPriority);

//

bool VistaSceneryWindowPlacement::update(REF_ WheresMyPoint::Value & _value, WheresMyPoint::Context & _context) const
{
	bool result = true;

	bool newValueProvided = false;

	auto& rg = _context.access_random_generator();

	WheresMyPoint::Value cellContextValue;
	if (!_context.get_owner()->restore_value_for_wheres_my_point(NAME(openWorldCellContext), cellContextValue))
	{
		warn_processing_wmp_tool(this, TXT("couldn't read open world cell context"));
	}
	else
	{
		if ( cellContextValue.get_type() != type_id<VectorInt2>())
		{
			error_processing_wmp_tool(this, TXT("couldn't read open world cell context - incorrect type"));
		}
		else
		{
			VectorInt2 cellCoord = cellContextValue.get_as<VectorInt2>();

			if (auto* piow = PilgrimageInstanceOpenWorld::get())
			{
				if (auto* room = piow->get_vista_scenery_room(cellCoord))
				{
					Array<Framework::PointOfInterestInstance*> pois;
					Optional<float> priority;
					pois.make_space_for(40);
					room->for_every_point_of_interest(NAME(vistaWindow), [&pois, &priority](Framework::PointOfInterestInstance* _fpoi)
					{
						float prio = _fpoi->get_parameters().get_value<float>(NAME(vistaWindowPriority), 0.0f);
						if (!priority.is_set() || prio > priority.get())
						{
							priority = prio;
							pois.clear();
						}
						if (prio >= priority.get())
						{
							pois.push_back(_fpoi);
						}
					});
					if (pois.is_empty())
					{
						warn_processing_wmp_tool(this, TXT("no available \"vistaWindow\" POIs"));
					}
					else
					{
						int chosenIdx = rg.get_int(pois.get_size());
						Transform poiPlacement = pois[chosenIdx]->calculate_placement();
						_value.access_as<Transform>() = poiPlacement;
						newValueProvided = true;
					}
				}
			}
		}
	}

	if (!newValueProvided)
	{
		if (_value.get_type() != type_id<Transform>())
		{
			error_processing_wmp_tool(this, TXT("no \"vistaWindow\" POI available for vista scenery window and existing value not transform, using random one"));
			float z = rg.get_float(5.0f, 30.0f);
			float yaw = rg.get_float(0.0f, 360.0f);
			_value.access_as<Transform>() = Transform(Vector3::zAxis * z, Rotator3(0.0f, yaw, 0.0f).to_quat());
		}
	}

	return result;
}

//
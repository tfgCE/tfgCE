#include "displayMap.h"

#include "..\..\framework\display\display.h"
#include "..\..\framework\display\displayDrawCommands.h"
#include "..\..\framework\library\library.h"
#include "..\..\framework\module\modulePresence.h"
#include "..\..\framework\object\object.h"
#include "..\..\framework\world\room.h"

using namespace TeaForGodEmperor;

//

// map icons
DEFINE_STATIC_NAME(mapIcon);

//

DisplayMap::DisplayMap()
{
}

void DisplayMap::add(Framework::IAIObject* _object, Framework::TexturePart* _tp, Optional<Colour> const & _colour)
{
	Object object;
	object.object = _object;
	object.tp = _tp;
	object.colour = _colour;
	objects.push_back(object);
}

void DisplayMap::update()
{
	objectsFromUpdate.clear();
	if (room)
	{
		for_every_ptr(object, room->get_objects())
		{
			if (auto* mapIcon = object->get_variables().get_existing<Framework::LibraryName>(NAME(mapIcon)))
			{
				ObjectFromUpdate ob;
				ob.object = object;
				ob.libraryName = *mapIcon;
				objectsFromUpdate.push_back(ob);
			}
		}
	}
}

void DisplayMap::draw(Framework::Display* _display, RangeInt2 const & _displayAt)
{
	displayAt = _displayAt;
	Range2 displayLimits;
	displayLimits.x.min = (float)displayAt.x.min + 1.0f;
	displayLimits.x.max = (float)displayAt.x.max - 1.0f;
	displayLimits.y.min = (float)displayAt.y.min + 1.0f;
	displayLimits.y.max = (float)displayAt.y.max - 1.0f;

	_display->add((new Framework::DisplayDrawCommands::ClearRegion())
		->rect(displayAt)
		->use_coordinates(Framework::DisplayCoordinates::Pixel)
		->immediate_draw());

	if (room)
	{
		todo_note(TXT("do not look for icons each frame, cache?"));

		mapRoom = Range2::empty;
		for_every(object, objectsFromUpdate)
		{
			if (Framework::IModulesOwner* imo = object->object.get())
			{
				if (imo->ai_get_room() == room)
				{
					mapRoom.include(imo->get_presence()->get_placement().get_translation().to_vector2());
				}
			}
		}

		for_every(object, objects)
		{
			if (Framework::IAIObject* ob = object->object.get())
			{
				if (ob->ai_get_room() == room)
				{
					mapRoom.include(ob->ai_get_placement().get_translation().to_vector2());
				}
			}
		}

		mapRoom.expand_by(mapRoom.length() * 0.4f);

		for_every(object, objectsFromUpdate)
		{
			if (Framework::IModulesOwner * imo = object->object.get())
			{
				if (imo->ai_get_room() == room)
				{
					if (auto* tp = ::Framework::Library::get_current()->get_texture_parts().find(object->libraryName))
					{
						Vector2 pt = mapRoom.get_pt_from_value(imo->get_presence()->get_placement().get_translation().to_vector2());
						Vector2 at = displayLimits.get_at(pt);
						_display->add((new Framework::DisplayDrawCommands::TexturePart())
							->use(tp)
							->at(at.to_vector_int2_cells())
							->use_colourise_ink(object->colour)
							->limit_to(displayLimits)
							->immediate_draw()
						);
					}
				}
			}
		}

		for_every(object, objects)
		{
			if (Framework::IAIObject* ob = object->object.get())
			{
				if (ob->ai_get_room() == room)
				{
					Vector2 pt = mapRoom.get_pt_from_value(ob->ai_get_placement().get_translation().to_vector2());
					Vector2 at = displayLimits.get_at(pt);
					_display->add((new Framework::DisplayDrawCommands::TexturePart())
						->use(object->tp)
						->at(at.to_vector_int2_cells())
						->use_colourise_ink(object->colour)
						->limit_to(displayLimits)
						->immediate_draw()
					);
				}
			}
		}
	}

}

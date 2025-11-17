#pragma once

namespace Framework
{
	template <class PathClass>
	void debug_draw_path(PathClass const & _path, Colour const & _colour, float _timeVisible = 1.0f)
	{
		if (!_path.is_active() ||
			!_path.get_owner_presence())
		{
			return;
		}
		Vector3 offsetOS = Vector3::zero;
		if (auto* p = _path.get_target_presence())
		{
			offsetOS = p->get_centre_of_presence_os().get_translation();
		}
		debug_context(_path.get_owner_presence()->get_in_room());
		debug_draw_time_based(_timeVisible, debug_draw_arrow(true, _colour, _path.get_owner_presence()->get_centre_of_presence_WS(), _path.get_placement_in_owner_room().location_to_world(offsetOS)));
		debug_no_context();
		debug_context(_path.get_in_final_room());
		debug_draw_time_based(_timeVisible, debug_draw_sphere(true, true, _colour, 0.2f, Sphere(_path.get_placement_in_final_room().location_to_world(offsetOS), 0.1f)));
		debug_no_context();
		Vector3 p;
		Framework::Room* r = _path.get_owner_presence()->get_in_room();
		for_every_ref(t, _path.get_through_doors())
		{
			debug_context(r);
			debug_draw_time_based(_timeVisible, debug_draw_line(true, _colour.with_alpha(0.5f), p, t->get_hole_centre_WS()));
			debug_no_context();
			if (!t->get_door_on_other_side())
			{
				break;
			}
			r = t->get_room_on_other_side();
			p = t->get_door_on_other_side()->get_hole_centre_WS();
		}
		debug_context(r);
		debug_draw_time_based(_timeVisible, debug_draw_line(true, _colour.with_alpha(0.5f), p, _path.get_placement_in_final_room().location_to_world(offsetOS)));
		debug_no_context();
	}
};

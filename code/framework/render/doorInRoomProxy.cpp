#include "doorInRoomProxy.h"

#include "..\world\doorInRoom.h"

#include "..\..\core\globalSettings.h"
#include "..\..\core\system\input.h"

#include "..\game\game.h"

#include "roomProxy.h"
#include "renderContext.h"
#include "renderScene.h"
#include "renderSystem.h"
#include "sceneBuildContext.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace Framework::Render;

//

DoorInRoomProxy* DoorInRoomProxy::build(SceneBuildContext & _context, DoorInRoom const * _doorInRoom, DoorInRoomProxy *& _addTolist, bool _cameThrough)
{
	if (Door const * door = _doorInRoom->get_door())
	{
		if (_doorInRoom->mesh.is_empty())
		{
			return nullptr;
		}

		if (_doorInRoom->meshesReady)
		{
			auto checkPlacement = _context.get_view_matrix().to_world(_doorInRoom->get_scaled_placement_matrix());
			if (!_context.get_frustum()->is_visible(checkPlacement, _doorInRoom->get_precise_collision().get_bounding_box()) ||
				!_context.get_planes()->is_visible(checkPlacement, _doorInRoom->get_precise_collision().get_bounding_box()))
			{
				return nullptr;
			}
		}

#ifdef AN_DEVELOPMENT
#ifdef AN_STANDARD_INPUT
		if (::System::Input::get()->has_key_been_pressed(::System::Key::I))
		{
			output_colour(0, 1, 1, 1);
			output(TXT("    %S /%S (%i)"), _doorInRoom->get_door()->get_door_type()->get_name().to_string().to_char(), _doorInRoom->get_door()->get_secondary_door_type()? _doorInRoom->get_door()->get_secondary_door_type()->get_name().to_string().to_char() : TXT("--"), ++_context.doorInRoomProxyCount);
			output_colour();
		}
#endif
#endif

		DoorInRoomProxy* doorInRoom = DoorInRoomProxy::get_one();

		doorInRoom->cameThrough = _cameThrough;
		doorInRoom->placementInRoomMat = _doorInRoom->get_scaled_placement_matrix();
		doorInRoom->appearanceCopy.hard_copy_from(_doorInRoom->mesh);
#ifdef RENDER_SCENE_WITH_ON_PREPARE_TO_RENDER
		if (_context.get_scene()->get_on_prepare_to_render())
		{
			for_every(meshInstance, doorInRoom->appearanceCopy.access_instances())
			{
				_context.get_scene()->get_on_prepare_to_render()(REF_ * meshInstance, _doorInRoom->get_in_room(), _doorInRoom->get_placement());
			}
		}
#endif
		doorInRoom->appearanceCopy.prepare_to_render();

		// add to list
		doorInRoom->next = _addTolist;
		_addTolist = doorInRoom;

		if (auto setup_door_in_room_proxy = Game::get()->get_customisation().render.setup_door_in_room_proxy)
		{
			setup_door_in_room_proxy(_context, doorInRoom, _doorInRoom);
		}

		_context.for_additional_render_of(_doorInRoom->get_in_room(), _doorInRoom, [&_addTolist, _cameThrough, _doorInRoom](::System::MaterialInstance const* _materialOverride)
			{
				DoorInRoomProxy* addDoorInRoom = DoorInRoomProxy::get_one();

				addDoorInRoom->cameThrough = _cameThrough;
				addDoorInRoom->placementInRoomMat = _doorInRoom->get_scaled_placement_matrix();
				addDoorInRoom->appearanceCopy.hard_copy_from(_doorInRoom->mesh, _materialOverride);
#ifdef RENDER_SCENE_WITH_ON_PREPARE_TO_RENDER
				if (_context.get_scene()->get_on_prepare_to_render())
				{
					for_every(meshInstance, addDoorInRoom->appearanceCopy.access_instances())
					{
						_context.get_scene()->get_on_prepare_to_render()(REF_ * meshInstance, _doorInRoom->get_in_room(), _doorInRoom->get_placement());
					}
			}
#endif
				addDoorInRoom->appearanceCopy.prepare_to_render();
				addDoorInRoom->next = _addTolist;
				_addTolist = addDoorInRoom;
			});

		return doorInRoom;
	}
	else
	{
		return nullptr;
	}
}

void DoorInRoomProxy::on_release()
{
	if (next)
	{
		next->release();
		next = nullptr;
	}
}

void DoorInRoomProxy::add_all_to(SceneBuildContext & _context, REF_ Meshes::Mesh3DRenderingBufferSet& _renderingBuffers) const
{
	DoorInRoomProxy const * doorInRoom = this;
	while (doorInRoom)
	{
#ifdef AN_RENDERER_STATS
		Stats::get().renderedDoorInRoomProxies.increase();
		_context.access_stats().renderedDoorInRoomProxies.increase();
#endif

		if (Render::System::get()->setup_shader_program_on_bound_for_door_in_room)
		{
			doorInRoom->appearanceCopy.add_to(_renderingBuffers, doorInRoom->placementInRoomMat,
				[doorInRoom](Matrix44 const& _renderingBufferModelViewMatrixForRendering, ::Meshes::Mesh3DRenderingContext& _renderingContext)
				{
					an_assert(!_renderingContext.setup_shader_program_on_bound);

					Matrix44 renderingBufferModelViewMatrixForRendering = _renderingBufferModelViewMatrixForRendering;

					_renderingContext.setup_shader_program_on_bound = [renderingBufferModelViewMatrixForRendering, doorInRoom](::System::ShaderProgram* _shaderProgram)
					{
						if (Render::System::get()->setup_shader_program_on_bound_for_door_in_room)
						{
							Render::System::get()->setup_shader_program_on_bound_for_door_in_room(_shaderProgram, renderingBufferModelViewMatrixForRendering, doorInRoom);
						}
					};
				});
		}
		else
		{
			doorInRoom->appearanceCopy.add_to(_renderingBuffers, doorInRoom->placementInRoomMat,
				nullptr);
		}

		// next
		doorInRoom = doorInRoom->next;
	}
}

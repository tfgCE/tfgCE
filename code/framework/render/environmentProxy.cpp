#include "environmentProxy.h"

#include "..\..\core\globalSettings.h"

#include "sceneBuildContext.h"
#include "presenceLinkProxy.h"
#include "renderContext.h"
#include "renderSystem.h"
#include "roomProxy.h"

#include "..\game\game.h"

#include "..\library\usedLibraryStored.inl"

#include "..\modulesOwner\modulesOwnerImpl.inl"

#include "..\world\region.h"
#include "..\world\room.h"
#include "..\world\doorInRoom.h"
#include "..\world\presenceLink.h"
#include "..\world\environment.h"
#include "..\world\environmentOverlay.h"

#include "..\video\texture.h"

#include "..\..\core\debug\debugRenderer.h"
#include "..\..\core\system\video\shaderProgramBindingContext.h"
#include "..\..\core\mesh\combinedMesh3dInstanceSet.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#ifdef AN_DEVELOPMENT_OR_PROFILER
//#define DRAW_ENVIRONMENT_PLACEMENT
#endif

//

using namespace Framework;
using namespace Framework::Render;

//

EnvironmentProxy* EnvironmentProxy::build(SceneBuildContext & _context, RoomProxy* _forRoomProxy, RoomProxy * _fromRoomProxy, DoorInRoom const * _throughDoorFromOuter)
{
	if (!_forRoomProxy)
	{
		return nullptr;
	}
	Room* _forRoom = _forRoomProxy->room;

	Environment* environment = _forRoom->get_environment();

	if (!environment)
	{
		return nullptr;
	}

	int activationLevel = 0;
	if (_throughDoorFromOuter)
	{
		if (Room * fromRoom = _throughDoorFromOuter->get_in_room())
		{
			if (fromRoom->get_environment() == _forRoom->get_environment())
			{
				activationLevel = fromRoom->get_environment_usage().get_activation_level() + 1;
			}
		}
	}
	Transform environmentPlacement = _forRoom->access_environment_usage().activate(activationLevel);

	EnvironmentProxy* environmentProxy = get_one();
	environmentProxy->forRoomProxy = _forRoomProxy;
	environmentProxy->forRoom = _forRoom;
	environmentProxy->environment = environment;
	environmentProxy->environmentMatrixFromRoomProxy = environmentPlacement.to_matrix();

#ifdef DRAW_ENVIRONMENT_PLACEMENT
	debug_context(_forRoom);
	debug_draw_transform_coloured(true, Transform::identity, Colour::grey, Colour::grey, Colour::grey);
	debug_draw_transform_size(true, environmentPlacement, 0.5f);
	debug_no_context();
#endif

	if (environment->get_info().is_location_fixed_against_camera())
	{
		Matrix44 const currentThroughDoorMatrix = _context.get_through_door_matrix();
		environmentProxy->environmentMatrixFromRoomProxy.set_translation(currentThroughDoorMatrix.location_to_local(_context.get_camera().get_placement().get_translation()));
	}
	else if (environment->get_info().does_propagate_placement_between_rooms())
	{
		if (_fromRoomProxy && _throughDoorFromOuter && !_throughDoorFromOuter->get_door()->does_block_environment_propagation())
		{
			if (EnvironmentProxy* fromEnvironmentProxy = _fromRoomProxy->get_environment())
			{
				if (fromEnvironmentProxy->environment == environment)
				{
					environmentProxy->environmentMatrixFromRoomProxy = _throughDoorFromOuter->get_other_room_matrix().to_local(fromEnvironmentProxy->environmentMatrixFromRoomProxy);
				}
			}
		}
	}

	// setup appearance set for room (copy existing)
	if (!_context.get_build_request().noEnvironmentMeshes)
	{
		environmentProxy->appearanceCopy.hard_copy_from(environment->get_appearance_for_rendering());
		environmentProxy->appearanceCopy.prepare_to_render();
	}
	else
	{
		environmentProxy->appearanceCopy.clear();
	}

	// params!
	environmentProxy->params.clear();

	// fill with environments (sets non existent)
	{
		Environment const * goUp = environment;
		while (goUp)
		{
			environmentProxy->params.fill_with(goUp->get_info().get_params());
			goUp = goUp->parent;
		}

		// fill all missing bits!
		if (auto* et = ::Render::System::get_default_fallback_environment_type())
		{
			environmentProxy->params.fill_with(et->get_info().get_params());
		}
	}

	// overlays (set/lerp/add) from region ladder
	{
		ARRAY_STACK(EnvironmentOverlay const*, regionOverlays, 8);
		auto* r = _forRoom->get_in_region();
		while (r)
		{
			for_every_ref(eo, r->get_environment_overlays())
			{
				regionOverlays.push_back(eo);
			}
			r = r->get_in_region();
		}

		for_every_reverse_ptr(eo, regionOverlays)
		{
			eo->apply_to(*environmentProxy, _forRoom);
		}
	}

	// overlays (set/lerp/add)
	for_every_ref(eo, _forRoom->get_environment_overlays())
	{
		eo->apply_to(*environmentProxy, _forRoom);
	}

	// add objects
	an_assert(environmentProxy->presenceLinks == nullptr);
	environmentProxy->presenceLinks = PresenceLinkProxy::build_all_in_room(_context, _fromRoomProxy, environment->presenceLinks, _throughDoorFromOuter);

	if (auto setup_environment_proxy = Game::get()->get_customisation().render.setup_environment_proxy)
	{
		setup_environment_proxy(_context, environmentProxy, _fromRoomProxy? _fromRoomProxy->get_environment() : nullptr, _forRoom);
	}

	return environmentProxy;
}

void EnvironmentProxy::setup_shader_binding_context(::System::ShaderProgramBindingContext * _bindingContext, ::System::VideoMatrixStack const & _matrixStack) const
{
	// get actual model view as it is room proxy's matrix, then apply environment from room proxy
	Matrix44 modelViewMatrix = _matrixStack.get_current().to_world(environmentMatrixFromRoomProxy);
	_matrixStack.ready_for_rendering(REF_ modelViewMatrix);
	params.setup_shader_binding_context(_bindingContext, modelViewMatrix);
}

void EnvironmentProxy::setup_shader_binding_context_no_proxy(::System::ShaderProgramBindingContext * _bindingContext, ::System::VideoMatrixStack const & _matrixStack)
{
}

void EnvironmentProxy::on_release()
{
	params.clear();
	if (next)
	{
		next->release();
		next = nullptr;
	}
	if (presenceLinks)
	{
		presenceLinks->release();
		presenceLinks = nullptr;
	}
}

void EnvironmentProxy::add_to(SceneBuildContext & _context, REF_ Meshes::Mesh3DRenderingBufferSet & _renderingBuffers) const
{
#ifdef AN_RENDERER_STATS
	Stats::get().renderedEnvironments.increase();
	_context.access_stats().renderedEnvironments.increase();
#endif

	appearanceCopy.add_to(_renderingBuffers, environmentMatrixFromRoomProxy);

	if (presenceLinks)
	{
		presenceLinks->add_all_to(_context, _renderingBuffers, environmentMatrixFromRoomProxy);
	}
}

void EnvironmentProxy::log(LogInfoContext& _log) const
{
	if (environment)
	{
		_log.log(TXT("environment \"%S\""), environment->get_name().to_char());
	}
	else
	{
		_log.log(TXT("no environment?"));
	}
	params.log(_log);
}

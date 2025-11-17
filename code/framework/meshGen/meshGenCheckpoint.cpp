#include "meshGenCheckpoint.h"

#include "meshGenGenerationContext.h"
#include "meshGenUtils.h"

#include "..\..\core\mesh\mesh3dPart.h"
#include "..\..\core\mesh\socketDefinitionSet.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace MeshGeneration;

//

Checkpoint::Checkpoint()
{
}

Checkpoint::Checkpoint(GenerationContext const & _context)
{
	make(_context);
}

void Checkpoint::make(GenerationContext const & _context)
{
	partsSoFarCount = _context.get_parts().get_size();
	movementCollisionPartsSoFarCount = _context.get_movement_collision_parts().get_size();
	preciseCollisionPartsSoFarCount = _context.get_precise_collision_parts().get_size();
	socketsGenerationIdSoFar = _context.get_current_sockets_generation_id();
	meshNodesSoFarCount = _context.get_mesh_nodes().get_size();
	poisSoFarCount = _context.get_pois().get_size();
	spaceBlockersSoFarCount = _context.get_space_blockers().get_size();
	bonesSoFarCount = _context.get_generated_bones().get_size();
	appearanceControllersSoFarCount = _context.get_appearance_controllers().get_size();
}

bool Checkpoint::operator ==(Checkpoint const & _other) const
{
	return
		partsSoFarCount == _other.partsSoFarCount &&
		movementCollisionPartsSoFarCount == _other.movementCollisionPartsSoFarCount &&
		preciseCollisionPartsSoFarCount == _other.preciseCollisionPartsSoFarCount &&
		socketsGenerationIdSoFar == _other.socketsGenerationIdSoFar &&
		meshNodesSoFarCount == _other.meshNodesSoFarCount &&
		poisSoFarCount == _other.poisSoFarCount &&
		spaceBlockersSoFarCount == _other.spaceBlockersSoFarCount && 
		bonesSoFarCount == _other.bonesSoFarCount &&
		appearanceControllersSoFarCount == _other.appearanceControllersSoFarCount;

}

void Checkpoint::apply_for_so_far(GenerationContext& _context, Transform const& _placement, Vector3 const& _scale) const
{
	if (_context.get_parts().is_index_valid(partsSoFarCount))
	{
		RefCountObjectPtr<::Meshes::Mesh3DPart> const * part = &_context.get_parts()[partsSoFarCount];
		for (int idx = partsSoFarCount; idx < _context.get_parts().get_size(); ++idx, ++part)
		{
			ApplyTo applyTo((*part)->access_vertex_data(), (*part)->get_number_of_vertices(), &(*part)->get_vertex_format());
			apply_scale(applyTo, _context, _scale);
			apply_placement(applyTo, _context, _placement);
		}
	}
	if (_context.get_movement_collision_parts().is_index_valid(movementCollisionPartsSoFarCount))
	{
		::Collision::Model* const* part = &_context.get_movement_collision_parts()[movementCollisionPartsSoFarCount];
		for (int idx = movementCollisionPartsSoFarCount; idx < _context.get_movement_collision_parts().get_size(); ++idx, ++part)
		{
			ApplyTo applyTo(*part);
			apply_scale(applyTo, _context, _scale);
			apply_placement(applyTo, _context, _placement);
		}
	}
	if (_context.get_precise_collision_parts().is_index_valid(preciseCollisionPartsSoFarCount))
	{
		::Collision::Model* const* part = &_context.get_precise_collision_parts()[preciseCollisionPartsSoFarCount];
		for (int idx = preciseCollisionPartsSoFarCount; idx < _context.get_precise_collision_parts().get_size(); ++idx, ++part)
		{
			ApplyTo applyTo(*part);
			apply_scale(applyTo, _context, _scale);
			apply_placement(applyTo, _context, _placement);
		}
	}
	if (socketsGenerationIdSoFar != _context.get_current_sockets_generation_id())
	{
		for_every(socket, _context.access_sockets().access_sockets())
		{
			if (socket->get_generation_id() > socketsGenerationIdSoFar)
			{
				ApplyTo applyTo(socket);
				apply_scale(applyTo, _context, _scale);
				apply_placement(applyTo, _context, _placement);
			}
		}
	}
	if (_context.get_mesh_nodes().is_index_valid(meshNodesSoFarCount))
	{
		MeshNodePtr* meshNode = &_context.access_mesh_nodes()[meshNodesSoFarCount];
		for (int idx = meshNodesSoFarCount; idx < _context.get_mesh_nodes().get_size(); ++idx, ++meshNode)
		{
			ApplyTo applyTo(meshNode->get());
			apply_scale(applyTo, _context, _scale);
			apply_placement(applyTo, _context, _placement);
		}
	}
	if (_context.get_pois().is_index_valid(poisSoFarCount))
	{
		PointOfInterestPtr* poi = &_context.access_pois()[poisSoFarCount];
		for (int idx = poisSoFarCount; idx < _context.get_pois().get_size(); ++idx, ++poi)
		{
			ApplyTo applyTo(poi->get());
			apply_scale(applyTo, _context, _scale);
			apply_placement(applyTo, _context, _placement);
		}
	}
	if (_context.get_space_blockers().blockers.is_index_valid(spaceBlockersSoFarCount))
	{
		SpaceBlocker* sb = &_context.access_space_blockers().blockers[spaceBlockersSoFarCount];
		for (int idx = spaceBlockersSoFarCount; idx < _context.access_space_blockers().blockers.get_size(); ++idx, ++sb)
		{
			ApplyTo applyTo(sb);
			apply_scale(applyTo, _context, _scale);
			apply_placement(applyTo, _context, _placement);
		}
	}
	if (_context.get_generated_bones().is_index_valid(bonesSoFarCount))
	{
		auto* bone = &_context.access_generated_bones()[bonesSoFarCount];
		for (int idx = bonesSoFarCount; idx < _context.get_generated_bones().get_size(); ++idx, ++bone)
		{
			ApplyTo applyTo(*bone);
			apply_scale(applyTo, _context, _scale);
			apply_placement(applyTo, _context, _placement);
		}
	}
	if (_context.get_appearance_controllers().is_index_valid(appearanceControllersSoFarCount))
	{
		todo_note(TXT("keep them as they are?"));
	}
}

bool Checkpoint::generate_with_checkpoint(GenerationContext& _context, ElementInstance& _parentInstance, int _idx, Element const* _element, SimpleVariableStorage const * _parameters, Transform const& _placement, Vector3 const& _scale)
{
	bool result = true;

	Checkpoint checkpoint(_context);
	_context.push_stack();
	_context.push_checkpoint(checkpoint);
	if (_parameters)
	{
		_context.set_parameters(*_parameters);
	}
	result &= _context.process(_element, &_parentInstance, _idx);
	_context.pop_checkpoint();
	_context.pop_stack();

	checkpoint.apply_for_so_far(_context, _placement, _scale);

	return result;
}

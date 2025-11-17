#pragma once

#include "..\..\core\math\math.h"

class SimpleVariableStorage;

namespace Framework
{
	namespace MeshGeneration
	{
		class Element;
		struct ElementInstance;
		struct GenerationContext;

		struct Checkpoint
		{
			Checkpoint();
			Checkpoint(GenerationContext const & _context);

			void make(GenerationContext const & _context);

			bool operator ==(Checkpoint const & _other) const;
			bool operator !=(Checkpoint const & _other) const { return !operator==(_other); }

			int partsSoFarCount = 0;
			int movementCollisionPartsSoFarCount = 0;
			int preciseCollisionPartsSoFarCount = 0;
			int socketsGenerationIdSoFar = 0;
			int meshNodesSoFarCount = 0;
			int poisSoFarCount = 0;
			int spaceBlockersSoFarCount = 0;
			int bonesSoFarCount = 0;
			int appearanceControllersSoFarCount = 0;

			void apply_for_so_far(GenerationContext& _context, Transform const& _placement, Vector3 const& _scale = Vector3::one) const;

			static bool generate_with_checkpoint(GenerationContext& _context, ElementInstance& _parentInstance, int _idx, Element const* _element, SimpleVariableStorage const* _parameters, Transform const& _placement, Vector3 const& _scale = Vector3::one);
		};

		struct NamedCheckpoint
		{
			Name id;
			Checkpoint start; // if not set, will always start from the beginning
			Optional<Checkpoint> end;
		};
	};
};

#pragma once

#include "..\..\..\core\types\optional.h"

struct Plane;

namespace Meshes
{
	struct Mesh3DPart;
};

namespace Framework
{
	namespace MeshGeneration
	{
		struct GenerationContext;
		struct Checkpoint;

		namespace Modifiers
		{
			namespace Utils
			{
				enum Flags
				{
					Mesh					= bit(0),
					MovementCollision		= bit(1),
					PreciseCollision		= bit(2),
					Sockets					= bit(3),
					MeshNodes				= bit(4),
					POIs					= bit(5),
					SpaceBlockers			= bit(6),
					Bones					= bit(7),
					AppearanceControllers	= bit(8),

					All = 0xffff
				};

				inline float mesh_threshold() { return 0.001f; }
				
				bool cut_using_plane(GenerationContext& _context, ::Meshes::Mesh3DPart* _part, Plane const& _plane);
				bool cut_using_plane(GenerationContext& _context, Checkpoint const& checkpoint, REF_ Checkpoint& now, Plane const& _plane, Optional<int> const& _flags = NP, Optional<bool> const& _dontCutMeshNodes = NP);

				bool create_copy(GenerationContext& _context, Checkpoint const& checkpoint, Checkpoint const& now, Optional<int> const& _flags = NP);
			};
		};
	};
};

#pragma once

#include "..\..\core\containers\arrayStack.h"
#include "..\..\core\pieceCombiner\pieceCombiner.h"

struct Range2;

namespace Random
{
	class Generator;
};

namespace Framework
{
	class RegionGenerator;
	class Room;
};

namespace TeaForGodEmperor
{
	struct RoomGenerationInfo;

	namespace RoomGeneratorUtils
	{
		void alter_environments_light_for(Framework::Room* _room);

		void gather_outer_connector_tags(OUT_ Array<Name> & _connectorTags, PieceCombiner::Generator<Framework::RegionGenerator>& _generator, Array<Name> const * _ignoreConnectorTags = nullptr);

		void gather_door_connector_tags(OUT_ Array<Name> & _connectorTags, Framework::Room const * _forRoom, Array<Name> const * _ignoreConnectorTags = nullptr);

		Transform get_placement_for_required_vr_space(Framework::Room* _room, Range2 const& _vrSpaceTaken /* relative to centre */, RoomGenerationInfo& roomGenerationInfo, Random::Generator& randomGenerator, bool alignToTiles = false);
	};
};

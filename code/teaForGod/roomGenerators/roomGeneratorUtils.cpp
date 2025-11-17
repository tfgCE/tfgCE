#include "roomGeneratorUtils.h"

#include "roomGenerationInfo.h"

#include "..\..\core\pieceCombiner\pieceCombinerImplementation.h"
#include "..\..\core\random\randomUtils.h"

#include "..\..\framework\world\doorInRoom.h"
#include "..\..\framework\world\environment.h"
#include "..\..\framework\world\room.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;

//

DEFINE_STATIC_NAME(lightDir);

// room vars
DEFINE_STATIC_NAME(maxPlayAreaSize);
DEFINE_STATIC_NAME(maxPlayAreaTileSize);

//

void RoomGeneratorUtils::alter_environments_light_for(Framework::Room* _room)
{
	return; // this is disabled for now until we've got shadowmaps (as there's no neeed to do that if we don't have shadowmaps)

	todo_important(TXT("this should actually create copy of param, although I think that the idea of specific lights is dropped ATM"));

	// alter environment's light to not fall through any door
	if (auto * environment = _room->get_environment())
	{
		if (auto * lightDirParam = environment->access_param(NAME(lightDir)))
		{
			float const threshold = 10.0f;

			Vector3 & lightDir = lightDirParam->as<Vector3>();
			Array<Range> prohibited;
			for_every_ptr(door, _room->get_all_doors())
			{
				float prohibitedYaw = mod(door->get_placement().get_axis(Axis::Y).to_rotator().yaw, 360.0f);
				Range prohibitedRange(prohibitedYaw - 90.0f + threshold, prohibitedYaw + 90.0f - threshold);
				if (prohibitedRange.min < 0.0f)
				{
					prohibited.push_back(Range(360.0f + prohibitedRange.min, 360.0f));
					prohibitedRange.min = 0.0f;
				}
				if (prohibitedRange.max > 360.0f)
				{
					prohibited.push_back(Range(prohibitedRange.max - 360.0f, 360.0f));
					prohibitedRange.max = 360.0f;
				}
				prohibited.push_back(prohibitedRange);
			}
			Array<Range> allowed;
			float currentYaw = 0.0f;
			while (currentYaw < 360.0f)
			{
				float nextYaw = currentYaw + 360.0f;
				for_every(proh, prohibited)
				{
					if (proh->min != proh->max &&
						proh->min >= currentYaw)
					{
						nextYaw = min(nextYaw, proh->min);
					}
				}
				nextYaw = min(360.0f, nextYaw);
				currentYaw = max(0.0f, currentYaw);
				if (nextYaw - currentYaw > 0.0f)
				{
					allowed.push_back(Range(currentYaw, nextYaw));
				}
				float nextAllowedYaw = nextYaw;
				for_every(proh, prohibited)
				{
					if (proh->min != proh->max &&
						proh->min == nextYaw)
					{
						nextAllowedYaw = max(nextAllowedYaw, proh->max);
					}
				}
				currentYaw = nextAllowedYaw;
			}
			Random::Generator rg = _room->get_individual_random_generator();
			rg.advance_seed(5792489, 2480234);
			if (allowed.is_empty())
			{
				lightDir.x = rg.get_float(-0.05f, 0.05f);
				lightDir.y = rg.get_float(-0.05f, 0.05f);
				lightDir.z = 1.0f;
			}
			else
			{
				int idx = RandomUtils::ChooseFromContainer<Array<Range>, Range>::choose(rg, allowed, [](Range const & _range) { return _range.length(); });
				float yaw = rg.get(allowed[idx]);
				Rotator3 rot(0.0f, yaw, 0.0f);
				Vector3 vec = rot.get_forward();
				float xy = sqrt(max(0.0f, 1.0f - sqr(lightDir.z)));
				lightDir.x = vec.x * xy;
				lightDir.y = vec.y * xy;
			}
			// for time being just turn it randomly
			lightDir.normalise();
		}
	}
}

//

void RoomGeneratorUtils::gather_outer_connector_tags(OUT_ Array<Name> & _connectorTags, PieceCombiner::Generator<Framework::RegionGenerator>& _generator, Array<Name> const* _ignoreConnectorTags)
{
	for_count(int, ic, _generator.get_outer_connector_count())
	{
		// this is called before region is generated, so we use what was provided to outer connector definition/setup
		if (auto* connector = _generator.get_outer_connector(ic))
		{
			auto const& connectorTag = connector->inheritedConnectorTag;
			if (connectorTag.is_set() &&
				(!_ignoreConnectorTags || !_ignoreConnectorTags->does_contain(connectorTag.get())))
			{
				_connectorTags.push_back(connectorTag.get());
			}
		}
	}
}

void RoomGeneratorUtils::gather_door_connector_tags(OUT_ Array<Name>& _connectorTags, Framework::Room const* _forRoom, Array<Name> const* _ignoreConnectorTags)
{
	for_every_ptr(door, _forRoom->get_all_doors())
	{
		// this is called before region is generated, so we use what was provided to outer connector definition/setup
		auto const& connectorTag = door->get_connector_tag();
		if (connectorTag.is_valid() &&
			(!_ignoreConnectorTags || !_ignoreConnectorTags->does_contain(connectorTag)))
		{
			_connectorTags.push_back(connectorTag);
		}
	}
}

Transform RoomGeneratorUtils::get_placement_for_required_vr_space(Framework::Room* _room, Range2 const& _vrSpaceTaken, RoomGenerationInfo & roomGenerationInfo, Random::Generator & randomGenerator, bool alignToTiles)
{
	Transform availableSpaceCentreVRSpacePlacement = Transform::identity;
	Range2 vrSpaceTaken = _vrSpaceTaken;

	bool rotate = false;
	Vector2 playAreaRectSize = roomGenerationInfo.get_play_area_rect_size();
	{
		float tileSize = roomGenerationInfo.get_tile_size();

		Optional<Vector2> maxPlayAreaSize;
		if (auto* maxPlayAreaTileSizePtr = _room->get_variable<Vector2>(NAME(maxPlayAreaTileSize)))
		{
			Vector2 maxPlayAreaTileSize = *maxPlayAreaTileSizePtr;
			Vector2 maxPlayAreaSizeFromTiles = maxPlayAreaTileSize * tileSize;
			Vector2 currMaxPlayAreaSize = maxPlayAreaSize.get(maxPlayAreaSizeFromTiles);

			currMaxPlayAreaSize.x = min(currMaxPlayAreaSize.x, maxPlayAreaSizeFromTiles.x);
			currMaxPlayAreaSize.y = min(currMaxPlayAreaSize.y, maxPlayAreaSizeFromTiles.y);

			maxPlayAreaSize = currMaxPlayAreaSize;
		}
		if (auto* maxPlayAreaSizePtr = _room->get_variable<Vector2>(NAME(maxPlayAreaSize)))
		{
			Vector2 currMaxPlayAreaSize = maxPlayAreaSize.get(*maxPlayAreaSizePtr);

			currMaxPlayAreaSize.x = min(currMaxPlayAreaSize.x, (*maxPlayAreaSizePtr).x);
			currMaxPlayAreaSize.y = min(currMaxPlayAreaSize.y, (*maxPlayAreaSizePtr).y);

			maxPlayAreaSize = currMaxPlayAreaSize;
		}
		if (maxPlayAreaSize.is_set())
		{
			Vector2 vrSpaceTakenSize = vrSpaceTaken.length();
			maxPlayAreaSize.access().x = max(maxPlayAreaSize.get().x, vrSpaceTakenSize.x);
			maxPlayAreaSize.access().y = max(maxPlayAreaSize.get().y, vrSpaceTakenSize.y);

			playAreaRectSize.x = min(playAreaRectSize.x, maxPlayAreaSize.get().x);
			playAreaRectSize.y = min(playAreaRectSize.y, maxPlayAreaSize.get().y);
		}
	}
	{
		Vector2 vrSpaceTakenSize = vrSpaceTaken.length();
		// rotate on random if would fit rotated
		if (vrSpaceTakenSize.y <= playAreaRectSize.x + 0.01f &&
			vrSpaceTakenSize.x <= playAreaRectSize.y + 0.01f)
		{
			rotate = randomGenerator.get_bool();
		}
		// if wouldn't fit, force rotate - check if rotated isn't worse (sometimes we might be slightly off (hence -0.03)
		float diffX = vrSpaceTakenSize.x - (playAreaRectSize.x - 0.03f);
		float diffY = vrSpaceTakenSize.y - (playAreaRectSize.y - 0.03f);
		float diffRotX = vrSpaceTakenSize.x - (playAreaRectSize.y + 0.01f);
		float diffRotY = vrSpaceTakenSize.y - (playAreaRectSize.x + 0.01f);
		if ((diffX > 0.0f || diffY > 0.0f) && // need to rotate
			(diffRotX < diffX && diffRotY < diffY)) // would fit rotated better than is now
		{
			rotate = true;
		}
	}
	if (rotate)
	{
		bool right = randomGenerator.get_bool();
		if (right)
		{
			Range2 prevVRSpaceTaken = vrSpaceTaken;
			vrSpaceTaken.x = prevVRSpaceTaken.y;
			vrSpaceTaken.y.min = -prevVRSpaceTaken.x.max;
			vrSpaceTaken.y.max = -prevVRSpaceTaken.x.min;
		}
		else
		{
			Range2 prevVRSpaceTaken = vrSpaceTaken;
			vrSpaceTaken.x.min = -prevVRSpaceTaken.y.max;
			vrSpaceTaken.x.max = -prevVRSpaceTaken.y.min;
			vrSpaceTaken.y = prevVRSpaceTaken.x;
		}
		availableSpaceCentreVRSpacePlacement.set_orientation(Rotator3(0.0f, right ? 90.0f : -90.0f, 0.0f).to_quat());
	}
	else
	{
		bool do180 = randomGenerator.get_bool();
		if (do180)
		{
			Range2 prevVRSpaceTaken = vrSpaceTaken;
			vrSpaceTaken.x.min = -prevVRSpaceTaken.x.max;
			vrSpaceTaken.x.max = -prevVRSpaceTaken.x.min;
			vrSpaceTaken.y.min = -prevVRSpaceTaken.y.max;
			vrSpaceTaken.y.max = -prevVRSpaceTaken.y.min;
		}
		availableSpaceCentreVRSpacePlacement.set_orientation(Rotator3(0.0f, do180 ? 180.0f : 0.0f, 0.0f).to_quat());
	}
	Range2 canBePlacedAt;
	canBePlacedAt.x.min = -playAreaRectSize.x * 0.5f - vrSpaceTaken.x.min;
	canBePlacedAt.x.max = playAreaRectSize.x * 0.5f - vrSpaceTaken.x.max;
	canBePlacedAt.y.min = -playAreaRectSize.y * 0.5f - vrSpaceTaken.y.min;
	canBePlacedAt.y.max = playAreaRectSize.y * 0.5f - vrSpaceTaken.y.max;
	Vector2 centreAt;
	centreAt.x = canBePlacedAt.x.get_at(clamp(randomGenerator.get_float(-0.25f, 1.25f), 0.0f, 1.0f));
	centreAt.y = canBePlacedAt.y.get_at(clamp(randomGenerator.get_float(-0.25f, 1.25f), 0.0f, 1.0f));
	if (alignToTiles)
	{
		todo_note(TXT("what? how? we don't know about door placement and this is the most important to align"));
	}
	availableSpaceCentreVRSpacePlacement.set_translation(centreAt.to_vector3());
	return availableSpaceCentreVRSpacePlacement;
}

#include "doorBasedShaft.h"

#include "..\..\game\game.h"

#include "..\..\..\core\debug\debugVisualiser.h"

#include "..\..\..\framework\debug\debugVisualiserUtils.h"
#include "..\..\..\framework\game\delayedObjectCreation.h"
#include "..\..\..\framework\game\game.h"
#include "..\..\..\framework\library\library.h"
#include "..\..\..\framework\meshGen\meshGenGenerationContext.h"
#include "..\..\..\framework\meshGen\meshGenParamImpl.inl"
#include "..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\framework\module\modulePresence.h"
#include "..\..\..\framework\object\scenery.h"
#include "..\..\..\framework\world\doorInRoom.h"
#include "..\..\..\framework\world\room.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace RoomGenerators;

//

DEFINE_STATIC_NAME(shaftCorner0);
DEFINE_STATIC_NAME(shaftCorner1);
DEFINE_STATIC_NAME(shaftCorner2);
DEFINE_STATIC_NAME(shaftCorner3);
DEFINE_STATIC_NAME(shaftRequiredZ);

DEFINE_STATIC_NAME(beyondDoor_width);
DEFINE_STATIC_NAME(beyondDoor_height);
DEFINE_STATIC_NAME(beyondDoor_depth);
DEFINE_STATIC_NAME(beyondDoor_offsetBeyond);

// unlikely to be used but provide extra context
DEFINE_STATIC_NAME(beyondDoor_allDoors);
DEFINE_STATIC_NAME(beyondDoor_dirOpposite); // door opposite dir
DEFINE_STATIC_NAME(beyondDoor_dirRight); // door opposite dir right (the door might be on left!)
DEFINE_STATIC_NAME(beyondDoor_dirLeft); // door opposite dir left (the door might be on right)
DEFINE_STATIC_NAME(beyondDoor_dirSide); // door opposite dir (left or right)

DEFINE_STATIC_NAME(pipeRadius);
DEFINE_STATIC_NAME(pipeA);
DEFINE_STATIC_NAME(pipeB);
DEFINE_STATIC_NAME(ipCount);
DEFINE_STATIC_NAME(ip0);
DEFINE_STATIC_NAME(ip1);
DEFINE_STATIC_NAME(ip2);

// global references
DEFINE_STATIC_NAME_STR(grGenericScenery, TXT("generic scenery"));

//

#ifdef AN_OUTPUT_WORLD_GENERATION
//#define OUTPUT_GENERATION
//#define OUTPUT_GENERATION_DEBUG_VISUALISER
#endif

//

REGISTER_FOR_FAST_CAST(DoorBasedShaftInfo);

DoorBasedShaftInfo::DoorBasedShaftInfo()
{
}

DoorBasedShaftInfo::~DoorBasedShaftInfo()
{
}

bool DoorBasedShaftInfo::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= shaftSceneryType.load_from_xml(_node, TXT("shaftSceneryType"));
	result &= beyondDoorDepthVar.load_from_xml(_node, TXT("beyondDoorDepthVar"));
	result &= beyondDoorBeWiderVar.load_from_xml(_node, TXT("beyondDoorBeWiderVar"));
	result &= beyondDoorSceneryType.load_from_xml(_node, TXT("beyondDoorSceneryType"));
	result &= beyondDoorMeshGenerator.load_from_xml(_node, TXT("beyondDoorMeshGenerator"));
	result &= pipeSceneryType.load_from_xml(_node, TXT("pipeSceneryType"));
	
	_lc.load_group_into(shaftSceneryType);
	_lc.load_group_into(beyondDoorSceneryType);
	_lc.load_group_into(beyondDoorMeshGenerator);
	_lc.load_group_into(pipeSceneryType);

	sizeInTiles.load_from_xml(_node, TXT("sizeInTiles"));
	extraMargin.load_from_xml(_node, TXT("extraMargin"));
	pipeRadius.load_from_xml(_node, TXT("pipeRadius"));
	horizontalPipeChance.load_from_xml(_node, TXT("horizontalPipeChance"));
	horizontalPipeDensity.load_from_xml(_node, TXT("horizontalPipeDensity"));
	horizontalPipeUsing2IPChance.load_from_xml(_node, TXT("horizontalPipeUsing2IPChance"));
	verticalPipeChance.load_from_xml(_node, TXT("verticalPipeChance"));
	verticalPipeDensity.load_from_xml(_node, TXT("verticalPipeDensity"));
	verticalPipeCluster.load_from_xml(_node, TXT("verticalPipeCluster"));
	pipeTopLimit.load_from_xml(_node, TXT("pipeTopLimit"));

	return result;
}

bool DoorBasedShaftInfo::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

bool DoorBasedShaftInfo::apply_renamer(Framework::LibraryStoredRenamer const & _renamer, Framework::Library* _library)
{
	bool result = base::apply_renamer(_renamer, _library);

	return result;
}

Framework::RoomGeneratorInfoPtr DoorBasedShaftInfo::create_copy() const
{
	DoorBasedShaftInfo * copy = new DoorBasedShaftInfo();
	*copy = *this;
	return Framework::RoomGeneratorInfoPtr(copy);
}

bool DoorBasedShaftInfo::generate(Framework::Room* _room, bool _subGenerator, REF_ Framework::RoomGeneratingContext& _roomGeneratingContext) const
{
	bool result = true;

	DoorBasedShaft sr(this);
	result &= sr.generate(Framework::Game::get(), _room, REF_ _roomGeneratingContext);

	result &= base::generate(_room, _subGenerator, REF_ _roomGeneratingContext);

	if (!_subGenerator)
	{
		_room->mark_mesh_generated();
	}

	return result;
}

//

DoorBasedShaft::DoorBasedShaft(DoorBasedShaftInfo const * _info)
: info(_info)
{
}

DoorBasedShaft::~DoorBasedShaft()
{
}

struct DoorPlacement
{
	Vector2 at = Vector2::zero;
	Vector2 fwd = Vector2::zero;
	Vector2 right = Vector2::zero;
	Range door = Range::empty; // relative to at
	Range doorFwd = Range::empty; // relative to at
	Range orgDoor = Range::empty;
};

struct PipePlacement
{
	float radius = 0.2f;
	float tRadius = 0.0f;
	int wall = 0;
	float t = 0.0f;
	Vector2 loc = Vector2::zero;
	Range z = Range::empty;

	PipePlacement() {}
	PipePlacement(float _radius, float _tRadius, int _wall, float _t, Vector2 const & _loc, Range const & _z)
	: radius(_radius), tRadius(_tRadius), wall(_wall), t(_t), loc(_loc), z(_z) {}
};

#define OFFSET_FROM_WALL 0.02f

bool DoorBasedShaft::generate(Framework::Game* _game, Framework::Room* _room, REF_ Framework::RoomGeneratingContext& _roomGeneratingContext)
{
	bool result = true;

	RoomGenerationInfo roomGenerationInfo = RoomGenerationInfo::get();
	Random::Generator rg = _room->get_individual_random_generator();
	rg.advance_seed(8956, 6387349);

	SimpleVariableStorage variables;
	_room->collect_variables(variables);
	info->apply_generation_parameters_to(variables);

#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
	DebugVisualiserPtr dv(new DebugVisualiser(String(TXT("door based shaft"))));
	dv->set_background_colour(Colour::lerp(0.05f, Colour::greyLight, Colour::blue));
	dv->activate();
#endif

	if (auto * shaftSceneryType = Framework::Library::get_current()->get_scenery_types().find(info->shaftSceneryType.get(variables)))
	{
		todo_note(TXT("this is for 2d xy doors only!"));
		Array<DoorPlacement> doorPlacements;
		Range allDoorsRange = Range::empty;
		float averageZ = 0.0f;
		// get door placements
		for_every_ptr(door, _room->get_all_doors())
		{
			averageZ += door->get_placement().get_translation().z;
			Vector2 doorAt = door->get_placement().get_translation().to_vector2();
			float doorAtZ = door->get_placement().get_translation().z;
			Vector2 doorFwd = -door->get_placement().get_axis(Axis::Y).to_vector2();
			Vector2 doorRight = doorFwd.rotated_right();
			doorAt -= doorFwd * 0.01f; // move back a little bit to allow portals to work
			float doorWidth = door->get_door()->calculate_vr_width();
			float halfDoorWidth = doorWidth * 0.5f;

			Range2 doorSize = door->calculate_size();
			allDoorsRange.include(doorAtZ + doorSize.y.min);
			allDoorsRange.include(doorAtZ + doorSize.y.max);

			bool alreadyExists = false;
			Transform doorPlacement = door->get_placement();
			for_every(existing, doorPlacements)
			{
				if (Vector2::dot(doorFwd, existing->fwd) > 0.999f)
				{
					// get door that is further
					float alongFwd = Vector2::dot(doorAt - existing->at, existing->fwd);
					if (alongFwd < 0.0f)
					{
						// just move back to include it
						Vector2 moveAlongFwd = existing->fwd * alongFwd;
						existing->at += moveAlongFwd;
						existing->doorFwd.min += alongFwd;
						existing->doorFwd.max += alongFwd;
					}
					float onDoorPlane = Vector2::dot(existing->right, doorAt - existing->at);
					float alongDoorFwd = Vector2::dot(existing->fwd, doorAt - existing->at);
					existing->door.include(onDoorPlane - halfDoorWidth);
					existing->door.include(onDoorPlane + halfDoorWidth);
					existing->doorFwd.include(alongDoorFwd);
					alreadyExists = true;
				}
			}
			if (alreadyExists)
			{
				continue;
			}
			DoorPlacement dp;
			dp.at = doorAt;
			dp.fwd = doorFwd;
			dp.right = doorRight;
			dp.door = Range(-halfDoorWidth, halfDoorWidth);
			dp.doorFwd = Range(0.0f);
			doorPlacements.push_back(dp);
		}
		averageZ = averageZ / max(0.01f, (float)_room->get_all_doors().get_size());

		ARRAY_PREALLOC_SIZE(Vector2, corners, 4);
		corners.set_size(4);

		Range sizeInTiles = info->sizeInTiles.get(variables);

		Range shaftSizeRange = sizeInTiles * roomGenerationInfo.get_tile_size();
		float shaftSize = rg.get(shaftSizeRange);
		float width = 0.0f;
		for_every(dp, doorPlacements)
		{
			width = max(width, dp->door.length());
		}
		width = max(width, shaftSizeRange.min);
		width = clamp(shaftSize, width, max(width, shaftSizeRange.max)); // prefer shaftSize, if not possible use width or max

		float extraMargin = info->extraMargin.get(variables);

		for_every(dp, doorPlacements)
		{
			dp->orgDoor = dp->door;
		}

		if (doorPlacements.get_size() == 1)
		{
			// same wall and point in the same direction
			auto const & dp = doorPlacements[0];
			float extraWidthOffset = rg.get_float(0.0f, 1.0f);
			float extraWidth = max(0.0f, width - dp.door.length());
			corners[0] = dp.at + (dp.door.max + max(extraMargin, extraWidth * extraWidthOffset)) * dp.right;
			corners[1] = dp.at + (dp.door.min - max(extraMargin, extraWidth * (1.0f - extraWidthOffset))) * dp.right;
			float depth = min(width, dp.doorFwd.max + shaftSize); // it should be at list "shaft size" from the furthest door
			corners[2] = corners[1] + dp.fwd * depth;
			corners[3] = corners[0] + dp.fwd * depth;
			for_every(c, corners)
			{
				*c -= dp.fwd * 0.02f; // extra bit
			}
		}
		else if (doorPlacements.get_size() == 2)
		{
			auto & dp0 = doorPlacements[0];
			auto & dp1 = doorPlacements[1];
			if (abs(Vector2::dot(dp0.right, dp1.right)) < 0.7f) // this will be perpendicular
			{
				Vector2 dp0RightReplacement = dp0.right;
				Vector2 dp1RightReplacement = dp1.right;
				DoorPlacement dpM;
				DoorPlacement dpS;
				if (rg.get_bool())
				{
					dpM = dp0;
					dpS = dp1;
					dp1RightReplacement = dp1.right.drop_using(dp0.right).normal();
				}
				else
				{
					dpM = dp1;
					dpS = dp0;
					dp0RightReplacement = dp0.right.drop_using(dp1.right).normal();
				}

				// find the furthest door point of S
				float sAt = Vector2::dot(dpS.at - dpM.at, dpM.right);
				float sLAt = Vector2::dot((dpS.at + dpS.right * dpS.door.min) - dpM.at, dpM.right);
				float sRAt = Vector2::dot((dpS.at - dpS.right * dpS.door.max) - dpM.at, dpM.right);

				if (sAt > 0.0f)
				{
					sAt = max(sLAt, sRAt);
				}
				else
				{
					sAt = min(sLAt, sRAt);
				}

				Vector2 intersection = dpM.at + dpM.right * sAt;

				// perpendicular
				corners[2] = intersection;

				// grow into opposite direction as interesection
				for_every(dp, doorPlacements)
				{
					int i = for_everys_index(dp);
					float iAt = Vector2::dot(intersection - dp->at, dp->right);
					dp->door.include(iAt);
					// stretch the other direction
					if (iAt > 0.0f)
					{
						dp->door.min = min(dp->door.min - extraMargin, iAt - width);
						corners[i] = dp->at + dp->right * dp->door.min;
					}
					else
					{
						dp->door.max = max(dp->door.max + extraMargin, iAt + width);
						corners[i] = dp->at + dp->right * dp->door.max;
					}
				}

				// calculate opposite intersection to fill the shape
				Vector2 oppIntersection;
				if (Vector2::calc_intersection(corners[0], dp1RightReplacement, corners[1], dp0RightReplacement, oppIntersection))
				{
					corners[3] = oppIntersection;
				}
				else
				{
					todo_important(TXT("what now?"));
				}
			}
			else if (Vector2::dot(dp0.right, dp1.right) < 0.0f)
			{
				// parallel opposing

				// grow to be wide enough to cover other door
				for_every(dp, doorPlacements)
				{
					int i = for_everys_index(dp);
					auto* odp = &doorPlacements[1 - i];
					dp->door.include(Vector2::dot(odp->at + odp->orgDoor.min * odp->right - dp->at, dp->right));
					dp->door.include(Vector2::dot(odp->at + odp->orgDoor.max * odp->right - dp->at, dp->right));
				}

				float extraWidthOffset = rg.get_float(0.0f, 1.0f);
				// build corners
				for_every(dp, doorPlacements)
				{
					float extraWidth = max(0.0f, width - dp->door.length());
					int i = for_everys_index(dp);
					corners[i * 2 + 0] = dp->at + (dp->door.min - max(extraMargin, extraWidth * extraWidthOffset)) * dp->right;
					corners[i * 2 + 1] = dp->at + (dp->door.max + max(extraMargin, extraWidth * (1.0f - extraWidthOffset))) * dp->right;

					extraWidthOffset = 1.0f - extraWidthOffset;
				}
			}
			else
			{
				// same wall in same direction

				float const use0 = rg.get_float(0.0f, 1.0f);
				Vector2 const wallFwd = (dp0.fwd * use0 + (1.0f - use0) * dp1.fwd).normal();
				Vector2 const wallRight = wallFwd.rotated_right();

				// get all door corners
				Array<Vector2> doorCorners;
				for_every(dp, doorPlacements)
				{
					// and a tiny bit behind
					doorCorners.push_back(dp->at + dp->right * dp->door.min - dp->fwd * OFFSET_FROM_WALL);
					doorCorners.push_back(dp->at + dp->right * dp->door.max - dp->fwd * OFFSET_FROM_WALL);
				}

				// find a point to get the most behind door corner
				Vector2 wallAt = dp0.at;
				for_every(dc, doorCorners)
				{
					float alongFwd = Vector2::dot(wallFwd, *dc - wallAt);
					if (alongFwd < 0.0f)
					{
						wallAt = *dc;
					}
				}

				// find desired width of wall, minimal
				Range wallWidth = Range(0.0f);
				for_every(dc, doorCorners)
				{
					wallWidth.include(Vector2::dot(wallRight, *dc - wallAt));
				}

				float extraWidthOffset = rg.get_float(0.0f, 1.0f);
				float extraWidth = max(0.0f, width - wallWidth.length());
				corners[0] = wallAt + (wallWidth.max + max(extraMargin, extraWidth * extraWidthOffset)) * wallRight;
				corners[1] = wallAt + (wallWidth.min - max(extraMargin, extraWidth * (1.0f - extraWidthOffset))) * wallRight;
				corners[2] = corners[1] + wallFwd * width;
				corners[3] = corners[0] + wallFwd * width;
			}
		}
		else
		{
			todo_important(TXT("implement_!"));
		}

		Vector2 centre = Vector2::zero;
		{
			for_every(corner, corners)
			{
				centre += *corner;
			}
			centre /= (float)corners.get_size();
		}

		// make corners clockwise
		{
			ARRAY_PREALLOC_SIZE(Vector2, orgCorners, 4);
			orgCorners = corners;
			corners.clear();
			while (!orgCorners.is_empty())
			{
				int lowestAngleIdx = NONE;
				float lowestAngle = 0.0f;
				for_every(c, orgCorners)
				{
					float angle = Rotator3::normalise_axis((*c - centre).angle());
					if (lowestAngleIdx == NONE || angle < lowestAngle)
					{
						lowestAngleIdx = for_everys_index(c);
						lowestAngle = angle;
					}
				}
				corners.push_back(orgCorners[lowestAngleIdx]);
				orgCorners.remove_fast_at(lowestAngleIdx);
			}
		}

		Array<PipePlacement> pipes;

		ARRAY_PREALLOC_SIZE(Range, hasDoor, 4);
		ARRAY_PREALLOC_SIZE(float, wallLengths, 4);
		float wallLength = 0.0f;
		{
			for_every(corner, corners)
			{
				int wallIdx = for_everys_index(corner);
				Range doorRange = Range::empty;
				Vector2 const * nextCorner = &corners[mod(wallIdx + 1, corners.get_size())];
				float wl = (*nextCorner - *corner).length();
				wallLengths.push_back(wl);
				wallLength += wl;

				float toCorner = (*corner - centre).angle();
				float toNextCorner = (*nextCorner - centre).angle();
				Vector2 c2n = *nextCorner - *corner;
				Vector2 c2nDir = c2n.normal();
				float c2nDist = c2n.length();
				for_every_ptr(door, _room->get_all_doors())
				{
					Vector2 doorAt = door->get_placement().get_translation().to_vector2();
					float doorAtZ = door->get_placement().get_translation().z;
					float toDoor = (doorAt - centre).angle();
					if (Rotator3::normalise_axis(toCorner - toDoor) < 0.0f &&
						Rotator3::normalise_axis(toNextCorner - toDoor) > 0.0f)
					{
						Range2 doorSize = door->calculate_size();
						doorRange.include(doorAtZ + doorSize.y.min);
						doorRange.include(doorAtZ + doorSize.y.max);

						Vector2 doorAtCorrected = doorAt + c2nDir * doorSize.x.centre();
						// pretend we are a pipe
						pipes.push_back(PipePlacement(doorSize.x.length(), 1.1f * doorSize.x.length() / wallLengths[wallIdx], wallIdx,
							Vector2::dot(doorAtCorrected - *corner, (*nextCorner - *corner).normal()) / c2nDist, doorAtCorrected,
							Range(doorAtZ + doorSize.y.min - 0.1f, doorAtZ + doorSize.y.max + 0.1f)));
					}
				}
				hasDoor.push_back(doorRange);
			}
		}

#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
		dv->start_gathering_data();
		dv->clear();

		roomGenerationInfo.get_play_area_zone().debug_visualise(dv.get(), Colour::blue);

		for_count(int, c, corners.get_size())
		{
			dv->add_line(corners[c], corners[(c + 1) % corners.get_size()], Colour::black);
		}

		for_every_ptr(door, _room->get_all_doors())
		{
			Framework::DebugVisualiserUtils::add_door_to_debug_visualiser(dv, door, door->get_placement().get_translation().to_vector2(), door->get_placement().get_axis(Axis::Forward).to_vector2(), door->calculate_size().x.length(), Colour::green);
		}

		dv->end_gathering_data();
		dv->show_and_wait_for_key();
		dv->deactivate();
#endif

		{
			shaftSceneryType->load_on_demand_if_required();

			Framework::Scenery* shaftScenery = nullptr;
			_game->perform_sync_world_job(TXT("spawn scenery"), [&shaftScenery, shaftSceneryType, _room]()
			{
				shaftScenery = new Framework::Scenery(shaftSceneryType, String::printf(TXT("shaft")));
				shaftScenery->init(_room->get_in_sub_world());
			});

			_room->collect_variables(shaftScenery->access_variables());

			Vector3 offsetZ = Vector3::zAxis * averageZ;
			shaftScenery->access_variables().access<Vector3>(NAME(shaftCorner0)) = corners[0].to_vector3();
			shaftScenery->access_variables().access<Vector3>(NAME(shaftCorner1)) = corners[1].to_vector3();
			shaftScenery->access_variables().access<Vector3>(NAME(shaftCorner2)) = corners[2].to_vector3();
			shaftScenery->access_variables().access<Vector3>(NAME(shaftCorner3)) = corners[3].to_vector3();
			shaftScenery->access_variables().access<Range>(NAME(shaftRequiredZ)) = allDoorsRange.moved_by(-averageZ);

			shaftScenery->access_individual_random_generator() = rg.spawn();

			info->apply_generation_parameters_to(shaftScenery->access_variables());

			shaftScenery->initialise_modules();

			_game->perform_sync_world_job(TXT("place shaft"), [&shaftScenery, _room, offsetZ]()
			{
				shaftScenery->get_presence()->place_in_room(_room, Transform(offsetZ, Quat::identity));
			});

			_game->on_newly_created_object(shaftScenery);
		}

		// beyond door
		if (info->beyondDoorSceneryType.is_set() || info->beyondDoorMeshGenerator.is_set())
		{
			auto* beyondDoorSceneryType = Framework::Library::get_current()->get_scenery_types().find(info->beyondDoorSceneryType.get(variables, Framework::LibraryName(), AllowToFail));
			if (!beyondDoorSceneryType)
			{
				beyondDoorSceneryType = Framework::Library::get_current()->get_global_references().get<Framework::SceneryType>(NAME(grGenericScenery));
			}
			if (beyondDoorSceneryType)
			{
				Name beyondDoorDepthVar;
				Name beyondDoorBeWiderVar;
				if (info->beyondDoorDepthVar.is_set())
				{
					beyondDoorDepthVar = info->beyondDoorDepthVar.get(variables, Name::invalid());
				}
				if (info->beyondDoorBeWiderVar.is_set())
				{
					beyondDoorBeWiderVar = info->beyondDoorBeWiderVar.get(variables, Name::invalid());
				}
				for_every_ptr(door, _room->get_all_doors())
				{
					Transform placement = door->get_outbound_transform();
					Transform usePlacement = placement;

					bool required = true;

					float offsetY = 0.01f; /* at least 1 cm to avoid clipping with door plane */
					float beyondDoorDepth = OFFSET_FROM_WALL;
					{
						if (beyondDoorDepthVar.is_valid())
						{
							if (auto* d = door->get_door())
							{
								// get door type as door type may change over time for door
								// this value is intrinsic for door type
								if (auto* dt = d->get_door_type())
								{
									beyondDoorDepth = dt->get_custom_parameters().get_value<float>(beyondDoorDepthVar, offsetY);
									offsetY = beyondDoorDepth;
								}
							}
						}
					}

					for_count(int, cIdx, corners.get_size())
					{
						Vector2 c = corners[cIdx];
						Vector2 n = corners[mod(cIdx + 1, corners.get_size())];

						// get normal to corner
						Vector2 i = (n - c).rotated_right().normal();

						// if we're at the wall and along its normal, we're good and we don't require mesh beyond
						Vector2 d = placement.get_translation().to_vector2();
						float dist = Vector2::dot(i, d - c);
						if (abs(dist) <= beyondDoorDepth)
						{
							Vector2 df = placement.get_axis(Axis::Forward).to_vector2();
							float along = Vector2::dot(i, df);
							if (abs(along) >= 0.99f)
							{
								required = false;
								break;
							}
						}
					}

					if (required)
					{
						beyondDoorSceneryType->load_on_demand_if_required();

						Framework::Scenery* scenery = nullptr;
						_game->perform_sync_world_job(TXT("spawn scenery"), [&scenery, beyondDoorSceneryType, _room]()
							{
								scenery = new Framework::Scenery(beyondDoorSceneryType, String::printf(TXT("beyond shaft door scenery")));
								scenery->init(_room->get_in_sub_world());
							});

						_room->collect_variables(scenery->access_variables());

						Range2 size = door->calculate_size();

						if (beyondDoorBeWiderVar.is_valid())
						{
							if (auto* d = door->get_door())
							{
								// get door type as door type may change over time for door
								// this value is intrinsic for door type
								if (auto* dt = d->get_door_type())
								{
									float beWider = dt->get_custom_parameters().get_value<float>(beyondDoorBeWiderVar, 0.0f);
									if (beWider > 0.0f)
									{
										size.expand_by(Vector2(beWider));
									}
								}
							}
						}

						usePlacement = usePlacement.to_world(Transform(Vector3(size.x.centre(), 0.0f, size.y.min), Quat::identity));

						bool dirOpposite = false;
						bool dirRight = false;
						bool dirLeft = false;
						Range3 allDoorsLocal = Range3::empty;
						for_every_ptr(ald, _room->get_all_doors())
						{
							Range2 size = ald->calculate_size();
							Transform placement = ald->get_placement();
							{
								Transform localDP = usePlacement.to_local(placement);
								Vector3 x = Vector3::xAxis * (size.x.length() * 0.5f);
								Vector3 z = Vector3::zAxis * size.y.length();
								allDoorsLocal.include(localDP.location_to_world(x));
								allDoorsLocal.include(localDP.location_to_world(-x));
								allDoorsLocal.include(localDP.location_to_world(x + z));
								allDoorsLocal.include(localDP.location_to_world(-x + z));
							}
							bool checkDir = ald != door;
							if (checkDir)
							{
								float yaw = Rotator3::normalise_axis(usePlacement.to_local(placement).get_axis(Axis::Forward).to_rotator().yaw);
								if (abs(yaw) > 135.0f)
								{
									dirOpposite = true;
								}
								else if (abs(yaw) > 45.0f)
								{
									if (yaw > 0.0f)
									{
										dirRight = true;
									}
									else
									{
										dirLeft = true;
									}
								}
							}
						}

						scenery->access_variables().access<float>(NAME(beyondDoor_width)) = size.x.length();
						scenery->access_variables().access<float>(NAME(beyondDoor_height)) = size.y.length();
						scenery->access_variables().access<float>(NAME(beyondDoor_depth)) = hardcoded 1.0f; // should be enough
						scenery->access_variables().access<float>(NAME(beyondDoor_offsetBeyond)) = offsetY;
						scenery->access_variables().access<Range3>(NAME(beyondDoor_allDoors)) = allDoorsLocal;
						scenery->access_variables().access<bool>(NAME(beyondDoor_dirOpposite)) = dirOpposite;
						scenery->access_variables().access<bool>(NAME(beyondDoor_dirRight)) = dirRight;
						scenery->access_variables().access<bool>(NAME(beyondDoor_dirLeft)) = dirLeft;
						scenery->access_variables().access<bool>(NAME(beyondDoor_dirSide)) = dirRight || dirLeft;

						scenery->access_individual_random_generator() = rg.spawn();

						info->apply_generation_parameters_to(scenery->access_variables());

						auto* useMeshGenerator = Framework::Library::get_current()->get_mesh_generators().find(info->beyondDoorMeshGenerator.get(variables, Framework::LibraryName(), AllowToFail));
						scenery->initialise_modules([useMeshGenerator](Framework::Module* _module)
						{
							if (useMeshGenerator)
							{
								if (auto* moduleAppearance = fast_cast<Framework::ModuleAppearance>(_module))
								{
									moduleAppearance->use_mesh_generator_on_setup(useMeshGenerator);
								}
							}
						});

						_game->perform_sync_world_job(TXT("place shaft"), [&scenery, _room, usePlacement]()
							{
								scenery->get_presence()->place_in_room(_room, usePlacement);
							});

						_game->on_newly_created_object(scenery);
					}
				}
			}
		}

		// pipes
		{
			Range pipeRadius = info->pipeRadius.get(variables);
			float horizontalPipeChance = info->horizontalPipeChance.get(variables);
			float horizontalPipeDensity = clamp(rg.get(info->horizontalPipeDensity.get(variables)), 0.0f, 1.0f);
			float horizontalPipeUsing2IPChance = info->horizontalPipeUsing2IPChance.get(variables);
			float verticalPipeChance = info->verticalPipeChance.get(variables);
			float verticalPipeDensity = clamp(rg.get(info->verticalPipeDensity.get(variables)), 0.0f, 1.0f);
			RangeInt verticalPipeCluster = info->verticalPipeCluster.get(variables);
			float pipeTopLimit = info->pipeTopLimit.get(variables);

			Range allDoorsRangeExtended = allDoorsRange.expanded_by(0.5f);
			
			if (info->pipeSceneryType.is_set())
			{
				if (auto * pipeSceneryType = Framework::Library::get_current()->get_scenery_types().find(info->pipeSceneryType.get(variables, Framework::LibraryName(), AllowToFail)))
				{
	#ifdef AN_DEVELOPMENT
					auto* g = Game::get();
	#endif
					if (rg.get_chance(horizontalPipeChance)
	#ifdef AN_DEVELOPMENT
						&& (!g || g->should_generate_dev_meshes(3))
	#endif
						)
					{
						bool use2IP = rg.get_chance(horizontalPipeUsing2IPChance);

						float z = -10.0f;
						while (z < min(pipeTopLimit, 10.0f))
						{
							z += rg.get_float(1.5f - horizontalPipeDensity, 8.0f - 6.0f * horizontalPipeDensity);
							float oz = z + rg.get_float(-1.0f, 1.0f);
							float radius = rg.get(pipeRadius);

							int at0 = rg.get_int(corners.get_size());
							int at1 = at0;
							while (at1 == at0)
							{
								at1 = rg.get_int(corners.get_size());
							}
							if (z >= -0.5f && z <= 2.5f &&
								(hasDoor[at0].does_contain(Range(z - radius, z + radius)) ||
								 hasDoor[at1].does_contain(Range(oz - radius, oz + radius)) ||
								 allDoorsRangeExtended.does_contain(Range(z - radius, z + radius)) || // keep elevators safe
								 allDoorsRangeExtended.does_contain(Range(oz - radius, oz + radius))))
							{
								continue;
							}
							float at0t = rg.get_float(0.1f, 0.9f);
							float at1t = rg.get_float(0.1f, 0.9f);

							int at0n = mod(at0 + 1, corners.get_size());
							int at1n = mod(at1 + 1, corners.get_size());

							{
								Framework::DelayedObjectCreation* doc = new Framework::DelayedObjectCreation();
								doc->inRoom = _room;
								doc->name = TXT("pipe");
								doc->priority = -10;
								doc->objectType = pipeSceneryType;
								doc->placement = Transform::identity;
								doc->randomGenerator = rg.spawn();

								_room->collect_variables(doc->variables);

								info->apply_generation_parameters_to(doc->variables);

								doc->variables.access<float>(NAME(pipeRadius)) = radius;

								Vector3 pipeALoc = (corners[at0] + (corners[at0n] - corners[at0]) * at0t).to_vector3();
								pipeALoc.z = z;

								Vector3 pipeBLoc = (corners[at1] + (corners[at1n] - corners[at1]) * at1t).to_vector3();
								pipeBLoc.z = z + rg.get_float(-1.0f, 1.0f);

								pipes.push_back(PipePlacement(radius, radius / wallLengths[at0], at0, at0t, pipeALoc.to_vector2(), Range(pipeALoc.z)));
								pipes.push_back(PipePlacement(radius, radius / wallLengths[at1], at1, at1t, pipeBLoc.to_vector2(), Range(pipeBLoc.z)));

								Transform pipeA = look_matrix(pipeALoc, (corners[at0n] - corners[at0]).rotated_right().to_vector3().normal(), Vector3::zAxis).to_transform();
								Transform pipeB = look_matrix(pipeBLoc, (corners[at1n] - corners[at1]).rotated_right().to_vector3().normal(), Vector3::zAxis).to_transform();

								doc->variables.access<Transform>(NAME(pipeA)) = pipeA;
								doc->variables.access<Transform>(NAME(pipeB)) = pipeB;

								if (use2IP)
								{
									doc->variables.access<int>(NAME(ipCount)) = 2;
									doc->variables.access<Vector3>(NAME(ip0)) = pipeA.location_to_world(Vector3::yAxis * radius * 2.5f);
									doc->variables.access<Vector3>(NAME(ip1)) = pipeB.location_to_world(Vector3::yAxis * radius * 2.5f);
								}
								else
								{
									doc->variables.access<int>(NAME(ipCount)) = 0;
								}

								_game->queue_delayed_object_creation(doc, true);
							}
						}
					}
					if (rg.get_chance(verticalPipeChance))
					{
						int wall = NONE;
						float t = 0.0f;
						float radius = 0.0f;
						float tRadius;
						float vz = 0.0f;
						Range vzRange;
						int leftInCluster = 0;
						Range vzRangeChange;
						bool vzRangeRandomChange;
						for_count(int, i, (int)((wallLength / pipeRadius.centre()) * verticalPipeDensity))
						{
							if (leftInCluster <= 0)
							{
								wall = rg.get_int(corners.get_size());
								t = rg.get_float(0.1f, 0.9f);
								radius = rg.get(pipeRadius);
								tRadius = (radius * 1.1f) / wallLengths[wall];
								t = clamp(t - 2.0f * tRadius, 2.0f * tRadius, 1.0f - 2.0f * radius / wallLengths[wall]);
								leftInCluster = rg.get(verticalPipeCluster);
								vz = rg.get_float(-15.0f, 15.0f);
								vzRange = Range::empty;
								vzRange.include(rg.get_float(-5.0f, -0.5f));
								vzRange.include(rg.get_float(0.5f, 5.0f));
								vzRangeChange.min = rg.get_float(-0.2f, 0.2f);
								vzRangeChange.max = rg.get_float(-0.2f, 0.2f);
								vzRangeRandomChange = rg.get_chance(0.3f);
							}

							int wallN = mod(wall + 1, corners.get_size());

							Range pipeRange(vz + vzRange.min - radius, vz + vzRange.max + radius);
							if (pipeRange.min > pipeTopLimit || pipeRange.max > pipeTopLimit)
							{
								// skip this one
								continue;
							}
							Vector2 pipeLoc = (corners[wall] + (corners[wallN] - corners[wall]) * t);

							t += 2.0f * tRadius;
							if (t > 1.0f - tRadius * 2.0f)
							{
								leftInCluster = 0;
							}

							bool overlaps = false;
							for_every(pipe, pipes)
							{
								if (pipe->z.overlaps(pipeRange))
								{
									if (pipe->wall == wall &&
										Range(pipe->t - pipe->tRadius * 0.98f, pipe->t + pipe->tRadius * 0.98f).overlaps(Range(t - tRadius * 0.98f, t + tRadius * 0.98f)))
									{
										overlaps = true;
										break;
									}
									if (pipe->wall != wall &&
										(pipe->loc - pipeLoc).length() < (pipe->radius + radius) * 2.0f)
									{
										overlaps = true;
										break;
									}
								}
							}
					
							if (! overlaps)
							{
								Framework::DelayedObjectCreation* doc = new Framework::DelayedObjectCreation();
								doc->inRoom = _room;
								doc->name = TXT("pipe");
								doc->priority = -10;
								doc->devSkippable = true;
								doc->objectType = pipeSceneryType;
								doc->placement = Transform::identity;
								doc->randomGenerator = rg.spawn();

								_room->collect_variables(doc->variables);

								info->apply_generation_parameters_to(doc->variables);

								doc->variables.access<float>(NAME(pipeRadius)) = radius;

								Vector3 pipeALoc = pipeLoc.to_vector3();
								pipeALoc.z = pipeRange.min + radius;

								Vector3 pipeBLoc = pipeALoc;
								pipeBLoc.z = pipeRange.max + radius;

								pipes.push_back(PipePlacement(radius, tRadius, wall, t, pipeALoc.to_vector2(), pipeRange));

								Vector3 forward = (corners[wallN] - corners[wall]).rotated_right().to_vector3().normal();
								Transform pipeA = look_matrix(pipeALoc, forward, Vector3::zAxis).to_transform();
								Transform pipeB = look_matrix(pipeBLoc, forward, Vector3::zAxis).to_transform();

								doc->variables.access<Transform>(NAME(pipeA)) = pipeA;
								doc->variables.access<Transform>(NAME(pipeB)) = pipeB;
								doc->variables.access<Vector3>(NAME(ip0)) = pipeA.get_translation() + forward * radius * 2.5f;
								doc->variables.access<Vector3>(NAME(ip1)) = pipeB.get_translation() + forward * radius * 2.5f;
								doc->variables.access<int>(NAME(ipCount)) = 2;

								_game->queue_delayed_object_creation(doc, true);
							}

							if (vzRangeRandomChange)
							{
								vzRange.min += rg.get_float(-0.5f, 0.5f);
								vzRange.max += rg.get_float(-0.5f, 0.5f);
							}
							else
							{
								vzRange.min += vzRangeChange.min;
								vzRange.max += vzRangeChange.max;
							}
							vzRange.min = min(vzRange.min, -0.5f);
							vzRange.max = max(vzRange.max, 0.5f);

							--leftInCluster;
						}
					}
				}
			}
		}
	}

	return result;
}


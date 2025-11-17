#include "simpleCorridor.h"

#include "..\roomGenerationInfo.h"

#include "..\..\..\core\debug\debugVisualiser.h"

#include "..\..\..\framework\debug\debugVisualiserUtils.h"
#include "..\..\..\framework\game\game.h"
#include "..\..\..\framework\library\library.h"
#include "..\..\..\framework\meshGen\meshGenGenerationContext.h"
#include "..\..\..\framework\meshGen\meshGenParamImpl.inl"
#include "..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\framework\module\moduleCollision.h"
#include "..\..\..\framework\module\modulePresence.h"
#include "..\..\..\framework\object\actor.h"
#include "..\..\..\framework\world\doorInRoom.h"
#include "..\..\..\framework\world\environment.h"
#include "..\..\..\framework\world\environmentInfo.h"
#include "..\..\..\framework\world\region.h"
#include "..\..\..\framework\world\room.h"
#include "..\..\..\framework\world\world.h"

#ifdef AN_CLANG
#include "..\..\..\framework\library\usedLibraryStored.inl"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace RoomGenerators;

//

DEFINE_STATIC_NAME(vrCorridorUsing180);

DEFINE_STATIC_NAME(corridorHeight);
DEFINE_STATIC_NAME(corridorWidth);
DEFINE_STATIC_NAME(corridorPreferStraight); // bool
DEFINE_STATIC_NAME(corridorPreferRegular); // bool			// will try to make 90' turns as much as possible
DEFINE_STATIC_NAME(corridorCompactFullChance); // float		// compactness is used for 2ip pointing in the same dir, if this is true, we will use minimal value for both ips
DEFINE_STATIC_NAME(corridorCompactness); // float			// this one is general compactness
DEFINE_STATIC_NAME(doorA);
DEFINE_STATIC_NAME(doorB);
DEFINE_STATIC_NAME(ipCount);
DEFINE_STATIC_NAME(ip0);
DEFINE_STATIC_NAME(ip1);

//

#ifdef AN_OUTPUT_WORLD_GENERATION
#define WITH_DEBUG_VISUALISER
#define OUTPUT_GENERATION
//#define OUTPUT_GENERATION_DEBUG_VISUALISER
//#define OUTPUT_GENERATION_VARIABLES
#else
#ifdef LOG_WORLD_GENERATION
#define OUTPUT_GENERATION
#endif
#endif

//

REGISTER_FOR_FAST_CAST(SimpleCorridorInfo);

SimpleCorridorInfo::SimpleCorridorInfo()
{
	set_room_generator_priority(-200);
}

SimpleCorridorInfo::~SimpleCorridorInfo()
{
}

bool SimpleCorridorInfo::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= corridorMeshGenerator.load_from_xml(_node, TXT("corridorMeshGenerator"));
	result &= sameDirMaxTileDistForIPs.load_from_xml(_node, TXT("sameDirMaxTileDistForIPs"));

	return result;
}

bool SimpleCorridorInfo::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

bool SimpleCorridorInfo::apply_renamer(Framework::LibraryStoredRenamer const & _renamer, Framework::Library* _library)
{
	bool result = base::apply_renamer(_renamer, _library);

	if (corridorMeshGenerator.is_value_set()) { _renamer.apply_to(corridorMeshGenerator.access()); }

	return result;
}

Framework::RoomGeneratorInfoPtr SimpleCorridorInfo::create_copy() const
{
	SimpleCorridorInfo * copy = new SimpleCorridorInfo();
	*copy = *this;
	return Framework::RoomGeneratorInfoPtr(copy);
}

bool SimpleCorridorInfo::internal_generate(Framework::Room* _room, bool _subGenerator, REF_ Framework::RoomGeneratingContext& _roomGeneratingContext) const
{
	bool result = true;

	SimpleCorridor sr(this);
	result &= sr.generate(Framework::Game::get(), _room, _subGenerator, REF_ _roomGeneratingContext);

	return result;
}

//

SimpleCorridor::SimpleCorridor(SimpleCorridorInfo const * _simpleCorridorInfo)
: info(_simpleCorridorInfo)
{
}

SimpleCorridor::~SimpleCorridor()
{
}

bool SimpleCorridor::generate(Framework::Game* _game, Framework::Room* _room, bool _subGenerator, REF_ Framework::RoomGeneratingContext& _roomGeneratingContext)
{
	scoped_call_stack_info(TXT("SimpleElevator::generate"));

	RoomGenerationInfo roomGenerationInfo = RoomGenerationInfo::get();
	Random::Generator randomGenerator = _room->get_individual_random_generator();
	randomGenerator.advance_seed(7835867, 286234);

	if (_room->get_all_doors().get_size() != 2)
	{
		error(TXT("corridor should have 2 doors"));
		return false;
	}

	_room->mark_check_if_doors_are_too_close(true);

#ifdef OUTPUT_GENERATION
	output(TXT("generating simple corridor %S (%S)"), _room->get_individual_random_generator().get_seed_string().to_char(), _room->get_name().to_char());
#endif

	Framework::DoorInRoom * dA = _room->get_all_doors()[0];
	Framework::DoorInRoom * dB = _room->get_all_doors()[1];
	Transform vrPlacementA = dA->get_vr_space_placement();
	Transform vrPlacementB = dB->get_vr_space_placement();

	if (_room->get_tags().get_tag(NAME(vrCorridorUsing180)))
	{
		vrPlacementB = Transform::do180.to_world(vrPlacementB);
#ifdef OUTPUT_GENERATION
		output(TXT("reverse vr placement B due to vrCorridorUsing180"));
#endif
	}

	float doorWidth = dA->get_door()->calculate_vr_width();
	if (doorWidth == 0.0f)
	{
		doorWidth = roomGenerationInfo.get_tile_size();
	}
	float halfDoorWidth = doorWidth * 0.5f;

	VR::Zone fullVRZone = roomGenerationInfo.get_play_area_zone();
	VR::Zone gameVRZone = Framework::Game::get()->get_vr_zone(_room);

	// 0.4 - quite generous
	an_assert(fullVRZone.does_contain(vrPlacementA.get_translation().to_vector2(), doorWidth * 0.4f));
	an_assert(fullVRZone.does_contain(vrPlacementB.get_translation().to_vector2(), doorWidth * 0.4f));

	VR::Zone beInZone = fullVRZone;

	if (gameVRZone.does_contain(vrPlacementA.get_translation().to_vector2(), doorWidth * 0.4f) &&
		gameVRZone.does_contain(vrPlacementB.get_translation().to_vector2(), doorWidth * 0.4f))
	{
		beInZone = gameVRZone;
	}

	dA->set_placement(vrPlacementA);
	dB->set_placement(vrPlacementB);
	
	Range2 doorASize = dA->calculate_size();
	Range2 doorBSize = dB->calculate_size();

	// replace with compatible sizes, but only height
	for_count(int, di, 2)
	{
		auto* d = di == 0 ? dA : dB;
		auto& doorSize = di == 0 ? doorASize : doorBSize;
		auto s = d->calculate_compatible_size();
		if (!s.is_empty())
		{
			doorSize.y = s.y;
		}
	}

	// ignore cell separators, they should get replaced anyway (do it in a separate pass as we first had to update size for both doors
	for_count(int, di, 2)
	{
		auto* d = di == 0 ? dA : dB;
		auto& doorSize = di == 0 ? doorASize : doorBSize;
		auto& otherDoorSize = di == 0 ? doorBSize : doorASize;
		if (auto* actualDoor = d->get_door())
		{
			if (actualDoor->is_world_separator_door_due_to(WorldSeparatorReason::CellSeparator))
			{
				doorSize = otherDoorSize;
			}
		}
	}

	todo_note(TXT("allow to override_ and choose corridor somehow?"));
	an_assert(almost_equal(doorASize.x.min, doorBSize.x.min) && almost_equal(doorASize.x.max, doorBSize.x.max), TXT("we require same size for both doors! make sure it is the same for \"%S\" (a:[%.3f-%.3f]x[%.3f-%.3f] b:[%.3f-%.3f]x[%.3f-%.3f])"),
		_room->get_name().to_char(),
		doorASize.x.min, doorASize.x.max, doorASize.y.min, doorASize.y.max,
		doorBSize.x.min, doorBSize.x.max, doorBSize.y.min, doorBSize.y.max);

	Transform doorAPlacement = vrPlacementA;
	Transform doorBPlacement = vrPlacementB;

	// place doors (as corridor mesh generators used them) at floor, centered
	// at floor! leave Z alone as it will mess up corridor making
	doorAPlacement.set_translation(doorAPlacement.get_translation() + doorAPlacement.get_axis(Axis::X) * (doorASize.x.max + doorASize.x.min) * 0.5f);
	doorBPlacement.set_translation(doorBPlacement.get_translation() + doorBPlacement.get_axis(Axis::X) * (doorBSize.x.max + doorBSize.x.min) * 0.5f);

	Rotator3 doorARotation = doorAPlacement.get_orientation().to_rotator();
	Rotator3 doorBRotation = doorBPlacement.get_orientation().to_rotator();
	float yawDiff = relative_angle(doorARotation.yaw - doorBRotation.yaw);
	float absYawDiff = abs(yawDiff);
	Vector3 relLocation = doorAPlacement.location_to_local(doorBPlacement.get_translation());
	float absRelLocationX = abs(relLocation.x);

#ifdef WITH_DEBUG_VISUALISER
	bool debugThis = false;
#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
	debugThis = true;
#endif
	if (_room->get_name() == TXT("vr corridor 3"))
	{
		//debugThis = true;
	}
	DebugVisualiserPtr dv(new DebugVisualiser(String::printf(TXT("simple corridor (%S)"), _room->get_name().to_char())));
	dv->set_background_colour(Colour::lerp(0.05f, Colour::greyLight, Colour::green));
	if (debugThis)
	{
		dv->activate();
	}
#endif

	Vector2 doorALoc = doorAPlacement.get_translation().to_vector2();
	Vector2 doorADir = doorAPlacement.get_axis(Axis::Y).to_vector2();

	Vector2 doorBLoc = doorBPlacement.get_translation().to_vector2();
	Vector2 doorBDir = doorBPlacement.get_axis(Axis::Y).to_vector2();

#ifdef WITH_DEBUG_VISUALISER
	dv->start_gathering_data();
	dv->clear();
	Framework::DebugVisualiserUtils::add_doors_to_debug_visualiser(dv, beInZone, dA, doorALoc, doorADir, dB, doorBLoc, doorBDir, doorWidth);
	dv->end_gathering_data();
	if (dv->is_active()) { dv->show_and_wait_for_key(); }
#endif

	SimpleVariableStorage variables;
	_room->collect_variables(REF_ variables);
	info->apply_generation_parameters_to(variables);

	variables.access<Transform>(NAME(doorA)) = doorAPlacement;
	variables.access<Transform>(NAME(doorB)) = doorBPlacement;
	// take max from both
	variables.access<float>(NAME(corridorHeight)) = max(doorASize.y.max, doorBSize.y.max); // not height as it may be off the ground
	variables.access<float>(NAME(corridorWidth)) = max(doorASize.x.length(), doorBSize.x.length());
	variables.invalidate<int>(NAME(ipCount));
	variables.invalidate<Vector3>(NAME(ip0));
	variables.invalidate<Vector3>(NAME(ip1));

	info->apply_generation_parameters_to(variables);

	bool corridorPreferStraight = variables.get_value<bool>(NAME(corridorPreferStraight), false);
	bool corridorPreferRegular = variables.get_value<bool>(NAME(corridorPreferRegular), false);
	float corridorCompactFullChance = variables.get_value<float>(NAME(corridorCompactFullChance), 0.0f);
	float corridorCompactness = variables.get_value<float>(NAME(corridorCompactness), 0.0f);

	// check different cases
	int mgIPs = NONE;
	bool mg0ipWasOk = false;
	// old check: if (relLocation.y < -halfDoorWidth * 0.98f || absRelLocationX > doorWidth * 0.98f)
	// but now we assume that if we were tasked with corridor we are given right doors
	{
		if (mgIPs == NONE && 
			absYawDiff > 135.0f && relLocation.y < 0.0f && absRelLocationX < -relLocation.y &&
			(-relLocation.y > doorWidth * 1.2f || absRelLocationX < -relLocation.y * 0.7f)) // make sure we have enough space to use 0ip
		{
			Random::Generator rg = _room->get_individual_random_generator();
			rg.advance_seed(5825, 9746283);

			// opposite, in front of each other or at further distance with chance to use 2ip
			if (-relLocation.y < doorWidth * 1.2f || // really close
				abs(relLocation.x) < doorWidth * 0.8f || // not enough space to stick 2ip in
				(!corridorPreferStraight && rg.get_chance(0.7f)) ||
				(corridorPreferStraight && rg.get_chance(0.1f)))
			{
				mg0ipWasOk = true;
				if (!corridorPreferStraight)
				{
					mgIPs = 0;
				}
#ifdef WITH_DEBUG_VISUALISER
				dv->start_gathering_data();
				dv->add_text((doorALoc + doorBLoc) * 0.5f, String(TXT("0ip")), Colour::greenDark, 0.1f);
				dv->end_gathering_data();
				if (dv->is_active()) { dv->show_and_wait_for_key(); }
#endif
			}
		}
		if (mgIPs == NONE &&
			absYawDiff >= 45.0f) // 0 is the same dir
		{
			// try finding one intersection point
			Vector2 intersection;
			if (Vector2::calc_intersection(doorALoc, -doorADir, doorBLoc, -doorBDir, intersection))
			{
				// clamp intersection point to play area zone, but only if differs more than a small bit
				{
					Vector2 clampedIntersection = beInZone.get_inside(intersection, halfDoorWidth * 0.9f);
					if ((clampedIntersection - intersection).length() > halfDoorWidth * 0.1f)
					{
						intersection = clampedIntersection;
					}
				}

#ifdef WITH_DEBUG_VISUALISER
				dv->start_gathering_data();
				dv->clear();
				Framework::DebugVisualiserUtils::add_doors_to_debug_visualiser(dv, beInZone, dA, doorALoc, doorADir, dB, doorBLoc, doorBDir, doorWidth);
				dv->add_circle(intersection, 0.05f, Colour::greenDark);
				dv->add_text(intersection, String(TXT("x")), Colour::greenDark, 0.1f);
				dv->end_gathering_data();
				if (dv->is_active()) { dv->show_and_wait_for_key(); }
#endif

				Vector2 intersectionLocalA = doorAPlacement.location_to_local(intersection.to_vector3()).to_vector2();
				Vector2 intersectionLocalB = doorBPlacement.location_to_local(intersection.to_vector3()).to_vector2();
				if (intersectionLocalA.y < 0.0f && abs(intersectionLocalA.x) < -intersectionLocalA.y &&
					intersectionLocalB.y < 0.0f && abs(intersectionLocalB.x) < -intersectionLocalB.y)
				{
					Random::Generator rg = _room->get_individual_random_generator();
					rg.advance_seed(9085, 297792);

					Vector3 relLocA = doorAPlacement.location_to_local(doorBPlacement.get_translation());
					Vector3 relLocB = doorBPlacement.location_to_local(doorAPlacement.get_translation());

					// they intersect in front of each other
					if ((corridorPreferStraight && rg.get_chance(0.9f)) ||
						(!corridorPreferStraight && rg.get_chance(0.1f)) ||
						relLocA.y > -doorWidth * 0.6f || relLocB.y > -doorWidth * 0.6f || // too close
						(absYawDiff < 135.0f)) // are at angle, we need bent corridor
					{
						mgIPs = 1;
						variables.access<Vector3>(NAME(ip0)) = intersection.to_vector3();
#ifdef WITH_DEBUG_VISUALISER
						dv->start_gathering_data();
						dv->add_text(intersection, String(TXT("1ip")), Colour::greenDark, 0.1f);
						dv->add_text(intersection, String::printf(TXT("absYawDiff: %.3f"), absYawDiff), Colour::greenDark, 0.1f, NP, VectorInt2(0,-1));
						dv->end_gathering_data();
						if (dv->is_active()) { dv->show_and_wait_for_key(); }
#endif
					}
					else
					{
						// if 1ip would handle this, 0ip will handle it too!
						mg0ipWasOk = true;
						if (!corridorPreferStraight)
						{
							mgIPs = 0;
						}
#ifdef WITH_DEBUG_VISUALISER
						dv->start_gathering_data();
						dv->add_text(intersection, String(TXT("0ip (1ip?)")), Colour::greenDark, 0.1f);
						dv->end_gathering_data();
						if (dv->is_active()) { dv->show_and_wait_for_key(); }
#endif
					}
				}
				else
				{
#ifdef WITH_DEBUG_VISUALISER
					dv->start_gathering_data();
					dv->add_text(intersection, String(TXT("failed")), Colour::red, 0.1f);
					dv->end_gathering_data();
					if (dv->is_active()) { dv->show_and_wait_for_key(); }
#endif
				}
			}
		}
		if (mgIPs == NONE)
		{
			// check two corners
			Vector2 ip0;
			Vector2 ip1;
			// ip0 and ip1 are here the furthest points
			if ((beInZone.calc_intersection(doorALoc, -doorADir, ip0, halfDoorWidth * 0.3f, halfDoorWidth * 0.95f) ||
				beInZone.calc_intersection(doorALoc, -doorADir, ip0, halfDoorWidth * 0.3f, halfDoorWidth * 0.75f) ||
				beInZone.calc_intersection(doorALoc, -doorADir, ip0, halfDoorWidth * 0.3f, halfDoorWidth * 0.60f)) &&
				(beInZone.calc_intersection(doorBLoc, -doorBDir, ip1, halfDoorWidth * 0.3f, halfDoorWidth * 0.95f) ||
					beInZone.calc_intersection(doorBLoc, -doorBDir, ip1, halfDoorWidth * 0.3f, halfDoorWidth * 0.75f) ||
					beInZone.calc_intersection(doorBLoc, -doorBDir, ip1, halfDoorWidth * 0.3f, halfDoorWidth * 0.60f)))
			{
				ip0 = beInZone.get_inside(ip0, halfDoorWidth * 0.85f);
				ip1 = beInZone.get_inside(ip1, halfDoorWidth * 0.85f);
				{
					float minDist = halfDoorWidth * 1.2f;
					float ip0close = Vector2::dot(ip0 - doorALoc, -doorADir);
					if (ip0close < minDist)
					{
						ip0 += -doorADir * (minDist - ip0close);
					}
					float ip1close = Vector2::dot(ip1 - doorBLoc, -doorBDir);
					if (ip1close < minDist)
					{
						ip1 += -doorBDir * (minDist - ip1close);
					}
				}
#ifdef WITH_DEBUG_VISUALISER
				dv->start_gathering_data();
				dv->clear();
				Framework::DebugVisualiserUtils::add_doors_to_debug_visualiser(dv, beInZone, dA, doorALoc, doorADir, dB, doorBLoc, doorBDir, doorWidth);
				dv->add_circle(ip0, 0.05f, Colour::greenDark);
				dv->add_circle(ip1, 0.05f, Colour::greenDark);
				dv->end_gathering_data();
				if (dv->is_active()) { dv->show_and_wait_for_key(); }
#endif

				Random::Generator rg = _room->get_individual_random_generator();
				rg.advance_seed(1234, 80678967);
				float max0Dist = (ip0 - doorALoc).length();
				float max1Dist = (ip1 - doorBLoc).length();
				float min0Dist = halfDoorWidth;
				float min1Dist = halfDoorWidth;
				if (absYawDiff < 30.0f)
				{
					Vector2 commonDir = (doorADir + doorBDir).normal();
					// make sure both min dist are in front of each other
					// first calculate relative distance, so if 0 is behind 1, we will be able to add something for 0
					float dist0From1 = max(0.0f, Vector2::dot(-commonDir, doorBLoc - doorALoc));
					float dist1From0 = max(0.0f, Vector2::dot(-commonDir, doorALoc - doorBLoc));
					// now affect min dist so they're common
					min0Dist = max(min0Dist, dist0From1 + halfDoorWidth);
					min1Dist = max(min1Dist, dist1From0 + halfDoorWidth);

					Framework::MeshGenParam<Range> sameDirMaxTileDistForIPsResolved = info->sameDirMaxTileDistForIPs;
					sameDirMaxTileDistForIPsResolved.fill_value_with(variables, AllowToFail);
					if (sameDirMaxTileDistForIPsResolved.is_value_set())
					{
						Range tileDistRange = sameDirMaxTileDistForIPsResolved.get();
						float tileDist = rg.get(tileDistRange);
						float maxAllowedDist = roomGenerationInfo.get_tile_size() * (tileDist - 0.5f); // half size less as we care about fitting into that space
						max0Dist = max(min0Dist, min(max0Dist, maxAllowedDist + max(0.0f, dist0From1)));
						max1Dist = max(min1Dist, min(max1Dist, maxAllowedDist + max(0.0f, dist1From0)));
					}
				}
				if (absYawDiff > 150.0f)
				{
					// opposite dir, reduce max dist, so they're in front of each other
					{
						Vector2 doorBLocFront = doorBLoc - doorBDir * halfDoorWidth;
						Vector2 doorAMax;
						if (Vector2::calc_intersection(doorALoc, doorADir, doorBLocFront, doorBDir.rotated_right(), OUT_ doorAMax))
						{
							max0Dist = (doorAMax - doorALoc).length();
							max0Dist = max(max0Dist, min0Dist);
						}
					}
					{
						Vector2 doorALocFront = doorALoc - doorADir * halfDoorWidth;
						Vector2 doorBMax;
						if (Vector2::calc_intersection(doorBLoc, doorBDir, doorALocFront, doorADir.rotated_right(), OUT_ doorBMax))
						{
							max1Dist = (doorBMax - doorBLoc).length();
							max1Dist = max(max1Dist, min1Dist);
						}
					}
#ifdef WITH_DEBUG_VISUALISER
					dv->start_gathering_data();
					dv->clear();
					Framework::DebugVisualiserUtils::add_doors_to_debug_visualiser(dv, beInZone, dA, doorALoc, doorADir, dB, doorBLoc, doorBDir, doorWidth);
					dv->add_circle(doorALoc - doorADir * min0Dist, 0.03f, Colour::greenDark);
					dv->add_circle(doorBLoc - doorBDir * min1Dist, 0.03f, Colour::greenDark);
					dv->add_circle(doorALoc - doorADir * max0Dist, 0.05f, Colour::greenDark);
					dv->add_circle(doorBLoc - doorBDir * max1Dist, 0.05f, Colour::greenDark);
					dv->add_text((doorALoc + doorBLoc) * 0.5f, String(TXT("doors opposite")), Colour::greenDark, 0.1f);
					dv->end_gathering_data();
					if (dv->is_active()) { dv->show_and_wait_for_key(); }
#endif
				}
				if (corridorCompactFullChance > 0.0f)
				{
					if (rg.get_chance(corridorCompactFullChance))
					{
						max0Dist = min0Dist;
						max1Dist = min1Dist;
					}
				}
				if (corridorCompactness > 0.0f)
				{
					float mul = clamp(1.0f - corridorCompactness, 0.0f, 1.0f);
					max0Dist = min0Dist + (max0Dist - min0Dist) * mul;
					max1Dist = min1Dist + (max1Dist - min1Dist) * mul;
				}
				if (absYawDiff < 60.0f)
				{
					// if both in the same'y direction
					// it should be in front of both ips, at least half door width, so we won't have very sharp corners "from behind"
					min0Dist = clamp(Vector2::dot(doorBLoc + (-doorBDir) * halfDoorWidth - doorALoc, -doorADir), halfDoorWidth, max0Dist);
					min1Dist = clamp(Vector2::dot(doorALoc + (-doorADir) * halfDoorWidth - doorBLoc, -doorBDir), halfDoorWidth, max1Dist);
				}
				float off0 = rg.get_float(min0Dist, max0Dist);
				Vector2 nip0 = doorALoc - doorADir * off0;
				float off1 = rg.get_float(min1Dist, max1Dist);
				Vector2 nip1 = doorBLoc - doorBDir * off1;

				int ipTriesLeft = 1000;
				while ((-- ipTriesLeft) > 0)
				{
					// if we're running out of options, true using true minimal value
					if (ipTriesLeft < 200)
					{
						min0Dist = halfDoorWidth;
						min1Dist = halfDoorWidth;
					}
					bool tryPerpendicular = ((corridorPreferStraight || corridorPreferRegular) && ipTriesLeft > 500 && rg.get_chance(corridorPreferRegular? 0.98f : 0.9f)) ||
											(!(corridorPreferStraight || corridorPreferRegular) && ipTriesLeft > 500 && rg.get_chance(corridorPreferRegular? 0.02f: 0.2f));
					bool tryVeryClose = absYawDiff > 135.0f && ipTriesLeft < 10;
					if (ipTriesLeft < 2 && absYawDiff <= 135.0f)
					{
						// try furthest
						nip0 = doorALoc - doorADir * off0;
						nip1 = doorBLoc - doorBDir * off1;
					}
					else if (tryVeryClose)
					{
						nip0 = doorALoc - doorADir * halfDoorWidth;
						nip1 = doorBLoc - doorBDir * halfDoorWidth;
					}
					else if (tryPerpendicular)
					{
						// try perendicularising one way or another
						Vector2 intersection;
						if (rg.get_chance(0.5f))
						{
							if (Vector2::calc_intersection(nip1, doorBDir.rotated_right(), doorALoc, doorADir, intersection))
							{
								nip0 = intersection;
							}
						}
						else
						{
							if (Vector2::calc_intersection(nip0, doorADir.rotated_right(), doorBLoc, doorBDir, intersection))
							{
								nip1 = intersection;
							}
						}
					}
					// outbound!
					Vector2 nip1LocalA = doorAPlacement.location_to_local(nip1.to_vector3()).to_vector2();
					Vector2 nip1LocalB = doorBPlacement.location_to_local(nip1.to_vector3()).to_vector2();
					Vector2 nip0LocalA = doorAPlacement.location_to_local(nip0.to_vector3()).to_vector2();
					Vector2 nip0LocalB = doorBPlacement.location_to_local(nip0.to_vector3()).to_vector2();
					float const threshold = 0.02f;
					// check if are in front of both doors and if are inside play area
					// this way we should be more sure that a corridor won't get through a door
					if (nip1LocalA.y < -halfDoorWidth + threshold &&
						nip1LocalB.y < -halfDoorWidth + threshold &&
						beInZone.does_contain(nip1, halfDoorWidth * 0.9f))
					{
						if (nip0LocalB.y < -halfDoorWidth + threshold &&
							nip0LocalA.y < -halfDoorWidth + threshold &&
							beInZone.does_contain(nip0, halfDoorWidth * 0.9f))
						{
							if (abs(Vector2::dot((doorALoc - nip0).normal(), (nip1 - nip0).normal())) > 0.3f ||
								abs(Vector2::dot((doorBLoc - nip1).normal(), (nip0 - nip1).normal())) > 0.3f)
							{
								// make angles a bit more right
								float alongADir = Vector2::dot(doorADir, (nip1 - nip0));
								float alongBDir = Vector2::dot(doorBDir, (nip0 - nip1));
								float howMuch = 0.1f;
								nip0 += howMuch * doorADir * alongADir;
								nip1 += howMuch * doorBDir * alongBDir;
								// if we go out of range, we will be randomly rechosen
							}
							else
							{
								if ((nip0 - nip1).length() < doorWidth * 0.9f)
								{
									mg0ipWasOk = true;
									if (!corridorPreferStraight || absYawDiff > 170.0f || (nip0 - nip1).length() < doorWidth * 0.2f)
									{
										mgIPs = 0;
									}
#ifdef WITH_DEBUG_VISUALISER
									dv->start_gathering_data();
									dv->clear();
									Framework::DebugVisualiserUtils::add_doors_to_debug_visualiser(dv, beInZone, dA, doorALoc, doorADir, dB, doorBLoc, doorBDir, doorWidth);
									dv->add_circle(nip0, 0.05f, Colour::greenDark);
									dv->add_circle(nip1, 0.05f, Colour::greenDark);
									dv->add_line(nip0, ip0, Colour::greenDark);
									dv->add_line(nip1, ip1, Colour::greenDark);
									dv->add_text((nip0 + nip1) * 0.5f, String(TXT("0ip (too close for 2ip)")), Colour::greenDark, 0.1f);
									dv->end_gathering_data();
									if (dv->is_active()) { dv->show_and_wait_for_key(); }
#endif
								}
								else
								{
									// intersectection points are more or less in front of each other
									mgIPs = 2;
									variables.access<Vector3>(NAME(ip0)) = nip0.to_vector3();
									variables.access<Vector3>(NAME(ip1)) = nip1.to_vector3();
#ifdef WITH_DEBUG_VISUALISER
									dv->start_gathering_data();
									dv->clear();
									Framework::DebugVisualiserUtils::add_doors_to_debug_visualiser(dv, beInZone, dA, doorALoc, doorADir, dB, doorBLoc, doorBDir, doorWidth);
									dv->add_circle(nip0, 0.05f, Colour::greenDark);
									dv->add_circle(nip1, 0.05f, Colour::greenDark);
									dv->add_line(nip0, ip0, Colour::greenDark);
									dv->add_line(nip1, ip1, Colour::greenDark);
									dv->add_line(doorALoc, ip0, Colour::greenDark.with_alpha(0.5f));
									dv->add_line(doorBLoc, ip1, Colour::greenDark.with_alpha(0.5f));
									dv->add_text((nip0 + nip1) * 0.5f, String(TXT("2ip")), Colour::greenDark, 0.1f);
									dv->end_gathering_data();
									if (dv->is_active()) { dv->show_and_wait_for_key(); }
#endif
								}
								break;
							}
						}
						else
						{
							float off0 = rg.get_float(min0Dist, max0Dist);
							nip0 = doorALoc - doorADir * off0;
							if (randomGenerator.get_chance(0.1f))
							{
								nip0 = ip0;
							}
						}
					}
					else
					{
						float off1 = rg.get_float(min1Dist, max1Dist);
						nip1 = doorBLoc - doorBDir * off1;
						if (randomGenerator.get_chance(0.1f))
						{
							nip1 = ip1;
						}
					}
				}
				if (ipTriesLeft <= 0)
				{
					nip0 = doorALoc - doorADir * max0Dist;
					nip1 = doorBLoc - doorBDir * max1Dist;
				}
				if (mgIPs == NONE && !mg0ipWasOk)
				{
					mgIPs = 2;
					/*
					if (absYawDiff > 100.0f)
					{
						// safest
						nip0 = doorALoc - doorADir * halfDoorWidth;
						nip1 = doorBLoc - doorBDir * halfDoorWidth;
					}
					else
					*/
					{
						// furthest
						nip0 = doorALoc - doorADir * max0Dist;
						nip1 = doorBLoc - doorBDir * max1Dist;
					}
					variables.access<Vector3>(NAME(ip0)) = nip0.to_vector3();
					variables.access<Vector3>(NAME(ip1)) = nip1.to_vector3();
#ifdef WITH_DEBUG_VISUALISER
					dv->start_gathering_data();
					dv->clear();
					Framework::DebugVisualiserUtils::add_doors_to_debug_visualiser(dv, beInZone, dA, doorALoc, doorADir, dB, doorBLoc, doorBDir, doorWidth);
					dv->add_circle(nip0, 0.05f, Colour::greenDark);
					dv->add_circle(nip1, 0.05f, Colour::greenDark);
					dv->add_line(nip0, ip0, Colour::greenDark);
					dv->add_line(nip1, ip1, Colour::greenDark);
					dv->add_line(doorALoc, ip0, Colour::greenDark.with_alpha(0.5f));
					dv->add_line(doorBLoc, ip1, Colour::greenDark.with_alpha(0.5f));
					dv->add_text((nip0 + nip1) * 0.5f, String(TXT("2ip (o)")), Colour::greenDark, 0.1f);
					dv->end_gathering_data();
					if (dv->is_active()) { dv->show_and_wait_for_key(); }
#endif
				}
			}
		}
	}

	if (mgIPs == NONE && mg0ipWasOk &&
		corridorPreferStraight &&
		absYawDiff > 135.0f &&
		absYawDiff < 179.0f /* don't turn straight corridor into 2ip */)
	{
		// if straight corridors are preferred, change 0ip into 2ip
		Vector2 nip0 = doorALoc - doorADir * abs(relLocation.y) * 0.25f;
		Vector2 nip1 = doorBLoc - doorBDir * abs(relLocation.y) * 0.25f;
		variables.access<Vector3>(NAME(ip0)) = nip0.to_vector3();
		variables.access<Vector3>(NAME(ip1)) = nip1.to_vector3();
		mgIPs = 2;
#ifdef WITH_DEBUG_VISUALISER
		dv->start_gathering_data();
		dv->clear();
		Framework::DebugVisualiserUtils::add_doors_to_debug_visualiser(dv, beInZone, dA, doorALoc, doorADir, dB, doorBLoc, doorBDir, doorWidth);
		dv->add_circle(nip0, 0.05f, Colour::greenDark);
		dv->add_circle(nip1, 0.05f, Colour::greenDark);
		dv->add_text((nip0 + nip1) * 0.5f, String(TXT("turned 0ip into 2ip")), Colour::greenDark, 0.1f);
		dv->end_gathering_data();
		if (dv->is_active()) { dv->show_and_wait_for_key(); }
#endif
	}

	if (mgIPs == NONE && mg0ipWasOk)
	{
		mgIPs = 0;
#ifdef WITH_DEBUG_VISUALISER
		dv->start_gathering_data();
		dv->clear();
		Framework::DebugVisualiserUtils::add_doors_to_debug_visualiser(dv, beInZone, dA, doorALoc, doorADir, dB, doorBLoc, doorBDir, doorWidth);
		dv->add_text((doorALoc + doorBLoc) * 0.5f, String(TXT("was meant to be 0ip but for some reason it wasn't")), Colour::greenDark, 0.1f);
		dv->end_gathering_data();
		if (dv->is_active()) { dv->show_and_wait_for_key(); }
#endif
	}

	an_assert(mgIPs >= 0);

	bool result = false;
	if (mgIPs >= 0)
	{
		variables.access<int>(NAME(ipCount)) = mgIPs;

#ifdef OUTPUT_GENERATION
		{
			Transform t = variables.get_value<Transform>(NAME(doorA), Transform::identity);
			output(TXT("  doorA: %S %S %.9f"), t.get_translation().to_string().to_char(), t.get_orientation().to_rotator().to_string().to_char(), t.get_scale());
		}
		{
			Transform t = variables.get_value<Transform>(NAME(doorB), Transform::identity);
			output(TXT("  doorB: %S %S %.9f"), t.get_translation().to_string().to_char(), t.get_orientation().to_rotator().to_string().to_char(), t.get_scale());
		}
		{
			Transform ta = variables.get_value<Transform>(NAME(doorA), Transform::identity);
			Transform tb = variables.get_value<Transform>(NAME(doorB), Transform::identity);
			an_assert(ta.get_translation().z == tb.get_translation().z);
		}
		output(TXT("simple corridor IPs: %i"), mgIPs);
		if (mgIPs > 0)
		{
			output(TXT("  ip0: %S"), variables.get_value<Vector3>(NAME(ip0), Vector3::zero).to_string().to_char());
		}
		if (mgIPs > 1)
		{
			output(TXT("  ip1: %S"), variables.get_value<Vector3>(NAME(ip1), Vector3::zero).to_string().to_char());
		}
#endif

#ifdef OUTPUT_GENERATION_VARIABLES
		LogInfoContext log;
		log.log(TXT("generating mesh using:"));
		{
			LOG_INDENT(log);
			variables.log(log);
		}	
		log.output_to_output();
#endif
		result = true;

		Framework::MeshGenParam<Framework::LibraryName> mgResolved = info->corridorMeshGenerator;
		mgResolved.fill_value_with(variables);
		if (mgResolved.is_value_set())
		{
			if (auto* actualMG = ::Framework::Library::get_current()->get_mesh_generators().find(mgResolved.get()))
			{
				auto generatedMesh = actualMG->Framework::MeshGenerator::generate_temporary_library_mesh(::Framework::Library::get_current(),
					::Framework::MeshGeneratorRequest()
						.for_wmp_owner(_room)
						.no_lods()
						.using_parameters(variables)
						.using_random_regenerator(_room->get_individual_random_generator())
						.using_mesh_nodes(_roomGeneratingContext.meshNodes));
				if (generatedMesh.get())
				{
					_room->add_mesh(generatedMesh.get());
					Framework::MeshNode::copy_not_dropped_to(generatedMesh->get_mesh_nodes(), _roomGeneratingContext.meshNodes);
				}
				else
				{
					error(TXT("could not generate mesh"));
					result = false;
				}
			}
			else
			{
				error(TXT("no mesh generator named \"%S\" found"), mgResolved.get().to_string().to_char());
				result = false;
			}
		}
		else
		{
			if (mgResolved.get_value_param().is_set())
			{
				error(TXT("no mesh generator param resolved \"%S\""), mgResolved.get_value_param().get().to_char());
			}
			else
			{
				error(TXT("no mesh generator param?"));
			}
			result = false;
		}

		_roomGeneratingContext.spaceUsed.include(variables.get_value<Transform>(NAME(doorA), Transform::identity).get_translation());
		_roomGeneratingContext.spaceUsed.include(variables.get_value<Transform>(NAME(doorB), Transform::identity).get_translation());
		if (mgIPs > 0)
		{
			_roomGeneratingContext.spaceUsed.include(variables.get_value<Vector3>(NAME(ip0), Vector3::zero));
		}
		if (mgIPs > 1)
		{
			_roomGeneratingContext.spaceUsed.include(variables.get_value<Vector3>(NAME(ip1), Vector3::zero));
		}
		_roomGeneratingContext.spaceUsed.expand_by(halfDoorWidth * Vector3::xy);
	}
	else
	{
		error(TXT("could not find proper case"));
		an_assert(false);
		result = false;
#ifdef AN_DEVELOPMENT
#ifndef WITH_DEBUG_VISUALISER
		DebugVisualiserPtr dv(new DebugVisualiser(String::printf(TXT("simple corridor"))));
		dv->set_background_colour(Colour::lerp(0.05f, Colour::greyLight, Colour::green));
		dv->activate();
#else
		if (!debugThis)
		{
			dv->set_background_colour(Colour::lerp(0.05f, Colour::greyLight, Colour::green));
			dv->activate();
		}
#endif
		dv->start_gathering_data();
		dv->clear();
		Framework::DebugVisualiserUtils::add_doors_to_debug_visualiser(dv, beInZone, dA, doorALoc, doorADir, dB, doorBLoc, doorBDir, doorWidth);
		dv->add_text(Vector2::zero, String(TXT("FAILED")), Colour::red, 0.3f);
		dv->end_gathering_data();
		if (dv->is_active()) { dv->show_and_wait_for_key(); }
#endif
	}

	if (!result)
	{
		todo_note(TXT("temp fallback"));
		Framework::Material * floorMaterial = Framework::Library::get_current()->get_materials().find(Framework::LibraryName(String(TXT("materials:main"))));
		_room->generate_plane(floorMaterial, nullptr, !_room->get_environment() || !_room->get_environment()->get_info().get_sky_box_material());
	}

#ifdef OUTPUT_GENERATION
	output(TXT("generated simple corridor \"%S\""), _room->get_name().to_char());
#endif

	if (!_subGenerator)
	{
		_room->place_pending_doors_on_pois();
		_room->mark_vr_arranged();
		_room->mark_mesh_generated();
	}

	return result;
}


#include "meshGenShapes.h"

#include "..\..\pilgrimage\pilgrimageInstanceOpenWorld.h"

#include "..\..\roomGenerators\roomGenerators\platformsLinear.h"

#include "..\..\..\framework\framework.h"
#include "..\..\..\framework\meshGen\meshGenElementShape.h"
#include "..\..\..\framework\meshGen\meshGenGenerationContext.h"
#include "..\..\..\framework\meshGen\meshGenValueDefImpl.inl"
#include "..\..\..\framework\world\regionType.h"
#include "..\..\..\framework\world\roomType.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace MeshGeneration;
using namespace Shapes;

//

// meshgen input params
DEFINE_STATIC_NAME(smallerPlatformsLengthBy);
DEFINE_STATIC_NAME(anchorAbove);
DEFINE_STATIC_NAME(anchorBelow);

// meshgen params for sub elements
DEFINE_STATIC_NAME(platformSize);
DEFINE_STATIC_NAME(laneFrom);
DEFINE_STATIC_NAME(laneTo);

//

class PlatformsLinearData
: public ::Framework::MeshGeneration::ElementShapeData
{
	FAST_CAST_DECLARE(PlatformsLinearData);
	FAST_CAST_BASE(::Framework::MeshGeneration::ElementShapeData);
	FAST_CAST_END();

	typedef ::Framework::MeshGeneration::ElementShapeData base;

public:
	override_ bool load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc);
	override_ bool prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext);

public:
	RefCountObjectPtr<::Framework::MeshGeneration::Element> platformElement;
	RefCountObjectPtr<::Framework::MeshGeneration::Element> laneElement;
	RefCountObjectPtr<::Framework::MeshGeneration::Element> doorElement;
};

REGISTER_FOR_FAST_CAST(PlatformsLinearData);

//

::Framework::MeshGeneration::ElementShapeData* Shapes::create_platforms_linear_data()
{
	return new PlatformsLinearData();
}

//

bool Shapes::platforms_linear(::Framework::MeshGeneration::GenerationContext& _context, ::Framework::MeshGeneration::ElementInstance& _instance, ::Framework::MeshGeneration::ElementShapeData const* _data)
{
	auto* data = fast_cast<PlatformsLinearData>(_data);

	bool dataAvailable = false;
	RoomGenerators::PlatformsLinearPlatformsSetup const * plpPlatformsSetup = nullptr;
	RoomGenerators::PlatformsLinearVerticalAdjustment const * plpVerticalAdjusment = nullptr;
	float requestedCellSize = 0.0f;
	float tileSize = RoomGenerationInfo::get().get_tile_size();
	float smallerPlatformsLengthBy = _context.get_value(NAME(smallerPlatformsLengthBy), 0.0f);
	float platformHeight = hardcoded 2.0f;

	if (auto* piow = PilgrimageInstanceOpenWorld::get())
	{
		requestedCellSize = piow->get_cell_size_inner();
			
		auto* cellSet = piow->get_cell_set_for_cell_context();

		if (cellSet) // check if exists, it is possible that the cell does not exist if we switched mid through
		{
			Framework::RoomType const * mainRoomType = nullptr;
			if (auto* rt = cellSet->roomType.get())
			{
				mainRoomType = rt;
			}
			else if (auto* rt = cellSet->regionType.get())
			{
				mainRoomType = piow->get_main_room_type_for_cell_context();
			}
			if (mainRoomType)
			{
				if (auto* rgi = mainRoomType->get_sole_room_generator_info())
				{
					if (auto* plrgi = fast_cast<RoomGenerators::PlatformsLinearInfo>(rgi))
					{
						dataAvailable = true;
						plpPlatformsSetup = &plrgi->get_platforms_setup();
						plpVerticalAdjusment = &plrgi->get_vertical_adjustment();
					}
				}
			}
		}
	}

	if (!dataAvailable)
	{
#ifdef AN_DEVELOPMENT
		if (Framework::is_preview_game())
		{
			static RoomGenerators::PlatformsLinearPlatformsSetup aPlatformsSetup;
			static RoomGenerators::PlatformsLinearVerticalAdjustment aVerticalAdjusment;

			aPlatformsSetup.lines = RangeInt(2);
			aPlatformsSetup.width = RangeInt(2);

			aVerticalAdjusment.yA = Range(-0.8f, -0.7f);
			aVerticalAdjusment.yB = Range(0.0f, 0.1f);
			aVerticalAdjusment.xA = Range(-0.1f, 0.1f);
			aVerticalAdjusment.xB = Range(-0.2f, 0.1f);
			aVerticalAdjusment.hPt = Range(0.3f, 0.4f);
			aVerticalAdjusment.hLimit = Range(1.0f, 5.0f);

			plpPlatformsSetup = &aPlatformsSetup;
			plpVerticalAdjusment = &aVerticalAdjusment;

			requestedCellSize = 30.0f;
		}
		else
#endif
		{
			error_generating_mesh(_instance, TXT("no data available for platforms linear"));

			// use anything
			static RoomGenerators::PlatformsLinearPlatformsSetup aPlatformsSetup;
			static RoomGenerators::PlatformsLinearVerticalAdjustment aVerticalAdjusment;

			aPlatformsSetup.lines = RangeInt(2);
			aPlatformsSetup.width = RangeInt(2);

			aVerticalAdjusment.yA = Range(-0.8f, -0.7f);
			aVerticalAdjusment.yB = Range(0.0f, 0.1f);
			aVerticalAdjusment.xA = Range(-0.1f, 0.1f);
			aVerticalAdjusment.xB = Range(-0.2f, 0.1f);
			aVerticalAdjusment.hPt = Range(0.3f, 0.4f);
			aVerticalAdjusment.hLimit = Range(1.0f, 5.0f);

			plpPlatformsSetup = &aPlatformsSetup;
			plpVerticalAdjusment = &aVerticalAdjusment;

			requestedCellSize = 30.0f;
		}
	}

	requestedCellSize -= smallerPlatformsLengthBy;

	Vector2 platformSize = Vector2::one * tileSize;
	platformSize = plpPlatformsSetup->maxPlayAreaSize.get_with_default(_context, platformSize);
	{
		Vector2 maxTileSize = plpPlatformsSetup->maxPlayAreaTileSize.get_with_default(_context, Vector2::zero);
		platformSize.x = max(platformSize.x, maxTileSize.x * tileSize);
		platformSize.y = max(platformSize.y, maxTileSize.y * tileSize);
	}
	Vector2 entrancePlatformSize = Vector2(platformSize.x, tileSize);

	Vector2 platformSeparation = Vector2::one * tileSize * 4.0f;
	platformSeparation = plpPlatformsSetup->platformSeparation.get_with_default(_context, platformSeparation.to_vector3()).to_vector2();

	Random::Generator random = _context.get_random_generator();
	float xA = random.get(plpVerticalAdjusment->xA.get_with_default(_context, Range(0.0f)));
	float xB = random.get(plpVerticalAdjusment->xB.get_with_default(_context, Range(0.0f)));
	float yA = random.get(plpVerticalAdjusment->yA.get_with_default(_context, Range(0.0f)));
	float yB = random.get(plpVerticalAdjusment->yB.get_with_default(_context, Range(0.0f)));
	Optional<Range> hPt;
	if (plpVerticalAdjusment->hPt.is_set())
	{
		hPt = plpVerticalAdjusment->hPt.get_with_default(_context, Range(0.0f));
	}
	Optional<Range> hLimit;
	if (plpVerticalAdjusment->hLimit.is_set())
	{
		hLimit = plpVerticalAdjusment->hLimit.get_with_default(_context, Range(0.0f));
	}

	int width = random.get(plpPlatformsSetup->width.get_with_default(_context, RangeInt(2)));
	int lines = random.get(plpPlatformsSetup->lines.get_with_default(_context, RangeInt(2)));
	lines += 2; // for entrances
	lines = lines * 5 / 3;

	struct Platform
	{
		VectorInt2 coord;
		Vector3 at;
		bool entrance = false;

		Platform() {}
		explicit Platform(VectorInt2 const& _coord = VectorInt2::zero, bool _entrance = false) : coord(_coord), entrance(_entrance) {}
	};
	Array<Platform> platforms;

	// mockup lanes
	{
		platforms.push_back(Platform(VectorInt2(0, 0), true));
		platforms.push_back(Platform(VectorInt2(0, lines - 1), true));

		int xCentre = random.get_int(width);

		for_range(int, y, 1, lines - 2)
		{
			platforms.push_back(Platform(VectorInt2(0, y)));

			for_range(int, x, 0, width - 1)
			{
				if (x != xCentre && random.get_chance(0.9f))
				{
					platforms.push_back(Platform(VectorInt2(x - xCentre, y)));
				}
			}
		}
	}

	// calculate locations of platforms
	{
		Vector2 spacing;
		float yBetweenEntrances = requestedCellSize - entrancePlatformSize.y;
		spacing.x = platformSize.x + platformSeparation.x;
		spacing.y = yBetweenEntrances / (float)(lines - 1);
		for_every(platform, platforms)
		{
			platform->at.x = spacing.x * (float)platform->coord.x;
			platform->at.y = spacing.y * (float)platform->coord.y - yBetweenEntrances * 0.5f;
			platform->at.z = 0.0f;
		}

		Range3 allPlatformsBBox = Range3::empty;
		for_every(platform, platforms)
		{
			allPlatformsBBox.include(platform->at);
		}

		float h = 0.0f;
		if (hPt.is_set())
		{
			h = hPt.get().centre() * max(allPlatformsBBox.x.length(), allPlatformsBBox.y.length());
			if (hLimit.is_set())
			{
				h = clamp(h, hLimit.get().min, hLimit.get().max);
			}
		}
		else
		{
			if (hLimit.is_set())
			{
				h = random.get(hLimit.get());
			}
		}

		for_every(platform, platforms)
		{
			Vector2 normalisedAt = Vector2(allPlatformsBBox.x.get_pt_from_value(platform->at.x),
										   allPlatformsBBox.y.get_pt_from_value(platform->at.y));

			normalisedAt = (normalisedAt - Vector2::half) * 2.0f;

			platform->at.z += h * (xA * sqr(normalisedAt.x) + xB * normalisedAt.x);
			platform->at.z += h * (yA * sqr(normalisedAt.y) + yB * normalisedAt.y);
		}

		Range3 spaceOccupied = Range3::empty;
		for_every(platform, platforms)
		{
			spaceOccupied.include(platform->at);
			spaceOccupied.include(platform->at + Vector3::zAxis * platformHeight);
		}

		float moveBy = 0.0f;
		{
			Range anchorAbove = _context.get_value(NAME(anchorAbove), Range::empty);
			Range anchorBelow = _context.get_value(NAME(anchorBelow), Range::empty);

			if (!anchorAbove.is_empty())
			{
				moveBy = -(spaceOccupied.z.max + random.get(anchorAbove));
			}
			if (!anchorBelow.is_empty())
			{
				moveBy = -(spaceOccupied.z.min - random.get(anchorBelow));
			}
		}
		for_every(platform, platforms)
		{
			platform->at.z += moveBy; // level out
		}
	}

	struct Lane
	{
		Vector3 from;
		Vector3 to;

		Lane() {}
		Lane(Vector3 const& _from, Vector3 const& _to) : from(_from), to(_to) {}
	};
	Array<Lane> lanes;

	// create lanes between platforms
	{
		for_every(p, platforms)
		{
			for_every(o, platforms)
			{
				if (o->coord.x == p->coord.x + 1 && o->coord.y == p->coord.y && random.get_chance(0.95f))
				{
					lanes.push_back(Lane(p->at, o->at));
				}
				if (o->coord.y == p->coord.y + 1 && o->coord.x == p->coord.x)
				{
					lanes.push_back(Lane(p->at, o->at));
				}
			}
		}
	}

	//

	// generate platforms
	if (auto* platformElement = data->platformElement.get())
	{
		SimpleVariableStorage platformVariables;
		for_every(platform, platforms)
		{
			Vector3 fullSize = (platform->entrance ? entrancePlatformSize : platformSize).to_vector3(platformHeight);
			Vector3 size;
			if (platform->entrance)
			{
				size = fullSize;
			}
			else
			{
				size.x = random.get_float(tileSize, fullSize.x);
				size.y = random.get_float(tileSize, fullSize.y);
				size.z = fullSize.z;
			}
			Vector2 allowedOffset = Vector2(fullSize.x - size.x, fullSize.y - size.y);
			if (platform->entrance)
			{
				allowedOffset.x = max(0.0f, allowedOffset.x - tileSize); // we need to have full door
			}
			Vector3 at = platform->at;
			at.x += random.get_float(-0.5f, 0.5f) * allowedOffset.x;
			at.y += random.get_float(-0.5f, 0.5f) * allowedOffset.y;
			platformVariables.access<Vector3>(NAME(platformSize)) = size;
			::Framework::MeshGeneration::Checkpoint::generate_with_checkpoint(_context, _instance,
				for_everys_index(platform), platformElement, &platformVariables,
				Transform(at, Quat::identity));
		}
	}

	// generate lanes
	if (auto* laneElement = data->laneElement.get())
	{
		SimpleVariableStorage laneVariables;
		for_every(lane, lanes)
		{
			laneVariables.access<Vector3>(NAME(laneFrom)) = lane->from;
			laneVariables.access<Vector3>(NAME(laneTo)) = lane->to;
			::Framework::MeshGeneration::Checkpoint::generate_with_checkpoint(_context, _instance,
				for_everys_index(lane), laneElement, &laneVariables,
				Transform::identity);
		}
	}

	// generate doors
	if (auto* doorElement = data->doorElement.get())
	{
		for_every(platform, platforms)
		{
			if (platform->entrance)
			{
				Transform placement;
				if (platform->coord.y == 0)
				{
					placement = look_matrix(platform->at - Vector3::yAxis * (entrancePlatformSize.y * 0.5f), -Vector3::yAxis, Vector3::zAxis).to_transform();
				}
				else
				{
					placement = look_matrix(platform->at + Vector3::yAxis * (entrancePlatformSize.y * 0.5f), Vector3::yAxis, Vector3::zAxis).to_transform();
				}
				::Framework::MeshGeneration::Checkpoint::generate_with_checkpoint(_context, _instance,
					for_everys_index(platform), doorElement, nullptr,
					placement);
			}
		}
	}

	return true;
}

//

#define LOAD_ELEMENT(memberVariable, what) \
	for_every(node, _node->children_named(what)) \
	{ \
		if (auto* childNode = node->first_child()) \
		{ \
			if (auto* element = ::Framework::MeshGeneration::Element::create_from_xml(childNode, _lc)) \
			{ \
				if (element->load_from_xml(childNode, _lc)) \
				{ \
					warn_loading_xml_on_assert(!memberVariable.is_set(), node, TXT("reloading %S element"), what); \
					memberVariable = element; \
				} \
				else \
				{ \
					error_loading_xml(node, TXT("problem loading %S element"), what); \
					result = false; \
				} \
			} \
		} \
	}

bool PlatformsLinearData::load_from_xml(IO::XML::Node const* _node, ::Framework::LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	LOAD_ELEMENT(platformElement, TXT("platformElement"));
	LOAD_ELEMENT(laneElement, TXT("laneElement"));
	LOAD_ELEMENT(doorElement, TXT("doorElement"));

	return result;
}

#define PREPARE_ELEMENT(memberVariable) \
	if (auto* e = memberVariable.get()) \
	{ \
		result &= e->prepare_for_game(_library, _pfgContext); \
	}

bool PlatformsLinearData::prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	PREPARE_ELEMENT(platformElement);
	PREPARE_ELEMENT(laneElement);
	PREPARE_ELEMENT(doorElement);

	return result;
}

#include "balconies.h"

#include "..\roomGenerationInfo.h"
#include "..\roomGeneratorUtils.h"

#include "..\..\utils.h"

#include "..\..\pilgrimage\pilgrimageInstanceOpenWorld.h"

#include "..\..\..\core\containers\arrayStack.h"
#include "..\..\..\core\debug\debugVisualiser.h"

#include "..\..\..\framework\debug\debugVisualiserUtils.h"
#include "..\..\..\framework\game\delayedObjectCreation.h"
#include "..\..\..\framework\game\game.h"
#include "..\..\..\framework\library\library.h"
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
#include "..\..\..\core\pieceCombiner\pieceCombinerImplementation.h"
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

#ifdef AN_OUTPUT_WORLD_GENERATION
#define OUTPUT_GENERATION
//#define OUTPUT_GENERATION_DEBUG_VISUALISER
//#define OUTPUT_GENERATION_VARIABLES
//#define OUTPUT_OPEN_WORLD_BALCONIES
#else
#ifdef LOG_WORLD_GENERATION
#define OUTPUT_GENERATION
#endif
#endif

//

DEFINE_STATIC_NAME(first);
DEFINE_STATIC_NAME(last);
DEFINE_STATIC_NAME(at);
DEFINE_STATIC_NAME(to);
DEFINE_STATIC_NAME(dirAtTo); // -1 or 1
DEFINE_STATIC_NAME(balconiesMaxSize);
DEFINE_STATIC_NAME(balconiesMaxTileSize);
DEFINE_STATIC_NAME(balconiesAlignToTiles);
DEFINE_STATIC_NAME(balconySize);
DEFINE_STATIC_NAME(balconyTileSize);
DEFINE_STATIC_NAME(balconyMeshGenerator);

// mesh nodes and mesh nodes variables
DEFINE_STATIC_NAME(randomSeed);
DEFINE_STATIC_NAME(balcony);
DEFINE_STATIC_NAME_STR(fakeBalcony, TXT("fake balcony"));
DEFINE_STATIC_NAME(balconyCount);
DEFINE_STATIC_NAME(fakeBalconyPreCount);
DEFINE_STATIC_NAME(fakeBalconyPostCount);
DEFINE_STATIC_NAME(maxBalconyWidth);
DEFINE_STATIC_NAME(index);
DEFINE_STATIC_NAME(availableSpaceForBalcony);
DEFINE_STATIC_NAME(balconyDoorWidth);
DEFINE_STATIC_NAME(balconyDoorHeight);
DEFINE_STATIC_NAME(wall);
DEFINE_STATIC_NAME(swapDoors);
DEFINE_STATIC_NAME_STR(leftDoor, TXT("left door"));
DEFINE_STATIC_NAME_STR(rightDoor, TXT("right door"));
DEFINE_STATIC_NAME_STR(availableSpaceCentre, TXT("available space centre")); // to know where it is placed in the world (will match node provided by the layout)
DEFINE_STATIC_NAME_STR(availableSpaceCorner, TXT("available space corner")); // to know the actual size of the balcony - this + doors will tell how much vr space is taken and the system will automatically try to fit it into the world

//

static bool fill_connector(REF_ PieceCombiner::Generator<Framework::RegionGenerator>& _generator, OUT_ PieceCombiner::Connector<Framework::RegionGenerator>& _connector, Array< PieceCombiner::Connector<Framework::RegionGenerator>> const& _connectors)
{
	for_every(c, _connectors)
	{
		if (c->check_required_parameters(&_generator))
		{
			_connector = *c;
			return true;
		}
	}
	return false;
}

//

void BalconiesPrepareVariablesContext::process()
{
	an_assert(variables || wmpOwner);
	an_assert(roomGenerationInfo);

	maxSize = Utils::get_value_from(NAME(balconiesMaxSize), Vector2::zero, variables, wmpOwner);
	maxTileSize = Utils::get_value_from(NAME(balconiesMaxTileSize), Vector2::zero, variables, wmpOwner);
	bool alignToTiles = Utils::get_value_from(NAME(balconiesAlignToTiles), false, variables, wmpOwner);

	tileSize = roomGenerationInfo->get_tile_size();

	// auto params
	if (!maxTileSize.is_zero())
	{
		if (!maxSize.is_zero())
		{
			maxSize.x = min(maxSize.x, max(2.0f, maxTileSize.x) * tileSize);
			maxSize.y = min(maxSize.y, max(2.0f, maxTileSize.y) * tileSize);
		}
		else
		{
			maxSize.x = max(2.0f, maxTileSize.x) * tileSize;
			maxSize.y = max(2.0f, maxTileSize.y) * tileSize;
		}
	}

	if (maxSize.is_zero())
	{
		maxSize = Vector2(1.0f * tileSize, 1.0f * tileSize);
	}

	maxSize.x = max(maxSize.x, 2.0f * tileSize);
	maxSize.y = max(maxSize.y, 2.0f * tileSize);

	// keep it inside
	maxSize.x = min(maxSize.x, roomGenerationInfo->get_play_area_rect_size().x);
	maxSize.y = min(maxSize.y, roomGenerationInfo->get_play_area_rect_size().y);

	availableSpaceForBalcony.x = maxSize.x;
	availableSpaceForBalcony.y = maxSize.y;

	if (alignToTiles)
	{
		availableSpaceForBalcony.x = round_to(availableSpaceForBalcony.x, tileSize);
		availableSpaceForBalcony.y = round_to(availableSpaceForBalcony.y, tileSize);
	}

	int iterationIdx = 0;
	// have enough place for doors
	while (availableSpaceForBalcony.x > maxSize.x ||
		   availableSpaceForBalcony.y > maxSize.y)
	{
		// rounding errors
		if (availableSpaceForBalcony.x > availableSpaceForBalcony.y - 0.01f &&
			availableSpaceForBalcony.x >= tileSize * 2.0f + 0.02f)
		{
			// prefer more square balconies
			availableSpaceForBalcony.x -= tileSize;
		}
		else
		{
			availableSpaceForBalcony.y -= tileSize;
		}
		// fail safe when too small size has been reached
		if (availableSpaceForBalcony.x <= tileSize * 2.0f + 0.02f &&
			availableSpaceForBalcony.y <= tileSize * 2.0f + 0.02f)
		{
			break;
		}
		// absolute fail safe
		++iterationIdx;
		if (iterationIdx > 1000)
		{
			availableSpaceForBalcony.x = tileSize * 2.0f;
			availableSpaceForBalcony.y = tileSize * 2.0f;
			break;
		}
	}

	if (alignToTiles)
	{
		availableSpaceForBalcony.x = round_to(availableSpaceForBalcony.x, tileSize);
		availableSpaceForBalcony.y = round_to(availableSpaceForBalcony.y, tileSize);
	}

	// keep it at least 2x2 tiles so two doors fit (on one side or corner)
	availableSpaceForBalcony.x = max(availableSpaceForBalcony.x, tileSize * 2.0f);
	availableSpaceForBalcony.y = max(availableSpaceForBalcony.y, tileSize * 2.0f);

	balconyDoorWidth = tileSize;
	balconyDoorHeight = roomGenerationInfo->get_door_height();

	maxBalconyWidth = maxSize.x;
}

//

REGISTER_FOR_FAST_CAST(BalconiesInfo);

BalconiesInfo::BalconiesInfo()
{
}

BalconiesInfo::~BalconiesInfo()
{
}

bool BalconiesInfo::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= balconyMeshGenerator.load_from_xml(_node, TXT("balconyMeshGenerator"));
	result &= fakeBalconyMeshGenerator.load_from_xml(_node, TXT("fakeBalconyMeshGenerator"));
	result &= mainMeshGenerator.load_from_xml(_node, TXT("mainMeshGenerator"));

	_lc.load_group_into(balconyMeshGenerator);
	_lc.load_group_into(fakeBalconyMeshGenerator);
	_lc.load_group_into(mainMeshGenerator);

	error_loading_xml_on_assert(balconyMeshGenerator.is_set(), result, _node, TXT("no balconyMeshGenerator provided"));
	error_loading_xml_on_assert(mainMeshGenerator.is_set(), result, _node, TXT("no mainMeshGenerator provided"));

	for_every(node, _node->children_named(TXT("environmentAnchor")))
	{
		setAnchorVariable = node->get_name_attribute_or_from_child(TXT("variableName"), setAnchorVariable);
		setAnchorVariable = node->get_name_attribute_or_from_child(TXT("setVariableName"), setAnchorVariable);
		getAnchorPOI = node->get_name_attribute_or_from_child(TXT("getFromPOI"), getAnchorPOI);
		roomCentrePOI = node->get_name_attribute_or_from_child(TXT("roomCentrePOI"), roomCentrePOI);
	}

	for_every(regionPieceNode, _node->children_named(TXT("asRegionPiece")))
	{
		dontModifyBalconyCount = regionPieceNode->get_bool_attribute_or_from_child_presence(TXT("dontModifyBalconyCount"), dontModifyBalconyCount);
		result &= balconyCount.load_from_xml(regionPieceNode, TXT("balconyCount"));
		result &= fakeBalconyPreCount.load_from_xml(regionPieceNode, TXT("fakeBalconyPreCount"));
		result &= fakeBalconyPostCount.load_from_xml(regionPieceNode, TXT("fakeBalconyPostCount"));

		ignoreAllConnectorTags = regionPieceNode->get_bool_attribute_or_from_child_presence(TXT("ignoreAllConnectorTags"), ignoreAllConnectorTags);
		for_every(node, regionPieceNode->children_named(TXT("ignore")))
		{
			Name ct = regionPieceNode->get_name_attribute(TXT("connectorTag"));
			if (ct.is_valid())
			{
				ignoreConnectorTags.push_back(ct);
			}
			else
			{
				error_loading_xml(node, TXT("no connectorTag set for ignore"));
				result = false;
			}
		}

		Framework::RegionGenerator::LoadingContext lc(_lc);
		for_every(node, regionPieceNode->children_named(TXT("connector")))
		{
			PieceCombiner::Connector<Framework::RegionGenerator> connector;
			if (connector.load_from_xml(node, &lc))
			{
				connector.be_normal_connector();
				connectors.push_back(connector);
			}
			else
			{
				error_loading_xml(node, TXT("couldn't load a connector"));
				result = false;
			}
		}

		balconyConnectorsLooped = regionPieceNode->get_bool_attribute_or_from_child_presence(TXT("balconiesLooped"), balconyConnectorsLooped);
		balconyConnectorsLooped = ! regionPieceNode->get_bool_attribute_or_from_child_presence(TXT("balconiesNotLooped"), ! balconyConnectorsLooped);
		for_every(node, regionPieceNode->children_named(TXT("balconiesNotLooped")))
		{
			if (auto* attr = node->get_attribute(TXT("openWorldBreakerLocalDir")))
			{
				openWorldBreakerLocalDir = DirFourClockwise::parse(attr->get_as_string());
			}
		}
		balconyConnectorsClockwise = regionPieceNode->get_bool_attribute_or_from_child_presence(TXT("balconiesClockwise"), balconyConnectorsClockwise);
		balconyConnectorsClockwise = !regionPieceNode->get_bool_attribute_or_from_child_presence(TXT("balconiesCounterClockwise"), !balconyConnectorsClockwise);
		maySkipFirstLastConnectors = regionPieceNode->get_bool_attribute_or_from_child_presence(TXT("balconicesMaySkipFirstLastConnectors"), maySkipFirstLastConnectors);

		balconyConnectors.clear();
		for_every(node, regionPieceNode->children_named(TXT("balconyConnectors")))
		{
			BalconyConnectors bc;
			if (auto* attr = node->get_attribute(TXT("looped")))
			{
				warn_loading_xml(node, TXT("\"looped\" is deprecated"));
				balconyConnectorsLooped = attr->get_as_bool();
			}
			for_every(n, node->children_named(TXT("localDir")))
			{
				if (auto* attr = n->get_attribute(TXT("is")))
				{
					bc.localDirIs.push_back(DirFourClockwise::parse(attr->get_as_string()));
				}
				else if (auto* attr = n->get_attribute(TXT("not")))
				{
					bc.localDirNot.push_back(DirFourClockwise::parse(attr->get_as_string()));
				}
				else
				{
					error_loading_xml(n, TXT("localDir without \"is\" or \"not\""));
					result = false;
				}
			}
			bc.priorityStartsAt = node->get_float_attribute_or_from_child(TXT("priorityStartsAt"), bc.priorityStartsAt);
			for_every(n, node->children_named(TXT("a")))
			{
				warn_loading_xml(n, TXT("please use \"next\", \"a\" is deprecated"));
				PieceCombiner::Connector<Framework::RegionGenerator> c;
				if (c.load_from_xml(n, &lc))
				{
					c.be_normal_connector();
					bc.next.push_back(c);
				}
				else
				{
					result = false;
				}
			}
			for_every(n, node->children_named(TXT("next")))
			{
				PieceCombiner::Connector<Framework::RegionGenerator> c;
				if (c.load_from_xml(n, &lc))
				{
					c.be_normal_connector();
					bc.next.push_back(c);
				}
				else
				{
					result = false;
				}
			}
			for_every(n, node->children_named(TXT("b")))
			{
				warn_loading_xml(n, TXT("please use \"prev\", \"b\" is deprecated"));
				PieceCombiner::Connector<Framework::RegionGenerator> c;
				if (c.load_from_xml(n, &lc))
				{
					c.be_normal_connector();
					bc.next.push_back(c);
				}
				else
				{
					result = false;
				}
			}
			for_every(n, node->children_named(TXT("prev")))
			{
				PieceCombiner::Connector<Framework::RegionGenerator> c;
				if (c.load_from_xml(n, &lc))
				{
					c.be_normal_connector();
					bc.prev.push_back(c);
				}
				else
				{
					result = false;
				}
			}
			for_every(n, node->children_named(TXT("first")))
			{
				PieceCombiner::Connector<Framework::RegionGenerator> c;
				if (c.load_from_xml(n, &lc))
				{
					c.be_normal_connector();
					bc.first.push_back(c);
				}
				else
				{
					result = false;
				}
			}
			for_every(n, node->children_named(TXT("last")))
			{
				PieceCombiner::Connector<Framework::RegionGenerator> c;
				if (c.load_from_xml(n, &lc))
				{
					c.be_normal_connector();
					bc.last.push_back(c);
				}
				else
				{
					result = false;
				}
			}
			balconyConnectors.push_back(bc);
		}
	}

	error_loading_xml_on_assert(!balconyConnectors.is_empty(), result, _node, TXT("no balcony connectors provided!"));

	return result;
}

bool BalconiesInfo::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	Framework::RegionGenerator::PreparingContext pc(_library);
	for_every(connector, connectors)
	{
		result &= connector->prepare(nullptr, &pc);
	}
	for_every(bc, balconyConnectors)
	{
		for_every(c, bc->next)
		{
			result &= c->prepare(nullptr, &pc);
		}
		for_every(c, bc->prev)
		{
			result &= c->prepare(nullptr, &pc);
		}
		for_every(c, bc->first)
		{
			result &= c->prepare(nullptr, &pc);
		}
		for_every(c, bc->last)
		{
			result &= c->prepare(nullptr, &pc);
		}
	}

	return result;
}

bool BalconiesInfo::apply_renamer(Framework::LibraryStoredRenamer const & _renamer, Framework::Library* _library)
{
	bool result = base::apply_renamer(_renamer, _library);

	return result;
}

Framework::RoomGeneratorInfoPtr BalconiesInfo::create_copy() const
{
	BalconiesInfo * copy = new BalconiesInfo();
	*copy = *this;
	return Framework::RoomGeneratorInfoPtr(copy);
}

RefCountObjectPtr<PieceCombiner::Piece<Framework::RegionGenerator>> BalconiesInfo::generate_piece_for_region_generator(REF_ PieceCombiner::Generator<Framework::RegionGenerator>& _generator, Framework::Region* _region, REF_ Random::Generator & _randomGenerator) const
{
	RefCountObjectPtr<PieceCombiner::Piece<Framework::RegionGenerator>> ptr;

	ptr = new PieceCombiner::Piece<Framework::RegionGenerator>();

	for_every(connector, connectors)
	{
		ptr->add_connector(*connector);
	}

	// create pairs of connectors for all balconies
	//
	// note that connector indices are not as below (in/out, first/last are before next/prev pairs)
	//
	//	non looped
	//				0			1			2				passage = balconies count - 1
	//		+-0-+		+-1-+		+-2-+		+-3-+		balcony
	//	  in0   1		2   3		4   5		6	7out	connector (first/last + next/prev) 
	//      0   1           2           3           4		connector tags = balconies count + 1
	//
	//	looped
	//	3			0			1			2			3	passage = balconies count
	//		+-0-+		+-1-+		+-2-+		+-3-+		balcony
	//	    0   1		2   3		4   5		6	7		connector (next/prev only)
	//          0           1           2           3		connector tags = balconies count
	int useBalconyCount = dontModifyBalconyCount? 0 : balconyCount.get(_randomGenerator);
	int providedBalconyCount = 0;
	if (_region)
	{
		providedBalconyCount = _region->get_variables().get_value<int>(NAME(balconyCount), useBalconyCount);
		useBalconyCount = providedBalconyCount;
	}
	bool useLoopedBalconies = balconyConnectorsLooped;
	if (!useLoopedBalconies && _generator.get_outer_connector_count() == 1 && !maySkipFirstLastConnectors)
	{
		warn(TXT("balconies generator for non looped balconies requires at least 2 outer connectors, only 1 provided, switching to looped. or use \"maySkipFirstLastConnectors\""));
		useLoopedBalconies = true;
	}
	useBalconyCount = max(useBalconyCount, useLoopedBalconies ? _generator.get_outer_connector_count() // for looped three exits we need three balconies - three passages between them
															  : _generator.get_outer_connector_count() - 1); // for non-looped three exits we need two balconies and a passage between them
	if (dontModifyBalconyCount && useBalconyCount != providedBalconyCount)
	{
		warn(TXT("provided balcony count different than required minimum! note that this might be fine as long as we have other ways to handle such exits (for tea for god those are \"special\" that connect to passages"));
		useBalconyCount = providedBalconyCount;
	}
	int connectorTagsRequired = useLoopedBalconies ? useBalconyCount : useBalconyCount + 1;
	int connectorTagsOffset = useLoopedBalconies ? 0 : 1;

	ARRAY_PREALLOC_SIZE(Name, outerConnectorsRequireConnectorTag, _generator.get_outer_connector_count());
	// get all connectors, so we can place them evenly among the balconies
	if (!ignoreAllConnectorTags)
	{
		RoomGeneratorUtils::gather_outer_connector_tags(outerConnectorsRequireConnectorTag, _generator, &ignoreConnectorTags);
	}
	ARRAY_PREALLOC_SIZE(Name, balconyProvideConnectorTag, connectorTagsRequired);
	ARRAY_PREALLOC_SIZE(DirFourClockwise::Type, balconyProvideDir, connectorTagsRequired);
	if (!outerConnectorsRequireConnectorTag.is_empty())
	{
		if (auto* piow = PilgrimageInstanceOpenWorld::get())
		{
			Optional<DirFourClockwise::Type> worldBreakerDir;
			Optional<DirFourClockwise::Type> preferredDir;
			if (openWorldBreakerLocalDir.is_set())
			{
				worldBreakerDir = piow->get_dir_to_world_for_cell_context(openWorldBreakerLocalDir.get());
			}
			else if (useLoopedBalconies)
			{
				preferredDir = piow->get_dir_for_cell_context();
			}
			PilgrimageInstanceOpenWorld::distribute_connector_tags_evenly(outerConnectorsRequireConnectorTag, OUT_ balconyProvideConnectorTag, OUT_ balconyProvideDir, 
				useLoopedBalconies, worldBreakerDir, preferredDir, connectorTagsRequired, balconyConnectorsClockwise? PilgrimageInstanceOpenWorldConnectorTagDistribution::Clockwise : PilgrimageInstanceOpenWorldConnectorTagDistribution::CounterClockwise,
				_region->get_individual_random_generator());
			if (!useLoopedBalconies && !maySkipFirstLastConnectors)
			{
				// we need connector tags at both ends
				PilgrimageInstanceOpenWorld::have_connector_tags_at_ends(balconyProvideConnectorTag);
			}
		}
		else
		{
			warn(TXT("no way to organise connector tags, may fail generation"))
			todo_implement;
		}
	}

	// first/last, at first and last balcony, add exits
	if (!useLoopedBalconies)
	{
		PieceCombiner::Connector<Framework::RegionGenerator> connectorFirst;
		PieceCombiner::Connector<Framework::RegionGenerator> connectorLast;

		auto& bcFirst = choose_balcony_connectors(balconyProvideDir.is_index_valid(0) ? Optional<DirFourClockwise::Type>(balconyProvideDir[0]) : NP, _randomGenerator);
		auto& bcLast = choose_balcony_connectors(balconyProvideDir.is_index_valid(useBalconyCount) ? Optional<DirFourClockwise::Type>(balconyProvideDir[useBalconyCount]) : NP, _randomGenerator);

		if (fill_connector(_generator, connectorFirst, bcFirst.first) &&
			fill_connector(_generator, connectorLast, bcLast.last))
		{
			if (!connectorFirst.name.is_valid())
			{
				connectorFirst.name = NAME(first);
			}
			if (!connectorLast.name.is_valid())
			{
				connectorLast.name = NAME(last);
			}

			connectorFirst.def.doorInRoomTags.set_tag(NAME(first));
			connectorLast.def.doorInRoomTags.set_tag(NAME(last));

			// set connector tags, one may exist or both, this should lead directly 
			if (balconyProvideConnectorTag.is_index_valid(0) &&
				balconyProvideConnectorTag[0].is_valid())
			{
				connectorFirst.connectorTag = balconyProvideConnectorTag[0];
				ptr->add_connector(connectorFirst);
			}
			else if (!maySkipFirstLastConnectors)
			{
				error(TXT("did not provide first connector, maybe use \"maySkipFirstLastConnectors\"?"));
			}

			if (balconyProvideConnectorTag.is_index_valid(useBalconyCount) &&
				balconyProvideConnectorTag[useBalconyCount].is_valid())
			{
				connectorLast.connectorTag = balconyProvideConnectorTag[useBalconyCount];
				ptr->add_connector(connectorLast);
			}
			else if (!maySkipFirstLastConnectors)
			{
				error(TXT("did not provide last connector, maybe use \"maySkipFirstLastConnectors\"?"));
			}

		}
		else
		{
			error(TXT("requires first and last valid connectors"));
		}
	}

	// create connectors for passages between balconies
	for_count(int, i, (useLoopedBalconies ? useBalconyCount : useBalconyCount - 1)) // passage count
	{
		int ni = (i + 1) % useBalconyCount;
		PieceCombiner::Connector<Framework::RegionGenerator> connectorNext;
		PieceCombiner::Connector<Framework::RegionGenerator> connectorPrev;

		auto& bc = choose_balcony_connectors(balconyProvideDir.is_index_valid(i + connectorTagsOffset) ? Optional<DirFourClockwise::Type>(balconyProvideDir[i + connectorTagsOffset]) : NP, _randomGenerator);

		if (fill_connector(_generator, connectorNext, bc.next) &&
			fill_connector(_generator, connectorPrev, bc.prev))
		{
			/*
				based on prototype

					group by priority to have same piece connector group handled nicely
					<connector name="0*-1" priority="1000" samePieceConnectorGroup="0-1" doorInRoomTags="at=0 to=1" doorType="testVR:door" plug="ring" canCreateNewPieces="yes"/>
					<connector name="0-*1" priority="1000" samePieceConnectorGroup="0-1" doorInRoomTags="at=1 to=0" doorType="testVR:door" socket="ring" canCreateNewPieces="no"/>

					except for first and last (unless looped, then we add them too)
			 */

			// connectorNext is on previous pointing at us, it is NEXT on previous balcony
			//	in the code it is (as you can see below) 0*-1

			connectorNext.priority = bc.priorityStartsAt + (float)i;
			connectorPrev.priority = bc.priorityStartsAt + (float)i;

			connectorNext.name = Name(String::printf(TXT("%i*-%i"), i, ni));
			connectorPrev.name = Name(String::printf(TXT("%i-*%i"), i, ni));
			connectorNext.samePieceConnectorGroup = Name(String::printf(TXT("%i-%i"), i, ni));
			connectorPrev.samePieceConnectorGroup = connectorNext.samePieceConnectorGroup;

			// provide connector tag only to the one that allows for creating pieces (and only for one of them
			if (balconyProvideConnectorTag.is_index_valid(i + connectorTagsOffset) &&
				balconyProvideConnectorTag[i + connectorTagsOffset].is_valid())
			{
				if (connectorNext.canCreateNewPieces)
				{
					connectorNext.provideConnectorTag = balconyProvideConnectorTag[i + connectorTagsOffset];
				}
				else if (connectorPrev.canCreateNewPieces)
				{
					connectorPrev.provideConnectorTag = balconyProvideConnectorTag[i + connectorTagsOffset];
				}
			}

			connectorNext.def.doorInRoomTags.set_tag(NAME(at), i);
			connectorNext.def.doorInRoomTags.set_tag(NAME(to), ni);
			connectorNext.def.doorInRoomTags.set_tag(NAME(dirAtTo), 1);
			connectorPrev.def.doorInRoomTags.set_tag(NAME(to), i);
			connectorPrev.def.doorInRoomTags.set_tag(NAME(at), ni);
			connectorPrev.def.doorInRoomTags.set_tag(NAME(dirAtTo), -1);

			// put layout cue caps, this will ensure that stuff inside has proper directions
			if (balconyProvideDir.is_index_valid(i + connectorTagsOffset))
			{
				DirFourClockwise::Type inDir = balconyProvideDir[i + connectorTagsOffset];
				// check what NEXT means
				DirFourClockwise::Type inDirNext = DirFourClockwise::prev_to(inDir);
				DirFourClockwise::Type inDirPrev = DirFourClockwise::next_to(inDir);
				if (! balconyConnectorsClockwise)
				{
					swap(inDirNext, inDirPrev);
				}

				connectorNext.def.doorInRoomTags.set_tag(PilgrimageInstanceOpenWorld::dir_to_layout_cue_cap_tag(inDirNext));
				connectorPrev.def.doorInRoomTags.set_tag(PilgrimageInstanceOpenWorld::dir_to_layout_cue_cap_tag(inDirPrev));
			}

			ptr->add_connector(connectorNext);
			ptr->add_connector(connectorPrev);
		}
		else
		{
			error(TXT("requires next and prev valid connectors"));
		}
	}

	return ptr;
}

bool BalconiesInfo::internal_generate(Framework::Room* _room, bool _subGenerator, REF_ Framework::RoomGeneratingContext& _roomGeneratingContext) const
{
	bool result = true;

	Balconies bs(this);
	if (_room->get_name().is_empty())
	{
		_room->set_name(String::printf(TXT("balconies \"%S\" : \"%S\""), _room->get_room_type()? _room->get_room_type()->get_name().to_string().to_char() : TXT("??"), _room->get_individual_random_generator().get_seed_string().to_char()));
	}
	result &= bs.generate(Framework::Game::get(), _room, _subGenerator, REF_ _roomGeneratingContext);

	result &= base::internal_generate(_room, _subGenerator, REF_ _roomGeneratingContext);
	return result;
}

BalconiesInfo::BalconyConnectors const& BalconiesInfo::choose_balcony_connectors(Optional<DirFourClockwise::Type> const & _dir, Random::Generator & _randomGenerator) const
{
	if (balconyConnectors.get_size() == 1)
	{
		return balconyConnectors[0];
	}
	Optional<DirFourClockwise::Type> worldDir = _dir;
	Optional<DirFourClockwise::Type> localDir;
	if (worldDir.is_set())
	{
		if (auto* piow = PilgrimageInstanceOpenWorld::get())
		{
			DirFourClockwise::Type cellDir = piow->get_dir_for_cell_context();
			localDir = DirFourClockwise::world_to_local(worldDir.get(), cellDir);
		}
	}

	Array<BalconyConnectors const*> bcs;
	for_every(bc, balconyConnectors)
	{
		if (localDir.is_set() && bc->localDirNot.does_contain(localDir.get()))
		{
			continue;
		}
		if (!bc->localDirIs.is_empty() &&
			(! localDir.is_set() ||
			 ! bc->localDirIs.does_contain(localDir.get())))
		{
			continue;
		}
		bcs.push_back(bc);
	}
	an_assert(! bcs.is_empty(), TXT("if it happened, we have incorrect setup but we still want to do something!"));
	if (!bcs.is_empty())
	{
		return *bcs[_randomGenerator.get_int(bcs.get_size())];
	}
	else
	{
		return balconyConnectors[_randomGenerator.get_int(balconyConnectors.get_size())];
	}
}

//

Balconies::Balconies(BalconiesInfo const * _info)
: info(_info)
{
}

Balconies::~Balconies()
{
}

struct BalconyInfo
{
	Optional<Transform> placement;
	Framework::DoorInRoom* leftDoor = nullptr;
	Framework::DoorInRoom* rightDoor = nullptr;
};

bool Balconies::generate(Framework::Game* _game, Framework::Room* _room, bool _subGenerator, REF_ Framework::RoomGeneratingContext& _roomGeneratingContext)
{
	bool result = true;
	RoomGenerationInfo roomGenerationInfo = RoomGenerationInfo::get();
	Random::Generator randomGenerator = _room->get_individual_random_generator();
	randomGenerator.advance_seed(45975, 20825);

#ifdef OUTPUT_GENERATION
	output(TXT("generating balconies %S"), randomGenerator.get_seed_string().to_char());
#endif

	SimpleVariableStorage variables;
	_room->collect_variables(REF_ variables);

#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
	DebugVisualiserPtr dv(new DebugVisualiser(String(TXT("balconies"))));
	dv->set_background_colour(Colour::lerp(0.05f, Colour::greyLight, Colour::purple));
	dv->activate();
#endif

	// get count basing on doors
	int balconyCount = 1;
	for_every_ptr(dir, _room->get_all_doors())
	{
		if (dir->get_tags().has_tag(NAME(at)))
		{
			int atBalcony = dir->get_tags().get_tag_as_int(NAME(at));
			balconyCount = max(balconyCount, atBalcony + 1);
		}
		if (dir->get_tags().has_tag(NAME(to)))
		{
			int toBalcony = dir->get_tags().get_tag_as_int(NAME(to));
			balconyCount = max(balconyCount, toBalcony + 1);
		}
	}

	auto* mainMeshGenerator = Framework::Library::get_current()->get_mesh_generators().find(info->mainMeshGenerator.get(variables));
	if (!mainMeshGenerator)
	{
		error(TXT("no main mesh generator provided"));
		an_assert(false);
		return false;
	}

	auto* balconyMeshGenerator = Framework::Library::get_current()->get_mesh_generators().find(info->balconyMeshGenerator.get(variables));
	if (!balconyMeshGenerator)
	{
		error(TXT("no balcony mesh generator provided"));
		an_assert(false);
		return false;
	}

	auto* fakeBalconyMeshGenerator = Framework::Library::get_current()->get_mesh_generators().find_may_fail(info->fakeBalconyMeshGenerator.get(variables));
	// may be not present

	{
		BalconiesPrepareVariablesContext bpvContext;
		bpvContext.use(roomGenerationInfo);
		bpvContext.use(variables);
		bpvContext.process();

		bool alignToTiles = variables.get_value(NAME(balconiesAlignToTiles), false);

		Transform mainPlacement = Transform::identity;

		Framework::UsedLibraryStored<Framework::Mesh> mainMesh;
		// generate main mesh with provided balcony count
		if (mainMeshGenerator)
		{
			SimpleVariableStorage mainVariables(variables);

			Random::Generator rg(randomGenerator);
			rg.advance_seed(97974, 979265);

			mainPlacement = Transform::identity;
			if (! PilgrimageInstanceOpenWorld::get())
			{
				// why do we have it anyway?
				mainPlacement = Transform(Vector3::zero, Rotator3(0.0f, rg.get_float(-40.0f, 40.0f), 0.0f).to_quat());
			}

			int fakeBalconyPreCount = info->fakeBalconyPreCount.get(rg);
			int fakeBalconyPostCount = info->fakeBalconyPostCount.get(rg);

			fakeBalconyPreCount = variables.get_value<int>(NAME(fakeBalconyPreCount), fakeBalconyPreCount);
			fakeBalconyPostCount = variables.get_value<int>(NAME(fakeBalconyPostCount), fakeBalconyPostCount);

			if (!fakeBalconyMeshGenerator)
			{
				fakeBalconyPreCount = 0;
				fakeBalconyPostCount = 0;
			}
			mainVariables.access<int>(NAME(balconyCount)) = balconyCount;
			mainVariables.access<int>(NAME(fakeBalconyPreCount)) = fakeBalconyPreCount;
			mainVariables.access<int>(NAME(fakeBalconyPostCount)) = fakeBalconyPostCount;
			mainVariables.access<float>(NAME(maxBalconyWidth)) = bpvContext.maxBalconyWidth;

#ifdef OUTPUT_GENERATION
			output(TXT("generate main mesh"));
#endif
			mainMesh = mainMeshGenerator->generate_temporary_library_mesh(::Framework::Library::get_current(),
				::Framework::MeshGeneratorRequest()
					.for_wmp_owner(_room)
					.no_lods()
					.using_parameters(mainVariables)
					.using_random_regenerator(rg)
					.using_mesh_nodes(_roomGeneratingContext.meshNodes));
#ifdef OUTPUT_GENERATION
			output(TXT("generated main mesh"));
#endif
			if (mainMesh.get())
			{
				_room->add_mesh(mainMesh.get(), mainPlacement);
				Framework::MeshNode::copy_not_dropped_to(mainMesh->get_mesh_nodes(), _roomGeneratingContext.meshNodes, mainPlacement);
			}
			else
			{
				error(TXT("could not generate main mesh"));
				result = false;
				AN_BREAK;
			}
		}

		ARRAY_PREALLOC_SIZE(BalconyInfo, balconies, balconyCount);
		for_count(int, i, balconyCount)
		{
			balconies.push_back(BalconyInfo());
		}

		for_every_ptr(dir, _room->get_all_doors())
		{
			if (dir->get_tags().get_tag(NAME(first)))
			{
				balconies.get_first().leftDoor = dir;
			}
			else if (dir->get_tags().get_tag(NAME(last)))
			{
				balconies.get_last().rightDoor = dir;
			}
			else if (dir->get_tags().has_tag(NAME(at)) &&
					 dir->get_tags().has_tag(NAME(to)))
			{
				int atBalcony = dir->get_tags().get_tag_as_int(NAME(at));
				int dirAtToBalcony = dir->get_tags().get_tag_as_int(NAME(dirAtTo));
				if (dirAtToBalcony < 0)
				{
					balconies[atBalcony].leftDoor = dir;
				}
				else
				{
					balconies[atBalcony].rightDoor = dir;
				}
			}
		}

		if (!PilgrimageInstanceOpenWorld::get() &&
			randomGenerator.get_bool())
		{
			todo_hack(TXT("for random levels do this to go from right to left, not only from left to right"));
			swap(balconies.get_first().leftDoor, balconies.get_last().rightDoor);
		}

		bool allBalconiesHaveBothDoors = true;
		for_every(b, balconies)
		{
			if (info->maySkipFirstLastConnectors && for_everys_index(b) == 0)
			{
				allBalconiesHaveBothDoors &= b->rightDoor != nullptr;
			}
			else if (info->maySkipFirstLastConnectors && for_everys_index(b) == balconies.get_size() - 1)
			{
				allBalconiesHaveBothDoors &= b->leftDoor != nullptr;
			}
			else
			{
				allBalconiesHaveBothDoors &= b->leftDoor != nullptr && b->rightDoor != nullptr;
			}
		}
		if (!allBalconiesHaveBothDoors &&
			_room->get_all_doors().get_size() > 1) // we allow this to happen if we have just one door
		{
			error(TXT("not all balconies have both door"));
			an_assert(false, TXT("not all balconies have both door"));
			result = false;
			AN_BREAK;
		}
		else if (mainMesh.get())
		{
			// do two passes, first one with normal balconies to place doors
			// second with fake balconies to place their fake doors against normal ones
			for_count(int, doFakeBalconies, 2)
			{
				int meshNodeIdx = 0;
				for_every_ref(meshNode, mainMesh->get_mesh_nodes()) // we will be referring to same mesh nodes and when we drop, the nodes in roomGeneratingContext will also be dropped
				{
					bool isBalcony = meshNode->name == NAME(balcony);
					bool isFakeBalcony = meshNode->name == NAME(fakeBalcony);
					if (isBalcony || isFakeBalcony)
					{
						++meshNodeIdx;
						if ((isBalcony && doFakeBalconies) ||
							(isFakeBalcony && !doFakeBalconies))
						{
							// skip
							continue;
						}
#ifdef OUTPUT_GENERATION
						if (isBalcony)
						{
							output(TXT(" + balcony @ %S"), meshNode->name.to_char());
						}
						if (isFakeBalcony)
						{
							output(TXT(" + fake balcony @ %S"), meshNode->name.to_char());
						}
#endif
						int const * pIndex = meshNode->variables.get_existing<int>(NAME(index));
						if ((isBalcony && pIndex) ||
							(isFakeBalcony))
						{
							int index = pIndex ? *pIndex : meshNodeIdx;
#ifdef OUTPUT_GENERATION
							output(TXT("   index : %i"), index);
#endif
							auto * balcony = isBalcony ? &balconies[index] : nullptr;

							if (balcony && balcony->placement.is_set())
							{
								error(TXT("balcony %i already done"), index);
								result = false;
								AN_BREAK;
							}
							else if ((isBalcony && balconyMeshGenerator) ||
								(isFakeBalcony && fakeBalconyMeshGenerator))
							{
#ifdef OUTPUT_GENERATION
								output(TXT("   generate"));
#endif
								if (balcony && meshNode->variables.get_value<bool>(NAME(swapDoors), false))
								{
#ifdef OUTPUT_GENERATION
									output(TXT("   swap doors"));
#endif
									swap(balcony->leftDoor, balcony->rightDoor);
								}

								Random::Generator rg(randomGenerator);
								rg.advance_seed(index * 72347, index * 97862);
								bool isOk = false;
								int triesIdx = 0; // most of them should be fine at the first try
								while (! isOk && triesIdx < 20)
								{
									Transform balconyPlacement = meshNode->placement;
									balconyPlacement = mainPlacement.to_world(balconyPlacement);

									Random::Generator useRG = rg;
									useRG = meshNode->variables.get_value<Random::Generator>(NAME(randomSeed), useRG);
									useRG.advance_seed(index * 72347, index * 97862);
									for_count(int, i, triesIdx)
									{
										useRG.advance_seed(0, 1);
									}

									SimpleVariableStorage balconyVariables(variables);
									balconyVariables.set_from(meshNode->variables);
									balconyVariables.access<Vector2>(NAME(availableSpaceForBalcony)) = bpvContext.availableSpaceForBalcony;
									balconyVariables.access<float>(NAME(balconyDoorWidth)) = bpvContext.balconyDoorWidth;
									balconyVariables.access<float>(NAME(balconyDoorHeight)) = bpvContext.balconyDoorHeight;
									balconyVariables.access<float>(NAME(maxBalconyWidth)) = bpvContext.maxBalconyWidth;
									balconyVariables.access<Random::Generator>(NAME(randomSeed)) = useRG;
									if (!isBalcony && balconyMeshGenerator)
									{
										balconyVariables.access<Framework::LibraryName>(NAME(balconyMeshGenerator)) = balconyMeshGenerator->get_name();
									}

#ifdef OUTPUT_GENERATION
									output(TXT("   generate balcony mesh"));
									for_every_ref(mn, _roomGeneratingContext.meshNodes)
									{
										output(TXT("    + mesh node \"%S\""), mn->name.to_char());
									}
#endif

									auto balconyMesh = (isBalcony ? balconyMeshGenerator : fakeBalconyMeshGenerator)->generate_temporary_library_mesh(::Framework::Library::get_current(),
										::Framework::MeshGeneratorRequest()
											.for_wmp_owner(_room)
											.no_lods()
											.using_parameters(balconyVariables)
											.using_random_regenerator(useRG)
											.using_mesh_nodes(_roomGeneratingContext.meshNodes));
									if (balconyMesh.get())
									{
#ifdef OUTPUT_GENERATION
										output(TXT("   balcony mesh generated"));
#endif
#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
										dv->clear();
										dv->start_gathering_data();
#endif

										if (auto* wallPlacement = balconyMesh->find_mesh_node(NAME(wall)))
										{
											balconyPlacement = balconyPlacement.to_world(wallPlacement->placement.inverted());
										}

										Transform leftDoorLocalPlacement = Transform::identity;
										Transform rightDoorLocalPlacement = Transform::identity;
										Transform availableSpaceCentreLocalPlacement = Transform::identity;
										if (auto* meshNode = balconyMesh->access_mesh_node(NAME(leftDoor)))
										{
											leftDoorLocalPlacement = meshNode->placement;
											meshNode->drop();
										}
										else if (isBalcony)
										{
											error(TXT("no \"left door\" for balcony"));
											result = false;
											AN_BREAK;
										}
										if (auto* meshNode = balconyMesh->access_mesh_node(NAME(rightDoor)))
										{
											rightDoorLocalPlacement = meshNode->placement;
											meshNode->drop();
										}
										else if (isBalcony)
										{
											error(TXT("no \"right door\" for balcony"));
											result = false;
											AN_BREAK;
										}
										if (auto* meshNode = balconyMesh->find_mesh_node(NAME(availableSpaceCentre)))
										{
											availableSpaceCentreLocalPlacement = meshNode->placement;
										}
										else if (isBalcony)
										{
											error(TXT("no \"available space centre\" for balcony"));
											result = false;
											AN_BREAK;
										}

										if (isBalcony)
										{
											// we want to fit available space into our playable vr space
											Transform availableSpaceCentreVRSpacePlacement = Transform::identity;
											{	// orient and move balcony in vr space
												Range2 vrSpaceTaken = Range2::empty; // relative to centre (availableSpaceCentreLocalPlacement / "available space centre")
												{	// prepare vr space taken
													// get space taken, first get doors to include whole doors and space beyond them
													vrSpaceTaken.include(RoomGenerationInfo::get_door_required_space(availableSpaceCentreLocalPlacement.to_local(leftDoorLocalPlacement), bpvContext.tileSize));
													vrSpaceTaken.include(RoomGenerationInfo::get_door_required_space(availableSpaceCentreLocalPlacement.to_local(rightDoorLocalPlacement), bpvContext.tileSize));
													// include corners
													for_every_ref(meshNode, balconyMesh->get_mesh_nodes())
													{
														if (meshNode->name == NAME(availableSpaceCorner))
														{
															vrSpaceTaken.include(availableSpaceCentreLocalPlacement.location_to_local(meshNode->placement.get_translation()).to_vector2());
														}
													}
												}
												availableSpaceCentreVRSpacePlacement = RoomGeneratorUtils::get_placement_for_required_vr_space(_room, vrSpaceTaken, roomGenerationInfo, randomGenerator, alignToTiles);
#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
												{
													Range2 vrSpaceTakenMoved = vrSpaceTaken.moved_by(availableSpaceCentreVRSpacePlacement.get_translation().to_vector2());
													Framework::DebugVisualiserUtils::add_range2_to_debug_visualiser(dv, vrSpaceTakenMoved, Colour::yellow);
												}
#endif
											}

											float balconyDoorWidth = balcony ? (balcony->leftDoor? balcony->leftDoor : balcony->rightDoor)->get_door()->calculate_vr_width() : 0.0f;
											if (balconyDoorWidth == 0.0f)
											{
												balconyDoorWidth = roomGenerationInfo.get_tile_size();
											}

#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
											roomGenerationInfo.get_play_area_zone().debug_visualise(dv.get(), Colour::magenta);

											Framework::DebugVisualiserUtils::add_doors_to_debug_visualiser(dv, roomGenerationInfo.get_play_area_zone(),
												balcony->leftDoor, balcony->leftDoor->get_vr_space_placement().get_translation().to_vector2(), balcony->leftDoor->get_vr_space_placement().get_axis(Axis::Y).to_vector2(),
												balcony->rightDoor, balcony->rightDoor->get_vr_space_placement().get_translation().to_vector2(), balcony->rightDoor->get_vr_space_placement().get_axis(Axis::Y).to_vector2(),
												balconyDoorWidth);
#endif
											Transform const leftDoorVRPlacement = availableSpaceCentreVRSpacePlacement.to_world(availableSpaceCentreLocalPlacement.to_local(leftDoorLocalPlacement));
											Transform const rightDoorVRPlacement = availableSpaceCentreVRSpacePlacement.to_world(availableSpaceCentreLocalPlacement.to_local(rightDoorLocalPlacement));

											VR::Zone leftDoorZone = balcony->leftDoor? balcony->leftDoor->calc_vr_zone_outbound_if_were_at(leftDoorVRPlacement) : VR::Zone();
											VR::Zone rightDoorZone = balcony->rightDoor? balcony->rightDoor->calc_vr_zone_outbound_if_were_at(rightDoorVRPlacement): VR::Zone();
#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
											leftDoorZone.debug_visualise(dv.get(), Colour::blue);
											rightDoorZone.debug_visualise(dv.get(), Colour::red);

											{
												Array<Vector2> points;
												for_every_ref(meshNode, balconyMesh->get_mesh_nodes())
												{
													if (meshNode->name == NAME(availableSpaceCorner))
													{
														points.push_back((availableSpaceCentreVRSpacePlacement.location_to_world(availableSpaceCentreLocalPlacement.location_to_local(meshNode->placement.get_translation()))).to_vector2());
													}
												}
												::Framework::DebugVisualiserUtils::add_poly_to_debug_visualiser(dv, points, Colour::green);
											}
#endif
											/*
											bool leftFits = (!balcony->leftDoor || roomGenerationInfo.get_play_area_zone().does_contain(leftDoorZone, -0.05f));
											bool rightFits = (!balcony->rightDoor || roomGenerationInfo.get_play_area_zone().does_contain(rightDoorZone, -0.05f));
											bool leftFits2 = (!balcony->leftDoor || roomGenerationInfo.get_play_area_zone().does_contain(leftDoorVRPlacement.get_translation().to_vector2(), balconyDoorWidth * 0.4f));
											bool rightFits2 = (!balcony->rightDoor || roomGenerationInfo.get_play_area_zone().does_contain(rightDoorVRPlacement.get_translation().to_vector2(), balconyDoorWidth * 0.4f));
											*/
											if ((!balcony->leftDoor || roomGenerationInfo.get_play_area_zone().does_contain(leftDoorZone, -0.05f)) &&
												(!balcony->rightDoor || roomGenerationInfo.get_play_area_zone().does_contain(rightDoorZone, -0.05f)) &&
												/* these two below may be not required */
												(!balcony->leftDoor || roomGenerationInfo.get_play_area_zone().does_contain(leftDoorVRPlacement.get_translation().to_vector2(), balconyDoorWidth * 0.4f)) &&
												(!balcony->rightDoor || roomGenerationInfo.get_play_area_zone().does_contain(rightDoorVRPlacement.get_translation().to_vector2(), balconyDoorWidth * 0.4f)))
											{
												bool allPointsFitIn = true;
												for_every_ref(meshNode, balconyMesh->get_mesh_nodes())
												{
													if (meshNode->name == NAME(availableSpaceCorner))
													{
														Vector2 corner = (availableSpaceCentreVRSpacePlacement.location_to_world(availableSpaceCentreLocalPlacement.location_to_local(meshNode->placement.get_translation()))).to_vector2();
														if (!roomGenerationInfo.get_play_area_zone().does_contain(corner, -0.1f))
														{
															allPointsFitIn = false;
															break;
														}
													}
												}
												if (allPointsFitIn)
												{
													isOk = true;
												}
											}

											if (!isOk)
											{
												/*
												DebugVisualiserPtr dv(new DebugVisualiser(String(TXT("balconies"))));
												dv->set_background_colour(Colour::lerp(0.05f, Colour::greyLight, Colour::purple));
												dv->activate();
												dv->clear();
												dv->start_gathering_data();

												roomGenerationInfo.get_play_area_zone().debug_visualise(dv.get(), Colour::magenta);
												if (balcony->leftDoor)
												{
													leftDoorZone.debug_visualise(dv.get(), Colour::blue);
												}
												if (balcony->rightDoor)
												{
													rightDoorZone.debug_visualise(dv.get(), Colour::red);
												}

												{
													Array<Vector2> points;
													for_every_ref(meshNode, balconyMesh->get_mesh_nodes())
													{
														if (meshNode->name == NAME(availableSpaceCorner))
														{
															points.push_back((availableSpaceCentreVRSpacePlacement.location_to_world(availableSpaceCentreLocalPlacement.location_to_local(meshNode->placement.get_translation()))).to_vector2());
														}
													}
													::Framework::DebugVisualiserUtils::add_poly_to_debug_visualiser(dv, points, Colour::green);
												}

												dv->end_gathering_data();
												dv->show_and_wait_for_key();
												*/
												warn(TXT("problem generating balconies, going beyond play area"));
											}

											if (isOk)
											{
#ifdef OUTPUT_GENERATION
												output(TXT("set vr placement for the balcony"));
#endif
												// grow into vr corridors only if is ok
												if (balcony->leftDoor &&
													(!balcony->leftDoor->is_vr_space_placement_set() ||
														!Framework::DoorInRoom::same_with_orientation_for_vr(balcony->leftDoor->get_vr_space_placement(), leftDoorVRPlacement)))
												{
													if (balcony->leftDoor->is_vr_placement_immovable())
													{
														balcony->leftDoor = balcony->leftDoor->grow_into_vr_corridor(NP, leftDoorVRPlacement, roomGenerationInfo.get_play_area_zone(), roomGenerationInfo.get_tile_size());
													}
													balcony->leftDoor->set_vr_space_placement(leftDoorVRPlacement);
												}
												if (balcony->rightDoor &&
													(!balcony->rightDoor->is_vr_space_placement_set() ||
														!Framework::DoorInRoom::same_with_orientation_for_vr(balcony->rightDoor->get_vr_space_placement(), rightDoorVRPlacement)))
												{
													if (balcony->rightDoor->is_vr_placement_immovable())
													{
														balcony->rightDoor = balcony->rightDoor->grow_into_vr_corridor(NP, rightDoorVRPlacement, roomGenerationInfo.get_play_area_zone(), roomGenerationInfo.get_tile_size());
													}
													balcony->rightDoor->set_vr_space_placement(rightDoorVRPlacement);
												}

#ifdef OUTPUT_GENERATION
												output(TXT("vr placement for the balcony set"));
#endif
											}
										}
										else
										{
											an_assert(isFakeBalcony);
											isOk = true;
										}

										if (isOk)
										{
#ifdef OUTPUT_GENERATION
											output(TXT("   add balcony to room"));
#endif
											_room->add_mesh(balconyMesh.get(), balconyPlacement);

#ifdef OUTPUT_GENERATION
											output(TXT("   added balcony to room"));
#endif

											Framework::MeshNode::copy_not_dropped_to(balconyMesh->get_mesh_nodes(), _roomGeneratingContext.meshNodes, balconyPlacement);

											if (balcony)
											{
												// world placement
												if (balcony->leftDoor)
												{
													balcony->leftDoor->set_placement(balconyPlacement.to_world(leftDoorLocalPlacement));
													balcony->leftDoor->set_group_id(index);
												}
												if (balcony->rightDoor)
												{
													balcony->rightDoor->set_placement(balconyPlacement.to_world(rightDoorLocalPlacement));
													balcony->rightDoor->set_group_id(index);
												}

												balcony->placement = balconyPlacement;
											}

											meshNode->drop();
#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
											dv->add_text(Vector2::zero, String(TXT("ok")), Colour::green, 0.1f);
#endif
#ifdef OUTPUT_GENERATION
											output(TXT("   generated and added"));
#endif
										}
										else
										{
#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
											dv->add_text(Vector2::zero, String(TXT("failed")), Colour::red, 0.1f);
#endif
										}
#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
										dv->end_gathering_data();
										dv->show_and_wait_for_key();
#endif
									}
									else
									{
#ifdef OUTPUT_GENERATION
										output(TXT("   could not generate balcony mesh"));
#endif

									}
#ifdef OUTPUT_GENERATION
									if (!isOk)
									{
										output(TXT("   failed generation"));
									}
#endif
									if (!isOk)
									{
										warn(TXT("could not generate balcony/fake balcony mesh"));
										++triesIdx;
									}
								}
								if (!isOk)
								{
									error(TXT("could not generate balcony/fake balcony mesh"));
									result = false;
									AN_BREAK;
								}
							}
						}
						else
						{
							error(TXT("no \"index\" variable provided for balcony mesh node"));
							result = false;
							AN_BREAK;
						}
					}
				}
			}
		}

		bool allDone = true;
		for_every(b, balconies)
		{
			allDone &= b->placement.is_set();
		}
		if (!allDone)
		{
			error(TXT("not enough mesh nodes for balconies provided"));
			result = false;
			AN_BREAK;
		}

		if (auto* piow = PilgrimageInstanceOpenWorld::get())
		{
			if (info->setAnchorVariable.is_valid())
			{
				Transform anchorProvided = Transform::identity;
				if (info->getAnchorPOI.is_valid())
				{
					_room->for_every_point_of_interest(info->getAnchorPOI, [&anchorProvided](Framework::PointOfInterestInstance* _poi)
						{
							anchorProvided = _poi->calculate_placement();
						});
				}

				Transform roomCentrePOI = anchorProvided;
				if (info->roomCentrePOI.is_valid())
				{
					_room->for_every_point_of_interest(info->roomCentrePOI, [&roomCentrePOI](Framework::PointOfInterestInstance* _poi)
						{
							roomCentrePOI = _poi->calculate_placement();
						});
				}

				if (info->openWorldBreakerLocalDir.is_set())
				{
					DirFourClockwise::Type worldBreakerDir = piow->get_dir_to_world_for_cell_context(info->openWorldBreakerLocalDir.get());
					Transform worldBreakerPlacementInWorld = look_matrix(Vector3::zero, DirFourClockwise::dir_to_vector_int2(worldBreakerDir).to_vector2().to_vector3(), Vector3::zAxis).to_transform();
					Transform worldBreakerPlacementInRoom = anchorProvided;

					_room->access_variables().access<Transform>(info->setAnchorVariable) = worldBreakerPlacementInRoom.to_world(worldBreakerPlacementInWorld.inverted());
				}
				else
				{
					piow->tag_open_world_directions_for_cell(_room);
					DirFourClockwise::Type dirs[] = { DirFourClockwise::Up
													, DirFourClockwise::Down
													, DirFourClockwise::Right
													, DirFourClockwise::Left };
					for_count(int, i, DirFourClockwise::NUM)
					{
						DirFourClockwise::Type inDir = dirs[i];
						if (auto* dir = piow->find_best_door_in_dir(_room, inDir))
						{
#ifdef OUTPUT_OPEN_WORLD_BALCONIES
							output(TXT("best door - tags %S"), dir->get_tags().to_string(true).to_char());
#endif
							BalconyInfo * balconyA = nullptr;
							BalconyInfo * balconyB = nullptr;

							for_every(balcony, balconies)
							{
								if (balcony->leftDoor == dir)
								{
#ifdef OUTPUT_OPEN_WORLD_BALCONIES
									output(TXT("balconies left door on balcony %i"), for_everys_index(balcony));
#endif
									balconyA = balcony;
									balconyB = nullptr;
									if (!balcony->leftDoor->get_tags().get_tag(NAME(first)) &&
										balcony->leftDoor->get_tags().has_tag(NAME(to)))
									{
#ifdef OUTPUT_OPEN_WORLD_BALCONIES
										output(TXT("neighbour balcony %i"), balcony->leftDoor->get_tags().get_tag_as_int(NAME(to)));
#endif
										balconyB = &balconies[balcony->leftDoor->get_tags().get_tag_as_int(NAME(to))];
									}
								}
								if (balcony->rightDoor == dir)
								{
#ifdef OUTPUT_OPEN_WORLD_BALCONIES
									output(TXT("balconies right door on balcony %i"), for_everys_index(balcony));
#endif
									balconyA = balcony;
									balconyB = nullptr;
									if (!balcony->rightDoor->get_tags().get_tag(NAME(last)) &&
										balcony->rightDoor->get_tags().has_tag(NAME(to)))
									{
#ifdef OUTPUT_OPEN_WORLD_BALCONIES
										output(TXT("neighbour balcony %i"), balcony->rightDoor->get_tags().get_tag_as_int(NAME(to)));
#endif
										balconyB = &balconies[balcony->rightDoor->get_tags().get_tag_as_int(NAME(to))];
									}
								}
							}

							if (balconyA)
							{
								Vector3 placementInDir = balconyA->placement.get().get_translation();
#ifdef OUTPUT_OPEN_WORLD_BALCONIES
								output(TXT("balcony A at %S"), balconyA->placement.get().get_translation().to_string().to_char());
#endif
								if (balconyB)
								{
									placementInDir += balconyB->placement.get().get_translation();
									placementInDir = placementInDir * 0.5f;
#ifdef OUTPUT_OPEN_WORLD_BALCONIES
									output(TXT("balcony B at %S"), balconyB->placement.get().get_translation().to_string().to_char());
#endif
								}
#ifdef OUTPUT_OPEN_WORLD_BALCONIES
								output(TXT("anchorProvided at %S"), anchorProvided.get_translation().to_string().to_char());
#endif
								Vector3 actualInDir = placementInDir - roomCentrePOI.get_translation();
								actualInDir.z = 0.0f;
								actualInDir.normalise();

								// take inDir into account
								actualInDir = actualInDir.rotated_by_yaw(-(float)inDir * 90.0f);

								// where room centre should be
								Transform roomCentrePOIAsShouldBe = look_matrix(roomCentrePOI.get_translation(), actualInDir, Vector3::zAxis).to_transform();

								Transform inDirInRoom = roomCentrePOIAsShouldBe.to_world(roomCentrePOI.to_local(anchorProvided));

								_room->access_variables().access<Transform>(info->setAnchorVariable) = inDirInRoom;
								break;
							}
						}
					}
				}
			}
		}
	}

#ifdef OUTPUT_GENERATION
	output(TXT("generated balconies"));
#endif

	if (!_subGenerator)
	{
		_room->place_pending_doors_on_pois();
		_room->mark_vr_arranged();
		_room->mark_mesh_generated();
	}

	// otherwise first try to use existing vr door placement - if it is impossible, try dropping vr placement on more and more pieces
	return result;
}
